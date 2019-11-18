/*
 * Copyright (c) 2019 Exploratory Engineering
 */
#include <zephyr.h>
#include <errno.h>
#include <stdio.h>
#include <net/socket.h>

#if !defined(CONFIG_LTE_LINK_CONTROL)
#error "Missing CONFIG_LTE_LINK_CONTROL"
#endif /* !defined(CONFIG_LTE_LINK_CONTROL) */

bool send_message(const char *message) {
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printf("Error opening socket: %d\n", errno);
		return false;
	}

	struct addrinfo *ai;
	int err = getaddrinfo("172.16.15.14", NULL, NULL, &ai);
	if (err < 0) {
		printf("Error getting address info: %d\n", err);
		goto error;
	}

	struct sockaddr_in dest = *(struct sockaddr_in *)ai->ai_addr;
	dest.sin_port = htons(1234);

	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
		printf("Error sending: %d\n", errno);
		goto error;
	}

	close(sock);
	return true;

error:
	close(sock);
	return false;
}

int min(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

int exec_at_cmd(int sock, const char *cmd, char *response, int response_len) {
	if (send(sock, cmd, strlen(cmd), 0) < 0) {
		printf("Error sending: %d\n", errno);
		return -1;
	}

	char buf[1024];
	int n = recv(sock, buf, sizeof(buf), 0);
	if (n < 0) {
		printf("Error receiving: %d\n", errno);
		return -1;
	}

	int ret_response_len = 0;

	char *p = buf;
	char *end = buf + n;
	while (p < end) {
		if (memcmp(p, "ERROR", 5) == 0) {
			printf("Command '%s' responded with ERROR.\n", cmd);
			return -1;
		}
		if (memcmp(p, "OK", 2) == 0) {
			break;
		}

		char *next = memchr(p, '\r', end-p);
		if (next == NULL) {
			next = end;
		}
		if (response != NULL) {
			ret_response_len = min(response_len, next-p);
			memcpy(response, p, ret_response_len);
		}

		// skip "\r\n"
		p = next + 2;
	}

	return ret_response_len;
}

bool set_apn() {
	int sock = socket(AF_LTE, 0, NPROTO_AT);
	if (sock < 0) {
		printf("Error opening socket: %d\n", errno);
		return false;
	}

	if (exec_at_cmd(sock, "AT+CFUN=1", NULL, 0) < 0 ||
		exec_at_cmd(sock, "AT+CGATT=0", NULL, 0) < 0 ||
		exec_at_cmd(sock, "AT+CGDCONT=0,\"IP\",\"mda.ee\"", NULL, 0) < 0 ||
		exec_at_cmd(sock, "AT+CGDCONT?", NULL, 0) < 0 ||
		exec_at_cmd(sock, "AT+CGATT=1", NULL, 0) < 0) {
		goto error;
	}
	while (true) {
		char resp[32];
		int n = exec_at_cmd(sock, "AT+CEREG?", resp, sizeof(resp));
		if (n < 0) {
			goto error;
		}
		if (n >= 10 && memcmp(resp, "+CEREG", 6) == 0 && resp[10] == '1') {
			break;
		}
	}

	close(sock);
	return true;

error:
	close(sock);
	return false;
}

bool print_imei_imsi() {
	int sock = socket(AF_LTE, 0, NPROTO_AT);
	if (sock < 0) {
		printf("Error opening socket: %d\n", errno);
		return false;
	}

	char resp[16];
	int n = exec_at_cmd(sock, "AT+CGSN", resp, sizeof(resp));
	if (n < 0) {
		printf("Error getting IMEI.\n");
		goto error;
	}
	resp[n] = '\0';
	printf("IMEI: %s\n", resp);

	n = exec_at_cmd(sock, "AT+CIMI", resp, sizeof(resp));
	if (n < 0) {
		printf("Error getting IMSI.\n");
		goto error;
	}
	resp[n] = '\0';
	printf("IMSI: %s\n", resp);

	close(sock);
	return true;

error:
	close(sock);
	return false;
}

void main() {
	printf("Example application started.\n"); 

	if (!print_imei_imsi()) {
		printf("Failed to get IMEI/IMSI.\n");
		goto end;
	}

	if (!set_apn()) {
		printf("Failed to set APN.\n");
		goto end;
	}
	printf("Connected!\n"); 

	if (!send_message("Hello, World!")) {
		printf("Failed to send.\n");
		goto end;
	}
	printf("Message sent!\n"); 

end:
	printf("Tutorial application complete.\n"); 
}
