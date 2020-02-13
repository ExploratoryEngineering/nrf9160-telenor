#pragma once

#include <net/coap.h>

// A coap_endpoint is a CoAP client and server.
typedef struct _coap_endpoint coap_endpoint;

// coap_endpoint_init creates a CoAP endpoint and starts listening for requests.
coap_endpoint *coap_endpoint_init(struct sockaddr *local_addr, socklen_t local_addr_len, struct coap_resource *resources);

// A coap_response_handler is called when a response is received or an error occurs.
// The response packet is freed after the handler returns, so the handler must copy any data it wants to retain.
// It is the responsibility of the response handler to acknowledge or reject the response.
typedef void (*coap_response_handler)(void *handler_data, int err, struct coap_packet *response);

// coap_endpoint_post performs a POST request towards the given address with the given path and payload.
// It returns an enum coap_response_code or a negative error code.
int coap_endpoint_post(coap_endpoint *ep, struct sockaddr *addr, socklen_t addr_len, const char *const *path, u8_t *payload, int payload_len);

// coap_endpoint_post_async asynchronously performs a POST request towards the given address with the given path and payload.
int coap_endpoint_post_async(coap_endpoint *ep, struct sockaddr *addr, socklen_t addr_len, const char *const *path, u8_t *payload, int payload_len, coap_response_handler response_handler, void *response_handler_data);

// coap_endpoint_respond responds to a CoAP request.  The response is piggybacked on a acknowledgement if the request was confirmable.
// This function must only be called from a request handler, not a different thread.
int coap_endpoint_respond(coap_endpoint *ep, struct coap_packet *request, enum coap_response_code code, u8_t *payload, u16_t payload_len, struct sockaddr *addr, socklen_t addr_len);

// coap_endpoint_acknowledge send an acknowledgement for the given packet if it is confirmable, otherwise it does nothing.
int coap_endpoint_acknowledge(coap_endpoint *ep, struct coap_packet *packet, struct sockaddr *addr, socklen_t addr_len);

// coap_endpoint_reset sends a reset message for the given packet.
int coap_endpoint_reset(coap_endpoint *ep, struct coap_packet *packet, struct sockaddr *addr, socklen_t addr_len);
