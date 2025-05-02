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

// Forward declaration
void enviar_requisicao_tcp(struct tcp_pcb *pcb);

static void callback_dns_resolvido(const char *nome_host, const ip_addr_t *ip_resolvido, void *arg) {
    if (!ip_resolvido) {
        printf("Erro: DNS não conseguiu resolver o host %s\n", nome_host);
        return;
    }

    printf("DNS resolveu o host %s para %s\n", nome_host, ipaddr_ntoa(ip_resolvido));

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) {
        printf("Erro ao criar pcb\n");
        return;
    }

    err_t erro_conexao = tcp_connect(pcb, ip_resolvido, PORTA_HTTP, NULL);
    if (erro_conexao != ERR_OK) {
        printf("Erro ao conectar: %d\n", erro_conexao);
        tcp_abort(pcb);
        return;
    }

    enviar_requisicao_tcp(pcb);
}

void enviar_dados_para_nuvem() {
    ip_addr_t endereco_ip;

    err_t resultado_dns = dns_gethostbyname(HOST, &endereco_ip, callback_dns_resolvido, NULL);
    if (resultado_dns == ERR_OK) {
        // IP já está em cache
        printf("Host %s já resolvido para %s\n", HOST, ipaddr_ntoa(&endereco_ip));
        struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
        if (!pcb) {
            printf("Erro ao criar pcb (cache)\n");
            return;
        }

        err_t erro_conexao = tcp_connect(pcb, &endereco_ip, PORTA_HTTP, NULL);
        if (erro_conexao != ERR_OK) {
            printf("Erro ao conectar (cache): %d\n", erro_conexao);
            tcp_abort(pcb);
            return;
        }

        enviar_requisicao_tcp(pcb);
    } else if (resultado_dns == ERR_INPROGRESS) {
        printf("Resolução DNS em andamento...\n");
    } else {
        printf("Erro inesperado ao iniciar resolução DNS: %d\n", resultado_dns);
    }
}

void enviar_requisicao_tcp(struct tcp_pcb *pcb) {
    // Coletar dados
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
    if (erro_envio == ERR_OK) {
        tcp_output(pcb);
        printf("Requisição enviada:\n%s\n", requisicao);
    } else {
        printf("Erro ao enviar dados para nuvem: %d\n", erro_envio);
        tcp_abort(pcb);
    }
    cyw43_arch_lwip_end();

    tcp_close(pcb);
}