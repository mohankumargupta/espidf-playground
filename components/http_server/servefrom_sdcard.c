/* SD card and FAT filesystem example.
 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/err.h"


#define BUFFER_CHUNK_SIZE 1000

static const char *TAG = "servefrom_sdcard";

// Default pin mapping
//#define PIN_NUM_MISO 2
//#define PIN_NUM_MOSI 15
//#define PIN_NUM_CLK  14
//#define PIN_NUM_CS   13

// Using hardware SPI pins
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// Attempt to mount SD card at /sdcard. Then can use FAT file operations.
void sdcard_init() {
	ESP_LOGI(TAG, "Using SD card in SPI mode");
	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
	slot_config.gpio_miso = PIN_NUM_MISO;
	slot_config.gpio_mosi = PIN_NUM_MOSI;
	slot_config.gpio_sck = PIN_NUM_CLK;
	slot_config.gpio_cs = PIN_NUM_CS;

	esp_vfs_fat_sdmmc_mount_config_t mount_config =
	{
				.format_if_mount_failed = false,
				.max_files = 5
	};

	sdmmc_card_t* card;
	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config,
			&mount_config, &card);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount filesystem. ");
		} else {
			ESP_LOGE(TAG, "Failed to initialize the card (%d). "
					"Make sure SD card lines have pull-up resistors in place.",
					ret);
		}
		return;
	}

	// Card has been initialized, print its properties
	sdmmc_card_print_info(stdout, card);
}

// HTTP Response payload
void http_send_header(struct netconn *newconn, char *mime, uint32_t length) {
	const char *header = "HTTP/1.0 200 OK\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: %s\nLength: %d\n\n";
    char response_header[100];
    sprintf(response_header, header, mime, length);
    netconn_write(newconn, response_header, strlen(response_header), NETCONN_COPY);
}

// HTTP Response payload
void http_send_payload(struct netconn *newconn, char *payload, uint32_t payload_size) {
	netconn_write(newconn, payload, payload_size, NETCONN_NOCOPY);
}

// Signals end of HTTP Response
void http_send_footer(struct netconn *newconn) {
	char response_footer[] = "\n\n";
	netconn_write(newconn, response_footer, strlen(response_footer), NETCONN_COPY);
}

// HTTP Response requires Content-Type to be set.
char* get_mimetype(char *path) {
	char *file_extension = strchr(path, '.');
    if (file_extension != NULL) {
    	if (strcmp(file_extension,".txt") == 0) {
    		return "text/plain";
    	}
    	else if (strcmp(file_extension, ".html") == 0) {
			return "text/html";
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
		else if (strcmp(file_extension, ".jpg") == 0) {
			return "image/jpeg";
		}
    }

    return NULL;

}

// Received request, possibly for resource located on SD card. Path is HTTP GET url (eg. /test.png)
void serve_file_from_sdcard(struct netconn *newconn, char *path) {
	char errorpage_404[] = "HTTP/1.0 404 Not Found\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: text/html\n\n";
	FRESULT fr;
	FILINFO fno;
	FIL fil;

	printf("Test for file: %s\n", path);
	fr = f_stat(path, &fno);
	switch (fr) {

	case FR_OK:
		printf("Size: %lu\n", fno.fsize);
		printf("Timestamp: %u/%02u/%02u, %02u:%02u\n",
				(fno.fdate >> 9) + 1980, fno.fdate >> 5 & 15, fno.fdate & 31,
				fno.ftime >> 11, fno.ftime >> 5 & 63);
		printf("Attributes: %c%c%c%c%c\n",
				(fno.fattrib & AM_DIR) ? 'D' : '-',
				(fno.fattrib & AM_RDO) ? 'R' : '-',
				(fno.fattrib & AM_HID) ? 'H' : '-',
				(fno.fattrib & AM_SYS) ? 'S' : '-',
				(fno.fattrib & AM_ARC) ? 'A' : '-');

		fr = f_open(&fil, path, FA_READ);
		ESP_LOGI(TAG, "File open from SD card return code:%d", fr)
		;
		printf("Heap size: %d\n", xPortGetFreeHeapSize());

		//char *buf = (char*) malloc(fno.fsize);

		/*
		if (buf == NULL) {
			printf("sdcard:buf not allocated memory\n");
			return;
		}
		 */

		char *mime = get_mimetype(path);
		http_send_header(newconn, mime, fno.fsize);

		// Because of limited heap space, need to read file in chunks and send over wire
		uint32_t b;
		//uint32_t total_filesize_sent = 0;
		char buf[BUFFER_CHUNK_SIZE];
		while (1) {
			f_read(&fil, buf, BUFFER_CHUNK_SIZE, &b);

			//ESP_LOGI(TAG, "Heap size: %d", xPortGetFreeHeapSize());
			http_send_payload(newconn, buf, b);
			//total_filesize_sent = total_filesize_sent + b;
			//printf("Bytes read: %u...", total_filesize_sent);

			//If the number of bytes read is less than the number of bytes we requested to read,
			//it implies that the entire file has been read, so we are done
			if (b < BUFFER_CHUNK_SIZE) {
				break;
			}
		}
		printf("\n");

		http_send_footer(newconn);
		f_close(&fil);
		break;

	case FR_NO_FILE:
		printf("It is not exist.\n");
		netconn_write(newconn, errorpage_404, sizeof(errorpage_404), NETCONN_COPY);
		break;

	default:
		printf("An error occured. (%d)\n", fr);
	}

	return;
}

void sdcard_cleanup() {
	// All done, unmount partition and disable SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	ESP_LOGI(TAG, "Card unmounted");
}

