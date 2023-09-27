/*
* Inpired by: Blackboardresource LF Øving 2, C. R. Fosse 18.04.2023
* This library handles communication over USART3 and Menu-logic for the fan controller
* Group 2(Ellen I. Johnsen; Eryk Siejka ; Michal Stakiewicz )
*/
#pragma  once
#include <avr/io.h>

//Sets up USART3
void USART3_init(void);
//Gives out a char written to the USART3-RXpin
uint8_t USART3_read();
