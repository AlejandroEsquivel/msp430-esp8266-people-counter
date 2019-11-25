# Wi-Fi Connected People Counter


This project essentially allows one to place a device near a door to count how many people come in and out of particular place. 

It is implemented with a pair of ultrasonic transducers, the TI MSP430G2553 microcontroller and the ESP8266 ESP-01 Wi-Fi module. 

## Server

Estblishes a TCP connection from the ESP8266 ESP-01 Wi-Fi module which recieves one of two signals +1 or -1 depending on whether someone walks in or out from a store respectively.

It broadcasts these signals to any connected Websocket clients

## Browser Frontend

Establishes connection with the above Websocket server to recieve +1 or -1 signals, and renders a plot showing the near real-time volume of people within a particular place.

# Explanation of Firmware

## Timer

The MSP430 comes with two Timers, which can be used to take time measurements. The special register TAR gives the Timer's current count. Moreover, there are two modes that the Timers can be in: Capture Mode, and Compare Mode.

### Capture Mode

In capture mode the Timer can use an external signal ("Capture Input") to trigger a time measurement, and will store the timer value in register TACCRx. 
The capture input can be used to trigger a measurement if the signal is either a rising edge, falling edge, or in both rising and falling edges, this can be configured in the CMx bits in the TACCTLx (Capture/Compare Control Register).

The capture input can be a select few of the GPIO pins on the MSP430 (P1.1, P1.2, P2.0-P2.5, ect.) depending on the version of the MSP430 (here we use MSP430g2553). We must enable the special function select bits (P1SEL/P2SEL) for the the pin we decide to use as the capture input. 

### Compare Mode

In compare mode, the timer counts from 0 to a certain value, which can be specified (the value stored in TACCR0 [Up Mode], or 0xFFFF [Continous Mode], or TACCR0 then back to 0 [Up/Down Mode]. These modes can be configured in the MCx bits in the TACTL (Timer Control Register).

Every time the Timer counts up to it's given period (TACCRx value) an interrupt is fired (given interrupts are enabled with the TAIE bit within TACTL).



## UART

The MSP430 comes with a USCI (Universal Serial Communications Interface) chip, which allows us to send data to some external source. The USCI can be configured to send data with asynchronous serial communication, which means that the transmitting and recieving clocks do not need to be synchronized, and the start/end of the data transmission is specified with start/stop signals, while a series of bits is sent sequentially in between. The hardware device to implement such asynchronous serial communication is UART, which the USCI implements if in UART mode.

There are a set of special registers that enable us to configure the USCI, particularly (UCAxCTL0 and UCAxCTL1) which are the "USCI Control Registers". UART mode is default, so it is optional to explicitly configure UCAxCTL0 to be in UART mode (UCMODEx = 00).

Data is transmitted physically bit by bit in serial communication, and information is represented by HIGH(1) or LOW(0) digital signals. These signals transition between these states (HIGH/LOW) over time to convey data. The "Baud Rate" in our case, represents how many bits per second are sent via serial communication. The baud rate can be configured with the special registers UCA0BR0, and UCA0BR0.

### Transmitting/Receiving

There are various interrupts and interrupt flags associated with transmitting or recieving data over UART. Particularly, interrupts are fired when data is ready to be sent from the host computer (or other external device), and when data has been transferred from the MSP430. The UCAxRXBUF \& UCAxTXBUF are special registers to read (Rx) and write (Tx) bytes.

When data is to be transferred, a byte can be copied to UCAxTXBUF, at which point the MSP430 begins transmitting the data over UART. The interrupt flag UCAxTXIFG indicates whether the byte has been successfully transferred, and is cleared during transmission. After the transmission is complete the UCAxTXIFG flag is truthy and an interrupt is fired.

The receiving process is similar, the special register UCAxRXBUF is read when data is available from a host computer(or external device). The interrupt flag UCA1RXIFG indicates whether there is data waiting to be consumed, and is automatically cleared when the UCAxRXBUF data is read. An interrupt fires when UCA1RXIFG is truthy or equivalently, when Rx byte transfer is ready. If data is not read and the external device transfers a new byte, the data in the Rx buffer will be overwritten and the UCOE (data overwritten) flag in the UCAxSTAT register will be truthy.

## HC-SR04 Ultrasonic sensor

The HC-SR04 is a common ultrasonic distance sensor which uses a piezo transducer to generate ultrasonic pulses and a receiver which detects ultrasonic pulses. Applying a voltage to a piezoelectric material causes is to deform which in turn causes a pressure wave or "sound wave" upon deformation. By deforming the piezoelectric material in the piezoelectric transducer at a certain frequency, an ultrasonic sound wave can be generated (any frequency over 20kHz). In theory the receiver can act in a similar manner but in reverse, receiving an ultrasonic pulse and generating minute voltages upon deformation of the piezoelectric material which would need to be amplified.

### Operation

By applying at least a 10 miscrosecond HIGH signal on the Trigger pin, eight ultrasonic 40kHz pulses are sent from the piezo transducer, at which point the Echo pin emits a HIGH signal until the reflected pulses are detected.

## Ultrasonic Distance Meter

In order to make a distance measurement, we must measure the time that the HC-SR04's Echo signal is high. There are many different approaches, here we will discuss an approach using the MSP430's Timer Capture mode.
The Timer's capture mode is to used in order to detect the rising and falling edges of the Echo signal and record the corresponding times of those events. In order to do this we will use MSP430's GPIO pin P1.1 to be the "Capture input" and to receive the Echo signal from the HC-SR04 as in input. The P1.1 pin is connected in series with a 1kOhm resistor (to protect the MSP430 from the 5V powered HC-SR04) and to the HC-SR04's Echo pin. Furthermore, the P2.1 pin is connected directly to the HC-SR04's Trigger pin so that we can programmatically begin a measurement.

The MSP430's P1.1 pin corresponds to TA0.0 (Timer0_A0) from Figure 1. In the datasheet, we see this pin can be a capture input, particularly a CCI0A input. This implies a few things. The corresponding Interrupt vector address is Timer0_A0/Timer0_A3 (0xFFF2) which can be referenced in code by 
```c
Timer0_A0 // as defined in the header files 
```

Additionally, we will use TACCTL0 capture/compare control register (TACCTLx where x = 0, as CCIxA is CCI0A due to the pin we selected). The TACCTL0 can be configured as follows:
```c
TACCTL0 = CM_3 + SCS + CCIS_0 + CAP + CCIE;
```

Which configures the Timer to capture the rising and falling edge of the Echo signal (CM_3), set the capture input signal to CCI0A/P1.1 (CCIS_0), enables capture mode (CAP), and enables interrupts (CCIE).
Now, when an Echo signal is received via pin P1.1, two interrupts will be fired, once when there is a rising edge and another when there is a falling edge. Each time the interrupts are fired, the internal time counter TAR is copied to the TACCR0 register, which can be stored then subtracted to give us the time it took for an ultrasonic pulse to travel to and back from some object. During each interrupt the special register TAIV indicates the source of the interrupt was (capture input, timer overflow [timer counted to 0]), furthermore the CCI bit in the TACCTL0 gives us the capture input value (P1.1 value) which can help us distinguish between a rising/falling edge.

As the master clock of the MSP430 runs at ~1MHz, we can use the command
`__delay_cycles(10);`
to wait for ~10 microseconds, and can be used to trigger a measurement by outputting HIGH to P2.1 (trigger pin) for ~10 microseconds.

UART can only only transmit data byte by byte, so in order to transmit higher precision measurements, we must break data that is to be transmitted into byte chunks. One way of doing this is by converting a numerical data type (long in this case) to a string, as a string is just an array of characters (char) which are one byte each, we can just send a series of characters over UART.
```c
while (!(IFG2 & UCA0TXIFG)); UCA0TXBUF = char_value;
```

The above code snippet waits until the interrupt flag UCA0TXIFG is 1 (indicating a new byte is ready to be transferred), and sends a (char) byte value through UART. A string can simply be sent by iterating the character array (the string) until the null-character ("\0") is encountered (indicating the end of the string).
