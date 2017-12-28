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

#define WEBSOCKETS_PORT 8080
#define SHA1_160 20

void websockets_handshake(struct netconn *newconn) {
	struct netbuf *incoming_netbuf;
	char *data;
	uint16_t data_length;

	const char WEBSOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	const char WEBSOCKET_KEY[] = "Sec-WebSocket-Key: ";
	const char WEBSOCKET_RSP[] = "HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.*s\r\n\r\n";

	err_t err = netconn_recv(newconn, &incoming_netbuf);
	if (incoming_netbuf != NULL && err == ERR_OK) {
		err = netbuf_data(incoming_netbuf, (void **) &data, &data_length);
		if (err == ERR_OK) {
			// Find the start of the sec_websocket_key value in data
			char *sec_websocket_key_start = strstr(data, WEBSOCKET_KEY)
					+ sizeof(WEBSOCKET_KEY) - 1;

			// The key is everything before CRLF
			char *sec_websocket_key = strtok(sec_websocket_key_start, "\r\n");
			printf("sec_websocket_key:%s\n", sec_websocket_key);

			// Concatenate Magic GUID to sec_websocket_key field
			char *new_sec_websocket_key = strcat(sec_websocket_key,
					WEBSOCKET_GUID);
			printf("new_sec_websocket_key:%s\n", new_sec_websocket_key);

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
			uint32_t s;
			unsigned char *encoded_key = base64_encode(sha_result,
					sizeof(sha_result), &s);
			//printf("Base64:%s\n", encoded_key);

			//Return the websockets handshake response
			char buf[256];
			//snprintf(buf, sizeof(buf), WEBSOCKET_RSP, encoded_key);
			sprintf(buf,  WEBSOCKET_RSP, s - 1, encoded_key);
			printf("Websocket handshake response:%s\n", buf);
			vTaskDelay(1500/portTICK_PERIOD_MS);
			netconn_write(newconn, buf, strlen(buf), NETCONN_COPY);


			while (true) {
				err_t err = netconn_recv(newconn, &incoming_netbuf);
				if (err == ERR_OK) {
					printf("received something from websockets\n");
					err = netbuf_data(incoming_netbuf, (void **) &data,
							&data_length);

					if (err != ERR_OK) {
						printf("Websockets not happy Jan!");
					}

					if (data_length == 0) {
						printf("no");
						continue;
					}


					 if (data_length == 32) {
					 printf("Something went wrong:%s\n", data);
					 continue;
					 }


					char masking_key[4];

					printf("data_length:%d", data_length);

					strncpy(masking_key, data + 2, 4);


					int payload_length = data[1] & 0x7F;
					printf("payload_length:%d\n", payload_length);

					char decoded[payload_length + 1];
					decoded[payload_length] = '\0';

					for (int i = 6; i < 6 + payload_length; i++) {
						char decoded_char = (data[i]) ^ (masking_key[(i - 6) % 4]);
						decoded[i - 6] = decoded_char;
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

					//float f = 1500/portTICK_PERIOD_MS;
					//printf("%f\n", f);
					//vTaskDelay(1500/portTICK_PERIOD_MS);
					char header[] = {0x81, 0x5, 'w', 'e', 'e','d','s'};

					vTaskDelay(1500/portTICK_PERIOD_MS);
					err_t result = netconn_write(newconn, header, sizeof(header), NETCONN_COPY);
					if ( result != ERR_OK) {
					  printf("whoopsys\n");
					  continue;
					}
					//netconn_write(newconn, payload, strlen(payload), NETCONN_COPY);
					//vTaskDelay(1500/portTICK_PERIOD_MS);
					//char headertwo[2] = {0x88, 0x2};
					//netconn_write(newconn, headertwo, sizeof(headertwo), NETCONN_COPY);
					printf("got here...\n");
				}

			}
		}
	}

	netconn_close(newconn);
	netbuf_delete(incoming_netbuf);
	printf("exiting...\n");
}

void websockets_callback(struct netconn *conn, enum netconn_evt event,
		uint16_t len) {
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

	printf("Starting websockets server...");

	//conn = netconn_new_with_callback(NETCONN_TCP, websockets_callback);
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, WEBSOCKETS_PORT);
	netconn_listen(conn);
	printf("listening");
	while (netconn_accept(conn, &newconn) == ERR_OK) {
		printf("something coming in...\n");
		websockets_handshake(newconn);
		printf("going out...\n");
	}

	printf("Ending websockets server...");

	netconn_close(conn);
	netconn_delete(conn);
	vTaskDelete(NULL);
}

