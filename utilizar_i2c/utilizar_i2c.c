#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306/ssd1306.h"

// Definição de pinos e canal I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

ssd1306_t display; // Variável de definição do display

/*
* Inicializa o barramento I2C
* @param i2c: Instância do barramento I2C (i2c0 ou i2c1)
* @param sda_pin: Pino SDA (Serial Data Line) = Dados
* @param scl_pin: Pino SCL (Serial Clock Line) = Clock
* @param baudrate: Taxa de transmissão em Hz
* @note Função para inicializar o barramento I2C com os pinos e taxa de transmissão especificados.
*        Configura os pinos SDA e SCL como funções I2C e ativa os resistores de pull-up.
*        A função imprime os pinos e a taxa de transmissão no console.
*/
void i2c_bus_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin, uint baudrate)
{
    i2c_init(i2c, baudrate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    printf("I2c inicializado: sda=%d, scl=%d, baudrate=%d Hz\n", sda_pin, scl_pin, baudrate);
}

/*
* Realiza a varredura do barramento I2C para encontrar dispositivos conectados.
* @param i2c: Instância do barramento I2C (i2c0 ou i2c1)
* @return Endereço do dispositivo encontrado ou 0 se nenhum dispositivo for encontrado.
* @note Esta função tenta ler de todos os endereços I2C possíveis (0x08 a 0x77) e retorna o endereço do primeiro dispositivo encontrado.
*       Se nenhum dispositivo for encontrado, retorna 0. A função imprime o status da varredura no console.
*/
uint8_t i2c_bus_scan(i2c_inst_t *i2c)
{
    printf("Iniciando scanner I2C...\n");
    uint8_t addr_device = 0x00; // Endereço do dispositivo encontrado

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        uint8_t rxdata; // Variável para armazenar os dados recebidos
        int result = i2c_read_blocking(i2c, addr, &rxdata, 1, false); // Tenta ler do dispositivo I2C
        if (result >= 0) { // Se a leitura for bem-sucedida
            addr_device = addr; // Armazena o endereço do dispositivo encontrado
            break; // Dispositivo encontrado, sai do loop
        }
        sleep_ms(5); 
    }
    printf("Scanner I2C concluído.\n");
    return addr_device; 
}

/*
* Inicializa o display OLED SSD1306.
* @param display: Ponteiro para a estrutura do display
* @param i2c: Instância do barramento I2C (i2c0 ou i2c1)
* @param addr_device: Endereço do dispositivo I2C
* @note Esta função inicializa o display OLED SSD1306 com as dimensões 128x64 e o endereço I2C especificado.
*       Ela também limpa o display.
*/
void display_init(ssd1306_t *display, i2c_inst_t *i2c, uint8_t addr_device)
{
    ssd1306_init(display, 128, 64, addr_device, i2c); // Inicializa o display com as dimensões e endereço I2C
    ssd1306_clear(display); // Limpa o display
}

/*
* Exibe um texto no display OLED SSD1306.
* @param display: Ponteiro para a estrutura do display
* @param text: Texto a ser exibido
* @note Esta função limpa o display e desenha uma string no centro do display.
*       O texto é exibido em uma fonte de tamanho 2.
*       A função também atualiza o display para mostrar o texto.
*/
void display_text(ssd1306_t *display, const char *text)
{
    ssd1306_clear(display);
    ssd1306_draw_string(display, 10, 32, 2, text); // Desenha a string no display
    ssd1306_show(display);
}

/*
* Função principal do programa.
* Inicializa o barramento I2C, realiza a varredura para encontrar o dispositivo, inicializa o display e exibe um contador.
* O contador é incrementado a cada segundo e exibido no display.
*/
int main()
{
    stdio_init_all();

    i2c_bus_init(I2C_PORT, I2C_SDA, I2C_SCL, 400000); 
    uint8_t addr_device = i2c_bus_scan(I2C_PORT);

    display_init(&display, I2C_PORT, addr_device);
    
    int count = 0;

    while (true) {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "Count: %d", count);
        display_text(&display, buffer);
        count++;
        sleep_ms(1000);
    }    
    return 0;
}
