#include "msp430.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TXD BIT2 // TXD on P1.2 (RX of ESP)
#define RXD BIT1 // RXD on P1.1 (TX of ESP)

#define ECHO_1 BIT0 // P2.0
#define TRIG_1 BIT1 // P2.1

#define ECHO_2 BIT5 // P2.5
#define TRIG_2 BIT4 // P2.4

#define RED_LED BIT0   // P1.0 LED
#define GREEN_LED BIT6 // P1.6 LED

#define WIFI_NETWORK_SSID "AE iPhone" // Rowan_IOT network
#define WIFI_NETWORK_PASS "guestpass" // No password for this network
#define TCP_SERVER "tcp.alejandroesquivel.com"
#define TCP_PORT "3100"

volatile unsigned int timer_reset_count = 0;

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
  wait_ms((int)(s*1000));
}



/* Write byte to USB-Serial interface */
void write_uart_byte(char value)
{
  while (!(IFG2 & UCA0TXIFG));
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

void write_uart_int(unsigned int i){
  char buf[sizeof(i) * 8 + 1];
  sprintf(buf, "%d\n", i);
  write_uart_string(buf);
}

void write_uart_long(unsigned long l)
{
  char buf[sizeof(l) * 8 + 1];
  sprintf(buf, "%ld\n", l);
  write_uart_string(buf);
}


void tx_end(float delay){
  write_uart_string("\r\n");
  wait_s(delay);
}

void tx(char *str, float delay)
{
  write_uart_string(str);
  tx_end(delay);
}

void tx_partial(char *str){
  write_uart_string(str);
}


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
      timer_reset_count++;
    break;
    //Otherwise Capture Interrupt
    default:
    // Read the CCI bit (ECHO signal) in CCTL0
    break;
  }
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

void esp_flash_config() {
  //  Flash Wifi-Mode: As client
  tx("AT+CWMODE_DEF=1",2); 
  // Flash ssid credentials
  tx_partial("AT+CWJAP_DEF=\"");
  tx_partial(WIFI_NETWORK_SSID);
  tx_partial("\",\"");
  tx_partial(WIFI_NETWORK_PASS);
  tx_partial("\"");
  tx_end(20);
}

void esp_runtime_config(void) {
  tx("ATE0",1); // Disable echo
  tx("AT+CIPMUX=0",1); // Single-Connection Mode
}

void tcp_connect(void){
   tx_partial("AT+CIPSTART=\"TCP\",\"");  // Establish TCP connection
   tx_partial(TCP_SERVER);
   tx_partial("\",");
   tx_partial(TCP_PORT);
   tx_end(10);
}


void send_measurement(char* data) {
  tx("AT+CIPSEND=1",1); // prepare to send x byte
  tx(data,0);
}

unsigned long ultrasonic_measurement(int ECHO_PIN, int TRIG_PIN){

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
        distance = (unsigned long)((end_time - start_time) / 0.00583090379);

        //only accept values within HC-SR04 acceptible measure ranges
        if (distance / 10000 >= 2.0 && distance / 10000 <= 400)
        {
          return distance;
        }
        else {
          return -1;
        }
      }
      prev_echo_val = curr_echo_val;
    }
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

  P1OUT |= RED_LED;

  // need to run only once
  //esp_flash_config();

  //esp_runtime_config();
  //tcp_connect();

  //sendMeasurement("!");

  P1OUT |= GREEN_LED;

  unsigned long d1;
  unsigned long d2;

  unsigned long now;

  unsigned int threshold = 40;

  unsigned int counter_1 = 0;
  unsigned int counter_2 = 0;

  unsigned long people_measure_start = 0;
  unsigned long last_count_taken = 0;

  unsigned int allow_double_measurement = 0;

  unsigned int previous_count = 0;
  unsigned int count = 0;

  unsigned long people = 0;

  while (1)
  {
    d1 = ultrasonic_measurement(ECHO_1, TRIG_1);
    d2 = ultrasonic_measurement(ECHO_2, TRIG_2);

    now = TAR / 1000; //miliseconds

  
    if (allow_double_measurement == 1 || (d1 > threshold || d2 > threshold))
    {

      if (counter_1 == 0 && d1 <= threshold)
      {
        counter_1 = 1;
        last_count_taken = now;
        allow_double_measurement = 1;
        if (counter_2 == 1)
        {
          allow_double_measurement = 0;
          people_measure_start = now;
          count--;
        }
      }

      if (counter_2 == 0 && d2 <= threshold)
      {
        counter_2 = 1;
        last_count_taken = now;
        allow_double_measurement = 1;
        //raising edge
        if (counter_1 == 1)
        {
          allow_double_measurement = 0;
          people_measure_start = now;
          count++;
        }
      }
    }

    //edge case where one sensor triggered, but second one not triggered
    if (now - last_count_taken > 1000) {
      last_count_taken = now;
      counter_1 = 0;
      counter_2 = 0;
    }

    if (counter_2 == 1 && counter_1 == 1 && now - people_measure_start >= 250)
    {

      people_measure_start = 0;

      if (count > previous_count)
      {
        people++;
        //send_measurement("+");
      }
      else if (count < previous_count)
      {
        people--;
        //send_measurement("-");
      }

      if (people < 0)
      {
        people = 0;
      }

      previous_count = count;

      write_uart_string("people: ");
      write_uart_long(people);

      /*Serial.print("People: ");
      Serial.print(people);
      Serial.print("\n");
      Serial.print("Count: ");
      Serial.print(count);
      Serial.print("\n");*/

      people_measure_start = now;

      counter_1 = 0;
      counter_2 = 0;
    }
  
    wait_ms(50);
  }
}