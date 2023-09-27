#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>

typedef uint8_t BITMASK;

//BITMASKS FOR MAIN_CTRL
#define	PWM_MASTER_ENABLE_bm		0b00000001
#define	PWM_TCA0_ENABLE_bm			0b00000010
#define	PWM_TCA1_ENABLE_bm			0b00000100
#define	PWM_TCB_ENABLE_bm			0b00001000
#define	PWM_PORTMUX_OVERRIDE_bm		0b00010000
#define	PWM_INTERRUPT_ENABLE_bm		0b00100000
#define	PWM_AUTOMODE_ENABLE_bm		0b01000000
#define	PWM_DATASAVING_ENABLE_bm	0b10000000

//BITMASKS FOR CMP_CTRL
#define	PWM_TCA0_CMP_0_bm			0b00000001
#define	PWM_TCA0_CMP_1_bm			0b00000010
#define	PWM_TCA0_CMP_2_bm			0b00000100
#define	PWM_TCA1_CMP_0_bm			0b00010000
#define	PWM_TCA1_CMP_1_bm			0b00100000
#define	PWM_TCA1_CMP_2_bm			0b01000000

//FUNCTION POINTERS
typedef uint8_t (*GET_DUTYCYCLE)(uint8_t fanNr);
typedef void (*SET_DUTYCYCLE)(uint8_t fanNr,uint8_t newDutyCycle);
typedef void (*PWM_RUN_PTR)();
typedef void (*PWM_INIT_PTR)();
typedef void (*PWM_PRELOAD_PTR)();

//STORAGE STRUCT USED FOR DATALOGGING
struct PWM_DATA{
	BITMASK FAN_ID;															//		 NR
	uint16_t PWM_OUTPUT;													//	  DUTY CYCLE
	uint16_t PWM_READ;														//  R_FAN1 R_FAN2
};

struct PWM_CONTROLLER{
	BITMASK MAIN_CTRL;
	/*
	0 - MASTER PWM ENABLE													
	1 - ENABLE TCA0															
	2 - ENABLE TCA1															
	3 - ENABLE TCB0															
	4 - ENABLE PORTMUX OVERRIDE												
	5 - ENABLE MASTER INTERRUPT												
	6 - ENABLE AUTO RUN														
	7 - ENABLE DATA SAVING													
	*/
	
	BITMASK CMP_CTRL;
	//CONTROLS WHICH FAN IS TO BE TURNED ON
	//PORT NAME: -CCC-DDD
	//TCAn CMPn: -210-210
	//REGISTER: b00000000;
		
	BITMASK READ_CTRL;
	//ENABLES WHICH FAN IS TO BE READ
	//0b00000000, 1 to enable reading of PWM signal
	
	uint8_t READ_SELECT;
	//Value of which pin to read, 0x40 to 0x47
	
	uint16_t TCA0_PERBUF_VAL;
	uint16_t TCA1_PERBUF_VAL;
	//CONTROLS WHICH VALUE TO LOAD AS PERBUF MAX
	
	uint16_t PWM_CMPBUF_DUTYCYCLE[6];
	//CONTROLS DUTY CYCLE OF THE FANS
	
	uint8_t AUTOSCALER[6];
	//TEMPERATURE VALUE
	
	volatile struct PWM_DATA DATALOG[96];
	//LIBRARY INTERNAL MEMORY FOR DATASAVING
	
	SET_DUTYCYCLE SET_DUTYCYCLE; 
	//FUNCTION POINTER TO SETING FOR SETING THE DUTYCYCLE
	
	GET_DUTYCYCLE GET_DUTYCYCLE;
	//FUNCTION POINTER FOR GETTING DUTYCYCLE
	
	PWM_RUN_PTR RUN;
	//FUNCTION POINTER TO GETING DUTYCYCLE
	
	PWM_INIT_PTR PWM_INIT_F;
	//FUNCTION POINTER TO INIT FUNCTION
	
	PWM_PRELOAD_PTR PWM_PRELOAD;
	//FUNCTION POINTER TO PRELOAD FUNCTION
};

//initalization of ctrl for the library
struct PWM_CONTROLLER PWM_CTRL;

//INIT FUNCTIONS

void PWM_INIT();															//Main Init Function

void TCA0_SINGLE_INIT();													//TCA0 Init Function
void TCA1_SINGLE_INIT();													//TCA1 Init Function

void TCB_INIT();															//TCB0 Init Function

//FUNCTIONS

uint8_t TCA_GET_DUTYCYCLE(uint8_t fanNr);									//Get duty cycle function
void TCA_SET_DUTYCYCLE(uint8_t fanNr,uint8_t newDutyCycle);					//Set duty cycle function
void PWM_RUN();																//'Update()' esque function
void PWM_Preload();														

void _PWM_FUNCTION_POINTER_INIT();											//Connects functions listed above to the struct
