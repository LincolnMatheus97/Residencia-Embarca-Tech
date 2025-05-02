#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cliente_http.h"
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "sensores/sensores.h"


#define HOST "api-iot-rse.onrender.com"
#define PORTA_HTTP 80

void enviar_dados_para_nuvem()
{
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb)
    {   
        printf("Erro ao criar pcb do cliente...\n");
        return;
    }

    ip_addr_t endereco_ip;
    err_t erro_dns = dns_gethostbyname(HOST, &endereco_ip, NULL, NULL);

    if (erro_dns != ERR_OK)
    {
        printf("Erro DNS ao resolver host %s: %d\n", HOST, erro_dns);
        return;
    }

    //Conectar ao servidor HTTP
    err_t erro_conex = tcp_connect(pcb, &endereco_ip, PORTA_HTTP, NULL);
    if (erro_conex != ERR_OK)
    {
        printf("Erro ao se conectar no host: %s:%d - Codigo: %d\n", HOST, PORTA_HTTP, erro_conex);
        tcp_abort(pcb);
        return;
    }

    //Coletar os dados dos sensores
    uint8_t x = ler_joystick_x();
    uint8_t y = ler_joystick_y();
    bool botao_a = botao_a_pressionado();
    bool botao_b = botao_b_pressionado();

    char corpo_json[128];
    snprintf(corpo_json, sizeof(corpo_json), 
            "{\"botao_a\": %d, \"botao_b\": %d, \"x\": %d, \"y\": %d}", botao_a, botao_b, x, y);
    
    char requisicao[512];
    snprintf(requisicao, sizeof(requisicao),
             "POST /dados HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             HOST, strlen(corpo_json), corpo_json);

    cyw43_arch_lwip_begin();
    err_t erro_envio = tcp_write(pcb, requisicao, strlen(requisicao), TCP_WRITE_FLAG_COPY);
    if (erro_envio == ERR_OK)
    {
        tcp_output(pcb);
        printf("Requisicao enviada:\n%s\n", requisicao);
    } 
    else
    {
        printf("Erro ao enviar dados para nuvem: %d \n", erro_envio);
        tcp_abort(pcb);
    }
    cyw43_arch_lwip_end();
    tcp_close(pcb);

}