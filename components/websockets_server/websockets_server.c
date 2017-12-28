#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_spi_flash.h"
#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/err.h"
#include "nvs_flash.h"
#include "crypto/base64.h"
#include "hwcrypto/sha.h"
#include "cJSON.h"
#include "mbedtls/sha1.h"

// websockets port
#define WEBSOCKETS_PORT 8080
// SHA1 is 20 bytes or 160-bit
#define SHA1_160 20
// Masking key is 4 bytes long
#define WEBSOCKETS_HEADER_MASKING_KEY 4

void websockets_serve_frames(struct netconn *newconn, char * data, uint16_t data_length) {
	// Masking key starts at the 3rd byte of the frame (data+2) if MASK (bit 8)  is 1
	char masking_key[WEBSOCKETS_HEADER_MASKING_KEY];
	strncpy(masking_key, data + 2, WEBSOCKETS_HEADER_MASKING_KEY);

	// Payload length is bit 9-15 of websocket frame
	int payload_length = data[1] & 0x7F;
	//printf("payload_length:%d\n", payload_length);

	// decoded will contain the unmasked payload
	char decoded[payload_length + 1];
	decoded[payload_length] = '\0';

	// First 2 bytes header, next 4 bytes masking key, masked payload starts after (assuming MASK (bit 8) is 1)
	const int start_of_payload_index = 6;

	// To get decoded, loop through the bytes of data and XOR the byte with the (i modulo 4)th byte of masking_key.
	for (int i = start_of_payload_index; i < start_of_payload_index + payload_length; i++) {
		char decoded_char = (data[i]) ^ (masking_key[(i - start_of_payload_index) % 4]);
		decoded[i - start_of_payload_index] = decoded_char;
	}

	printf("decoded:%s\n", decoded);
	printf("\n");

	/*
	 cJSON * root = cJSON_Parse(decoded);

	 cJSON *datapoints = cJSON_GetObjectItemCaseSensitive(root, "datapoints");
	 if (cJSON_IsNumber(datapoints)) {
	 printf("datapoints:%d\n",  datapoints->valueint);
	 }

	 cJSON *offset = cJSON_GetObjectItemCaseSensitive(root, "offset");
	 if (cJSON_IsNumber(offset)) {
	 printf("offset:%d\n",  offset->valueint);
	 }

	 cJSON *plot_type = cJSON_GetObjectItemCaseSensitive(root, "type");
	 printf("plot_type:%s\n",  plot_type->valuestring);

	 */

	char header[] = { 0x81, 0x5, 'w', 'e', 'e', 'd', 's' };

	vTaskDelay(1500 / portTICK_PERIOD_MS);
	err_t result = netconn_write(newconn, header, sizeof(header), NETCONN_COPY);
	if (result != ERR_OK) {
		printf("whoopsys\n");
		return;
	}
}

void websockets_handshake(struct netconn *newconn) {
	struct netbuf *incoming_netbuf;
	char *data;
	uint16_t data_length;

	const char WEBSOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	const char WEBSOCKET_KEY[] = "Sec-WebSocket-Key: ";
	const char WEBSOCKET_RSP[] =
			"HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.*s\r\n\r\n";

	err_t err = netconn_recv(newconn, &incoming_netbuf);
	if (err != ERR_OK || incoming_netbuf == NULL) {
		return;
	}
	err = netbuf_data(incoming_netbuf, (void **) &data, &data_length);
	if (err != ERR_OK) {
		return;
	}
	// Find the start of the sec_websocket_key value in data
	char *sec_websocket_key_start = strstr(data, WEBSOCKET_KEY)
			+ sizeof(WEBSOCKET_KEY) - 1;

	// The key is everything before CRLF
	char *sec_websocket_key = strtok(sec_websocket_key_start, "\r\n");
	//printf("sec_websocket_key:%s\n", sec_websocket_key);

	// Concatenate Magic GUID to sec_websocket_key field
	char *new_sec_websocket_key = strcat(sec_websocket_key, WEBSOCKET_GUID);
	//printf("new_sec_websocket_key:%s\n", new_sec_websocket_key);

	// Take SHA1
	uint8_t sha_result[SHA1_160];
	esp_sha(SHA1, (unsigned char *) new_sec_websocket_key,
			strlen(new_sec_websocket_key), sha_result);
	/*
	 printf("SHA1:");
	 for (int i = 0; i < SHA1_160; i++) {
	 printf("%x ", sha_result[i]);
	 }
	 printf("\n");
	 */

	//Then base64
	uint32_t s; // base64 encoded_key length
	unsigned char *encoded_key = base64_encode(sha_result, sizeof(sha_result), &s);
	//printf("Base64:%s\n", encoded_key);

	//Return the websockets handshake response
	char buf[256];
	sprintf(buf, WEBSOCKET_RSP, s - 1, encoded_key);
	printf("Websocket handshake response:%s\n", buf);
	vTaskDelay(1500 / portTICK_PERIOD_MS);
	netconn_write(newconn, buf, strlen(buf), NETCONN_COPY);

	// Now ready to accept websocket traffic from client
	while (true) {
		// blocking until get some websocket traffic from client
		err_t err = netconn_recv(newconn, &incoming_netbuf);
		if (err != ERR_OK) {
			continue;
		}
		// data contains websocket frame
		err = netbuf_data(incoming_netbuf, (void **) &data, &data_length);

		if (err != ERR_OK) {
			continue;
		}

		if (data_length == 0) {
			printf("no");
			continue;
		}

		if (data_length == 32) {
			printf("Something went wrong:%s\n", data);
			continue;
		}

		websockets_serve_frames(newconn, data, data_length);

	}

	netconn_close(newconn);
	netbuf_delete(incoming_netbuf);
	printf("exiting...\n");
}

void websockets_callback(struct netconn *conn, enum netconn_evt event, uint16_t len) {
	switch (event) {
	case NETCONN_EVT_RCVPLUS:
		printf("receive plus\n");
		break;
	case NETCONN_EVT_RCVMINUS:
		printf("receive minus\n");
		break;
	case NETCONN_EVT_SENDPLUS:
		printf("send plus\n");
		break;
	case NETCONN_EVT_SENDMINUS:
		printf("send minus\n");
		break;
	default:
		printf("None of the above");

	}
}

void websockets_server(void *pvParameters) {
	struct netconn *conn, *newconn;

	//conn = netconn_new_with_callback(NETCONN_TCP, websockets_callback);
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, WEBSOCKETS_PORT);
	netconn_listen(conn);

	while (netconn_accept(conn, &newconn) == ERR_OK) {
		printf("Accepting new websocket connection...\n");
		websockets_handshake(newconn);
		printf("Leaving websocket connnection...\n");
	}

	netconn_close(conn);
	netconn_delete(conn);
	vTaskDelete(NULL);
}

