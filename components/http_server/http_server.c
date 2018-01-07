#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/err.h"
#include "crypto/base64.h"
#include "hwcrypto/sha.h"
//#include "cJSON.h"
#include "servefrom_sdcard.h"

#define HTTP_PORT 80
#define SHA1_160 20

static const char *TAG = "http_server";

// Received an incoming request
void http_serve_request(struct netconn *newconn) {
	char index_html[] =
			"HTTP/1.0 200 OK\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: text/html\n\n<!doctype html><html><body><div><a href=\"/boo\">Response 1</a></div><div><a></a></div></body></html>\n\n";
	char boo_html[] =
			"HTTP/1.0 200 OK\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: text/html\n\n<html><body><div><h1>Page 1</h1><div><a href=\"/\">Back</a></div></body></html>\n\n";
    struct netbuf *incoming_netbuf;
	char *data; // Contains HTTP GET request header
	uint16_t data_length; // length of HTTP GET request header

    err_t err = netconn_recv(newconn, &incoming_netbuf);
    if (incoming_netbuf != NULL && err==ERR_OK) {
    	  err = netbuf_data(incoming_netbuf, (void **) &data, &data_length );
    	  if (err == ERR_OK) {

			/*
			 * HTTP Request Parsing
			 * --------------------------------
			 * HTTP GET starts with:
			 *
			 * GET /boo HTTP/1.1
			 *
			 * To extract the path(/boo):
			 * 1. start_of_path begins the character following the first space
			 * 2. end_of_path points to the next space. Used to calculate path length (4 in the example above)
			 */

    		  const char *start_of_path = strchr(data, ' ') + sizeof(char);
    		  const char *end_of_path = strchr(start_of_path, ' ');
    		  int path_length = end_of_path - start_of_path;

    		  char path[path_length + 1];
    		  path[path_length] = '\0'; // null character terminator, character arrays don't have them
    		  strncpy(path, start_of_path, path_length);

    		  if (strcmp(path,"/") == 0) {
				// Serve from string constant
				netconn_write(newconn, index_html, sizeof(index_html), NETCONN_COPY);
    		  }


    		  else if (strcmp(path, "/boo") == 0) {
				// Serve from string constant
				netconn_write(newconn, boo_html, sizeof(boo_html), NETCONN_COPY);
    		  }

    		  else {
    			    // Try to serve file from SD card
    			    serve_file_from_sdcard(newconn, path);
    		}


    	  }
    }

	ESP_LOGI(TAG, "About to close connection");
    netconn_close(newconn);
	ESP_LOGI(TAG, "About to delete netbuf");
    netbuf_delete(incoming_netbuf);

}

void http_server(void *pvParameters) {
    struct netconn *conn, *newconn;

    sdcard_init();

	ESP_LOGI(TAG, "Starting web server...");
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, HTTP_PORT);
    netconn_listen(conn);

    while(netconn_accept(conn, &newconn) == ERR_OK) {
    	  http_serve_request(newconn);
    }

	ESP_LOGI(TAG, "Ending web server...");

    netconn_close(conn);
    netconn_delete(conn);
    vTaskDelete(NULL);
}










