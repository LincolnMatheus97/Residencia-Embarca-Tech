/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
// Informações de copyright e licença do código

#include <stdio.h>              // Biblioteca padrão para entrada e saída
#include "btstack.h"            // Biblioteca para manipulação da pilha Bluetooth (BTstack)
#include "pico/cyw43_arch.h"    // Biblioteca de arquitetura CYW43 para Raspberry Pi Pico
#include "pico/btstack_cyw43.h" // Biblioteca para integrar BTstack com CYW43
#include "hardware/adc.h"       // Biblioteca para controle do ADC (Conversor Analógico-Digital)
#include "pico/stdlib.h"        // Biblioteca padrão da Pico para GPIO, temporização, etc.
#include "server_common.h"      // Cabeçalho com definições e funções comuns para o servidor

#define HEARTBEAT_PERIOD_MS 1000 // Define o período de batimento cardíaco em milissegundos

// Variáveis estáticas e estruturas usadas no programa
static btstack_timer_source_t heartbeat;                                       // Estrutura para gerenciar o temporizador de batimento cardíaco
static btstack_packet_callback_registration_t hci_event_callback_registration; // Registro de callback para eventos HCI (Host Controller Interface)

// Variáveis para conexão Bluetooth
static hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID; // Handle da conexão Bluetooth, inicializado como inválido
static int notification_enabled = 0;                                // Flag para indicar se notificações estão habilitadas

// Função chamada a cada batimento cardíaco
static void heartbeat_handler(struct btstack_timer_source *ts)
{
    static uint32_t counter = 0; // Contador para rastrear os ciclos do temporizador
    counter++;                   // Incrementa o contador

    // Atualiza a temperatura a cada 10 batimentos
    if (counter % 10 == 0)
    {
        poll_temp(); // Função para ler a temperatura
        if (le_notification_enabled)
        {                                                      // Verifica se notificações estão habilitadas
            att_server_request_can_send_now_event(con_handle); // Solicita permissão para enviar dados
        }
    }

    // Inverte o estado do LED
    static int led_on = true;                           // Variável para armazenar o estado do LED
    led_on = !led_on;                                   // Alterna o estado
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on); // Atualiza o estado do LED GPIO

    // Reinicia o temporizador
    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS); // Configura o próximo batimento
    btstack_run_loop_add_timer(ts);                      // Adiciona o temporizador à fila de execução
}

// Função principal do programa
int main()
{
    stdio_init_all(); // Inicializa a entrada/saída padrão (printf, scanf, etc.)
    if (cyw43_arch_init())
    {                                                // Inicializa a arquitetura CYW43
        printf("failed to initialise cyw43_arch\n"); // Mensagem de erro caso falhe
        return -1;                                   // Finaliza o programa com erro
    }

    adc_init();                               // Inicializa o ADC para uso do sensor de temperatura
    adc_select_input(ADC_CHANNEL_TEMPSENSOR); // Seleciona o canal do sensor de temperatura
    adc_set_temp_sensor_enabled(true);        // Habilita o sensor de temperatura interno

    l2cap_init(); // Inicializa o protocolo L2CAP (Logical Link Control and Adaptation Protocol)
    sm_init();    // Inicializa o Security Manager (gerenciamento de segurança Bluetooth)

    // Inicializa o servidor ATT (Attribute Protocol) com o perfil de dados e callbacks
    att_server_init(profile_data, att_read_callback, att_write_callback);

    // Registra o callback para eventos HCI
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Registra o callback para pacotes do servidor ATT
    att_server_register_packet_handler(packet_handler);

    // Configura e adiciona o temporizador de batimento cardíaco
    heartbeat.process = &heartbeat_handler;                      // Define a função de processamento do batimento
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS); // Configura o temporizador
    btstack_run_loop_add_timer(&heartbeat);                      // Adiciona o temporizador à fila

    hci_power_control(HCI_POWER_ON); // Liga o controlador HCI para ativar o Bluetooth

    btstack_run_loop_execute(); // Inicia o loop de execução principal do BTstack

    /*
    Código condicional que permite alternar entre usar `btstack_run_loop_execute()`
    ou um loop manual que apenas dorme (desativado por padrão).
    */
#if 0 // btstack_run_loop_execute() não é necessário, então não é usado aqui
    btstack_run_loop_execute();
#else
    while (true)
    {                   // Loop infinito alternativo
        sleep_ms(1000); // Aguarda 1 segundo
    }
#endif

    return 0; // Finaliza o programa com sucesso
}
