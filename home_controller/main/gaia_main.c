#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h" 
#include "esp_log.h"
#include "wifi_manager.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "cJSON.h"

#include "server_connection.c"
#include "io_pins.c"

#define OPEN_STATUS 1
#define CLOSE_STATUS 0
#define STOP_STATUS -1
#define DEFAULT_STATUS 2


bool isControlManual = false; 
int serverStatus = DEFAULT_STATUS;
int buttonPressStatus = DEFAULT_STATUS;
static xQueueHandle gpio_evt_queue = NULL;

TaskHandle_t refresh_gate_status_handler = NULL;


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t switch_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &switch_num, NULL);
}


static void handle_switch_press(void* arg)
{
    uint32_t switch_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &switch_num, portMAX_DELAY)) {
            ESP_LOGI("main", "switch_num - %d, turning on Manual Control Mode", switch_num);
            isControlManual = true;
            switch (switch_num)
            {
                case OPEN_SWITCH_INPUT:
                    buttonPressStatus = OPEN_STATUS;
                    break;

                case CLOSE_SWITCH_INPUT:
                    buttonPressStatus = CLOSE_STATUS;
                    break;

                case STOP_SWITCH_INPUT:
                    buttonPressStatus = STOP_STATUS;
                    break;
                
                default:
                    ESP_LOGE("main", "Unknown switch_number, stopping gate");
                    buttonPressStatus = DEFAULT_STATUS;
                    break;
            }
        }
    }
}



// Always execute, and keep refreshing the `serverStatus` variable
void refresh_server_status_task(void *pvParameters)
{
    for(;;)
    {
        ESP_LOGI("main","Auto Control Active, button=%d, server=%d", buttonPressStatus, serverStatus);
        serverStatus = fetch_gate_status();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// Keep trying to update the server with latest button press (if in manual mode)
void update_server_statue_task(void *pvParameters)
{
    for(;;)
    {
        if (isControlManual)
        {   
            ESP_LOGI("main","Manual Control Active, button=%d, server=%d", buttonPressStatus, serverStatus);
            update_gate_status(buttonPressStatus);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}


void wifi_on_connected(void *pvParameter){
	ESP_LOGI("main", "--------- WiFi Connected");
}

void wifi_on_disconnected(void *pvParameter){
	ESP_LOGI("main", "--------- WiFi Disconnected");
}

void app_main()
{
    /******** WiFi Setup ***********/
    wifi_manager_start();
    wifi_manager_set_callback(EVENT_STA_GOT_IP, &wifi_on_connected);
    wifi_manager_set_callback(EVENT_STA_DISCONNECTED, &wifi_on_disconnected);


    /******* Switch and Interrupt Setup *********/
    initialize_pin_outputs();
    initialize_switch_inputs(gpio_isr_handler);
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    xTaskCreate(handle_switch_press, "handle_switch_press", 2048, NULL, 10, NULL);

    // Start two tasks, one for handling Auto mode and another for Manual Mode
    xTaskCreate(refresh_server_status_task, "refresh_server_status_task", 8192, NULL, 5, NULL);
    xTaskCreate(update_server_statue_task, "update_server_statue_task", 8192, NULL, 5, NULL);

    for(;;)
    {
        int finalState = serverStatus;
        if (isControlManual){
            // if server is updated, then serverStatus would have successfully changed to the current buttonStatus
            if (serverStatus == buttonPressStatus)
            {
                isControlManual = false;
            }
            finalState = buttonPressStatus;
        }

        ESP_LOGI("main","Final State is %d", finalState);
        switch (finalState)
        {
            case CLOSE_STATUS:
                closeGate();
                break;

            case OPEN_STATUS:
                openGate();
                break;

            case STOP_STATUS:
                stopGate();
                break;
        
            default:
                ESP_LOGE("main", "Unknown Button Press Status in Manual Control Mode");
                stopGate();
                break;
        }    
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

