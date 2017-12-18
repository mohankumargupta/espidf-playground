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
    struct netbuf *buf;
    err_t err;
    
    struct netbuf *returnbuf = netbuf_new();
    char *data = netbuf_alloc(returnbuf, 12);
    
    strlcpy("Hello world!", data, 12);
    
    while((err = netconn_recv(newconn, &buf)) == ERR_OK) {
        printf("Request received!\n");
        netconn_send(newconn, returnbuf);
    }
    
    netconn_close(newconn);
}

void http_server(void *pvParameters) {
    struct netconn *conn, *newconn; 
    err_t err;
    
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, HTTP_PORT);
    netconn_listen(conn);
    do {
      err = netconn_accept(conn, &newconn);
      http_serve_request(newconn);
      netconn_delete(newconn);
    } while (err == ERR_OK);
    
    
    vTaskDelete(NULL);
}

void app_main() {
    nvs_flash_init();
    //wifi_ap();
    xTaskCreate(&http_server, "http_server", 8192, NULL, 5, NULL);
    
}






