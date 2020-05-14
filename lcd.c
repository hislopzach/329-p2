#include "lcd.h"
#include <string.h>

/*LCD_init:
Initialize the LCD port for use with LCD, sends setup commands and clears
display
*/
void LCD_init(void)
{
  LCD_PORT->DIR = 0xFF; /* make P4 pins output for data and controls */
  delayMs(30);          /* initialization sequence */
  LCD_nibble_write(0x30, 0);
  delayMs(10);
  LCD_nibble_write(0x30, 0);
  delayMs(1);
  LCD_nibble_write(0x30, 0);
  delayMs(1);
  LCD_nibble_write(0x20, 0); /* use 4-bit data mode */
  delayMs(1);

  LCD_command(0x28);          /* set 4-bit data, 2-line, 5x7 font */
  LCD_command(0x06);          /* move cursor right after each char */
  LCD_command(CLEAR_DISPLAY); /* clear screen, move cursor to home */
  LCD_command(0x0F);          /* turn on display, cursor blinking */
}

/* With 4-bit mode, each command or data is sent twice with upper
 * nibble first then lower nibble.
 */
void LCD_nibble_write(unsigned char data, unsigned char control)
{
  data &= 0xF0;                        /* clear lower nibble for control */
  control &= 0x0F;                     /* clear upper nibble for data */
  LCD_PORT->OUT = data | control;      /* RS = 0, R/W = 0 */
  LCD_PORT->OUT = data | control | EN; /* pulse E */
  delayMs(0);
  LCD_PORT->OUT = data; /* clear E */
  LCD_PORT->OUT = 0;
}

/*LCD_command
send a command the LCD
*/
void LCD_command(unsigned char command)
{
  LCD_nibble_write(command & 0xF0, 0); /* upper nibble first */
  LCD_nibble_write(command << 4, 0);   /* then lower nibble */

  if (command < 4)
    delayMs(4); /* commands 1 and 2 need up to 1.64ms */
  else
    delayMs(1); /* all others 40 us */
}

/* Helper function that sends the clear display command to the LCD */
void LCD_clear(void)
{
  LCD_command(CLEAR_DISPLAY);
}

/*send a char to the lcd at the current position*/
void LCD_data(unsigned char data)
{
  LCD_nibble_write(data & 0xF0, RS); /* upper nibble first */
  LCD_nibble_write(data << 4, RS);   /* then lower nibble  */

  delayMs(1);
}

/* delay milliseconds when system clock is at 3 MHz */
void delayMs(int n)
{
  int i, j;

  for (j = 0; j < n; j++)
    for (i = 750; i > 0; i--)
      ; /* Delay */
}

/*convert a single digit integer to a char*/
char intToChar(uint8_t number)
{
  return number + '0';
}

// write strings to lcd. If argument is null, that line will not be updated
void LCD_write_strings(char top_line[], char bottom_line[])
{
  int i;
  //  printf("top line: %s\n", top_line ? top_line : "<empty>");
  //  printf("bottom line: %s\n", bottom_line ? bottom_line : "<empty>");

  // write top line if provided
  if (top_line) {
    LCD_command(CURSOR_FIRST_LINE);
    if (strlen(top_line) > LCD_LINESIZE) {
      printf("top_line too big!");
    }
    for (i = 0; i < strlen(top_line); i++) {
      LCD_data(top_line[i]);
    }
  }
  if (bottom_line) {
    LCD_command(CURSOR_SECOND_LINE);
    if (strlen(bottom_line) > LCD_LINESIZE) {
      printf("bottom_line too big!");
    }
    for (i = 0; i < strlen(bottom_line); i++) {
      LCD_data(bottom_line[i]);
    }
  }
}
