#pragma once
#include <stdint.h>

// Mode 13h
#define VGA_GFX ((volatile uint8_t *)0xA0000)
#define SCR_W 320
#define SCR_H 200

#define C_BLACK 0
#define C_BLUE 1
#define C_GREEN 2
#define C_CYAN 3
#define C_RED 4
#define C_MAGENTA 5
#define C_BROWN 6
#define C_LGRAY 7
#define C_DGRAY 8
#define C_LBLUE 9
#define C_LGREEN 10
#define C_LCYAN 11
#define C_LRED 12
#define C_LMAG 13
#define C_YELLOW 14
#define C_WHITE 15

void put_pixel(int x, int y, uint8_t color);
void clear_screen(uint8_t color);
void fill_rect(int x, int y, int w, int h, uint8_t color);
void draw_rect(int x, int y, int w, int h, uint8_t color);
void draw_char(int x, int y, char c, uint8_t color);
void draw_string(int x, int y, const char *s, uint8_t color);
void draw_window(int x, int y, int w, int h, const char *title);

void draw_boot_screen(void);
void draw_desktop(void);

void term_enter(void);
void term_print(const char *s, uint8_t term_color);