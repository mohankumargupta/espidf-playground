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
//#include "lwip/arch.h"
//#include "lwip/err.h"


static const char *TAG = "servefrom_sdcard";

// This example can use SDMMC and SPI peripherals to communicate with SD card.
// By default, SDMMC peripheral is used.
// To enable SPI mode, uncomment the following line:

#define USE_SPI_MODE

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.

//#define PIN_NUM_MISO 2
//#define PIN_NUM_MOSI 15
//#define PIN_NUM_CLK  14
//#define PIN_NUM_CS   13

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#endif //USE_SPI_MODE

#define TRUE 1
#define FALSE 0

void sdcard_init() {
	ESP_LOGI(TAG, "Using SPI peripheral");

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
	slot_config.gpio_miso = PIN_NUM_MISO;
	slot_config.gpio_mosi = PIN_NUM_MOSI;
	slot_config.gpio_sck = PIN_NUM_CLK;
	slot_config.gpio_cs = PIN_NUM_CS;

	esp_vfs_fat_sdmmc_mount_config_t mount_config = { .format_if_mount_failed =
			true, .max_files = 5 };

	sdmmc_card_t* card;
	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config,
			&mount_config, &card);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG,
					"Failed to mount filesystem. "
							"If you want the card to be formatted, set format_if_mount_failed = true.");
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

void http_send_header(struct netconn *newconn, char *mime, int length) {
	const char *header = "HTTP/1.0 200 OK\nDate: Fri, 22 Dec 2017 01:28:02 GMT\nServer: Esp32\nContent-Type: %s\nLength: %d\n\n";
    char response_header[100];
    sprintf(response_header, header, mime, length);
    netconn_write(newconn, response_header, strlen(response_header), NETCONN_COPY);
}

void http_send_payload(struct netconn *newconn, char *payload, int payload_size) {
	netconn_write(newconn, payload, payload_size, NETCONN_COPY);
}

void http_send_footer(struct netconn *newconn) {
	char response_footer[] = "\n\n";
	netconn_write(newconn, response_footer, strlen(response_footer), NETCONN_COPY);
}


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
	        printf("result:%d!\n", fr);
	        printf("Heap size: %d\n", xPortGetFreeHeapSize());

	        char *buf = (char*) malloc(fno.fsize);

	        if (buf == NULL) {
	        	printf("sdcard:buf not allocated memory\n");
	        	return;
	        }

		    uint32_t b;
	        f_read(&fil, buf, fno.fsize, &b);
	        printf("Bytes read: %d\n", b);
	        //buf[fno.fsize]='\0';

	        http_send_header(newconn, "image/png", fno.fsize);
	        http_send_payload(newconn, buf, fno.fsize);
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
	// All done, unmount partition and disable SDMMC or SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	ESP_LOGI(TAG, "Card unmounted");
}

