#include "msp430.h"
/*
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
*/

#define TXD BIT2 // TXD on P1.2 (RX of ESP)
#define RXD BIT1 // RXD on P1.1 (TX of ESP)

#define ECHO_1 BIT0 // P2.0
#define TRIG_1 BIT1 // P2.1

#define ECHO_2 BIT5 // P2.5
#define TRIG_2 BIT4 // P2.4

#define RED_LED BIT0   // P1.0 LED
#define GREEN_LED BIT6 // P1.6 LED

#define WIFI_NETWORK_SSID "AE iPhone"
#define WIFI_NETWORK_PASS "guestpass"

#define TCP_SERVER "tcp.alejandroesquivel.com"
#define TCP_PORT "3100"

volatile unsigned int timer_reset_count = 0;


int threshold = 40;

int counter_1 = 0;
int counter_2 = 0;

int state = -1;
int prev_state = -1;


long last_measurement_taken = 0;

signed int people = 0;
signed int prev_people = 0;


void wait_ms(unsigned int ms)
{
  unsigned int i;
  for (i = 0; i <= ms; i++)
  {
    // Clock is ~1MHz so 1E3/1E6 = 1E-3 (1ms) seconds
    __delay_cycles(1000);
  }
}

void wait_s(float s)
{
  wait_ms((int)(s * 1000));
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

/*
void write_uart_long(unsigned long l)
{
  char buf[sizeof(l) * 8 + 1];
  sprintf(buf, "%ld\n", l);
  write_uart_string(buf);
}*/

void tx_end(float delay)
{
  write_uart_string("\r\n");
  wait_s(delay);
}

void tx(char *str, float delay)
{
  write_uart_string(str);
  tx_end(delay);
}

void tx_partial(char *str)
{
  write_uart_string(str);
}

#if defined(__TI_COMPILER_VERSION__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void ta1_isr(void)
#else
void __attribute__((interrupt(TIMER0_A0_VECTOR))) ta1_isr(void)
#endif
{
  timer_reset_count++;
  TACTL &= ~CCIFG; // reset the interrupt flag
}

/* Setup UART */
void init_uart(void)
{
  // Set TXD, and RXD
  P1SEL |= TXD + RXD;
  P1SEL2 |= TXD + RXD;

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
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
  BCSCTL2 &= ~(DIVS_3);

  // Stop timer before modifiying configuration
  TACTL = MC_0;

  /* 
  1. Capture Rising & Falling edge of ECHO signal
  2. Sync with clock
  5. Enable interrupt
  */
  CCTL0 |= CM_3 + SCS + CCIE;
  // Select SMCLK with no divisions, continous mode.
  TACTL |= TASSEL_2 + MC_2 + ID_0;
}

void reset_timer(void)
{
  //Clear timer
  TACTL |= TACLR;
}

void esp_flash_config()
{
  //  Flash Wifi-Mode: As client
  tx("AT+CWMODE_DEF=1", 2);
  // Flash ssid credentials
  tx_partial("AT+CWJAP_DEF=\"");
  tx_partial(WIFI_NETWORK_SSID);
  tx_partial("\",\"");
  tx_partial(WIFI_NETWORK_PASS);
  tx_partial("\"");
  tx_end(20);
}

void esp_runtime_config(void)
{
  tx("ATE0", 1);        // Disable echo
  tx("AT+CIPMUX=0", 1); // Single-Connection Mode
}

void tcp_connect(void)
{
  tx_partial("AT+CIPSTART=\"TCP\",\""); // Establish TCP connection
  tx_partial(TCP_SERVER);
  tx_partial("\",");
  tx_partial(TCP_PORT);
  tx_end(10);
}

unsigned long millis(void)
{
  return TAR;
}

void send_measurement(char *data)
{
  tx("AT+CIPSEND=1", 1); // prepare to send x byte
  tx(data, 0);
}

unsigned long ultrasonic_measurement(int ECHO_PIN, int TRIG_PIN)
{

  // Enable ultrasound pulses
  P2OUT |= TRIG_PIN;
  // Send pulse for 10us
  __delay_cycles(10);
  // Disable TRIGGER
  P2OUT &= ~TRIG_PIN;

  unsigned int prev_echo_val = 0;
  unsigned int curr_echo_val;

  unsigned long start_time;
  unsigned long end_time;
  unsigned long distance;

  while (1)
  {
    curr_echo_val = P2IN & ECHO_PIN;
    // Rising edge
    if (curr_echo_val > prev_echo_val)
    {
      reset_timer();
      timer_reset_count = 0;
      start_time = TAR;
    }
    // Falling edge
    else if (curr_echo_val < prev_echo_val)
    {
      end_time = TAR + (timer_reset_count * 0xFFFF);
      distance = (unsigned long)((end_time - start_time) / 58);

      //only accept values within HC-SR04 acceptible measure ranges
      if (distance >= 2.0 && distance <= 400)
      {
        return distance;
      }
      else
      {
        return -1;
      }
    }
    prev_echo_val = curr_echo_val;
  }
}


void blah(void)
{
  P1OUT |= RED_LED;

  if (prev_people < people)
  {
    send_measurement("+");
  }
  else if (prev_people > people)
  {
    send_measurement("-");
  }

  counter_1 = 0;
  counter_2 = 0;
  prev_state = state;
  state = 0;
  prev_people = people;

  reset_timer();
  timer_reset_count = 0;
  last_measurement_taken = millis();
}


void main(void)
{
  // Stop Watch Dog Timer
  WDTCTL = WDTPW + WDTHOLD;

  P1OUT = 0;
  P2OUT = 0;

  // Enable LEDs, TRIC/ECHO pins
  P2DIR |= TRIG_1 + TRIG_2;
  P2DIR &= ~(ECHO_1 + ECHO_2);
  P1DIR |= RED_LED + GREEN_LED;

  init_timer();
  init_uart();

  // Global Interrupt Enable
  __enable_interrupt();

  IE2 &= ~UCA0RXIE; // Disable UART RX interrupt before we send data

  wait_ms(500);

  // need to run only once
  //esp_flash_config();

  esp_runtime_config();
  tcp_connect();

  //sendMeasurement("!");

  P1OUT |= GREEN_LED;

  unsigned long d1;
  unsigned long d2;

  while (1)
  {
    // 1 then 2 means to the right (rel. to facing the front of board [closest to ultrasound sensors])
    d1 = ultrasonic_measurement(ECHO_1, TRIG_1);
    d2 = ultrasonic_measurement(ECHO_2, TRIG_2);

    P1OUT &= ~(RED_LED);

    if (state != 4)
    {
      if (d1 <= threshold)
      {
        counter_1 = 1;
        reset_timer();
      }

      if (d2 <= threshold)
      {
        counter_2 = 1;
        reset_timer();
      }

      if (counter_1 == 1 && counter_2 == 0)
      {
        state = 1;
      }
      else if (counter_1 == 0 && counter_2 == 1)
      {
        state = 2;
      }
      else if (counter_1 == 1 && counter_2 == 1)
      {
        state = 3;
      }
      else
      {
        state = 0;
      }

      if (state == 3 && prev_state == 1)
      {
        state = 4;
        reset_timer();
        people++;
      }
      else if (state == 3 && prev_state == 2)
      {
        state = 4;
        reset_timer();
        people--;
      }


      //timeout of weird state
      if ( millis() >= 50000)
      {
        reset_timer();
        timer_reset_count = 0;
        last_measurement_taken = millis();

        counter_1 = 0;
        counter_2 = 0;
        prev_state = state;
        state = 0;
      }
      /*
      switch (state)
      {
      case 0:
        write_uart_string("0\n");
        break;
      case 1:
        write_uart_string("1\n");
        break;
      case 2:
        write_uart_string("2\n");
        break;
      case 3:
        write_uart_string("3\n");
        break;
      case 4:
        write_uart_string("4\n");
        break;
      }*/

      prev_state = state;
    }
    else {

      P1OUT |= RED_LED;

      //Serial.println(people);
      //Serial.print("People Count: ");
      //Serial.println(people);

      if(prev_people < people){
        send_measurement("+");
      }
      else if(prev_people > people){
        send_measurement("-");
      }

      counter_1 = 0;
      counter_2 = 0;
      prev_state = state;
      state = 0;

      prev_people = 0;
      people = 0;
      
      reset_timer();
      last_measurement_taken = millis();
    }
  }
}