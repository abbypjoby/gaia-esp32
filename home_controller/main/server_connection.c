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
#include "esp_crt_bundle.h"

const char *uri = "https://pura-develop-default-rtdb.asia-southeast1.firebasedatabase.app/gate/status.json";
const char *TAG = "Server_Connection";


esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
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
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
    }
    return ESP_OK;
}

// esp_err_t _http_event_handle(esp_http_client_event_t *evt)
// {
//     switch(evt->event_id) {
//         case HTTP_EVENT_ERROR:
//             ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
//             break;
//         case HTTP_EVENT_ON_CONNECTED:
//             ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
//             break;
//         case HTTP_EVENT_HEADER_SENT:
//             ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
//             break;
//         case HTTP_EVENT_ON_HEADER:
//             ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER");
//             printf("%.*s", evt->data_len, (char*)evt->data);
//             break;

//         case HTTP_EVENT_ON_DATA:
//             ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//             if (esp_http_client_is_chunked_response(evt->client)) {
//                 printf("%.*s", evt->data_len, (char*)evt->data);
//                 if (evt->user_data) {
//                     memcpy(evt->user_data, evt->data, evt->data_len);
//                 }
//             }
//             break;

//         case HTTP_EVENT_ON_FINISH:
//             ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
//             break;
//         case HTTP_EVENT_DISCONNECTED:
//             ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
//             break;
//     }
//     return ESP_OK;
// }



int fetch_gate_status(void)
{
    int is_on = 2;
    char local_response_buffer[2048] = {0};

    esp_http_client_config_t config = {
        .url = uri,
        .event_handler = _http_event_handle,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
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
    } else {
        ESP_LOGE(TAG, "Fetch Gate status failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    return is_on;
}


int update_gate_status(int status)
{
    int is_on = 2;
    char local_response_buffer[2048] = {0};

    ESP_LOGI(TAG, "URI to update %s", uri);
    esp_http_client_config_t config = {
        .url = uri,
        .event_handler = _http_event_handle,
        .user_data = local_response_buffer,    
        .disable_auto_redirect = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };


    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    char status_data[20];
    snprintf(status_data, sizeof(status_data), "{\"is_on\":%d}",status);
    
    esp_http_client_set_method(client, HTTP_METHOD_PUT);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, status_data, strlen(status_data));
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
    ESP_LOGI(TAG, "Successfully Update Gate Status, ResponseStatus = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));

        cJSON *root = cJSON_Parse(local_response_buffer);
        is_on = cJSON_GetObjectItem(root,"is_on")->valueint;
        ESP_LOGI(TAG, "Updated the status to ----> is_on = %d", is_on);
        cJSON_Delete(root);
    } else {
        ESP_LOGE(TAG, "Update Gate status failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    return is_on;
}

