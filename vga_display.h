#ifndef VGA_DISPLAY_H
#define VGA_DISPLAY_H

// 8-bit colors
#define red 0xe0
#define dark_red 0x60
#define orange 0xf0
#define green 0x1c
#define dark_green 0x0c
#define blue 0x03
#define dark_blue 0x01
#define yellow 0xfc
#define cyan 0x1f
#define magenta 0xe3
#define black 0x00
#define gray (0x60 | 0x0c | 0x01)
#define white 0xff

extern volatile unsigned int *vga_pixel_ptr;

extern volatile unsigned int *vga_char_ptr;

// VGA Function Definitions
void VGA_text (int, int, char *);
void VGA_text_clear();
void VGA_box (int, int, int, int, short);
void VGA_rect (int, int, int, int);
void VGA_line(int, int, int, int, short) ;
void VGA_Vline(int, int, int, short) ;
void VGA_Hline(int, int, int, short) ;
void VGA_disc (int, int, int, short);
void VGA_circle (int, int, int, short);
void drawColorPalette();

#endif
