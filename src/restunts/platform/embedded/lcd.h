#include <xcore/channel.h>
#include <xcore/parallel.h>


// Output settings
#define LCD_WIDTH   320
#define LCD_HEIGHT  240
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// commands to LCD
#define DD_SET_PALETTE 0
#define DD_WRITE       1

#define HORIZONTAL_OFFSET ((LCD_WIDTH - SCREEN_WIDTH) / 2)
#define VERTICAL_OFFSET ((LCD_HEIGHT - SCREEN_HEIGHT) / 2)

DECLARE_JOB(lcd, (chanend_t));
void lcd(chanend_t c_lcd_trigger);
DECLARE_JOB(remote_render, (chanend_t, chanend_t));
void remote_render(chanend_t c_lcd_trigger, chanend_t c_framebuffer);
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue);
