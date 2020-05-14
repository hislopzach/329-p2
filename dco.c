#include "dco.h"
#include "msp432p401r.h"
void set_DCO(int frequency)
{
  CS->KEY = CS_KEY_VAL;  // Unlock CS module for register access
  CS->CTL0 = 0;
  switch (frequency) {
    case MHZ_1_5:
      CS->CTL0 = CS_CTL0_DCORSEL_0;
      break;
    case MHZ_3:
      CS->CTL0 = CS_CTL0_DCORSEL_1;
      break;
    case MHZ_6:
      CS->CTL0 = CS_CTL0_DCORSEL_2;
      break;
    case MHZ_12:
      CS->CTL0 = CS_CTL0_DCORSEL_3;
      break;
    case MHZ_24:
      CS->CTL0 = CS_CTL0_DCORSEL_4;
      break;
  }
  CS->CTL1 = CS_CTL1_SELA_2 | CS_CTL1_SELS_3 | CS_CTL1_SELM_3;
  CS->KEY = 0;  // Lock CS module
}
