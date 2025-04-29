#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306/ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

ssd1306_t display; 

void i2c_bus_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin, uint baudrate)
{
    i2c_init(i2c, baudrate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    printf("I2c inicializado: sda=%d, scl=%d, baudrate=%d Hz\n", sda_pin, scl_pin, baudrate);
}

uint8_t i2c_bus_scan(i2c_inst_t *i2c)
{
    printf("Iniciando scanner I2C...\n");
    uint8_t addr_device = 0x00;

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        uint8_t rxdata;
        int result = i2c_read_blocking(i2c, addr, &rxdata, 1, false);
        if (result >= 0) {
            addr_device = addr;
        }
        sleep_ms(5); 
    }
    return addr_device;
    printf("Scanner finalizado.\n");
}

void display_init(ssd1306_t *display, i2c_inst_t *i2c, uint8_t addr_device)
{
    ssd1306_init(display, 128, 64, addr_device, i2c);
    ssd1306_clear(display);
    printf("Display inicializado: %d\n", addr_device);
}

void display_text(ssd1306_t *display, const char *text)
{
    ssd1306_clear(display);
    ssd1306_draw_string(display, 10, 32, 2, text);
    ssd1306_show(display);
}

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
