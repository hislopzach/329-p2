#include "msp432p401r.h"
#define COL1 BIT5
#define COL2 BIT6
#define COL3 BIT7
#define ROW1 BIT0
#define ROW2 BIT1
#define ROW3 BIT2
#define ROW4 BIT4  // bit 4 instead of bit 3 because we can't use 5.3

#define KEYPAD_PORT P5

#define NO_KEY 255

void keypad_init(void);
uint8_t keypad_getkey(void);
char key_to_char(uint8_t key);
char keypad_getkey_blocking(void);
