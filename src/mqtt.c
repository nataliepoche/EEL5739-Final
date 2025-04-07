#include "mqtt.h"
#include "certs.h"
#include "cJSON.h"
#include "driver/gpio.h"

const char *TAG_MQTT = "MQTT_EXAMPLE";
const char *CLIENT_ID = "Final_Thermo_id";

#define LED_PIN GPIO_NUM_2 // Change to your relay or indicator pin
#define TEMP_HIGH 75.0     // Turn AC ON above this
#define TEMP_LOW 70.0      // Turn AC OFF below this

bool ac_on = false;

void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_event_handler_temp(void* handler_args, esp_event_base_t base,
                                  int32_t event_id, void* event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_DATA: {
            char* topic = strndup(event->topic, event->topic_len);
            char* data = strndup(event->data, event->data_len);
            ESP_LOGI("MQTT", "Received data on topic: %s", topic);
            ESP_LOGI("MQTT", "Payload: %s", data);
            // Parse the incoming JSON
            cJSON* root = cJSON_Parse(data);
            if (root != NULL) {
                cJSON* temp_item = cJSON_GetObjectItem(root, "temperature");
                if (temp_item && cJSON_IsNumber(temp_item)) {
                    float temp = temp_item->valuedouble;
                    if (temp > TEMP_HIGH && !ac_on) {
                        gpio_set_level(LED_PIN, 1); // AC ON
                        ESP_LOGI("AC", "AC turned ON (temp: %.2f)", temp);
                        ac_on = true;
                    } else if (temp < TEMP_LOW && ac_on) {
                        gpio_set_level(LED_PIN, 0); // AC OFF
                        ESP_LOGI("AC", "AC turned OFF (temp: %.2f)", temp);
                        ac_on = false;
                    }
                }
                cJSON_Delete(root);
            }
            free(topic);
            free(data);
            break;
        }
        default:
            break;
    }
}

void mqtt_app_start(const char* mqtt_broker_url, esp_mqtt_client_handle_t* client)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_broker_url,
        .cert_pem = AWS_ROOT_CA,         // AWS CA
        .client_cert_pem = DEVICE_CERT,  // Device Certificate
        .client_key_pem = PRIV_KEY,       // Private key
        .client_id = CLIENT_ID
    };

    *client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(*client, ESP_EVENT_ANY_ID, mqtt_event_handler_temp, NULL);
    esp_mqtt_client_start(*client);
}
