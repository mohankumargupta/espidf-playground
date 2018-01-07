#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "wifi_ap.h"
#include "http_server.h"
#include "websockets_server.h"
#include "esp_err.h"
#include "servefrom_sdcard.h"

/*
static esp_err_t event_handler(void *ctx, system_event_t *event) {
	return ESP_OK;
}
*/

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_ap();
	//xTaskCreate(&http_server, "http_server", 8192, NULL, 5, NULL);
	xTaskCreate(&http_server, "http_server", 20000, NULL, 5, NULL);
    xTaskCreate(&websockets_server, "websockets_server", 8192, NULL, 4, NULL);


/*
	nvs_flash_init();
	    tcpip_adapter_init();
	    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	    wifi_config_t sta_config = {
	        .sta = {
	            .ssid = "mycrib",
	            .password = "peachspeak38",
	        }
	    };
	    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
	    ESP_ERROR_CHECK( esp_wifi_start() );
	    ESP_ERROR_CHECK( esp_wifi_connect() );

	    xTaskCreate(&http_server, "http_server", 8192, NULL, 5, NULL);
	    xTaskCreate(&websockets_server, "websockets_server", 8192, NULL, 8, NULL);
*/


}
