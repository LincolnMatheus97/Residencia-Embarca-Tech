#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306/ssd1306.h"

#define I2C_PORT i2c1
#define SDA_PIN 14
#define SCL_PIN 15

ssd1306_t display;

void inic_i2c(i2c_inst_t *i2c, uint sda, uint scl, uint baundrate)
{
    i2c_init(i2c, baundrate);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

uint8_t ender_dispo_i2c(i2c_inst_t *i2c)
{
    uint8_t endereco = 0x00;

    for (uint8_t end_atual = 0x08; end_atual < 0x77; end_atual++)
    {   
        uint8_t dados_visto;
        int confirmacao = i2c_read_blocking(i2c, end_atual, &dados_visto, 1, false);
        if (confirmacao > 0)
        {
            endereco = end_atual;
            break;
        }
        sleep_ms(50);
    }
    return endereco;
}

void inic_display(ssd1306_t *display, uint8_t end_display, i2c_inst_t *i2c)
{
    ssd1306_init(display, 128, 64, end_display, i2c);
}

void escrev_display(ssd1306_t *display, uint32_t x_texto, uint32_t y_texto, uint32_t tam_fonte, const char *texto)
{
    ssd1306_draw_string(display, x_texto, y_texto, tam_fonte, texto);
}

void mostrar_display(ssd1306_t *display)
{
    ssd1306_show(display);
}

void limpar_display(ssd1306_t *display)
{
    ssd1306_clear(display);
}

int main()
{
    stdio_init_all();

    inic_i2c(I2C_PORT, SDA_PIN, SCL_PIN, 400 * 1000);
    
    uint8_t end_display = ender_dispo_i2c(I2C_PORT);
    inic_display(&display, end_display, I2C_PORT);

    int contador = 0;

    while (1)
    {   
        limpar_display(&display);

        char titulo[20];
        snprintf(titulo, sizeof(titulo), "PROTOCOLO I2C");
        escrev_display(&display, 20, 0, 1, titulo);

        char mensagem[20];
        snprintf(mensagem, sizeof(mensagem), "CONT: %d", contador);
        escrev_display(&display, 0, 32, 2, mensagem);

        mostrar_display(&display);
        (contador >= 20) ? contador = 0 : contador++;
        sleep_ms(1000);
    }

    return 0;
}