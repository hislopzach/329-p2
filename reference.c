/*
 * Project 2
 * Derek Nola and Bevin Tang
 */

#include "keypad.h"
#include "msp.h"
#include "timers.h"

#include <stdio.h>
#include <stdlib.h>

#define SQR 0x0D
#define SINE 0x0E
#define SAW 0x0F
#define TRI 0xA0

#define SINE_STEPS 64
#define SAW_STEPS 62
#define TRI_STEPS 32
#define VOLT_SCALE 4560

// GLOBALS
char GKEY = 'E';
float duty_cycle = 0.5f;
uint32_t freq = 100;
int16_t wave_type = SQR;
volatile unsigned int TempDAC_Value = 0;

// Freq delays arrays for all the wave types
uint16_t SINE_DELAYS[5] = {0x0670, 0x0338, 0x0225, 0x019C, 0x0149};
uint16_t TRI_DELAYS[5] = {0x0770, 0x03B8, 0x027A, 0x01DA, 0x017A};
uint16_t SAW_DELAYS[5] = {0x0780, 0x03B8, 0x028A, 0x01EC, 0x018C};

// Amount to increase dac output by each cycle
int16_t DAC_COUNT = 0;
int16_t SINE_INC = 32;
int16_t SAW_INC = 75;
int16_t TRI_INC = 145;

int16_t SINE_DELAY, SAW_DELAY, TRI_DELAY;

// Square wave delays for varying DC
uint16_t SQRBase, SQRLimit;

int32_t isin(int32_t deg, int32_t scale);
void setWaveDelays();
void Drive_DAC(unsigned int level);

int main(int argc, char const* argv[])
{
  // INIT Keypad
  keypad_init();

  WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;  // Stop watchdog timer

  set_DC0(FREQ_12_MHz);

  // Configure SMCLK to output P4.3
  P4->DIR |= BIT3;
  P4->SEL0 |= BIT3;

  // generate default 100 Hz square wave with 50% duty cycle
  calcAdvTimer(&SQRLimit, &SQRBase, freq, duty_cycle);
  setWaveDelays(freq / 100 - 1);

  // Configure port bits for SPI
  P3->DIR |= BIT2;        // Will use BIT4 to activate /CE on the DAC
  P1SEL0 |= BIT6 + BIT5;  // Configure P1.6 and P1.5 for UCB0SIMO and UCB0CLK
  P1SEL1 &= ~(BIT6 + BIT5);

  // SPI Setup
  EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SWRST;  // Put eUSCI state machine in reset

  EUSCI_B0->CTLW0 =
      EUSCI_B_CTLW0_SWRST |  // Remain eUSCI state machine in reset
      EUSCI_B_CTLW0_MST |    // Set as SPI master
      EUSCI_B_CTLW0_SYNC |   // Set as synchronous mode
      EUSCI_B_CTLW0_CKPL |   // Set clock polarity high
      EUSCI_B_CTLW0_MSB;     // MSB first

  EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SSEL__SMCLK;  // SMCLK
  EUSCI_B0->BRW = 0x01;  // divide by 16, clock = fBRCLK/(UCBRx)
  EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;  // Initialize USCI state machine,
                                            // SPI now waiting for something to
                                            // be placed in TXBUF

  EUSCI_B0->IFG |= EUSCI_B_IFG_TXIFG;  // Clear TXIFG flag

  // Setup interrupt and timer
  TIMER_A0->CCTL[0] = TIMER_A_CCTLN_CCIE;  // TACCR0 interrupt enabled
  TIMER_A0->CCR[0] = SQRBase;

  TIMER_A0->CTL = TIMER_A_CTL_SSEL__SMCLK |  // SMCLK, continuous mode
                  TIMER_A_CTL_MC__CONTINUOUS;

  // Enable global interrupt
  __enable_irq();
  NVIC->ISER[0] = 1 << ((TA0_0_IRQn)&31);

  while (1) {
    GKEY = num_to_char(keypad_getkey());
    // main switch statement changes
    // global values based off keypress
    switch (GKEY) {
      case '1':
        freq = 100;
        break;
      case '2':
        freq = 200;
        break;
      case '3':
        freq = 300;
        break;
      case '4':
        freq = 400;
        break;
      case '5':
        freq = 500;
        break;
      case '6':
        TempDAC_Value = 0;
        DAC_COUNT = 0;
        wave_type = TRI;
        break;
      case '7':
        TempDAC_Value = 0;
        wave_type = SQR;
        break;
      case '8':
        TempDAC_Value = 0;
        wave_type = SINE;
        break;
      case '9':
        TempDAC_Value = 0;
        DAC_COUNT = 0;
        wave_type = SAW;
      case '*':
        if (wave_type == SQR && duty_cycle - 0.1f > 0.1f)
          duty_cycle -= 0.1f;
        break;
      case '0':
        if (wave_type == SQR)
          duty_cycle = 0.5f;
        break;
      case '#':
        if (wave_type == SQR && duty_cycle + 0.1f < 0.91f)
          duty_cycle += 0.1f;
        break;
    }
    calcAdvTimer(&SQRLimit, &SQRBase, freq, duty_cycle);
    delay_ms(100);
  }

  return 0;
}

void TA0_0_IRQHandler(void)
{
  uint16_t SINE_INC = 360 / SINE_STEPS;

  static DBLE_COUNT = 0;
  if (wave_type == SQR) {
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

    // Special Case for 100Hz to prevent overflow of timer
    if (freq == 100) {
      if (TempDAC_Value == 0 && DBLE_COUNT == 0) {
        TIMER_A0->CCR[0] += SQRLimit;
        TempDAC_Value = VOLT_SCALE;
        DBLE_COUNT = 0;
      }
      else if (TempDAC_Value != 0 && DBLE_COUNT == 0) {
        DBLE_COUNT++;
        TIMER_A0->CCR[0] += SQRLimit;
      }
      else if (DBLE_COUNT == 1) {
        DBLE_COUNT++;
        TempDAC_Value = 0;
        TIMER_A0->CCR[0] += SQRBase;
      }
      else if (DBLE_COUNT == 2) {
        TIMER_A0->CCR[0] += SQRBase;
        DBLE_COUNT = 0;
      }
    }
    else {
      // all other frequencies SQR wave
      if (!TempDAC_Value) {
        TIMER_A0->CCR[0] += SQRLimit;
        TempDAC_Value = VOLT_SCALE;
      }
      else {
        TIMER_A0->CCR[0] += SQRBase;
        TempDAC_Value = 0;
      }
    }
    Drive_DAC(TempDAC_Value);
  }
  else if (wave_type == SINE) {
    // SINE Wave delay
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

    // Can only do 360 due to the isin function
    // operating on a degree scale
    if (DAC_COUNT > 360)
      DAC_COUNT = 0;
    // calc sin value with a half scale to oscillate +-2.5V
    TempDAC_Value = isin(DAC_COUNT, VOLT_SCALE / 2);
    Drive_DAC(TempDAC_Value);
    DAC_COUNT += SINE_INC;           // change voltage
    TIMER_A0->CCR[0] += SINE_DELAY;  // reset timer
  }
  else if (wave_type == SAW) {
    // Saw wave delay
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

    if (DAC_COUNT >= SAW_STEPS) {
      DAC_COUNT = 0;
      TempDAC_Value = 0;  // reset voltage
    }
    Drive_DAC(TempDAC_Value);
    TempDAC_Value += SAW_INC;       // increase voltage
    TIMER_A0->CCR[0] += SAW_DELAY;  // reset timer
    DAC_COUNT++;
  }
  else if (wave_type == TRI) {
    // Tri wave delay
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

    if (DAC_COUNT >= TRI_STEPS) {
      TRI_INC *= -1;  // invert shift once count hits 5V limit
      DAC_COUNT = 0;
    }
    Drive_DAC(TempDAC_Value);
    TempDAC_Value += TRI_INC;       // change voltage
    TIMER_A0->CCR[0] += TRI_DELAY;  // reset timer
    DAC_COUNT++;
  }
}

// A sine approximation via Bhaskara I's approx.
// @param x    Angle (0 to 360 allowed)
// @return     sine value*scale
int32_t isin(int32_t deg, int32_t scale)
{
  int8_t shift = 1;
  if (deg > 180) {
    deg -= 180;
    shift = -1;
  }
  int32_t num = (deg << 2) * (180 - deg);
  float dem = 40500 - (deg * (180 - deg));
  return scale * (num / dem * shift + 1);
}

// sets the appropriate delays for each wave type
void setWaveDelays(uint32_t freq)
{
  SINE_DELAY = SINE_DELAYS[freq];
  SAW_DELAY = SAW_DELAYS[freq];
  TRI_DELAY = TRI_DELAYS[freq];
}

// Drives the DAC Output
void Drive_DAC(unsigned int level)
{
  unsigned int DAC_Word = 0;
  DAC_Word = (0x1000) | (2 * level / 5 & 0x0FFF);  // 0x1000 sets DAC for Write
                                                   // to DAC, Gain = 2, /SHDN =
                                                   // 1 and put 12-bit level
                                                   // value in low 12 bits.

  P3->OUT &= ~BIT2;  // Clear P3.2 (drive /CS low on DAC)
                     // Using a port output to do this for now

  EUSCI_B0->TXBUF = (unsigned char)(DAC_Word >> 8);  // Shift upper byte of
                                                     // DAC_Word 8-bits to right

  EUSCI_B0->TXBUF =
      (unsigned char)(DAC_Word & 0x00FF);  // Transmit lower byte to DAC

  P3->OUT |= BIT2;  // Set P3.2   (drive /CS high on DAC)

  return;
}