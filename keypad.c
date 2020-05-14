#include "keypad.h"
/* Keypad.c: Matrix keypad scanning
 *
 * This program scans a 4x3 matrix keypad and returns the number of the
 * key that is pressed. The getkey() function is non-blocking, so it will
 * return 0xFF if no key is pressed to allow 0 to be used for key 0.
 * * key will return 10 and # will return 12.
 *
 * The getkey() function assumes that only a single key is pressed. If keys
 * on multiple columns are pressed, the key on the farthest left column will
 * be detected and the others will be ignored. If multiple keys in the same
 * column are pressed, the function will return an incorrect value.
 *
 *
 */

/* this function initializes Port 4 that is connected to the keypad.
 * All pins are configured as GPIO input pin. The row pins have
 * the pull-down resistors enabled.
 */
void keypad_init(void)
{
  KEYPAD_PORT->DIR = 0;  // make all pins an input
  KEYPAD_PORT->REN |=
      (ROW1 | ROW2 | ROW3 | ROW4);  // enable resistor for row pins
  KEYPAD_PORT->OUT &= ~(ROW1 | ROW2 | ROW3 | ROW4);  // make row pins pull-down
}

/*
 * This is a non-blocking function to read the keypad.

 *
 */
uint8_t keypad_getkey(void)
{
  uint8_t row, col, key;

  /* check to see any key pressed */
  KEYPAD_PORT->DIR |= (COL1 | COL2 | COL3);  // make the column pins outputs
  KEYPAD_PORT->OUT |= (COL1 | COL2 | COL3);  // drive all column pins high
  __delay_cycles(25);                        // wait for signals to settle

  row = KEYPAD_PORT->IN & (ROW1 | ROW2 | ROW3 | ROW4);  // read all row pins

  if (row == 0)  // if all rows are low, no key pressed
    return 0xFF;

  /* If a key is pressed, it gets here to find out which key.
   * It activates one column at a time and reads the input to see
   * which row is active. */

  for (col = 0; col < 3; col++) {
    // zero out bits 6-4
    KEYPAD_PORT->OUT &= ~(COL1 | COL2 | COL3);

    // shift a 1 into the correct column depending on which to turn on
    KEYPAD_PORT->OUT |= (COL1 << col);
    __delay_cycles(25);  // wait for signals to settle

    row = KEYPAD_PORT->IN &
          (ROW1 | ROW2 | ROW3 | ROW4);  // mask only the row pins

    if (row != 0)
      break;  // if the input is non-zero, key detected
  }

  KEYPAD_PORT->OUT &= ~(COL1 | COL2 | COL3);  // drive all columns low
  KEYPAD_PORT->DIR &= ~(COL1 | COL2 | COL3);  // disable the column outputs

  if (col == 3)
    return 0xFF;  // if we get here, no key was detected

  // rows are read in binary, so powers of 2 (1,2,4,8)
  // use these shortcuts for log2(row) + 1
  //+1 is because rows are 1-indexed
  if (row == 4)
    row = 3;
  if (row == 16)
    row = 4;

  /*******************************************************************
   * IF MULTIPLE KEYS IN A COLUMN ARE PRESSED THIS WILL BE INCORRECT *
   *******************************************************************/

  // calculate the key value based on the row and columns where detected
  if (col == 0)
    key = row * 3 - 2;
  if (col == 1)
    key = row * 3 - 1;
  if (col == 2)
    key = row * 3;

  if (key == 11)
    key = 0;  // fix for 0 key

  return key;
}
/* key_to_char
given a interger (provided by keyboard_getkey()), returns the corresponding char
pressed on the keypad
*/
char key_to_char(uint8_t key)
{
  switch (key) {
    case 10:
      return '*';
    case 12:
      return '#';
    case 255:
      return '\0';
    default:
      return intToChar(key);
  }
}

/* A blocking version of get_key that will wait until a key is pressed until it
 * returns the pressed key */
char keypad_getkey_blocking(void)
{
  uint8_t keypad_value;
  char last_pressed;
  while (1) {
    keypad_value = keypad_getkey();
    if (keypad_value != NO_KEY) {
      // printf("key pressed: %d\n", keypad_value);
      last_pressed = key_to_char(keypad_value);
      // printf("char out: %c\n", last_pressed);
      return last_pressed;
    }
  }
}
