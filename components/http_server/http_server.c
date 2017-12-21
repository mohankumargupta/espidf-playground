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


#define HTTP_PORT 80

void http_serve_request(struct netconn *newconn) {
    char html[] = "HTTP/1.0 200 OK\nDate: Thu, 21 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: text/html\n\n<html><body>Hello World!</body></html>\n\n";
    struct netbuf *incoming_netbuf;
    char *data;
    uint16_t data_length;

    err_t err = netconn_recv(newconn, &incoming_netbuf);
    if (incoming_netbuf != NULL && err==ERR_OK) {
    	  err = netbuf_data(incoming_netbuf, (void **) &data, &data_length );
    	  if (err == ERR_OK) {
    		  //printf("Request received:\n%s\n", data);
    	  }
    }
    printf("Response:\n%s\n", html);
    netconn_write(newconn, html, sizeof(html), NETCONN_COPY);
    netconn_close(newconn);
    netbuf_delete(incoming_netbuf);
}

void http_server(void *pvParameters) {
    struct netconn *conn, *newconn;

    printf("Starting web server...");
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, HTTP_PORT);
    netconn_listen(conn);
    while(netconn_accept(conn, &newconn) == ERR_OK) {
    	  http_serve_request(newconn);
    }

    printf("Ending web server...");

    netconn_close(conn);
    netconn_delete(conn);
    vTaskDelete(NULL);
}










