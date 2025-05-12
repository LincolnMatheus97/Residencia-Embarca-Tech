#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306/ssd1306.h"

#define I2C_PORT i2c1
#define SDA_PIN 14
#define SCL_PIN 15

ssd1306_t display;

void inic_bar_i2c(i2c_inst_t *i2c, uint sda, uint scl, uint baundrate)
{
    i2c_init(i2c, baundrate);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

uint8_t end_disp_i2c(i2c_inst_t *i2c)
{
    uint8_t endereco = 0x00;

    for (uint8_t end_atual = 0x08; end_atual <= 0x77; end_atual++)
    {
        uint8_t dados_vistos;
        int confirmacao = i2c_read_blocking(i2c, end_atual, &dados_vistos, 1, false);

        if (confirmacao >= 0)
        {
            endereco = end_atual;
            break;
        }
        sleep_ms(50);
    }
    return endereco;
}

void inic_display(ssd1306_t *display, uint16_t larg_disp, uint16_t alt_disp, uint8_t end_disp, i2c_inst_t *i2c)
{
    ssd1306_init(display, larg_disp, alt_disp, end_disp, i2c);
}

void escrev_display(ssd1306_t *display, uint32_t larg_texto, uint32_t alt_texto, uint32_t tam_texto, const char* texto)
{
    ssd1306_draw_string(display, larg_texto, alt_texto, tam_texto, texto);
}

void exib_display(ssd1306_t *display)
{
    ssd1306_show(display);
}

void limp_display(ssd1306_t *display)
{
    ssd1306_clear(display);
}

int main()
{
    stdio_init_all();
    inic_bar_i2c(I2C_PORT, SDA_PIN, SCL_PIN, 400 * 1000);

    uint8_t ender_disp = end_disp_i2c(I2C_PORT);
    inic_display(&display, 128, 64, ender_disp, I2C_PORT);
    
    int contador = 0;

    while (1)
    {
        limp_display(&display);

        char texto_1[20];
        snprintf(texto_1, sizeof(texto_1), "Protc. Comunic. I2C", texto_1);
        escrev_display(&display, 0, 0, 1, texto_1);
        
        char texto_2[20];
        snprintf(texto_2, sizeof(texto_2), "CONT: %d", contador);
        escrev_display(&display, 10, 32, 2, texto_2);

        exib_display(&display);
        (contador >= 20) ? contador = 0 : contador++;
        sleep_ms(1000);
    }
    return 0;
}