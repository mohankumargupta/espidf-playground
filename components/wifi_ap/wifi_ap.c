#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_ap";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
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
static void wifi_ap(void)
{
   tcpip_adapter_init();
   ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
   tcpip_adapter_ip_info_t info;
   memset(&info, 0, sizeof(info));
   IP4_ADDR(&info.ip, 10, 10, 20, 1);
   IP4_ADDR(&info.gw, 10, 10, 20, 1);
   IP4_ADDR(&info.netmask, 255, 255, 255, 0);
   ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
   ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "esp32",
            .password = "peachspeak38",
            .authmode = WIFI_AUTH_WPA2_PSK,
			.channel = 6,
			.max_connection = 10,
			.beacon_interval = 400
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_ap();
}
