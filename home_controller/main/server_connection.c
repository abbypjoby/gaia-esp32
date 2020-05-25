#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_system.h" 
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "cJSON.h"


const char *TAG = "Server_Connection";
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;

        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
                if (evt->user_data) {
                    memcpy(evt->user_data, evt->data, evt->data_len);
                }
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}



int fetch_gate_status(void)
{
    int is_on = 2;
    char local_response_buffer[1024] = {0};

    esp_http_client_config_t config = {
        .url = "http://pallathatta.herokuapp.com/node/1",
        .event_handler = _http_event_handle,
        .user_data = local_response_buffer
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
    ESP_LOGI(TAG, "Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));

        cJSON *root = cJSON_Parse(local_response_buffer);
        is_on = cJSON_GetObjectItem(root,"is_on")->valueint;
        ESP_LOGI(TAG, "Fetched the latest status ----->  is_on = %d", is_on);
        cJSON_Delete(root);
    }
    esp_http_client_cleanup(client);

    return is_on;
}


int update_gate_status(int status)
{
    int is_on = 2;
    char local_response_buffer[1024] = {0};

    char uri[100];
    snprintf(uri, sizeof(uri), "http://pallathatta.herokuapp.com/node/update?node_id=1&is_on=%d",status);

    ESP_LOGI(TAG, "URI to update %s", uri);
    esp_http_client_config_t config = {
        .url = uri,
        .event_handler = _http_event_handle,
        .user_data = local_response_buffer
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
    ESP_LOGI(TAG, "Successfully Update Gate Status, ResponseStatus = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));

        cJSON *root = cJSON_Parse(local_response_buffer);
        is_on = cJSON_GetObjectItem(root,"is_on")->valueint;
        ESP_LOGI(TAG, "Updated the status to ----> is_on = %d", is_on);
        cJSON_Delete(root);
    }
    esp_http_client_cleanup(client);

    return is_on;
}

