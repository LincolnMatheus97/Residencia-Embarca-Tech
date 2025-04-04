#include "ssd1306_i2c.h"

// Calcula o tamanho do buffer de renderização
extern void calculate_render_area_buffer_length(struct render_area *area);                  

// Envia um comando para o display
extern void ssd1306_send_command(uint8_t cmd);                                              

// Envia uma lista de comandos para o display
extern void ssd1306_send_command_list(uint8_t *ssd, int number);                            

// Envia o buffer para o display
extern void ssd1306_send_buffer(uint8_t ssd[], int buffer_length);                          

// Inicializa o display
extern void ssd1306_init();                                                                 

// Atualiza uma parte do display com uma área de renderização
extern void render_on_display(uint8_t *ssd, struct render_area *area);                      

// Desenha um único caractere no display
extern void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character);       

// Desenha uma string, chamando a função de desenhar caractere várias vezes
extern void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, char *string);         