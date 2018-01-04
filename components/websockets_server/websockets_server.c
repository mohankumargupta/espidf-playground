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
#include "cJSON.h"

// websockets port
#define WEBSOCKETS_PORT 8080
// SHA1 is 20 bytes or 160-bit
#define SHA1_160 20
// Masking key is 4 bytes long
#define WEBSOCKETS_HEADER_MASKING_KEY 4

#define WEB_DELAY_IN_MS 1500

static const char* TAG = "websocket_server";
static const char* FRAME_TAG = "websocket_server_frames";

/*  Function called after websocket handshake is completed.
 *
 */
void websockets_serve_frames(struct netconn *newconn) {
	struct netbuf *incoming_netbuf;
	char *data; //Websocket payload usually masked by client
	uint16_t data_length;

	while (1) {
		ESP_LOGI(FRAME_TAG, "Entering netconn_recv...");
		ESP_LOGI(FRAME_TAG, "Heap size: %d", xPortGetFreeHeapSize());
		// blocking until get some websocket traffic from client
		err_t err = netconn_recv(newconn, &incoming_netbuf);
		if (err != ERR_OK) {
			continue;
		}
		// data contains websocket frame
		err = netbuf_data(incoming_netbuf, (void **) &data, &data_length);

		if (data_length == 0) {
			return;
		}

		if (data_length > 0)
				{
			//printf("websocket frame first byte: 0x%02x\n", data[0]);
			//printf("websocket frame second byte: 0x%02x\n", data[1]);
			if (data[0] != 0x81) {
				return;
			}

		}

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
		for (int i = start_of_payload_index;
				i < start_of_payload_index + payload_length; i++) {
			char decoded_char = (data[i]) ^ (masking_key[(i - start_of_payload_index) % 4]);
			decoded[i - start_of_payload_index] = decoded_char;
		}

		printf("decoded:%s\n", decoded);
		printf("\n");

		/*
		 *  the incoming json has this form
		 *  {
		 *   "datapoints": 10,
		 *   "type": "scatter",
		 *   "offset": 100
		 *  }
		 *
		 */
		cJSON *root = cJSON_Parse(decoded);

		cJSON *datapoints = cJSON_GetObjectItemCaseSensitive(root,
				"datapoints");
		if (cJSON_IsNumber(datapoints)) {
			printf("datapoints:%d\n", datapoints->valueint);
		}

		cJSON *offset = cJSON_GetObjectItemCaseSensitive(root, "offset");
		if (cJSON_IsNumber(offset)) {
			printf("offset:%d\n", offset->valueint);
		}

		cJSON *plot_type = cJSON_GetObjectItemCaseSensitive(root, "type");
		printf("plot_type:%s\n", plot_type->valuestring);

		int number_of_datapoints = datapoints->valueint;
		int x_axis[number_of_datapoints];
		for (int i = 1; i <= number_of_datapoints; i++) {
			x_axis[i - 1] = i;
		}
		int y_axis[number_of_datapoints];
		for (int i = 0; i < number_of_datapoints; i++) {
			y_axis[i] = 110 + i;
		}

		cJSON *x = cJSON_CreateIntArray(x_axis, number_of_datapoints);
		cJSON *y = cJSON_CreateIntArray(y_axis, number_of_datapoints);

		cJSON_AddItemToObject(root, "x", x);
		cJSON_AddItemToObject(root, "y", y);

		//printf("%s\n", cJSON_Print(root));
		char *json = cJSON_Print(root);
		//printf("JSON:%s\n JSON Length:%d\n", json, strlen(json));

		uint16_t send_payload_length = strlen(json);

		printf("Send payload length: %d\n", send_payload_length);

		//char header[2];
		char *header = NULL;

		uint8_t header_length = 0;

		if (send_payload_length <= 125) {
			header_length = 2;
			header = (char *) malloc(header_length);
			header[0] = 0x81;
			header[1] = send_payload_length;

		}

		else if (send_payload_length > 125 && send_payload_length < 65535) {
			header_length = 4;
			header = (char *) malloc(header_length);
			header[0] = 0x81;
			header[1] = 126;
			header[2] = (send_payload_length >> 8) & 0xFF;
			header[3] = send_payload_length & 0xFF;
		}

		// Don't handle websocket frames bigger anf 65536 bytes
		else {
			ESP_LOGE(TAG, "Frame Payload size bigger than 65535 bytes");
			return;
		}


		printf("Websocket frame send header: 0x%02x 0x%02x\n", header[0], header[1]);
		printf("Websocket frame send payload: %s\n", json);
		vTaskDelay(1500 / portTICK_PERIOD_MS);
		err_t result = netconn_write(newconn, header, header_length,
				NETCONN_COPY);
		vTaskDelay(1500 / portTICK_PERIOD_MS);
		result = netconn_write(newconn, json, strlen(json), NETCONN_COPY);

		free(json);
		cJSON_Delete(root);

		if (result != ERR_OK) {
			printf("whoopsys\n");

		}

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

