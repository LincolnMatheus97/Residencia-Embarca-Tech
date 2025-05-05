#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306/ssd1306.h"

#define CANAL_I2C i2c1
#define PIN_SDA 14
#define PIN_SCL 15

ssd1306_t display;

void inic_comun_i2c(i2c_inst_t *i2c, uint sda, uint scl, uint baudrate)
{
    i2c_init(i2c, baudrate);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

uint8_t ender_dispo_conect(i2c_inst_t *i2c)
{
    uint8_t end_disp = 0x00;
    for (uint8_t end_atual = 0x08; end_atual <= 0x77; end_atual++)
    {
        uint8_t dados_vistos;
        int confirmacao = i2c_read_blocking(i2c, end_atual, &dados_vistos, 1, false);
        if (confirmacao >= 0)
        {
            end_disp = end_atual;
            break;
        }
    }
    return end_disp;
}

void inic_display(ssd1306_t *display, uint16_t larg_disp, uint16_t alt_disp, uint8_t end_disp, i2c_inst_t *i2c)
{
    ssd1306_init(display, larg_disp, alt_disp, end_disp, i2c);
    ssd1306_clear(display);
}

void mostr_display(ssd1306_t *display, uint32_t larg_texto, uint32_t alt_texto, uint32_t font_texto, const char* texto)
{
    ssd1306_clear(display);
    ssd1306_draw_string(display, larg_texto, alt_texto, font_texto, texto);
    ssd1306_show(display);
}

int main()
{
    stdio_init_all();
    inic_comun_i2c(CANAL_I2C, PIN_SDA, PIN_SCL, 400 * 1000);
    
    uint8_t end_disply = ender_dispo_conect(CANAL_I2C);
    inic_display(&display, 128, 64, end_disply, CANAL_I2C);

    int contador = 0;

    while (1)
    {
        char texto[20];
        snprintf(texto, sizeof(texto), "Count %d", contador);

        mostr_display(&display, 10, 32, 2, texto);
        (contador >= 20) ? contador = 0 : contador ++;
        sleep_ms(1000);
    }
    return 0;
}