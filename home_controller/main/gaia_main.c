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
int gateStatus = DEFAULT_STATUS;
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
                    stopGate();
                    break;
            }

        }
    }
}



// Keep updating gateStatus every 2 seconds
void refresh_gate_status(void *pvParameters)
{
    for(;;)
    {
        gateStatus = fetch_gate_status();
        ESP_LOGI(TAG, "Finish http example");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// If in manual Mode, update the Server with buttonPressStatus
void update_server_gate_status(void *pvParameters)
{
    for(;;)
    {
        if (isControlManual && buttonPressStatus >= -1 && buttonPressStatus <= 1)
        {   
            // Update server here
            update_gate_status(buttonPressStatus);
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            // if server started to respond, turn on Auto Control Mode
            if (gateStatus == buttonPressStatus){
                isControlManual = false;
            }
        }
    }
    vTaskDelete(NULL);
}


void wifi_on_connected(void *pvParameter){
    isControlManual = false; 
    //API call - start task and refresh gateStatus every 2 seconds
	ESP_LOGI("main", "--------- WiFi Connected");
}

void wifi_on_disconnected(void *pvParameter){
    isControlManual = true; 
	ESP_LOGI("main", "--------- WiFi Disconnected");

    // Try to reconnect
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

    // Start two tasks, one for refreshing gate status, another to update with manual override values
    xTaskCreate(refresh_gate_status, "refresh_gate_status", 8192, NULL, 5, NULL);
    xTaskCreate(update_server_gate_status, "update_server_gate_status", 8192, NULL, 5, NULL);

    for(;;)
    {
        if (isControlManual)
        {
            ESP_LOGI("main","Manual Mode Active");
            // Try to update the Server with Manual Signal
            // if successful, turn to serverControlMode
            switch (buttonPressStatus)
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
        } 
        
        else
        {   
            ESP_LOGI("main","Auto Mode Active");
            switch (gateStatus)
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
                    // status 2 means API call failed, so some network issue,
                    // therefore set to manualControl mode
                    stopGate();
                    isControlManual = true;
                    break;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

}
