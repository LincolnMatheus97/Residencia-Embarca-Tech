#include "servidor_tcp.h"
#include "sensores/sensores.h"
#include "pico/cyw43_arch.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
* Função para fechar a conexão do cliente e liberar os recursos associados ao estado
* @param estado_conexao Ponteiro para o estado da conexão TCP
* @note Esta função é chamada quando a conexão do cliente é fechada ou quando ocorre um erro
*/
void fechar_conexao_cliente(ESTADO_CONEXAO_TCP *estado_conexao)
{   
    // Verifica se o estado da conexão é nulo
    if (!estado_conexao) return;

    struct tcp_pcb *pcb = estado_conexao->pcb; // Ponteiro para o PCB da conexão
    
    // Verifica se o PCB não é nulo, se sim, fecha a conexão
    if (pcb)
    {
        cyw43_arch_lwip_begin();
        tcp_arg(pcb, NULL);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);
        err_t erro_fechamento = tcp_close(pcb);
        cyw43_arch_lwip_end();

        if (erro_fechamento != ERR_OK)
        {
            printf("Erro ao fechar pcb. Pode já estar fechado! Abortando...\n");
            cyw43_arch_lwip_begin();
            tcp_abort(pcb);
            cyw43_arch_lwip_end();
        }
        estado_conexao->pcb = NULL;
    }
    // Verifica se os dados foram alocados e libera a memória
    if (estado_conexao->dados_alocados && estado_conexao->dados_resposta)
    {
        free(estado_conexao->dados_resposta);
        estado_conexao->dados_resposta = NULL;
    }
    // Libera a memória do estado da conexão
    free(estado_conexao);
}

/*
* Função chamada o cliente confirma o recebimento de dados enviados anteriormente
* @param estado_conexao Ponteiro para o estado da conexão TCP
* @param pcb_cliente Ponteiro para o PCB do cliente
* @param bytes_confirmados Número de bytes confirmados pelo cliente
* @return ERR_OK se o envio for bem-sucedido, ou um código de erro em caso de falha
*/
err_t callback_dados_enviados(void *estado_conexao, struct tcp_pcb *pcb_cliente, u16_t bytes_confirmados)
{   
     // Ponteiro para o estado da conexão TCP
    ESTADO_CONEXAO_TCP *estado_atual = (ESTADO_CONEXAO_TCP *)estado_conexao;
    
    // Verifica se o estado da conexão é nulo, se sim, retorna erro de argumento
    if (!estado_atual) return ERR_ARG; // Estado nulo
    
    return enviar_chunk(estado_atual); // Envia o próximo chunk de dados
}

/*
* Função chamada quando ocorre um erro na conexão
* @param arg_estado_conexao Ponteiro para o estado da conexão TCP
* @param codigo_erro Código de erro retornado pela conexão
* @note Esta função é chamada quando ocorre um erro na conexão TCP
*/
void callback_erro_conexao(void *arg_estado_conexao, err_t codigo_erro)
{   
    // Ponteiro para o estado da conexão TCP
    ESTADO_CONEXAO_TCP *estado_atual = (ESTADO_CONEXAO_TCP *)arg_estado_conexao;
    printf("Erro %d\n", codigo_erro); // Imprime o código de erro

    // Verifica se o estado da conexão não é nulo, se sim, libera os recursos
    if (estado_atual)
    {   
        // Verifica se os dados foram alocados e libera a memória
        if (estado_atual->dados_alocados && estado_atual->dados_resposta) 
        {
            free(estado_atual->dados_resposta);     // Libera a memória alocada para os dados de resposta
            estado_atual->dados_resposta = NULL;    // Define o ponteiro de dados de resposta como nulo
        }
        // Libera a memória do estado da conexão
        free(estado_atual);
    }
}

/*
* Função para enviar os dados em chunks
* @param estado_conexao Ponteiro para o estado da conexão TCP
* @return ERR_OK se o envio for bem-sucedido, ou um código de erro em caso de falha
* @note Esta função envia os dados em partes (chunks) para evitar problemas de memória
*       pois o tamanho total dos dados pode ser maior que o buffer de envio do PCB do servidor
*/
err_t enviar_chunk(ESTADO_CONEXAO_TCP *estado_conexao)
{   
    // Verifica se o estado da conexão é nulo, ou se o PCB ou os dados de resposta são nulos, se sim, retorna erro de argumento
    if (!estado_conexao || !estado_conexao->pcb || !estado_conexao->dados_resposta) return ERR_ARG;

    struct tcp_pcb *pcb = estado_conexao->pcb; // Ponteiro para o PCB da conexão

    // Verifica se o tamanho enviado é maior ou igual ao tamanho total, se sim, fecha a conexão e retorna erro OK
    if (estado_conexao->tamanho_enviado >= estado_conexao->tamanho_total)
    {
        fechar_conexao_cliente(estado_conexao);
        return ERR_OK;
    }

    // Tamanho do próximo chunk a ser enviado
    size_t tamanho_prox_chunk = estado_conexao->tamanho_total - estado_conexao->tamanho_enviado;
    // Verifica se o tamanho do próximo chunk é maior que o tamanho máximo permitido, se sim, ajusta para o tamanho máximo
    if (tamanho_prox_chunk > TCP_SND_BUF_CHUNK_SIZE)
    {
        tamanho_prox_chunk = TCP_SND_BUF_CHUNK_SIZE;
    }

    u16_t tenho_espaco_mem = 0; // Cria variável para armazenar o espaço de memória disponível
    cyw43_arch_lwip_begin();
    // Armazena no espaço de memória disponível o espaço de envio do PCB
    tenho_espaco_mem = tcp_sndbuf(pcb);
    cyw43_arch_lwip_end();

    /*
    // Verifica se o espaço de memória disponível é menor que o tamanho do próximo chunk, se sim, envia um aviso de entrega e retorna erro OK
    // Isso pode ocorrer se o buffer de envio estiver cheio ou se o tamanho do chunk for maior que o espaço disponível
    */
    if (tenho_espaco_mem < tamanho_prox_chunk)
    {
        cyw43_arch_lwip_begin();
        tcp_output(pcb);            // Envia um aviso de entrega para o cliente
        cyw43_arch_lwip_end();
        return ERR_OK;              // Retorna erro OK para indicar que o envio foi bem-sucedido
    }

    err_t envio_chunk = ERR_OK;     // Variável para armazenar o resultado do envio do chunk
    cyw43_arch_lwip_begin();
    /*
    // Envia o chunk de dados para o cliente, usando o tamanho do próximo chunk e a flag TCP_WRITE_FLAG_COPY
    // A flag TCP_WRITE_FLAG_COPY indica que os dados devem ser copiados para o buffer de envio do PCB
    // Isso é útil para evitar problemas de concorrência e garantir que os dados sejam enviados corretamente
    */
    envio_chunk = tcp_write(pcb, estado_conexao->dados_resposta + estado_conexao->tamanho_enviado, tamanho_prox_chunk, TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();

    // Verifica se o envio do chunk foi bem-sucedido, se sim, atualiza o tamanho enviado e envia um aviso de entrega
    if (envio_chunk == ERR_OK)
    {
        estado_conexao->tamanho_enviado += tamanho_prox_chunk; // Atualiza o tamanho enviado com o tamanho do próximo chunk
        err_t erro_aviso_entrega = ERR_OK; 
        cyw43_arch_lwip_begin();
        erro_aviso_entrega = tcp_output(pcb); // Envia um aviso de entrega para o cliente
        cyw43_arch_lwip_end();

        // Verifica se o aviso de entrega foi bem-sucedido, se não, imprime mensagem de erro
        if (erro_aviso_entrega != ERR_OK)
        {
            printf("Envio chunk: Erro no envio...");
        }
        // Verifica se o tamanho enviado é maior ou igual ao tamanho total, se sim, fecha a conexão e retorna erro OK
        if (estado_conexao->tamanho_enviado >= estado_conexao->tamanho_total)
        {
            printf("Envio chunk: Ultimo chunk enviado. Fechando conexao...");
            fechar_conexao_cliente(estado_conexao);
        }
        return ERR_OK;
    }
    // Verifica se o envio do chunk falhou devido a falta de memória, se sim, imprime mensagem de erro e envia um aviso de entrega
    else if (envio_chunk == ERR_MEM)
    {
        printf("Envio chuck: Erro inesperado de memoria...");
        cyw43_arch_lwip_begin();
        tcp_output(pcb);
        cyw43_arch_lwip_end();
        return ERR_OK;
    }
    /*
    // Por fim, se o envio do chunk falhou devido a outro erro, imprime mensagem de erro e fecha a conexão e retorna erro de envio do chunk
    // Isso pode ocorrer se o PCB estiver fechado ou se houver um erro de rede
    */
    else
    {
        printf("Envio chuck: Erro fatal de envio do chuck. Fechando conexao...");
        fechar_conexao_cliente(estado_conexao);
        return envio_chunk;
    }
}

/*
* Função chamada quando o cliente envia dados para o servidor
* @param arg_estado_conexao Ponteiro para o estado da conexão TCP
* @param cliente_pcb Ponteiro para o PCB do cliente
* @param dados_recebidos Ponteiro para os dados recebidos do cliente
* @param erro_recev Código de erro retornado pela conexão
* @return ERR_OK se os dados forem recebidos com sucesso, ou um código de erro em caso de falha 
* @note Esta função é chamada quando o servidor recebe dados do cliente
*       Ela processa a requisição do cliente e envia a resposta apropriada ou um erro
*       dependendo do conteúdo da requisição
*/
err_t dados_recebidos_cliente(void *arg_estado_conexao, struct tcp_pcb *cliente_pcb, struct pbuf *dados_recebidos, err_t erro_recev)
{   
    // Ponteiro para o estado da conexão TCP
    ESTADO_CONEXAO_TCP *estado_atual = (ESTADO_CONEXAO_TCP *)arg_estado_conexao; 
    /*
    // Verifica se o erro de recebimento é diferente de ERR_OK, se sim,
    // imprime mensagem de erro e libera e verifica os dados recebidos
    // Se os dados recebidos não forem nulos, libera a memória dos dados recebidos
    // Se o estado atual não for nulo, fecha a conexão do cliente
    */
    if (erro_recev != ERR_OK)
    {
        printf("Erro de recebimento %d\n", erro_recev);
        if (dados_recebidos)
        {
            cyw43_arch_lwip_begin();
            tcp_recved(cliente_pcb, dados_recebidos->tot_len);
            pbuf_free(dados_recebidos);
            cyw43_arch_lwip_end();
        }
        if (estado_atual)
            fechar_conexao_cliente(estado_atual);
        return erro_recev;
    }
    // Verifica se os dados recebidos são nulos, se sim, imprime mensagem de desconexão e fecha a conexão do cliente
    if (!dados_recebidos)
    {
        printf("Cliente desconectou...\n");
        if (estado_atual)
            fechar_conexao_cliente(estado_atual);
        return ERR_OK;
    }

    char requisicao[1024];          // Buffer para armazenar a requisição recebida
    err_t return_err = ERR_OK;      // Variável para armazenar o erro de retorno

    cyw43_arch_lwip_begin();
    // Verifica se o tamanho dos dados recebidos é maior ou igual ao tamanho da requisição, se sim, imprime mensagem de erro e fecha a conexão do cliente
    if (dados_recebidos->tot_len >= sizeof(requisicao)) 
    {
        printf("Erro: Requisição muito longa (%u bytes)\n", dados_recebidos->tot_len);
        tcp_recved(cliente_pcb, dados_recebidos->tot_len);
        pbuf_free(dados_recebidos);
        tcp_abort(cliente_pcb);
        cyw43_arch_lwip_end();
        if (estado_atual)
            free(estado_atual);
        return ERR_MEM;
    }
    pbuf_copy_partial(dados_recebidos, requisicao, dados_recebidos->tot_len, 0);    // Copia os dados recebidos para o buffer de requisição
    requisicao[dados_recebidos->tot_len] = '\0';                                    // Adiciona o caractere nulo ao final da requisição
    tcp_recved(cliente_pcb, dados_recebidos->tot_len);                              // Indica que os dados foram recebidos
    pbuf_free(dados_recebidos);                                                     // Libera a memória dos dados recebidos
    dados_recebidos = NULL;                                                         // Define os dados recebidos como nulo para evitar acesso indevido
    cyw43_arch_lwip_end();

    printf("Recebido: %s\n", requisicao);

    /*
    // Verifica se a requisição contém o método GET e a rota /status ou /joystick, se sim, processa a requisição e envia a resposta 
    // Caso contrário, verifica se a requisição contém o método GET e a rota /, se sim, processa a requisição e envia a resposta HTML por chunks
    // Caso contrário, imprime mensagem de rota não encontrada e envia resposta 404
    */
    if (strstr(requisicao, "GET /status") != NULL)
    {
        printf("Rota /status\n");
        char resposta[256];
        snprintf(resposta, sizeof(resposta),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "{\"botao_a_press\": %s, \"botao_b_press\" : %s}",
                 botao_a_pressionado() ? "true" : "false",
                 botao_b_pressionado() ? "true" : "false");

        cyw43_arch_lwip_begin();
        err_t erro_entrega_resposta = tcp_write(cliente_pcb, resposta, strlen(resposta), TCP_WRITE_FLAG_COPY);
        if (erro_entrega_resposta == ERR_OK)
        {
            tcp_output(cliente_pcb);
        }
        else
        {
            printf("Erro %d na entrega para os status do botoes\n", erro_entrega_resposta);
        }
        tcp_arg(cliente_pcb, NULL);
        tcp_sent(cliente_pcb, NULL);
        tcp_recv(cliente_pcb, NULL);
        tcp_err(cliente_pcb, NULL);
        err_t erro_fechamento_resposta = tcp_close(cliente_pcb);
        cyw43_arch_lwip_end();
        if (erro_fechamento_resposta != ERR_OK)
        {
            printf("Erro %d ao fechar o requerimento status dos botoes. Abortando...\n", erro_fechamento_resposta);
            cyw43_arch_lwip_begin();
            tcp_abort(cliente_pcb);
            cyw43_arch_lwip_end();
        }
        if (estado_atual)
            free(estado_atual);
        return ERR_OK;
    }
    else if (strstr(requisicao, "GET /joystick") != NULL)
    {
        printf("Rota /joystick\n");
        char resposta[256];
        snprintf(resposta, sizeof(resposta),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "{\"joystick_x\": %u, \"joystick_y\": %u}",
                 ler_joystick_x(), ler_joystick_y());

        cyw43_arch_lwip_begin();
        err_t erro_entrega_resposta = tcp_write(cliente_pcb, resposta, strlen(resposta), TCP_WRITE_FLAG_COPY);
        if (erro_entrega_resposta == ERR_OK)
        {
            tcp_output(cliente_pcb);
        }
        else
        {
            printf("Erro %d na entrega da resposta para a leitura do joystick\n", erro_entrega_resposta);
        }
        tcp_arg(cliente_pcb, NULL);
        tcp_sent(cliente_pcb, NULL);
        tcp_recv(cliente_pcb, NULL);
        tcp_err(cliente_pcb, NULL);
        err_t erro_fechamento_resposta = tcp_close(cliente_pcb);
        cyw43_arch_lwip_end();
        if (erro_fechamento_resposta != ERR_OK)
        {
            printf("Erro %d ao fechar o requimento de resposta do /joystick. Abortando....\n", erro_fechamento_resposta);
            cyw43_arch_lwip_begin();
            tcp_abort(cliente_pcb);
            cyw43_arch_lwip_end();
        }
        if (estado_atual)
            free(estado_atual);
        return ERR_OK;
    }
    else if (strstr(requisicao, "GET / ") != NULL)
    {
        printf("Rota /\n");
        if (!estado_atual)
        {
            estado_atual = (ESTADO_CONEXAO_TCP *)calloc(1, sizeof(ESTADO_CONEXAO_TCP));
            if (!estado_atual)
            {
                printf("Erro ao alocar memoria para estado HTML.\n");
                cyw43_arch_lwip_begin();
                tcp_abort(cliente_pcb);
                cyw43_arch_lwip_end();
                return ERR_MEM;
            }
            estado_atual->pcb = cliente_pcb;
            estado_atual->dados_alocados = false;

            cyw43_arch_lwip_begin();
            tcp_arg(cliente_pcb, estado_atual);
            tcp_sent(cliente_pcb, callback_dados_enviados);
            tcp_err(cliente_pcb, callback_erro_conexao);
            cyw43_arch_lwip_end();
        }
        else
        {
            if (estado_atual->dados_alocados && estado_atual->dados_resposta)
            {
                free(estado_atual->dados_resposta);
                estado_atual->dados_resposta = NULL;
            }
            estado_atual->pcb = cliente_pcb;
            estado_atual->tamanho_enviado = 0;
            estado_atual->tamanho_total = 0;
            estado_atual->dados_alocados = false;
        }

        const size_t tamanho_buffer_html = 4096;    // Tamanho do buffer para armazenar o HTML
        // Aloca memória para armazenar o HTML
        estado_atual->dados_resposta = (char *)malloc(tamanho_buffer_html);
        // Verifica se a alocação foi bem-sucedida, se não, imprime mensagem libera a memória do estado atual e aborta a conexão
        if (!estado_atual->dados_resposta)
        {
            printf("Erro ao alocar memoria para HTML.\n");
            free(estado_atual);
            cyw43_arch_lwip_begin();
            tcp_arg(cliente_pcb, NULL);
            tcp_abort(cliente_pcb);
            cyw43_arch_lwip_end();
            return ERR_MEM;
        }
        estado_atual->dados_alocados = true;    // Marca que os dados foram alocados

        /*
        // Preenche o buffer com o HTML
        // O HTML contém informações sobre o status dos botões e a posição do joystick
        // O JavaScript faz requisições periódicas para atualizar o status dos botões e a posição do joystick
        */
        snprintf(estado_atual->dados_resposta, tamanho_buffer_html,
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html; charset=UTF-8\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "<!DOCTYPE html>\n"
                 "<html>\n"
                 "<head>\n"
                 "<meta charset=\"UTF-8\">\n"
                 "<title>Pico W Status</title>\n"
                 "<style>\n"
                 "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; font-size: 24px; }\n"
                 ".status { font-weight: bold; color: #555; margin-bottom: 15px; }\n"
                 "h1 { margin-bottom: 10px; }\n"
                 "</style>\n"
                 "</head>\n"
                 "<body>\n"
                 "<h1>Status dos Botões</h1>\n"
                 "<p>Botão A: <span id=\"buttonAStatus\" class=\"status\">Aguardando...</span></p>\n"
                 "<p>Botão B: <span id=\"buttonBStatus\" class=\"status\">Aguardando...</span></p>\n"
                 "<h1>Posição do Joystick</h1>\n"
                 "<p>Eixo X: <span id=\"joystickXStatus\" class=\"status\">Aguardando...</span></p>\n"
                 "<p>Eixo Y: <span id=\"joystickYStatus\" class=\"status\">Aguardando...</span></p>\n"
                 "<script>\n"
                 "const statusElementA = document.getElementById('buttonAStatus');\n"
                 "const statusElementB = document.getElementById('buttonBStatus');\n"
                 "const joystickXElement = document.getElementById('joystickXStatus');\n"
                 "const joystickYElement = document.getElementById('joystickYStatus');\n"
                 "\n"
                 "function updateStatus() {\n"
                 "  fetch('/status')\n"
                 "    .then(response => response.json())\n"
                 "    .then(data => {\n"
                 "      statusElementA.textContent = data.botao_a_press ? 'Pressionado!' : 'Solto';\n"
                 "      statusElementA.style.color = data.botao_a_press ? 'red' : '#555';\n"
                 "      statusElementB.textContent = data.botao_b_press ? 'Pressionado!' : 'Solto';\n"
                 "      statusElementB.style.color = data.botao_b_press ? 'blue' : '#555';\n"
                 "    })\n"
                 "    .catch(error => {\n"
                 "      console.error('Erro ao buscar status dos botões:', error);\n"
                 "      statusElementA.textContent = 'Erro';\n"
                 "      statusElementA.style.color = 'orange';\n"
                 "      statusElementB.textContent = 'Erro';\n"
                 "      statusElementB.style.color = 'orange';\n"
                 "    });\n"
                 "\n"
                 "  fetch('/joystick')\n"
                 "    .then(response => response.json())\n"
                 "    .then(data => {\n"
                 "      joystickXElement.textContent = data.joystick_x;\n"
                 "      joystickYElement.textContent = data.joystick_y;\n"
                 "      joystickXElement.style.color = 'green';\n"
                 "      joystickYElement.style.color = 'green';\n"
                 "    })\n"
                 "    .catch(error => {\n"
                 "      console.error('Erro ao buscar status do joystick:', error);\n"
                 "      joystickXElement.textContent = 'Erro';\n"
                 "      joystickXElement.style.color = 'orange';\n"
                 "      joystickYElement.textContent = 'Erro';\n"
                 "      joystickYElement.style.color = 'orange';\n"
                 "    });\n"
                 "}\n"
                 "setInterval(updateStatus, 1000);\n"
                 "document.addEventListener('DOMContentLoaded', updateStatus);\n"
                 "</script>\n"
                 "</body>\n"
                 "</html>\n");

        estado_atual->tamanho_total = strlen(estado_atual->dados_resposta); // Armazena o tamanho total dos dados de resposta
        estado_atual->tamanho_enviado = 0;
        
        
        printf("Iniciando envio HTML, separando por chunks...\n"); 
        err_t erro_envio_chunk = enviar_chunk(estado_atual);    // Começa o envio dos chunk de dados
        // Verifica se o envio do chunk falhou, se sim, imprime mensagem de erro e fecha a conexão do cliente
        if (erro_envio_chunk != ERR_OK && erro_envio_chunk != ERR_MEM)
        {
            printf("Erro inicial %d ao chamar enviar os chunks para HTML.\n", erro_envio_chunk);
            if (estado_atual)
            {
                fechar_conexao_cliente(estado_atual); // Fecha a conexão do cliente
            }
            return erro_envio_chunk; // Retorna o erro de envio do chunk
        }
    }
    else
    {
        printf("Rota não encontrada '%s'. Enviando 404.\n", requisicao);
        const char *resp404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        cyw43_arch_lwip_begin();
        tcp_write(cliente_pcb, resp404, strlen(resp404), TCP_WRITE_FLAG_COPY);
        tcp_output(cliente_pcb);
        tcp_arg(cliente_pcb, NULL);
        tcp_sent(cliente_pcb, NULL);
        tcp_recv(cliente_pcb, NULL);
        tcp_err(cliente_pcb, NULL);
        err_t erro_fechamento = tcp_close(cliente_pcb);
        cyw43_arch_lwip_end();
        if (erro_fechamento != ERR_OK)
        {
            printf("Erro %d ao fechar 404. Abortando...\n", erro_fechamento);
            cyw43_arch_lwip_begin();
            tcp_abort(cliente_pcb);
            cyw43_arch_lwip_end();
        }
        if (estado_atual)
            free(estado_atual);
    }

    return return_err;
}

/*
* Função chamada quando uma nova conexão é aceita pelo servidor
* @param arg_aceite Ponteiro para os argumentos de aceite (não utilizado)
* @param pcb_cliente Ponteiro para o PCB do cliente
* @param erro_aceite Código de erro retornado pela conexão
* @return ERR_OK se a conexão for aceita com sucesso, ou um código de erro em caso de falha
* @note Esta função é chamada quando um cliente se conecta ao servidor TCP
*/
err_t nova_conexao_aceita(void *arg_aceite, struct tcp_pcb *pcb_cliente, err_t erro_aceite)
{
    LWIP_UNUSED_ARG(arg_aceite); // Ignora o argumento de aceite
    // Verifica se o erro de aceite é diferente de ERR_OK ou se o PCB do cliente é nulo, se sim, imprime mensagem de erro e retorna erro de valor
    if (erro_aceite != ERR_OK || pcb_cliente == NULL)
    {
        printf("Erro ao aceitar, nova conexao: %d\n", erro_aceite);
        return ERR_VAL;
    }

    printf("Nova conexao aceita de %s:%d\n", ipaddr_ntoa(&pcb_cliente->remote_ip), pcb_cliente->remote_port);

    cyw43_arch_lwip_begin();
    tcp_setprio(pcb_cliente, TCP_PRIO_NORMAL);              // Define a prioridade do PCB do cliente
    tcp_arg(pcb_cliente, NULL);                             // Define o argumento do PCB do cliente como nulo
    tcp_recv(pcb_cliente, dados_recebidos_cliente);         // Define a função de recebimento de dados do PCB do cliente
    cyw43_arch_lwip_end();

    return ERR_OK;
}
/*
* Função para inicializar o servidor TCP
* @param numero_porta Número da porta em que o servidor irá escutar
* @return ERR_OK se a inicialização for bem-sucedida, ou um código de erro em caso de falha
*/
err_t inicializar_servidor_tcp(uint16_t numero_porta)
{
    struct tcp_pcb *servidor_pcb = NULL;    // Ponteiro para o PCB do servidor
    struct tcp_pcb *fila_conexao = NULL;    // Ponteiro para a fila de conexões do servidor

    printf("Criando PCB...\n");
    cyw43_arch_lwip_begin();
    servidor_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);    // Cria um novo PCB para o servidor
    cyw43_arch_lwip_end();
    if (!servidor_pcb)
    {
        printf("Erro ao criar PCB...\n");
        return ERR_MEM;
    }

    printf("Fazendo o enderecamento na porta %u...\n", numero_porta);
    err_t endereco_ip = ERR_OK;             // Variável para armazenar o erro de endereçamento IP
    cyw43_arch_lwip_begin();
    endereco_ip = tcp_bind(servidor_pcb, IP_ANY_TYPE, numero_porta);   // Faz o bind do PCB do servidor ao endereço IP e porta especificados
    cyw43_arch_lwip_end();
    // Verifica se o endereçamento IP falhou, se sim, imprime mensagem de erro e fecha o PCB do servidor
    if (endereco_ip != ERR_OK)
    {
        printf("Erro no enderecamento de IP do servidor: %d\n", endereco_ip);
        cyw43_arch_lwip_begin();
        tcp_close(servidor_pcb);
        cyw43_arch_lwip_end();
        return endereco_ip;
    }

    printf("Colocando em modo de escuta o servidor...\n");
    cyw43_arch_lwip_begin();
    fila_conexao = tcp_listen(servidor_pcb);   // Coloca o PCB do servidor em modo de escuta
    cyw43_arch_lwip_end();
    if (!fila_conexao)
    {
        printf("Erro na fila de conexao do servidor.\n");
        return ERR_MEM;
    }

    cyw43_arch_lwip_begin();
    tcp_accept(fila_conexao, nova_conexao_aceita); // Define a função de aceite de novas conexões
    cyw43_arch_lwip_end();

    printf("Servidor inicializado com sucesso.\n");
    return ERR_OK;
}