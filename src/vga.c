#include "vga.h"
#include "font8x8.h"

void put_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= SCR_W || y < 0 || y >= SCR_H)
        return;
    VGA_GFX[y * SCR_W + x] = color;
}

void clear_screen(uint8_t color)
{
    for (int i = 0; i < SCR_W * SCR_H; i++)
        VGA_GFX[i] = color;
}

void fill_rect(int x, int y, int w, int h, uint8_t color)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            put_pixel(x + i, y + j, color);
}

void draw_rect(int x, int y, int w, int h, uint8_t color)
{
    for (int i = 0; i < w; i++)
    {
        put_pixel(x + i, y, color);
        put_pixel(x + i, y + h - 1, color);
    }
    for (int j = 0; j < h; j++)
    {
        put_pixel(x, y + j, color);
        put_pixel(x + w - 1, y + j, color);
    }
}

void draw_char(int x, int y, char c, uint8_t color)
{
    unsigned char uc = (unsigned char)c;
    if (uc > 127)
        return;
    const uint8_t *glyph = font8x8_basic[uc];
    for (int row = 0; row < 8; row++)
    {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++)
            if (bits & (1 << col))
                put_pixel(x + col, y + row, color);
    }
}

void draw_string(int x, int y, const char *s, uint8_t color)
{
    int cx = x;
    while (*s)
    {
        if (*s == '\n')
        {
            y += 9;
            cx = x;
        }
        else
        {
            draw_char(cx, y, *s, color);
            cx += 8;
        }
        s++;
    }
}

static int str_px_len(const char *s)
{
    int n = 0;
    while (*s++)
        n++;
    return n * 8;
}

static void draw_string_centered(int cy, const char *s, uint8_t color)
{
    int w = str_px_len(s);
    int x = (SCR_W - w) / 2;
    if (x < 0)
        x = 0;
    draw_string(x, cy, s, color);
}

void draw_window(int x, int y, int w, int h, const char *title)
{
    fill_rect(x, y, w, h, C_LGRAY);             // corp fereastra
    fill_rect(x + 1, y + 1, w - 2, 14, C_BLUE); // bara de titlu
    draw_string(x + 4, y + 4, title, C_WHITE);  // titlu
    for (int i = 0; i < w; i++)
    {
        put_pixel(x + i, y, C_WHITE);
        put_pixel(x + i, y + h - 1, C_DGRAY);
    }
    for (int j = 0; j < h; j++)
    {
        put_pixel(x, y + j, C_WHITE);
        put_pixel(x + w - 1, y + j, C_DGRAY);
    }
}

static void boot_delay(int loops)
{
    for (volatile int i = 0; i < loops * 1000000; i++)
        ;
}

void draw_boot_screen(void)
{
    clear_screen(C_BLACK);

    int w = 200, h = 70;
    int x = (SCR_W - w) / 2;
    int y = (SCR_H - h) / 2;
    draw_window(x, y, w, h, "Geamuri 98");

    draw_string_centered(y + 30, "Geamuri 98 is booting", C_BLACK);

    int bw = 160, bh = 12;
    int bx = (SCR_W - bw) / 2;
    int by = y + 46;
    draw_rect(bx, by, bw, bh, C_DGRAY);
    for (int t = 0; t < 5; t++)
    {
        fill_rect(bx + 2, by + 2, (bw - 4) * (t + 1) / 5, bh - 4, C_BLUE);
        boot_delay(600);
    }
    boot_delay(600);
}

void draw_desktop(void)
{
    clear_screen(C_LBLUE);

    fill_rect(0, 188, 320, 12, C_LGRAY);
    for (int i = 0; i < 320; i++)
        put_pixel(i, 188, C_WHITE);

    fill_rect(2, 190, 40, 8, C_LGRAY);
    draw_rect(2, 190, 40, 8, C_DGRAY);
    draw_string(5, 191, "Start", C_BLACK);

    draw_window(70, 60, 180, 70, "Bun venit!");
    draw_string(80, 82, "Salut de la echipa:", C_BLACK);
    draw_string(80, 94, "-> Petrut ;)", C_BLUE);
    draw_string(80, 104, "-> Miodrag :3", C_BLUE);
    draw_string(80, 114, "-> Oprea :D", C_BLUE);
}

// mod pentru afisarea proceselor in alb negru

#define TERM_COLS (SCR_W / 8)
#define TERM_ROWS (SCR_H / 9)
#define LINE_H 9

static int term_x = 0;
static int term_y = 0;
static uint8_t term_fg = C_WHITE;
static uint8_t term_color;

void term_enter(void)
{
    clear_screen(C_BLACK);
    term_x = 0;
    term_y = 0;
    term_fg = C_WHITE;
}

// scroll, muta totul cu o linie in sus, sterge ultima linie
static void term_scroll(void)
{
    int line_bytes = SCR_W * LINE_H;
    for (int i = 0; i < SCR_W * (SCR_H - LINE_H); i++)
        VGA_GFX[i] = VGA_GFX[i + line_bytes];
    for (int i = SCR_W * (SCR_H - LINE_H); i < SCR_W * SCR_H; i++)
        VGA_GFX[i] = C_BLACK;
}

static void term_newline(void)
{
    term_x = 0;
    term_y++;
    if (term_y >= TERM_ROWS)
    {
        term_scroll();
        term_y = TERM_ROWS - 1;
    }
}

void term_print(const char *s, uint8_t term_color)
{
    while (*s)
    {
        char c = *s++;
        if (c == '\n')
        {
            term_newline();
        }
        else if (c == '\r')
        {
            term_x = 0;
        }
        else
        {
            draw_char(term_x * 8, term_y * LINE_H, c, term_color);
            term_x++;
            if (term_x >= TERM_COLS)
                term_newline();
        }
    }
}