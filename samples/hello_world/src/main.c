/*
 * Copyright (c) 2019 Exploratory Engineering
 */
#include <zephyr.h>
#include <errno.h>
#include <stdio.h>
#include <net/socket.h>
#include <modem_info.h>

bool print_imei_imsi() {
	if (modem_info_init() != 0) {
		printf("Modem info could not be initialized");
		return false;
	}

	u8_t buf[32] = {0};
	int len = modem_info_string_get(MODEM_INFO_IMEI, buf);
	if (len <= 0) {
		printf("Failed reading IMEI: %d\n", len);
		return false;
	}
	printf("IMEI: %s\n", buf);

	len = modem_info_string_get(MODEM_INFO_IMSI, buf);
	if (len <= 0) {
		printf("Failed reading IMSI: %d\n", len);
		return false;
	}
	printf("IMSI: %s\n", buf);

	return true;
}

bool send_message(const char *message) {
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printf("Error opening socket: %d\n", errno);
		return false;
	}

	static struct sockaddr_in remote_addr = {
		sin_family: AF_INET,
		sin_port:   htons(1234),
	};
	net_addr_pton(AF_INET, "172.16.15.14", &remote_addr.sin_addr);

	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
		printf("Error sending: %d\n", errno);
		close(sock);
		return false;
	}

	close(sock);
	return true;
}

void main() {
	print_imei_imsi();

	if (send_message("Hello, World!")) {
		printf("Message sent!\n"); 
	} else {
		printf("Failed to send\n");
	}

	printf("Example application complete\n"); 
}
