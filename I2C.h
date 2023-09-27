/*
* Inpired by: Blackboardresource LF Øving 7, C. R. Fosse 18.04.2023
* This library handles communication with I2C to an AHT10 unit
* Group 2(Ellen I. Johnsen; Eryk Siejka ; Michal Stakiewicz )
*/
#pragma  once
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

void I2C_init(void);
void AHT10_Data_Conversion(uint8_t *data, int storage[]);
void AHT10_Data_Read(uint8_t data[]);
void Print_Binary(uint8_t num);
