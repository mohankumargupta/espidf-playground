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

    const char WEBSOCKET_HEADER[] = "Upgrade: websocket\r\n";
    const char WEBSOCKET_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const char WEBSOCKET_KEY[] = "Sec-WebSocket-Key: ";
    const char WEBSOCKET_RSP[] = "HTTP/1.1 101 Switching Protocols\r\n" \
                          "Upgrade: websocket\r\n" \
                          "Connection: Upgrade\r\n" \
                          "Sec-WebSocket-Accept: %s\r\n\r\n";




    err_t err = netconn_recv(newconn, &incoming_netbuf);
    if (incoming_netbuf != NULL && err==ERR_OK) {
    	  err = netbuf_data(incoming_netbuf, (void **) &data, &data_length );
    	  if (err == ERR_OK) {

    		  // Find the start of the sec_websocket_key value in data
    		  char *sec_websocket_key_start = strstr(data, WEBSOCKET_KEY) + sizeof(WEBSOCKET_KEY) - 1;

    		  // The key is everything before CRLF
    		  char *sec_websocket_key = strtok(sec_websocket_key_start,"\r\n");
    		  printf("sec_websocket_key:%s\n", sec_websocket_key);

    		  // Concatenate Magic GUID to sec_websocket_key field
    		  char *new_sec_websocket_key = strcat(sec_websocket_key, WEBSOCKET_GUID);
    		  printf("new_sec_websocket_key:%s\n", new_sec_websocket_key);

    		  // Take SHA1
    		  uint8_t sha_result[SHA1_160];
    		  esp_sha(SHA1,  (unsigned char *)new_sec_websocket_key, strlen(new_sec_websocket_key),  sha_result);
          printf("SHA1:");
    		  for (int i=0;i<SHA1_160; i++) {
            printf("%x ", sha_result[i]);
        	  }
        	  printf("\n");


        	  //Then base64
        	  uint32_t s;
    		  unsigned char *encoded =  base64_encode(sha_result, sizeof(sha_result),  &s);
    		  printf("Base64:%s\n", encoded);

    	  }
    }

    netconn_close(newconn);
    netbuf_delete(incoming_netbuf);

}

void websockets_callback(struct netconn *conn, enum netconn_evt event, uint16_t len) {
  switch(event) {
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
    conn = netconn_new_with_callback(NETCONN_TCP, websockets_callback);
    netconn_bind(conn, NULL, WEBSOCKETS_PORT);
    netconn_listen(conn);
    while(netconn_accept(conn, &newconn) == ERR_OK) {
    	  websockets_handshake(newconn);
    }

    printf("Ending websockets server...");

    netconn_close(conn);
    netconn_delete(conn);
    vTaskDelete(NULL);
}










