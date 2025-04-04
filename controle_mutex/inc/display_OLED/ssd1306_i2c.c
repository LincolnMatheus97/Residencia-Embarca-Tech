#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "inc/display_OLED/ssd1306_font.h"
#include "inc/display_OLED/ssd1306_i2c.h"

// Calcular quanto do buffer será destinado à área de renderização
void calculate_render_area_buffer_length(struct render_area *area) {
    area->buffer_length = (area->end_column - area->start_column + 1) * (area->end_page - area->start_page + 1);
}

// Processo de escrita do i2c espera um byte de controle, seguido por dados
void ssd1306_send_command(uint8_t command) {
    uint8_t buffer[2] = {0x80, command};
    i2c_write_blocking(i2c1, ssd1306_i2c_address, buffer, 2, false);
}

// Envia uma lista de comandos ao hardware
void ssd1306_send_command_list(uint8_t *ssd, int number) {
    for (int i = 0; i < number; i++) {
        ssd1306_send_command(ssd[i]);
    }
}

// Copia buffer de referência num novo buffer, a fim de adicionar o byte de controle desde o início
void ssd1306_send_buffer(uint8_t ssd[], int buffer_length) {
    uint8_t *temp_buffer = malloc(buffer_length + 1);

    temp_buffer[0] = 0x40;
    memcpy(temp_buffer + 1, ssd, buffer_length);

    i2c_write_blocking(i2c1, ssd1306_i2c_address, temp_buffer, buffer_length + 1, false);

    free(temp_buffer);
}

// Cria a lista de comandos (com base nos endereços definidos em ssd1306_i2c.h) para a inicialização do display
void ssd1306_init() {
    uint8_t commands[] = {
        ssd1306_set_display, ssd1306_set_memory_mode, 0x00,
        ssd1306_set_display_start_line, ssd1306_set_segment_remap | 0x01, 
        ssd1306_set_mux_ratio, ssd1306_height - 1,
        ssd1306_set_common_output_direction | 0x08, ssd1306_set_display_offset,
        0x00, ssd1306_set_common_pin_configuration,
    
#if ((ssd1306_width == 128) && (ssd1306_height == 32))
    0x02,
#elif ((ssd1306_width == 128) && (ssd1306_height == 64))
    0x12,
#else
    0x02,
#endif
        ssd1306_set_display_clock_divide_ratio, 0x80, ssd1306_set_precharge,
        0xF1, ssd1306_set_vcomh_deselect_level, 0x30, ssd1306_set_contrast,
        0xFF, ssd1306_set_entire_on, ssd1306_set_normal_display,
        ssd1306_set_charge_pump, 0x14, ssd1306_set_scroll | 0x00,
        ssd1306_set_display | 0x01,
    };

    ssd1306_send_command_list(commands, count_of(commands));
}

// Atualiza uma parte do display com uma área de renderização
void render_on_display(uint8_t *ssd, struct render_area *area) {
    uint8_t commands[] = {
        ssd1306_set_column_address, area->start_column, area->end_column,
        ssd1306_set_page_address, area->start_page, area->end_page
    };

    ssd1306_send_command_list(commands, count_of(commands));
    ssd1306_send_buffer(ssd, area->buffer_length);
}

// Adquire os pixels para um caractere (de acordo com ssd1306_font.h)
inline int ssd1306_get_font(uint8_t character)
{
  if (character >= 'A' && character <= 'Z') {
    return character - 'A' + 1;
  }
  else if (character >= '0' && character <= '9') {
    return character - '0' + 27;
  }
  else
    return 0;
}

// Desenha um único caractere no display
void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) {
        return;
    }

    y = y / 8;

    character = toupper(character);
    int idx = ssd1306_get_font(character);
    int fb_idx = y * 128 + x;

    for (int i = 0; i < 8; i++) {
        ssd[fb_idx++] = font[idx * 8 + i];
    }
}

// Desenha uma string, chamando a função de desenhar caractere várias vezes
void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, char *string) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) {
        return;
    }

    while (*string) {
        ssd1306_draw_char(ssd, x, y, *string++);
        x += 8;
    }
}
