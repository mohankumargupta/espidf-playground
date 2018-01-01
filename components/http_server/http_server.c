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
#include "servefrom_sdcard.h"


#define HTTP_PORT 80
#define SHA1_160 20

char* get_mimetype(char *path) {
	char *file_extension = strchr(path, '.');
    if (file_extension != NULL) {
    	if (strcmp(file_extension,".txt") == 0) {
    		return "text/plain";
    	}
    	else if (strcmp(file_extension,".css") == 0) {
    		return "text/css";
    	}
    	else if (strcmp(file_extension,".js") == 0) {
    		return "text/javascript";
    	}
    	else if (strcmp(file_extension,".png") == 0) {
    		return "image/png";
    	}
    }

    return NULL;

}

void http_serve_request(struct netconn *newconn) {
    char html[] = "HTTP/1.0 200 OK\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: text/html\n\n<!doctype html><html><body><div><a href=\"/boo\">Response 1</a></div><div><a></a></div></body></html>\n\n";
    char firstpage_html[] = "HTTP/1.0 200 OK\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: text/html\n\n<html><body><div><h1>Page 1</h1><div><a href=\"/\">Back</a></div></body></html>\n\n";
    char errorpage_404[] = "HTTP/1.0 404 Not Found\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: text/html\n\n";
    char known_mimetype_html[] = "HTTP/1.0 200 OK\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: %s\n\n%s\n\n";

    struct netbuf *incoming_netbuf;
    char *data;
    uint16_t data_length;

    err_t err = netconn_recv(newconn, &incoming_netbuf);
    if (incoming_netbuf != NULL && err==ERR_OK) {
    	  err = netbuf_data(incoming_netbuf, (void **) &data, &data_length );
    	  if (err == ERR_OK) {

    		  /*
    		   * Simplified HTTP Request Parsing
    		   * --------------------------------
    		   * HTTP GET starts with:
    		   *
    		   * GET /boo HTTP/1.1
    		   *
    		   * To extract the path(/boo):
    		   * 1. start of path begins the character following the first space
    		   * 2. end of path happens preceding the next space
    		   */

    		  const char *start_of_path = strchr(data, ' ') + sizeof(char);
    		  const char *end_of_path = strchr(start_of_path, ' ');

    		  int path_length = end_of_path - start_of_path;
    		  char path[path_length + 1];
    		  path[path_length] = 0; // null character terminator, character arrays don't have them
    		  strncpy(path, start_of_path, path_length);

    		  if (strcmp(path,"/") == 0) {
    			  netconn_write(newconn, html, sizeof(html), NETCONN_COPY);
    		  }

    		  else if (strcmp(path, "/boo") == 0) {
    			  netconn_write(newconn, firstpage_html, sizeof(firstpage_html), NETCONN_COPY);
    		  }

    		  //else if (strcmp(path, "/boo.txt") == 0) {
    		  else {
    			    char *filecontents;
    			    //unsigned int file_length;
    			    filecontents = serve_file_from_sdcard(path);
    			    //printf("File Length:%u\n", file_length);
    			    if (filecontents == NULL) {
    			    	netconn_write(newconn, errorpage_404, sizeof(errorpage_404), NETCONN_COPY);
    			    }
    			    else {
    			    	printf("Contents of file %s:%s\n", path, filecontents);
    			    	//netconn_write(newconn, errorpage_404, sizeof(errorpage_404), NETCONN_COPY);
    			    	//char returnhtml[13];

    			    	char mime[] = "text/plain";
    			    	size_t needed = snprintf(NULL, 0, known_mimetype_html, mime, filecontents);
    			    	printf("size:%u\n", needed);
    			    	char *returnhtml = (char *) malloc(needed * sizeof(char) + 1);

    			    	//char *returnhtml;

    			    	sprintf(returnhtml, known_mimetype_html, mime, filecontents);
    			    	netconn_write(newconn, returnhtml, strlen(returnhtml), NETCONN_COPY);
                        printf("Does it crash here?");


                        //char *mime = get_mimetype(path);
    			    	//char *mime = "text/plain";


    			    	//printf("Does it get here before crash?");
    			    	//sprintf(returnhtml, known_mimetype_html, mime, filecontents);
    			    	//printf(returnhtml);
    			    	//netconn_write(newconn, returnhtml, strlen(returnhtml), NETCONN_COPY);
    			    	free(filecontents);
    			    	//free(returnhtml);
    			    }
    		}



    		  //unsigned int s;
    		  //const char *input_key = "dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    		  //uint8_t sha_result[SHA1_160];
    		  //esp_sha(SHA1, (unsigned char *) input_key, strlen(input_key),  sha_result);
    		  //unsigned char *raw = (unsigned char *) input;
    		  //unsigned char *encoded =  base64_encode(raw, strlen(input), (size_t *) &s);



    		  //printf("Request received:\n%s\n", path);
    		  //printf("Path length:%d\n", path_length);
    		  //printf("SHA1:");
    		  //for (int i=0;i<SHA1_160; i++) {
    			  //printf("%x ", sha_result[i]);
    		  //}
    		  //printf("\n");
    		  //printf("Base64 encode of abc - size:%d answer:%s ", s, encoded);
    	  }
    }

    printf("About to close connection\n");
    netconn_close(newconn);
    printf("About to delete netbuf\n");
    netbuf_delete(incoming_netbuf);

}

void return_html_sdcard() {

}

void http_server(void *pvParameters) {
    struct netconn *conn, *newconn;

    sdcard_init();

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










