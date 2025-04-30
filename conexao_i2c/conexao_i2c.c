#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306/ssd1306.h"

#define CANAL_I2C i2c1
#define SDA_I2C 14
#define SCL_I2C 15

ssd1306_t display;

void inic_comun_i2c(i2c_inst_t *i2c, uint sda, uint scl, uint baudrate)
{
    i2c_init(i2c, baudrate);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);

    printf("Comunicao I2C estabelecida... SDA = %d, SCL = %d e BOUDRATE = %d\n", sda, scl, baudrate);
}

uint8_t detec_disp_i2c(i2c_inst_t *i2c)
{   
    uint8_t end_dispo = 0x00;
    int quant_dispo = 0;
    for (uint8_t end = 0x08; end <= 0x77; end++)
    {
        uint8_t dado_lidos;
        int bytes_lidos = i2c_read_blocking(i2c, end, &dado_lidos, 1, false);
        if (bytes_lidos >= 0)
        {
            end_dispo = end;
            break;
        }
        sleep_ms(5);
    }
    return end_dispo;
}

void inic_display(ssd1306_t *display, uint16_t largura_disp, uint16_t altura_disp, uint8_t end_disp, i2c_inst_t *i2c)
{
    ssd1306_init(display, largura_disp, altura_disp, end_disp, i2c);
    ssd1306_clear(display);
}

void mostrar_display(ssd1306_t *display, uint32_t larg_texto, uint32_t altr_texto, uint32_t tamanho_font, const char *texto)
{
    ssd1306_clear(display);
    ssd1306_draw_string(display, larg_texto, altr_texto, tamanho_font, texto);
    ssd1306_show(display);
}

int main(){
    stdio_init_all();

    inic_comun_i2c(CANAL_I2C, SDA_I2C, SCL_I2C, 40 * 1000);
    uint8_t end_display = detec_disp_i2c(CANAL_I2C);

    inic_display(&display, 128, 64, end_display, CANAL_I2C);

    int contador = 0;

    while (1)
    {
        char texto[20];
        snprintf(texto, sizeof(texto), "Count %d", contador);

        mostrar_display(&display, 10, 32, 2, texto);

        (contador >= 20) ? contador = 0 : contador++;

        sleep_ms(1000);
    }
    return 0;
}