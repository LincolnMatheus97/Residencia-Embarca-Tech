#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

// Declaração das variaveis dos pinos 
const uint pino_botaoA = 5;
const uint pino_led_vermelho = 13;

// Declaração das filas 
QueueHandle_t fila_botao;
QueueHandle_t fila_led;

// Função de inicialização
void setup() {
    stdio_init_all();
    gpio_init(pino_botaoA);
    gpio_set_dir(pino_botaoA, GPIO_IN);
    gpio_pull_up(pino_botaoA);
    gpio_init(pino_led_vermelho);
    gpio_set_dir(pino_led_vermelho, GPIO_OUT);
}

// Função para debouncing do botão
bool debounce_botao(uint pino_botao) {
    static uint32_t ultimo_pressionamento = 0;
    uint32_t pressionado = to_ms_since_boot(get_absolute_time());

    if (gpio_get(pino_botao) == 0) {
        if (pressionado - ultimo_pressionamento > 300) {
            ultimo_pressionamento = pressionado;
            sleep_ms(50);
            return true;
        }
    }
    return false;
}

// Função da Tarefa 1, que faz a leitura do botão e sinaliza a Tarefa 2 se o botão foi pressionado
void vTarefa1(void *parametro) {
    for(;;) {
        if (debounce_botao(pino_botaoA)) {
            uint8_t sinal = 1;        
            xQueueSend(fila_botao, &sinal, portMAX_DELAY); // Sinalizo a Tarefa 2 que o botão foi pressionado
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Função da Tarefa 2, que aguarda o sinal da Tarefa 1 e sinaliza a Tarefa 3 para acender o LED
void vTarefa2(void *parametro) {
    for (;;) {
        uint8_t sinal;
        if (xQueueReceive(fila_botao, &sinal, portMAX_DELAY) == pdTRUE) {
            xQueueSend(fila_led, &sinal, portMAX_DELAY);        // Encaminho o sinal para a fila do LED
        }
    }
}

// Função da Tarefa 3, que aguarda o sinal da Tarefa 2 e acende o LED
void vTarefa3(void *parametro) {
    for (;;) {
        uint8_t sinal;
        if (xQueueReceive(fila_led, &sinal, portMAX_DELAY) == pdTRUE) {
            gpio_put(pino_led_vermelho, 1);         // Ascendo o LED        
            vTaskDelay(250 / portTICK_PERIOD_MS);
            gpio_put(pino_led_vermelho, 0);         // Apago o LED
        }
    }
}

int main() {
    setup();

    // Crio filas para comunicação entre tarefas
    fila_botao = xQueueCreate(10, sizeof(uint8_t));
    fila_led = xQueueCreate(10, sizeof(uint8_t));

    // Verifico se as filas foram criadas com sucesso
    if (fila_botao != NULL && fila_led != NULL) {

        // Crio as tarefas
        xTaskCreate(vTarefa1, "tarefa1", 256, NULL, 1, NULL);
        xTaskCreate(vTarefa2, "tarefa2", 256, NULL, 1, NULL);
        xTaskCreate(vTarefa3, "tarefa3", 256, NULL, 1, NULL);

        // Inicializo o escalonador de tarefas 
        vTaskStartScheduler();
    }

    // Caso as filas não tenham sido criadas com sucesso, o programa entra em loop infinito
    while(1){};
    return 0;
}