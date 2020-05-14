/*
 * p3_3.c: Initialize and display "hello" on the LCD using 4-bit data mode.
 * Data and control pins share Port 4.
 */

// #include "msp.h"
#include "msp432p401r.h"
// ports
#define RS 1 /* P4.0 mask */
#define RW 2 /* P4.1 mask */
#define EN 4 /* P4.2 mask */
#define LCD_PORT P4

// commands
#define CLEAR_DISPLAY 1
#define CURSOR_FIRST_LINE 0x80
#define CURSOR_SECOND_LINE 0xC0

// constants
#define LCD_LINESIZE 20

char intToChar(uint8_t number);
void delayMs(int n);
void LCD_nibble_write(unsigned char data, unsigned char control);
void LCD_command(unsigned char command);
void LCD_clear(void);
void LCD_data(unsigned char data);
void LCD_init(void);
void LCD_write_strings(char top_line[], char bottom_line[]);
