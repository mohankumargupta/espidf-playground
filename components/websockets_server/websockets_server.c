#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
//#include "esp_spi_flash.h"
#include "esp_log.h"
#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/err.h"
//#include "nvs_flash.h"
#include "crypto/base64.h"
#include "hwcrypto/sha.h"
//#include "cJSON.h"
#include "process_websocket_frame.h"

#define WEBSOCKETS_PORT 8080 // websockets port
#define SHA1_160 20 // SHA1 is 20 bytes or 160-bit
#define WEBSOCKETS_HEADER_MASKING_KEY 4 // Masking key is 4 bytes long
#define WEB_DELAY_IN_MS 1500

static const char* TAG = "websocket_server";
static const char* FRAME_TAG = "websocket_server_frames";

// Process incoming websocket frame
void websockets_serve_frame(struct netconn *newconn) {
	struct netbuf *incoming_netbuf;
	char *data; //Websocket payload usually masked by client
	uint16_t data_length;
	uint16_t masking_key_index = 0;
	uint16_t payload_length = 0;

	ESP_LOGI(FRAME_TAG, "Entering netconn_recv...");
	ESP_LOGI(FRAME_TAG, "Heap size: %d", xPortGetFreeHeapSize());
	// blocking until get some websocket traffic from client
	err_t err = netconn_recv(newconn, &incoming_netbuf);
	if (err != ERR_OK) {
		return;
	}
	// data contains websocket frame
	err = netbuf_data(incoming_netbuf, (void **) &data, &data_length);

	if (data_length == 0) {
		return;
	}

	if (data_length > 0) {
		printf("websocket frame first byte: 0x%02x\n", data[0]);
		printf("websocket frame second byte: 0x%02x\n", data[1]);
		if (data[0] != 0x81) {
			return;
		}

		if (data_length < 126) {
			masking_key_index = 2;
			// Payload length is bit 9-15 of websocket frame
			payload_length = data[1] & 0x7F;

		}

		else {
			masking_key_index = 4;
			// Payload length is bit 16-31 of websocket frame
			payload_length = data[2] << 8;
			payload_length += data[3];
		}

		printf("payload_length:%d\n", payload_length);

	}

	// Masking key starts at the 3rd byte of the frame (data+2) if MASK (bit 8)  is 1
	char masking_key[WEBSOCKETS_HEADER_MASKING_KEY];
	strncpy(masking_key, data + masking_key_index, WEBSOCKETS_HEADER_MASKING_KEY);



	// the payload of the incoming websocket frame are masked with the masking key
	char unmasked_payload[payload_length + 1];
	unmasked_payload[payload_length] = '\0';

	// First 2 bytes header,then optionally , next 4 bytes masking key, masked payload starts after (assuming MASK (bit 8) is 1)
	int start_of_payload_index = masking_key_index + 4;

	// To unmask, loop through the bytes of data and XOR the byte with the (i modulo 4)th byte of masking_key.
	for (int i = start_of_payload_index;
			i < start_of_payload_index + payload_length; i++) {
		char decoded_char = (data[i]) ^ (masking_key[(i - start_of_payload_index) % 4]);
		unmasked_payload[i - start_of_payload_index] = decoded_char;
	}

	//printf("Unmasked Payload:%s\n", unmasked_payload);

	// Process incoming json and prepare response json
	char *json = process_incoming_websocket_frame_payload(unmasked_payload);

	if (json == NULL) {
		return;
	}

	uint16_t send_payload_length = strlen(json);
	//printf("Send payload length: %d\n", send_payload_length);
	char *header = NULL;
	uint8_t header_length = 0;

	// websocket protocol: byte 1 of header contains payload length if payload length <= 125
	if (send_payload_length <= 125) {
		header_length = 2;
		header = (char *) malloc(header_length);
		header[0] = 0x81;
		header[1] = send_payload_length;

	}

	// websocket protocol: byte 1 contains 126
	//                     extended payload length field: bytes 2 and 3
	else if (send_payload_length > 125 && send_payload_length < 65535) {
		header_length = 4;
		header = (char *) malloc(header_length);
		header[0] = 0x81;
		header[1] = 126;
		header[2] = (send_payload_length >> 8) & 0xFF;
		header[3] = send_payload_length & 0xFF;
	}

	// Don't handle websocket frames bigger than 65535 bytes
	else {
		ESP_LOGE(TAG, "Frame Payload size bigger than 65535 bytes");
		return;
	}

	//printf("Websocket frame send header: 0x%02x 0x%02x\n", header[0], header[1]);
	//printf("Websocket frame send payload: %s\n", json);
	//vTaskDelay(1500 / portTICK_PERIOD_MS);
	err_t result = netconn_write(newconn, header, header_length, NETCONN_COPY);
	//vTaskDelay(1500 / portTICK_PERIOD_MS);
	result = netconn_write(newconn, json, strlen(json), NETCONN_COPY);

	if (header != NULL) {
		free(header);
	}

	if (json != NULL) {
		free(json);
	}

	if (result != ERR_OK) {
		printf("whoopsys\n");

	}

}

/*  Function called after websocket handshake is completed.
 *
 */
void websockets_serve_frames(struct netconn *newconn) {
	while (1) {
		websockets_serve_frame(newconn);
	}
}

/*
 * Function called when there is a HTTP websocket request from client.
 *
 * newconn: netconn connection object
 * data: HTTP request
 * data_length: Length of HTTP Request
 */
void websockets_handshake(struct netconn *newconn, char *data,
		uint32_t data_length) {
	const char WEBSOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	const char WEBSOCKET_KEY[] = "Sec-WebSocket-Key: ";
	const char WEBSOCKET_RSP[] =
			"HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.*s\r\n\r\n";

	ESP_LOGI(TAG, "Entering websocket handshake...");
	ESP_LOGI(TAG, "Heap size: %d", xPortGetFreeHeapSize());

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
	unsigned char *encoded_key = base64_encode(sha_result, sizeof(sha_result),
			&s);
	//printf("Base64:%s\n", encoded_key);

	//Return the websockets handshake response
	char buf[256];
	sprintf(buf, WEBSOCKET_RSP, s - 1, encoded_key);
	printf("Websocket handshake response:\n%s\n", buf);
	vTaskDelay(1500 / portTICK_PERIOD_MS);
	netconn_write(newconn, buf, strlen(buf), NETCONN_COPY);
	websockets_serve_frames(newconn);
}

void websockets_handle_request(struct netconn *newconn) {

	struct netbuf *incoming_netbuf;
	char *data;
	uint16_t data_length;
	const char WEBSOCKET_HANDSHAKE_UPGRADE[] = "Upgrade: websocket";

	// Grab incoming data and data_length
	err_t err = netconn_recv(newconn, &incoming_netbuf);
	if (err != ERR_OK || incoming_netbuf == NULL) {
		printf("websocket_request: netconn_recv error");
		return;
	}
	err = netbuf_data(incoming_netbuf, (void **) &data, &data_length);
	if (err != ERR_OK) {
		printf("websocket_request: netbuf_data error");
		return;
	}

	//printf("Websocket Handshake Data:\n%s\n", data);
	//printf("Websocket Data Length: %d\n", data_length);

	if (strstr(data, WEBSOCKET_HANDSHAKE_UPGRADE) != NULL) {
		ESP_LOGI(TAG, "Websocket handshake incoming...");
		websockets_handshake(newconn, data, data_length);
	}


}

void websockets_server(void *pvParameters) {
	struct netconn *conn, *newconn;

	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, WEBSOCKETS_PORT);
	netconn_listen(conn);

	while (netconn_accept(conn, &newconn) == ERR_OK) {
		ESP_LOGI(TAG, "Accepting new websocket connection...");
		websockets_handle_request(newconn);
		ESP_LOGI(TAG, "Leaving websocket connnection...");
		vTaskDelay(WEB_DELAY_IN_MS / portTICK_PERIOD_MS);
	}

	netconn_close(conn);
	netconn_delete(conn);
	vTaskDelete(NULL);
}

