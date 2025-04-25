# Pico W - Servidor Web para Controle e Monitoramento

Este projeto demonstra a criação de um servidor web embarcado em um Raspberry Pi Pico W, utilizando a linguagem C e a pilha TCP/IP lwIP. O servidor permite monitorar o estado de botões e de um joystick conectados ao Pico W através de uma interface web acessível na rede local via Wi-Fi.

O sistema funciona de duas maneiras principais:

1.  **Servidor Web:** Serve uma página HTML interativa que exibe o estado atual dos sensores (botões e joystick) em tempo real.
2.  **API Simples:** Fornece endpoints (`/status` e `/joystick`) que retornam o estado atual dos sensores em formato JSON, permitindo que outras aplicações ou a própria página HTML consumam esses dados.

## Funcionalidades Atuais

*   Conexão Wi-Fi à rede local.
*   Servidor HTTP rodando na porta 80.
*   Página HTML dinâmica exibindo:
    *   Estado do Botão A (Pressionado/Solto).
    *   Estado do Botão B (Pressionado/Solto).
    *   Valor do Eixo X do Joystick.
    *   Valor do Eixo Y do Joystick.
*   Endpoint `/status` (JSON) para estado dos botões.
*   Endpoint `/joystick` (JSON) para valores do joystick.
*   Atualização automática da interface web a cada segundo usando JavaScript (`fetch`).
*   Implementação robusta de envio de dados TCP para lidar com respostas HTML maiores que o buffer de envio.

## Linha do Tempo da Evolução do Projeto

Este projeto passou por várias fases de desenvolvimento, começando de uma prova de conceito simples até a versão atual mais completa e robusta.

### Fase 1: Prova de Conceito - Monitoramento Básico (Botão A)

*   **Objetivo:** Criar um servidor web mínimo no Pico W capaz de se conectar ao Wi-Fi e exibir o estado de um único botão (Botão A) em uma página web.
*   **Implementação Inicial:**
    *   O código inicial focava em estabelecer a conexão Wi-Fi e configurar um servidor TCP básico usando a lwIP.
    *   Uma única função de callback (`tcp_server_recv` na versão antiga) era responsável por receber a requisição HTTP do cliente.
    *   Dentro dessa callback, o código verificava se a requisição era para a raiz (`GET /`) ou para o status (`GET /status`).
    *   Para `/status`, lia o estado do Botão A e montava uma resposta JSON simples.
    *   Para `/`, montava uma string contendo todo o código HTML da página.
    *   **Tentativa de Envio Direto:** A resposta (JSON ou HTML) era enviada de uma só vez usando `tcp_write()`. A conexão era fechada (`tcp_close()`) logo em seguida.
*   **Limitações Encontradas:**
    *   **Falha no Envio de HTML:** O principal problema surgiu ao tentar enviar a página HTML. Como o código HTML era relativamente grande (para os padrões de um microcontrolador), ele frequentemente excedia o tamanho do buffer de envio do TCP (`TCP_SND_BUF`) disponível na lwIP. Isso resultava em chamadas `tcp_write()` retornando `ERR_MEM` (erro de memória/buffer cheio), e a página não era enviada corretamente ou ficava incompleta.
    *   **Fechamento Prematuro:** Fechar a conexão imediatamente após `tcp_write()` não garantia que todos os dados no buffer tivessem sido efetivamente transmitidos pela rede antes do fechamento.

### Fase 2: Superando Limitações - Envio por Chunks e Gerenciamento de Estado

*   **Necessidade:** Era preciso uma forma de enviar a resposta HTML, que era grande demais para o buffer, de maneira confiável.
*   **Solução Implementada:**
    *   **Envio por Chunks (Pedaços):** A estratégia adotada foi dividir a resposta HTML em pedaços menores (chunks) que coubessem no buffer de envio do TCP.
    *   **Gerenciamento de Estado (`ESTADO_CONEXAO_TCP`):** Foi criada uma estrutura (`ESTADO_CONEXAO_TCP`) para armazenar informações sobre o processo de envio para cada conexão:
        *   Ponteiro para o PCB da conexão (`pcb`).
        *   Ponteiro para os dados completos da resposta (`dados_resposta`).
        *   Tamanho total da resposta (`tamanho_total`).
        *   Quantidade de dados já enviados (`tamanho_enviado`).
        *   Flag indicando se `dados_resposta` foi alocado dinamicamente (`dados_alocados`).
    *   **Função `enviar_chunk()`:** Uma função dedicada foi criada para calcular o tamanho do próximo chunk a ser enviado (baseado no espaço disponível no buffer e nos dados restantes) e chamar `tcp_write()` com esse chunk.
    *   **Callback `tcp_sent` (`callback_dados_enviados()`):** Este callback é fundamental. A lwIP o chama *depois* que o cliente confirma o recebimento de dados enviados anteriormente. Isso indica que há mais espaço no buffer de envio. Dentro deste callback, chamamos `enviar_chunk()` novamente para enviar o próximo pedaço. Esse ciclo continua até que todos os dados sejam enviados.
    *   **Alocação Dinâmica:** Para otimizar o uso de memória RAM (escassa no Pico W), o buffer para armazenar o HTML (`estado_atual->dados_resposta`) passou a ser alocado dinamicamente com `malloc()` apenas quando a rota `/` é requisitada, e liberado com `free()` na função `fechar_conexao_cliente()`.
    *   **Fechamento Controlado:** A conexão só é fechada (`fechar_conexao_cliente()`) após o envio bem-sucedido do último chunk ou em caso de erro irrecuperável.

### Fase 3: Expansão de Funcionalidades - Botão B e Joystick

*   **Objetivo:** Tornar a aplicação mais interativa e útil, monitorando mais sensores.
*   **Implementação:**
    *   Adicionado código para inicializar e ler o estado de um segundo botão (Botão B).
    *   Adicionado código para inicializar e ler os valores dos eixos X e Y de um joystick analógico conectado aos pinos ADC.
    *   O endpoint `/status` foi atualizado para incluir o estado do Botão B no JSON.
    *   Um novo endpoint `/joystick` foi criado para fornecer os valores X e Y do joystick em formato JSON.
    *   A página HTML e o JavaScript foram atualizados para:
        *   Exibir o status do Botão B.
        *   Exibir os valores X e Y do Joystick.
        *   Fazer requisições `fetch` separadas para `/status` e `/joystick` a cada segundo para obter os dados atualizados.

### Estado Atual: Servidor Híbrido Robusto

O projeto agora representa um servidor web embarcado mais completo e robusto. Ele demonstra como:

*   Servir conteúdo estático (HTML) e dinâmico (JSON via API).
*   Lidar com as limitações de memória e buffer de rede em microcontroladores usando técnicas como envio por chunks e gerenciamento de estado da conexão.
*   Criar uma interface web interativa que consome dados de endpoints de API no próprio dispositivo.

## Conceitos Fundamentais Utilizados

*   **TCP/IP:** Conjunto de protocolos base para a comunicação na Internet e redes locais, garantindo entrega confiável dos dados.
*   **HTTP:** Protocolo de aplicação (sobre TCP) usado para a comunicação entre o navegador (cliente) e o servidor web (Pico W).
*   **lwIP:** Implementação leve da pilha TCP/IP, adequada para sistemas embarcados como o Pico W.
*   **PCB (Protocol Control Block):** Estrutura interna da lwIP usada para gerenciar o estado e os recursos de cada conexão TCP individual. Essencial para que o servidor possa lidar com clientes.
*   **Callbacks:** Funções (como `dados_recebidos_cliente`, `callback_dados_enviados`, `nova_conexao_aceita`) que são registradas na lwIP e chamadas por ela quando eventos específicos ocorrem na rede (dados recebidos, dados enviados com sucesso, nova conexão, erro).
*   **Envio por Chunks:** Técnica de dividir dados grandes em pedaços menores para envio sobre TCP, necessária quando o tamanho dos dados excede a capacidade do buffer de envio.

## Tecnologias

*   **Hardware:** BitDogLab (Raspberry Pi Pico W), botões analogicos e joystick
*   **Linguagem:** C
*   **SDK:** Raspberry Pi Pico SDK
*   **Rede:** Biblioteca lwIP (integrada ao SDK), Driver CYW43 (para Wi-Fi)
*   **Interface:** HTML, CSS, JavaScript (com `fetch` API)