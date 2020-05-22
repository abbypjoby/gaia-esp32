#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h" 
#include "esp_log.h"

#define GATE_OPEN_OUTPUT    2
#define GATE_CLOSE_OUTPUT   4
#define OPEN_LED_OUTPUT    27
#define CLOSE_LED_OUTPUT   25
#define STOP_LED_OUTPUT    26
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<OPEN_LED_OUTPUT) | (1ULL<<CLOSE_LED_OUTPUT) | (1ULL<<STOP_LED_OUTPUT) | (1ULL<<GATE_OPEN_OUTPUT) | (1ULL<<GATE_CLOSE_OUTPUT))

#define OPEN_SWITCH_INPUT   21
#define CLOSE_SWITCH_INPUT  18
#define STOP_SWITCH_INPUT   19
#define GPIO_INPUT_PIN_SEL  ((1ULL<<OPEN_SWITCH_INPUT) | (1ULL<<CLOSE_SWITCH_INPUT) | (1ULL<<STOP_SWITCH_INPUT))

#define ESP_INTR_FLAG_DEFAULT 0


void initialize_pin_outputs()
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

void initialize_switch_inputs(gpio_isr_t gpio_isr_handler)
{
    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(OPEN_SWITCH_INPUT, gpio_isr_handler, (void*) OPEN_SWITCH_INPUT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(CLOSE_SWITCH_INPUT, gpio_isr_handler, (void*) CLOSE_SWITCH_INPUT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(STOP_SWITCH_INPUT, gpio_isr_handler, (void*) STOP_SWITCH_INPUT);
}

void turnOffLedIndication()
{
    gpio_set_level(OPEN_LED_OUTPUT, 0);
    gpio_set_level(CLOSE_LED_OUTPUT, 0);
    gpio_set_level(STOP_LED_OUTPUT, 0);
}

void showLedIndication(int ledPin)
{
    turnOffLedIndication();
    gpio_set_level(ledPin, 1);
}

void openGate()
{
    ESP_LOGI("main", "Opening");
    showLedIndication(OPEN_LED_OUTPUT);
    gpio_set_level(GATE_OPEN_OUTPUT, 1);
    gpio_set_level(GATE_CLOSE_OUTPUT, 0);
}

void closeGate() 
{
    ESP_LOGI("main", "Closing");
    showLedIndication(CLOSE_LED_OUTPUT);
    gpio_set_level(GATE_OPEN_OUTPUT, 0);
    gpio_set_level(GATE_CLOSE_OUTPUT, 1);
}

void stopGate()
{
    ESP_LOGI("main", "Stopping");
    showLedIndication(STOP_LED_OUTPUT);
    gpio_set_level(GATE_OPEN_OUTPUT, 0);
    gpio_set_level(GATE_CLOSE_OUTPUT, 0);
}

