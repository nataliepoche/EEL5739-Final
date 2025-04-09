#include "nvs_flash.h"
#include "mqtt.h"
#include "host.h"
#include "esp_log.h"
#include "driver/gpio.h"

const char* Wifi_SSID = "SETUP-A799";
const char* Wifi_Pass = "Angle1478";
const char* Mqtt_Broker_Url  = "mqtts://a3ei6kcz3omkm6-ats.iot.us-east-2.amazonaws.com";
host_t host;

#define LED_PIN GPIO_NUM_2 // Change to your relay or indicator pin

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    host.wifi_creds.Wifi_SSID = Wifi_SSID;
    host.wifi_creds.Wifi_Pass = Wifi_Pass;
    init_host(&host);
    // Attach custom handler
    //esp_mqtt_client_register_event(&(host.mqtt_client), MQTT_EVENT_DATA, mqtt_event_handler_cb, NULL);
    mqtt_app_start(Mqtt_Broker_Url, &(host.mqtt_client));
    esp_mqtt_client_subscribe(host.mqtt_client, "Final_TempHumidity", 1);

    while(1) {
      vTaskDelay(MS2TICK(500));
    }
}