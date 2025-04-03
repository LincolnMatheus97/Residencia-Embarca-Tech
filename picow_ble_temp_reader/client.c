#include <stdio.h>           // Biblioteca padrão para entrada/saída (printf, etc.)
#include "btstack.h"         // Biblioteca BTstack para funcionalidades Bluetooth
#include "pico/cyw43_arch.h" // Biblioteca específica para a arquitetura CYW43 (Wi-Fi/Bluetooth no Pico W)
#include "pico/stdlib.h"     // Biblioteca padrão para funcionalidades do Raspberry Pi Pico (GPIO, timers, etc.)

#if 0
#define DEBUG_LOG(...) printf(__VA_ARGS__) // Define macro para logs de depuração (desativado por padrão)
#else
#define DEBUG_LOG(...)
#endif

#define LED_QUICK_FLASH_DELAY_MS 100 // Atraso em ms para piscar rápido no LED
#define LED_SLOW_FLASH_DELAY_MS 1000 // Atraso em ms para piscar lento no LED

typedef enum
{
    TC_OFF,                              // Cliente desligado
    TC_IDLE,                             // Cliente ocioso
    TC_W4_SCAN_RESULT,                   // Aguardando resultado de varredura BLE
    TC_W4_CONNECT,                       // Aguardando conexão com dispositivo
    TC_W4_SERVICE_RESULT,                // Aguardando descoberta de serviço
    TC_W4_CHARACTERISTIC_RESULT,         // Aguardando descoberta de característica
    TC_W4_ENABLE_NOTIFICATIONS_COMPLETE, // Aguardando habilitação de notificações
    TC_W4_READY                          // Cliente pronto para processar dados
} gc_state_t;

// Callback para eventos HCI
static btstack_packet_callback_registration_t hci_event_callback_registration;
static gc_state_t state = TC_OFF;                          // Estado atual do cliente BLE
static bd_addr_t server_addr;                              // Endereço do servidor BLE
static bd_addr_type_t server_addr_type;                    // Tipo de endereço do servidor BLE
static hci_con_handle_t connection_handle;                 // Handle da conexão HCI
static gatt_client_service_t server_service;               // Serviço GATT do servidor
static gatt_client_characteristic_t server_characteristic; // Característica GATT
// protocolo GATT (Generic Attribute Profile)
static bool listener_registered = false;                 // Indica se notificações estão registradas
static gatt_client_notification_t notification_listener; // Listener p/ notificar
static btstack_timer_source_t heartbeat;                 // Timer para controle do LED
static btstack_timer_source_t heartbeat_timer;
static bool led_on = false; // Tracks LED state

static void client_start(void)
{
    printf("Iniciando varredura de dispositivos BLE...\n");
    DEBUG_LOG("Start scanning!\n");
    state = TC_W4_SCAN_RESULT;                  // Define o estado para "aguardando resultados de varredura"
    gap_set_scan_parameters(0, 0x0030, 0x0030); // Configura parâmetros de varredura BLE
    gap_start_scan();
}

static bool advertisement_report_contains_service(uint16_t service, uint8_t *advertisement_report) // Inicia varredura BLE
{
    const uint8_t *adv_data = gap_event_advertising_report_get_data(advertisement_report); // Dados do relatório
    uint8_t adv_len = gap_event_advertising_report_get_data_length(advertisement_report);  // Tamanho dos dados
    ad_context_t context;
    for (ad_iterator_init(&context, adv_len, adv_data); ad_iterator_has_more(&context); ad_iterator_next(&context))
    {
        uint8_t data_type = ad_iterator_get_data_type(&context); // Tipo de dado no anúncio
        uint8_t data_size = ad_iterator_get_data_len(&context);  // Tamanho do dado
        const uint8_t *data = ad_iterator_get_data(&context);    // Conteúdo do dado
        if (data_type == BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS)
        {
            for (int i = 0; i < data_size; i += 2)
            {
                uint16_t type = little_endian_read_16(data, i); // Lê o UUID de serviço
                // UUID (Universally Unique Identifier) é um identificador único universal utilizado para identificar de
                // forma exclusiva entidades em sistemas distribuídos. No contexto de Bluetooth Low Energy (BLE), um UUID
                // é amplamente usado para identificar serviços, características e descritores no protocolo GATT (Generic Attribute Profile).
                if (type == service)
                    return true; // Retorna verdadeiro se encontrar o serviço
            }
        }
    }
    return false; // Serviço não encontrado
}

//* VERSÃO ESTENDIDA DA FUNÇÃO
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint8_t att_status;
    switch (state)
    {
    case TC_W4_SERVICE_RESULT:
        switch (hci_event_packet_get_type(packet))
        {
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            // store service (we expect only one)
            DEBUG_LOG("Storing service\n");
            gatt_event_service_query_result_get_service(packet, &server_service);
            break;
        case GATT_EVENT_QUERY_COMPLETE:
            att_status = gatt_event_query_complete_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS)
            {
                printf("SERVICE_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                gap_disconnect(connection_handle);
                break;
            }
            // service query complete, look for characteristic
            state = TC_W4_CHARACTERISTIC_RESULT;
            DEBUG_LOG("Search for env sensing characteristic.\n");
            gatt_client_discover_characteristics_for_service_by_uuid16(handle_gatt_client_event, connection_handle, &server_service, ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE);
            break;
        default:
            break;
        }
        break;
    case TC_W4_CHARACTERISTIC_RESULT:
        switch (hci_event_packet_get_type(packet))
        {
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            DEBUG_LOG("Storing characteristic\n");
            gatt_event_characteristic_query_result_get_characteristic(packet, &server_characteristic);
            break;
        case GATT_EVENT_QUERY_COMPLETE:
            att_status = gatt_event_query_complete_get_att_status(packet);
            if (att_status != ATT_ERROR_SUCCESS)
            {
                printf("CHARACTERISTIC_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                gap_disconnect(connection_handle);
                break;
            }
            // register handler for notifications
            listener_registered = true;
            gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_client_event, connection_handle, &server_characteristic);
            // enable notifications
            DEBUG_LOG("Enable notify on characteristic.\n");
            state = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
            gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, connection_handle,
                                                                  &server_characteristic, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
            break;
        default:
            break;
        }
        break;
    case TC_W4_ENABLE_NOTIFICATIONS_COMPLETE:
        switch (hci_event_packet_get_type(packet))
        {
        case GATT_EVENT_QUERY_COMPLETE:
            DEBUG_LOG("Notifications enabled, ATT status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
            if (gatt_event_query_complete_get_att_status(packet) != ATT_ERROR_SUCCESS)
                break;
            state = TC_W4_READY;
            break;
        default:
            break;
        }
        break;
    case TC_W4_READY:
        switch (hci_event_packet_get_type(packet))
        {
        case GATT_EVENT_NOTIFICATION:
        {
            uint16_t value_length = gatt_event_notification_get_value_length(packet);
            const uint8_t *value = gatt_event_notification_get_value(packet);
            DEBUG_LOG("Indication value len %d\n", value_length);
            if (value_length == 2)
            {
                float temp = little_endian_read_16(value, 0);
                printf("read temp %.2f degc\n", temp / 100);
            }
            else
            {
                printf("Unexpected length %d\n", value_length);
            }
            listener_registered = true; // Notifications enabled
            break;
        }
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            listener_registered = false; // Reset on disconnect
            break;
        default:
            printf("Unknown packet type 0x%02x\n", hci_event_packet_get_type(packet));
            break;
        }
        break;
    default:
        printf("error\n");
        break;
    }
}
//*/
/*
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type); // Ignora variáveis não usadas
    UNUSED(channel);
    UNUSED(size);
    uint8_t att_status;
    switch(state){
        case TC_W4_SERVICE_RESULT:  // Estado: Aguardando descoberta de serviços
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_SERVICE_QUERY_RESULT:
                    gatt_event_service_query_result_get_service(packet, &server_service); // Salva serviço encontrado
                    break;
                case GATT_EVENT_QUERY_COMPLETE:
                    att_status = gatt_event_query_complete_get_att_status(packet);        // Status da consulta
                    if (att_status != ATT_ERROR_SUCCESS){
                        printf("SERVICE_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                        gap_disconnect(connection_handle);  }                           // Desconecta em caso de erro
                        break;
                    state = TC_W4_CHARACTERISTIC_RESULT;                                // Atualiza estado
                    gatt_client_discover_characteristics_for_service_by_uuid16(
                        handle_gatt_client_event, connection_handle, &server_service, ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE);
                    break;            }
            break; }   }     // Outros estados semelhantes
*/
/*
static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                gap_local_bd_addr(local_addr);
                printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                client_start();
            } else {
                state = TC_OFF;
            }
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            if (state != TC_W4_SCAN_RESULT) return;
            // check name in advertisement
            if (!advertisement_report_contains_service(ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING, packet)) return;
            // store address and type
            gap_event_advertising_report_get_address(packet, server_addr);
            server_addr_type = gap_event_advertising_report_get_address_type(packet);
            // stop scanning, and connect to the device
            state = TC_W4_CONNECT;
            gap_stop_scan();
            printf("Connecting to device with addr %s.\n", bd_addr_to_str(server_addr));
            gap_connect(server_addr, server_addr_type);
            break;
        case HCI_EVENT_LE_META:
            // wait for connection complete
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    if (state != TC_W4_CONNECT) return;
                    connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    // initialize gatt client context with handle, and add it to the list of active clients
                    // query primary services
                    DEBUG_LOG("Search for env sensing service.\n");
                    state = TC_W4_SERVICE_RESULT;
                    gatt_client_discover_primary_services_by_uuid16(handle_gatt_client_event, connection_handle, ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING);
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            // unregister listener
            connection_handle = HCI_CON_HANDLE_INVALID;
            if (listener_registered){
                listener_registered = false;
                gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
            }
            printf("Disconnected %s\n", bd_addr_to_str(server_addr));
            if (state == TC_OFF) break;
            client_start();
            break;
        default:
            break;
    }
}
*/
static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    // Ignora parâmetros não utilizados para evitar avisos do compilador
    UNUSED(size);
    UNUSED(channel);

    // Declaração de variável para armazenar o endereço Bluetooth local
    bd_addr_t local_addr;

    // Verifica se o tipo do pacote recebido é um evento HCI
    if (packet_type != HCI_EVENT_PACKET)
        return;

    // Obtém o tipo específico do evento HCI a partir do pacote
    uint8_t event_type = hci_event_packet_get_type(packet);

    // Processa o evento com base no tipo identificado
    switch (event_type)
    {

    // Evento de inicialização da pilha Bluetooth (BTstack)
    case BTSTACK_EVENT_STATE:
        // Verifica se o estado do BTstack é "funcionando"
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
        {
            // Obtém o endereço Bluetooth local
            gap_local_bd_addr(local_addr);

            // Imprime no console que a pilha Bluetooth está pronta e o endereço local
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

            // Inicia a varredura para dispositivos BLE
            client_start();
        }
        else
        {
            // Se o estado não for "funcionando", define o estado do cliente como desligado
            state = TC_OFF;
        }
        break;

    // Evento de relatório de anúncios BLE durante a varredura
    case GAP_EVENT_ADVERTISING_REPORT:
        // Verifica se o cliente está no estado correto para processar anúncios
        if (state != TC_W4_SCAN_RESULT)
            return;

        // Verifica se o dispositivo encontrado contém o serviço esperado (identificado pelo UUID)
        if (!advertisement_report_contains_service(ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING, packet))
            return;

        // Armazena o endereço do dispositivo remoto
        gap_event_advertising_report_get_address(packet, server_addr);

        // Armazena o tipo de endereço do dispositivo remoto (público ou aleatório)
        server_addr_type = gap_event_advertising_report_get_address_type(packet);

        // Atualiza o estado para "aguardando conexão"
        state = TC_W4_CONNECT;

        // Para a varredura BLE
        gap_stop_scan();

        // Imprime no console que está conectando ao dispositivo encontrado
        printf("Connecting to device with addr %s.\n", bd_addr_to_str(server_addr));

        // Inicia a conexão com o dispositivo remoto
        gap_connect(server_addr, server_addr_type);
        break;

    // Evento de metaeventos relacionados ao BLE
    case HCI_EVENT_LE_META:
        // Processa subeventos específicos dentro do metaevento
        switch (hci_event_le_meta_get_subevent_code(packet))
        {

        // Subevento indicando que a conexão foi concluída com sucesso
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
            // Verifica se o cliente estava aguardando a conclusão da conexão
            if (state != TC_W4_CONNECT)
                return;

            // Armazena o handle da conexão estabelecida
            connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

            // Mensagem informando que o cliente foi conectado
            printf("Cliente conectado com sucesso. Handle de conexão: 0x%04x\n", connection_handle);

            // Log para depuração
            DEBUG_LOG("Search for env sensing service.\n");

            // Atualiza o estado para "aguardando resultado da descoberta de serviço"
            state = TC_W4_SERVICE_RESULT;

            // Inicia a descoberta de serviços GATT no dispositivo remoto
            gatt_client_discover_primary_services_by_uuid16(
                handle_gatt_client_event,                   // Callback para processar os resultados
                connection_handle,                          // Handle da conexão
                ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING // UUID do serviço desejado
            );
            break;

        default:
            // Ignora subeventos não processados
            break;
        }
        break;

    // Evento de desconexão com o dispositivo BLE
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        // Define o handle da conexão como inválido
        connection_handle = HCI_CON_HANDLE_INVALID;

        // Verifica se um listener de notificações estava registrado
        if (listener_registered)
        {
            // Cancela o registro do listener
            listener_registered = false;
            gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
        }

        // Imprime no console que o dispositivo foi desconectado
        printf("Disconnected %s\n", bd_addr_to_str(server_addr));

        // Reinicia a varredura BLE se o cliente não estiver desligado
        if (state == TC_OFF)
            break;
        client_start();
        break;

    default:
        // Ignora todos os outros tipos de eventos que não são explicitamente processados
        break;
    }
}

static void heartbeat_handler(struct btstack_timer_source *ts)
{
    // Variável estática para determinar o estado de "flash rápido" do LED
    static bool quick_flash;
    // Variável estática para rastrear se o LED está ligado ou desligado
    // static bool led_on = true;
    led_on = true;

    // Inverte o estado do LED (liga/desliga)
    led_on = !led_on;

    // Atualiza o estado do LED físico usando a GPIO do controlador CYW43
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    /*
    // Se um listener estiver registrado e o LED estiver ligado, ativa o "flash rápido"
    if (listener_registered && led_on) {
        quick_flash = !quick_flash;
    }
    // Caso nenhum listener esteja registrado, desativa o "flash rápido"
    else if (!listener_registered) {
        quick_flash = false;
    }


    // Ajusta o timer para o próximo intervalo com base no estado do LED
    // Usa "flash rápido" ou "flash lento" dependendo das condições
    btstack_run_loop_set_timer(ts, (led_on || quick_flash) ? LED_QUICK_FLASH_DELAY_MS : LED_SLOW_FLASH_DELAY_MS);
    */

    // Adjust LED flash rate based on connection status
    btstack_run_loop_set_timer(ts, listener_registered ? LED_QUICK_FLASH_DELAY_MS : LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(ts);

    // Reinsere o timer no loop principal para ser acionado novamente
    btstack_run_loop_add_timer(ts);
}

int main()
{
    // Inicializa o sistema de entrada e saída padrão (printf, etc.)
    stdio_init_all();

    // Inicializa a arquitetura CYW43 (Wi-Fi e Bluetooth)
    if (cyw43_arch_init())
    {
        // Caso a inicialização falhe, imprime mensagem de erro e encerra o programa
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    // Inicializa o protocolo L2CAP (canal lógico no Bluetooth)
    l2cap_init();

    // Inicializa o Security Manager (SM) para BLE
    sm_init();
    // Define as capacidades de entrada/saída como "sem entrada ou saída" (usado para emparelhamento BLE)
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    // Configura o servidor ATT (Attribute Protocol), mas sem callbacks (NULL)
    att_server_init(NULL, NULL, NULL);

    // Inicializa o cliente GATT para interagir com dispositivos BLE
    gatt_client_init();

    // Registra o callback para eventos HCI, associando a função hci_event_handler
    hci_event_callback_registration.callback = &hci_event_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Configura um timer para controlar o LED e associa ao handler heartbeat_handler
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(&heartbeat);

    // Liga o Bluetooth no controlador (modo ON)
    hci_power_control(HCI_POWER_ON);
    btstack_run_loop_execute();

    /*
        // Define o modo de execução do loop principal do BTstack
    #if 1 // Este bloco será executado se 'polling' estiver habilitado
        // Inicia o loop principal do BTstack
        btstack_run_loop_execute();
    #else
        // Modo alternativo: executa um loop infinito de espera
        while (true) {
            // Simula uma pausa de 1 segundo (não usado neste programa)
            sleep_ms(1000);
        }
    #endif
    */
    // Retorna 0 para indicar que o programa foi concluído com sucesso
    return 0;
}
