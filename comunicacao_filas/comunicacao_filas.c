#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

const uint ADC_PIN = 26; // ADC0

QueueHandle_t fila_ADC;

void setup() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0); // ADC0
}

void ADC_Task(void *pvParameters) {
    while (1) {
        uint16_t valor = adc_read(); // Lê o valor do ADC
        xQueueSend(fila_ADC, &valor, portMAX_DELAY); // Envia o valor para a fila
        vTaskDelay(pdMS_TO_TICKS(100)); // Aguarda 100 ms
    }
}

void ExibirADC_Task(void *pvParameters) {
    uint16_t valor;
    while (1) {
        if (xQueueReceive(fila_ADC, &valor, portMAX_DELAY) == pdTRUE) { // Recebe o valor da fila
            printf("Valor ADC: %u\n", valor); // Exibe o valor no console
        }
    }
}

int main () {
    setup();

    fila_ADC = xQueueCreate(10, sizeof(uint16_t)); // Cria a fila com 10 elementos de 16 bits
    if (fila_ADC != NULL) {
        xTaskCreate(ADC_Task, "ADC_Task", 256, NULL, 1, NULL);
        xTaskCreate(ExibirADC_Task, "ExibirADC_Task", 256, NULL, 2, NULL);
        vTaskStartScheduler(); // Inicia o escalonador
    }

    while (1) {
        // Loop infinito
    }
}