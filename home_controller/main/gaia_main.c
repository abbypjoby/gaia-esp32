#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h" 
#include "esp_log.h"
#include "wifi_manager.h"
#include "blink.c"


void cb_connection_ok(void *pvParameter){
	ESP_LOGI("main", "Callback function, I have a connection!");
}

void app_main()
{
    wifi_manager_start();
    wifi_manager_set_callback(EVENT_STA_GOT_IP, &cb_connection_ok);

    blinkLED();
}
