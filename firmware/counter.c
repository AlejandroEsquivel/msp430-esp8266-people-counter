#include "msp430.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TXD BIT2 // TXD on P1.2 (RX of ESP)
#define RXD BIT1 // RXD on P1.1 (TX of ESP)

#define RED_LED BIT0 // P1.0 LED
#define GREEN_LED BIT6 // P1.6 LED

#define WIFI_NETWORK_SSID	"AE iPhone" // Rowan_IOT network
#define WIFI_NETWORK_PASS	"guestpass" // No password for this network

volatile unsigned long start_time;
volatile unsigned long end_time;
volatile unsigned long delta_time;
volatile unsigned long distance;

void wait_ms(unsigned int ms)
{
  unsigned int i;
  for (i = 0; i <= ms; i++)
  {
    // Clock is ~1MHz so 1E3/1E6 = 1E-3 (1ms) seconds
    __delay_cycles(1000);
  }
}

/* Write byte to USB-Serial interface */
void write_uart_byte(char value)
{
  while (!(IFG2 & UCA0TXIFG))
    ;
  // wait for TX buffer to be ready for new data
  // UCA0TXIFG register will be truthy when available to recieve new data to computer.
  UCA0TXBUF = value;
}

void write_uart_string(char *str)
{
  unsigned int i = 0;
  while (str[i] != '\0')
  {
    //write string byte by byte
    write_uart_byte(str[i++]);
  }
}

void put(const unsigned c){
  while (!(IFG2 & UCA0TXIFG))
    ;
  // wait for TX buffer to be ready for new data
  // UCA0TXIFG register will be truthy when available to recieve new data to computer.
  UCA0TXBUF = c;
}

void TX(char *s){
  while (*s) put(*s++);
}

void crlf(void)
{
  TX("/r/n");
}


#if defined(__TI_COMPILER_VERSION__)
#pragma vector = USCI0RX_ISR
__interrupt void ta1_isr(void)
#else
void __attribute__((interrupt(USCI0RX_ISR))) rx_isr(void)
#endif
{
  
}


/*
#if defined(__TI_COMPILER_VERSION__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void ta1_isr(void)
#else
void __attribute__((interrupt(TIMER0_A0_VECTOR))) ta1_isr(void)
#endif
{
  switch (TAIV)
  {
    //Timer overflow
    case 10:
    break;
    //Otherwise Capture Interrupt
    default:
    // Read the CCI bit (ECHO signal) in CCTL0
    break;
  }
  TACTL &= ~CCIFG; // reset the interrupt flag
}
*/



/* Setup UART */
void init_uart(void)
{
  // Set TXD, and RXD
  P1SEL |= TXD + RXD;
  P1SEL2 |= TXD + RXD;

  // For Baud rate: 115200, and BRCLK frequency of 1MHz (SMLCK)
  // We require ( UCBRx = 8, UCBRSx = 6, UCBRFx = 0) 

  // Use SMCLK - 1MHz clock
  UCA0CTL1 |= UCSSEL_2;
  // 9600 Baud
  UCA0BR0 = 104;
  UCA0BR1 = 0;
  UCA0MCTL = UCBRS0;
  // Initialize USCI state machine - enable
  UCA0CTL1 &= ~UCSWRST;
}

void init_timer(void)
{
  DCOCTL = 0;
  /* Use internal calibrated 1MHz clock: */
  BCSCTL1 = CALBC1_1MHZ; // Set range (y)
  DCOCTL = CALDCO_1MHZ; // (y)
  BCSCTL2 &= ~(DIVS_3); // SMCLK = DCO = 1MHz (x - not used)

  // Stop timer before modifiying configuration
  TACTL = MC_0;

  /* 
  1. Capture Rising & Falling edge of ECHO signal
  2. Sync with clock
  3. Set Capture Input signal to CCI0A
  4. Enable capture mode
  5. Enable interrupt
  */
  CCTL0 |= CM_3 + SCS + CCIS_0 + CAP + CCIE;
  // Select SMCLK with no divisions, continous mode.
  TACTL |= TASSEL_2 + MC_2 + ID_0;
}

void reset_timer(void)
{
  //Clear timer 
  TACTL |= TACLR;
}

void main(void) {
  // Stop Watch Dog Timer
  WDTCTL = WDTPW + WDTHOLD; 

  // Enable LEDs, reset pins output vals
  P2DIR = 0xFF;
  P1OUT = 0;
  P2OUT = 0;
  P1DIR |= RED_LED + GREEN_LED;

  init_timer();
  init_uart();
  

  // Global Interrupt Enable
  __enable_interrupt();
  
  IE2 &= ~UCA0RXIE; // Disable UART RX interrupt before we send data

  wait_ms(500);
  P1OUT|=RED_LED;

  TX("AT+RST");
  crlf();
	wait_ms(20000);	// Delay
	TX("AT+CWMODE_DEF=1");	    // Set mode to client (save to flash)
	// Modem-sleep mode is the default sleep mode for CWMODE = 1
	// ESP8266 will automatically sleep and go into "low-power" (15mA average) when idle
	crlf();	
	wait_ms(20000);	// Delay
	TX("AT+CWJAP_DEF=\"");      // Assign network settings (save to flash)
	TX(WIFI_NETWORK_SSID);
	TX("\",\"");
	TX(WIFI_NETWORK_PASS);	    // Connect to WiFi network
	TX("\"");
	crlf();
  // Delay, wait for ESP8266 to fetch IP and connect
  P1OUT|=GREEN_LED;
  while(1){
    __delay_cycles(50000);
  }
}