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

uint8_t id_disp_conectado(i2c_inst_t *i2c)
{
    uint8_t end_dispo = 0x00;
    for (uint8_t end_provisorio = 0x08; end_provisorio <= 0x77; end_provisorio++)
    {
        uint8_t dados_visto;
        int bytes_lidos = i2c_read_blocking(i2c, end_provisorio, &dados_visto, 1, false);
        if (bytes_lidos >= 0)
        {
            end_dispo = end_provisorio;
            break;
        }
    }
    return end_dispo;
}

void inic_display(ssd1306_t *display, uint16_t larg_disp, uint16_t alt_disp, uint8_t end_dispo, i2c_inst_t *i2c)
{
    ssd1306_init(display, larg_disp, alt_disp, end_dispo, i2c);
    ssd1306_clear(display);
}

void exibir_display(ssd1306_t *display, uint32_t larg_text, uint32_t alt_text, uint32_t tam_font, const char *texto)
{
    ssd1306_clear(display);
    ssd1306_draw_string(display, larg_text, alt_text, tam_font, texto);
    ssd1306_show(display);
}

int main()
{
    stdio_init_all();

    inic_comun_i2c(CANAL_I2C, PIN_SDA, PIN_SCL, 400 * 1000);
    uint8_t end_display = id_disp_conectado(CANAL_I2C);
    
    inic_display(&display, 128, 64, end_display, CANAL_I2C);

    int contador = 0;

    while (1)
    {
        char texto[20];
        snprintf(texto, sizeof(texto), "COUNT %d", contador);

        exibir_display(&display, 10, 32, 2, texto);
        (contador >= 20) ? contador = 0 : contador ++;

        sleep_ms(1000);
    }
    
    return 0;
}