#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "wifi_ap.h"

static const char *TAG = "wifi_ap";

static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
	case SYSTEM_EVENT_AP_START:
		ESP_LOGI(TAG, "SYSTEM_EVENT_AP_START");
		break;
	case SYSTEM_EVENT_AP_STOP:
		ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STOP");
	default:
		break;
	}
	return ESP_OK;
}

/* Initialize Wi-Fi as sta and set scan method */
void wifi_ap(void) {
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 4, 1);
	IP4_ADDR(&info.gw, 192, 168, 4, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config =
			{
				.ap =
						{
							.ssid = WIFI_AP_SSID,
							.password = WIFI_AP_PASSWORD,
							.authmode = WIFI_AUTH_WPA2_PSK,
							.channel = WIFI_AP_CHANNEL,
							.max_connection = WIFI_AP_MAX_CONNECTIONS,
							.beacon_interval = WIFI_AP_BEACON_INTERVAL
						},
			};

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

