#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h" 
#include "esp_log.h"
#include "wifi_manager.h"
#include "io_pins.c"


bool isControlManual = true; 
static xQueueHandle gpio_evt_queue = NULL;

void wifi_on_connected(void *pvParameter){
    isControlManual = false; 
	ESP_LOGI("main", "--------- WiFi Connected");
}

void wifi_on_disconnected(void *pvParameter){
    isControlManual = true; 
	ESP_LOGI("main", "--------- WiFi Disconnected");
    // Try to reconnect
}



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
                    openGate();
                    break;

                case CLOSE_SWITCH_INPUT:
                    closeGate();
                    break;

                case STOP_SWITCH_INPUT:
                    stopGate();
                    break;
                
                default:
                    ESP_LOGE("main", "Unknown switch_number, stopping gate");
                    stopGate();
                    break;
            }

        }
    }
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



    for(;;)
    {
        if (isControlManual)
        {
            // Read Manual button signals / use saved state

            // Update output
            
            // Try to update the Server with Manual Signal
            // if successful, turn to serverControlMode

        } 
        
        else
        {   
            // Call API to fetch latest state
            // Update Output
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}
