/*
* Inpired by: Blackboardresource LF Øving 2, C. R. Fosse 18.04.2023
* This library handles communication over USART3 and Menu-logic for the fan controller
* Group 2(Ellen I. Johnsen; Eryk Siejka ; Michal Stakiewicz )
*/
#include "UART.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>

#define F_CPU 4000000UL
#define USART3_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 *(float)BAUD_RATE )) + 0.5)

// Prototypes
static int USART_printChar(char c, FILE *stream );
void USART3_init(void);
void USART3_sendChar(char c);
void USART3_sendString (char *str);
uint8_t USART3_read();
void USART_readString();

// Setting up FDEV_SETUP_STREAM to be able to use printChar --> printf
static FILE USART_stream = FDEV_SETUP_STREAM (USART_printChar, NULL , _FDEV_SETUP_WRITE );

/* Init-function that sets up USART-communication over given pins and baud rate */
void USART3_init (void)
{
	PORTB.DIRSET = PIN0_bm;								// RX - innpupinne
	PORTB.DIRCLR = PIN1_bm;								// TX - outputpinne
	USART3.BAUD = (uint16_t)USART3_BAUD_RATE(115200);	// Setter Baudrate
	USART3.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;		// Opner(ENABLE) TX og RX 
	stdout = &USART_stream;
	
    USART3.CTRLC |= USART_CHSIZE_8BIT_gc;				// DONT ASK

}



/* Function for sending a single character */
void USART3_sendChar(char c){
	while (!(USART3.STATUS & USART_DREIF_bm)) // Waits until USART is free
	{
		;
	}
	USART3.TXDATAL = c;						  // Writes c to to TX.DATA(output)
}

/* Function that enables printf */
static int USART_printChar(char c,FILE*stream)
{
	USART3_sendChar(c);
	return 0;
}

/* Function that break downs string and sends every character through USART3_sendChar */
void USART3_sendString(char*str){
	for (size_t i = 0; i < strlen (str); i ++)
	{
		USART3_sendChar(str[i]) ;
	}
}


/* Funksjon som leser av siffer som er tatt i mot*/
uint8_t USART3_read(){
	 while (!(USART3.STATUS & USART_RXCIF_bm))
	 {
		;
	 }
	 return USART3.RXDATAL;
}


