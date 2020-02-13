#include <zephyr.h>

#include <logging/log.h>
#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>

#include "coap.h"

LOG_MODULE_REGISTER(app_coap, CONFIG_APP_LOG_LEVEL);

#define MAX_COAP_MSG_LEN 512
#define MAX_COAP_OPTIONS 16

// An inflight_packet represents a packet that has not been acknowledged (and not responded to, if it is a request).
// This struct takes ownership of the packet and is responsible for freeing it.
typedef struct {
	bool retransmit;
	struct coap_packet packet;
	struct sockaddr addr;
	socklen_t addr_len;
	u8_t retransmission_count;
	s32_t timeout, deadline;

	coap_response_handler response_handler;
	void *response_handler_data;
} inflight_packet;

typedef struct _coap_endpoint {
	int sock;
	struct coap_resource *resources;
	inflight_packet *inflight_packets;
	size_t inflight_packets_len;
} coap_endpoint;

static K_FIFO_DEFINE(tx_fifo);
static K_FIFO_DEFINE(rx_fifo);

typedef struct {
	void *fifo_reserved;
	struct coap_packet packet;
	struct sockaddr addr;
	socklen_t addr_len;
	coap_response_handler response_handler;
	void *response_handler_data;
} packet_tx_data_item;

typedef struct {
	void *fifo_reserved;
	u8_t data[MAX_COAP_MSG_LEN];
	struct coap_packet packet;
	struct coap_option options[MAX_COAP_OPTIONS];
	struct sockaddr addr;
	socklen_t addr_len;
} packet_rx_data_item;

static void recv_thread(void *p1, void *p2, void *p3) {
	int sock = (int)p1;
	
	while (true) {
		packet_rx_data_item *rx = k_calloc(1, sizeof(packet_rx_data_item));
		if (!rx) {
			LOG_ERR("unable to allocate memory for packet");
			return;
		}
		rx->addr_len = sizeof(rx->addr);

		int len = recvfrom(sock, rx->data, sizeof(rx->data), 0, &rx->addr, &rx->addr_len);
		if (len < 0) {
			LOG_ERR("recvfrom: %d", errno);
			return;
		}

		int err = coap_packet_parse(&rx->packet, rx->data, sizeof(rx->data), rx->options, MAX_COAP_OPTIONS);
		if (err < 0) {
			LOG_ERR("coap_packet_parse: %d", err);
			k_free(rx);
			continue;
		}

		k_fifo_put(&rx_fifo, rx);
	}
}

#define RECV_THREAD_STACK_SIZE 1024
#define RECV_THREAD_PRIORITY -1
K_THREAD_STACK_DEFINE(recv_thread_stack_area, RECV_THREAD_STACK_SIZE);
static struct k_thread recv_thread_data;
static k_tid_t recv_thread_tid;

// new_inflight_packet creates a new inflight_packet, taking ownership of the packet if it is confirmable.
static inflight_packet *new_inflight_packet(coap_endpoint *ep, packet_tx_data_item *tx) {
	inflight_packet *new_inflight_packets = k_calloc(ep->inflight_packets_len + 1, sizeof(inflight_packet));
	if (new_inflight_packets == NULL) {
		return NULL;
	}
	memcpy(new_inflight_packets, ep->inflight_packets, ep->inflight_packets_len * sizeof(inflight_packet));
	k_free(ep->inflight_packets);
	ep->inflight_packets = new_inflight_packets;
	ep->inflight_packets_len++;

	inflight_packet *inflight = &ep->inflight_packets[ep->inflight_packets_len - 1];
	if (coap_header_get_type(&tx->packet) == COAP_TYPE_CON) {
		inflight->retransmit = true;
		inflight->packet = tx->packet;
		inflight->addr = tx->addr;
		inflight->addr_len = tx->addr_len;
		inflight->retransmission_count = 0;
		inflight->timeout = K_SECONDS(2) + sys_rand32_get() % K_SECONDS(1);
		inflight->deadline = k_uptime_get_32() + inflight->timeout;
	}
	inflight->response_handler = tx->response_handler;
	inflight->response_handler_data = tx->response_handler_data;
	return inflight;
}

static void free_inflight_packet(coap_endpoint *ep, inflight_packet *inflight) {
	for (int i = 0; i < ep->inflight_packets_len; i++) {
		if (&ep->inflight_packets[i] == inflight) {
			ep->inflight_packets_len--;
			memcpy(&ep->inflight_packets[i], &ep->inflight_packets[ep->inflight_packets_len], sizeof(inflight_packet));
			return;
		}
	}
}

// next_retransmission returns the inflight packet with the earliest retransmission deadline, or NULL.
static inflight_packet *next_retransmission(coap_endpoint *ep) {
	inflight_packet *next = NULL;
	for (size_t i = 0; i < ep->inflight_packets_len; i++) {
		inflight_packet *inflight = &ep->inflight_packets[i];
		if (inflight->retransmit && (next == NULL || inflight->deadline < next->deadline)) {
			next = inflight;
		}
	}

	return next;
}

#define COAP_CODE_CLASS_MASK (7<<5)

static bool is_request(const struct coap_packet *p) {
	u8_t code = coap_header_get_code(p);
	return (code & COAP_CODE_CLASS_MASK) == 0 && code != COAP_CODE_EMPTY;
}

static bool is_response(const struct coap_packet *p) {
	u8_t code = coap_header_get_code(p);
	return (code & COAP_CODE_CLASS_MASK) != 0;
}

// get_inflight_packet searches for an inflight packet that has a matching address and a matching id or token.
static inflight_packet *get_inflight_packet(coap_endpoint *ep, packet_rx_data_item *rx) {
	u16_t id = coap_header_get_id(&rx->packet);
	u8_t token[8];
	u8_t token_len = coap_header_get_token(&rx->packet, token);

	for (size_t i = 0; i < ep->inflight_packets_len; i++) {
		inflight_packet *inflight = &ep->inflight_packets[i];
		u8_t tok[8];
		bool addrs_equal = inflight->addr_len == rx->addr_len && memcmp(&inflight->addr, &rx->addr, rx->addr_len) == 0;
		bool ids_equal = coap_header_get_id(&inflight->packet) == id;
		bool tokens_equal = coap_header_get_token(&inflight->packet, tok) == token_len && memcmp(tok, token, token_len) == 0;

		if (addrs_equal && (ids_equal || tokens_equal)) {
			return inflight;
		}
	}

	return NULL;
}

// transmit_packet sends a packet for the first time (not a retransmission).
// It transfers ownership of the packet to an inflight_packet or frees it.
static void transmit_packet(packet_tx_data_item *tx, coap_endpoint *ep, int sock) {
	inflight_packet *inflight = new_inflight_packet(ep, tx);
	if (inflight == NULL) {
		tx->response_handler(tx->response_handler_data, -ENOMEM, NULL);
		k_free(tx->packet.data);
		return;
	}

	if (sendto(sock, tx->packet.data, tx->packet.offset, 0, &tx->addr, tx->addr_len) < 0) {
		LOG_ERR("sendto: %d", errno);
	}

	if (coap_header_get_type(&tx->packet) != COAP_TYPE_CON) {
		k_free(tx->packet.data);
		if (!is_request(&tx->packet)) {
			free_inflight_packet(ep, inflight);
		}
	}
}

// retransmit_packet retransmits a packet, or signals an error and frees packet.data if the maximum number of retransmissions is exceeded.
static void retransmit_packet(coap_endpoint *ep, inflight_packet *inflight) {
	inflight->retransmission_count++;
	inflight->timeout *= 2;
	inflight->deadline += inflight->timeout;

	const u8_t max_retransmission_count = 4;
	if (inflight->retransmission_count > max_retransmission_count) {
		inflight->response_handler(inflight->response_handler_data, -EAGAIN, NULL);
		free_inflight_packet(ep, inflight);
		return;
	}

	if (sendto(ep->sock, inflight->packet.data, inflight->packet.offset, 0, &inflight->addr, inflight->addr_len) < 0) {
		LOG_ERR("sendto: %d", errno);
	}
}

static enum coap_response_code response_code_for_error(int err) {
	switch (-err) {
	case ENOENT:
		return COAP_RESPONSE_CODE_NOT_FOUND;
	case EPERM:
		return COAP_RESPONSE_CODE_NOT_ALLOWED;
	case EINVAL:
		return COAP_RESPONSE_CODE_BAD_REQUEST;
	}
	return COAP_RESPONSE_CODE_INTERNAL_ERROR;
}

static void receive_packet(coap_endpoint *ep, packet_rx_data_item *rx) {
	// TODO: Optionally detect duplicate packets (retransmissions) and deal with them in the same way as the orignal transmission.

	if (is_request(&rx->packet)) {
		int err = coap_handle_request(&rx->packet, ep->resources, rx->options, MAX_COAP_OPTIONS, &rx->addr, rx->addr_len);
		if (err != 0) {
			int err = coap_endpoint_respond(ep, &rx->packet, response_code_for_error(err), NULL, 0, &rx->addr, rx->addr_len);
			if (err != 0) {
				LOG_ERR("coap_endpoint_respond: %d", err);
				return;
			}
		}
		return;
	}

	inflight_packet *inflight = get_inflight_packet(ep, rx);
	if (inflight != NULL) {
		if (coap_header_get_type(&rx->packet) == COAP_TYPE_RESET) {
			inflight->response_handler(inflight->response_handler_data, -ECANCELED, NULL);
			free_inflight_packet(ep, inflight);
			return;
		}

		if (is_response(&rx->packet)) {
			inflight->response_handler(inflight->response_handler_data, 0, &rx->packet);
			free_inflight_packet(ep, inflight);
			return;
		}

		// At this point we know rx->packet is neither a request, a response, nor a reset, so it must be an empty acknowledgment.

		if (is_request(&inflight->packet)) {
			inflight->retransmit = false;
			return;
		}

		if (is_response(&inflight->packet)) {
			free_inflight_packet(ep, inflight);
			return;
		}
	}

	// rx->packet is not a request nor did it match any inflight packets.
	// We should reject it so that it is not retransmitted.  It may be a ping.
	if (coap_header_get_type(&rx->packet) == COAP_TYPE_CON) {
		int err = coap_endpoint_reset(ep, &rx->packet, &rx->addr, rx->addr_len);
		if (err != 0) {
			LOG_ERR("coap_endpoint_reset: %d", err);
		}
		return;
	}
}

// txrx_thread transmits, retransmits, and receives packets.
// Packets to be sent are passed to this thread via tx_fifo.
// Packets are received in another thread and passed to this one via rx_fifo.
// The separate receiving thread is necessary because it is not possible to use k_poll with both sockets and FIFOs.
static void txrx_thread(void *p1, void *p2, void *p3) {
	coap_endpoint *ep = p1;

	recv_thread_tid = k_thread_create(
		&recv_thread_data, recv_thread_stack_area, K_THREAD_STACK_SIZEOF(recv_thread_stack_area),
		recv_thread,
		(void *)ep->sock, NULL, NULL,
		RECV_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (recv_thread_tid <= 0) {
		LOG_ERR("k_thread_create failed");
		return;
	}

	struct k_poll_event events[2] = {
	    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &tx_fifo, 0),
	    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &rx_fifo, 0),
	};

	while (true) {
		s32_t timeout = K_FOREVER;
		inflight_packet *retransmission = next_retransmission(ep);
		if (retransmission != NULL) {
			timeout = retransmission->deadline - k_uptime_get_32();
		}

		int ret = k_poll(events, sizeof(events)/sizeof(struct k_poll_event), timeout);
		if (ret == -EAGAIN) {
			retransmit_packet(ep, retransmission);
		} else if (ret == 0) {
			if (events[0].state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
				events[0].state = K_POLL_STATE_NOT_READY;

				packet_tx_data_item *tx = k_fifo_get(&tx_fifo, K_FOREVER);
				transmit_packet(tx, ep, ep->sock);
				k_free(tx);
			}
			if (events[1].state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
				events[1].state = K_POLL_STATE_NOT_READY;

				packet_rx_data_item *rx = k_fifo_get(&rx_fifo, K_FOREVER);
				receive_packet(ep, rx);
				k_free(rx);
			}
		} else {
			LOG_ERR("k_poll: %d", ret);
			continue;
		}
	}

	close(ep->sock);
}

#define TXRX_THREAD_STACK_SIZE 2048
#define TXRX_THREAD_PRIORITY -1
K_THREAD_STACK_DEFINE(txrx_thread_stack_area, TXRX_THREAD_STACK_SIZE);
static struct k_thread txrx_thread_data;
static k_tid_t txrx_thread_tid = 0;

coap_endpoint *coap_endpoint_init(struct sockaddr *local_addr, socklen_t local_addr_len, struct coap_resource *resources) {
	if (txrx_thread_tid != 0) {
		LOG_ERR("only a single CoAP ep is supported at this time");
		return NULL;
	}
	
	coap_endpoint *ep = k_calloc(1, sizeof(coap_endpoint));
	if (!ep) {
		return NULL;
	}

	ep->resources = resources;

	ep->sock = socket(local_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (ep->sock < 0) {
		LOG_ERR("socket: %d", errno);
		return NULL;
	}

	int ret = bind(ep->sock, local_addr, local_addr_len);
	if (ret < 0) {
		LOG_ERR("bind: %d", errno);
		close(ep->sock);
		return NULL;
	}

	txrx_thread_tid = k_thread_create(
		&txrx_thread_data, txrx_thread_stack_area, K_THREAD_STACK_SIZEOF(txrx_thread_stack_area),
		txrx_thread,
		ep, NULL, NULL,
		TXRX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (txrx_thread_tid <= 0) {
		LOG_ERR("k_thread_create failed");
		return NULL;
	}

	return ep;
}


typedef struct {
	coap_endpoint *ep;
	struct k_sem sem;
	int ret;
	struct sockaddr *addr;
	socklen_t addr_len;
} post_handler_data;

void post_handler(void *p, int err, struct coap_packet *response) {
	post_handler_data *data = p;
	if (err) {
		data->ret = err;
	} else {
		data->ret = coap_header_get_code(response);
	}

	err = coap_endpoint_acknowledge(data->ep, response, data->addr, data->addr_len);
	if (err != 0) {
		LOG_ERR("coap_endpoint_acknowledge: %d", err);
	}

	k_sem_give(&data->sem);
}

int coap_endpoint_post(coap_endpoint *ep, struct sockaddr *addr, socklen_t addr_len, const char *const *path, u8_t *payload, int payload_len) {
	post_handler_data post_handler_data = {
		ep: ep,
		addr: addr,
		addr_len: addr_len,
	};
	k_sem_init(&post_handler_data.sem, 0, 1);

	int err = coap_endpoint_post_async(ep, addr, addr_len, path, payload, payload_len, post_handler, &post_handler_data);
	if (err != 0) {
		LOG_ERR("coap_endpoint_post_async: %d", err);
		return err;
	}

	k_sem_take(&post_handler_data.sem, K_FOREVER);
	return post_handler_data.ret;
}

int coap_endpoint_post_async(coap_endpoint *ep, struct sockaddr *addr, socklen_t addr_len, const char *const *path, u8_t *payload, int payload_len, coap_response_handler response_handler, void *response_handler_data) {
	u8_t *data = k_calloc(1, MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	packet_tx_data_item *tx = k_calloc(1, sizeof(packet_tx_data_item));
	if (!tx) {
		k_free(data);
		return -ENOMEM;
	}

	tx->addr = *addr;
	tx->addr_len = addr_len;
	tx->response_handler = response_handler;
	tx->response_handler_data = response_handler_data;

	int err;

	err = coap_packet_init(&tx->packet, data, MAX_COAP_MSG_LEN, 1, COAP_TYPE_CON, 8, coap_next_token(), COAP_METHOD_POST, coap_next_id());
	if (err < 0) {
		LOG_ERR("coap_packet_init: %d", err);
		goto error;
	}

	for (const char *const *p = path; p && *p; p++) {
		err = coap_packet_append_option(&tx->packet, COAP_OPTION_URI_PATH, *p, strlen(*p));
		if (err < 0) {
			LOG_ERR("coap_packet_append_option: %d", err);
			goto error;
		}
	}

	if (payload != NULL && payload_len > 0) {
		err = coap_packet_append_payload_marker(&tx->packet);
		if (err < 0) {
			LOG_ERR("coap_packet_append_payload_marker: %d", err);
			goto error;
		}

		err = coap_packet_append_payload(&tx->packet, payload, payload_len);
		if (err < 0) {
			LOG_ERR("coap_packet_append_payload: %d", err);
			goto error;
		}
	}

	k_fifo_put(&tx_fifo, tx);

	return 0;

error:
	k_free(tx);
	k_free(data);

	return err;
}

int coap_endpoint_acknowledge(coap_endpoint *ep, struct coap_packet *packet, struct sockaddr *addr, socklen_t addr_len) {
	if (coap_header_get_type(packet) != COAP_TYPE_CON) {
		return 0;
	}

	struct coap_packet ack;
	u8_t data[4];
	int err = coap_packet_init(&ack, data, sizeof(data), 1, COAP_TYPE_ACK, 0, NULL, COAP_CODE_EMPTY, coap_header_get_id(packet));
	if (err < 0) {
		LOG_ERR("coap_packet_init: %d", err);
		return err;
	}

	if (sendto(ep->sock, ack.data, ack.offset, 0, addr, addr_len) < 0) {
		LOG_ERR("sendto: %d", errno);
		return -errno;
	}

	return 0;
}

int coap_endpoint_reset(coap_endpoint *ep, struct coap_packet *packet, struct sockaddr *addr, socklen_t addr_len) {
	if (coap_header_get_type(packet) == COAP_TYPE_ACK || coap_header_get_type(packet) == COAP_TYPE_RESET) {
		LOG_ERR("coap_endpoint_reset must not be called for ACK or RESET packets");
		return -EINVAL;
	}

	struct coap_packet reset;
	u8_t data[4];
	int err = coap_packet_init(&reset, data, sizeof(data), 1, COAP_TYPE_RESET, 0, NULL, COAP_CODE_EMPTY, coap_header_get_id(packet));
	if (err < 0) {
		LOG_ERR("coap_packet_init: %d", err);
		return err;
	}

	if (sendto(ep->sock, reset.data, reset.offset, 0, addr, addr_len) < 0) {
		LOG_ERR("sendto: %d", errno);
		return -errno;
	}

	return 0;
}

int coap_endpoint_respond(coap_endpoint *ep, struct coap_packet *request, enum coap_response_code code, u8_t *payload, u16_t payload_len, struct sockaddr *addr, socklen_t addr_len) {
	struct coap_packet response;
	u8_t data[MAX_COAP_MSG_LEN];
	u8_t type = COAP_TYPE_ACK;
	if (coap_header_get_type(request) == COAP_TYPE_NON_CON) {
		type = COAP_TYPE_NON_CON;
	}
	u8_t token[8];
	int err = coap_packet_init(&response, data, sizeof(data), 1, type, coap_header_get_token(request, token), token, code, coap_header_get_id(request));
	if (err < 0) {
		LOG_ERR("coap_packet_init: %d", err);
		return err;
	}

	if (payload != NULL && payload_len > 0) {
		err = coap_packet_append_payload_marker(&response);
		if (err < 0) {
			LOG_ERR("coap_packet_append_payload_marker: %d", err);
			return err;
		}

		err = coap_packet_append_payload(&response, payload, payload_len);
		if (err < 0) {
			LOG_ERR("coap_packet_append_payload: %d", err);
			return err;
		}
	}

	if (sendto(ep->sock, response.data, response.offset, 0, addr, addr_len) < 0) {
		LOG_ERR("sendto: %d", errno);
		return -errno;
	}

	return 0;
}
