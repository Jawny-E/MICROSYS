#include "PWM.h"

//Main initialization function meant to be called once
void PWM_INIT(){
	
	PWM_RUN();
	
	if((PWM_CTRL.MAIN_CTRL & 0b00000001)){					//MAIN_CTRL INTI
		if(PWM_CTRL.MAIN_CTRL & 0b00000010){				//TCA0 ENABLE		
			if((PWM_CTRL.CMP_CTRL & 0b00000001)){			//Enable 'compare and enable flag'(CMPn) in CTRLB register therefore enabling PWM generation.
				TCA0.SINGLE.CTRLB |= TCA0_SINGLE_CMP0BUF;
			}
			if(PWM_CTRL.CMP_CTRL & 0b00000010){
				TCA0.SINGLE.CTRLB |= TCA0_SINGLE_CMP1BUF;
			}
			if(PWM_CTRL.CMP_CTRL & 0b00000100){
				TCA0.SINGLE.CTRLB |= TCA0_SINGLE_CMP2BUF;
			}
			
			PORTD.DIR = 0xFF;								//Sets port as output so the waveform function can be output
			TCA0_SINGLE_INIT();								//Calls main initialization function
		}
		
		if((PWM_CTRL.MAIN_CTRL & 0b00000100)){				//TCA1 ENABLE
			if(PWM_CTRL.CMP_CTRL & 0b00010000){				//Enable 'compare and enable flag'(CMPn) in CTRLB register therefore enabling PWM generation.
				TCA1.SINGLE.CTRLB |= TCA1_SINGLE_CMP0BUF;
			}
			if(PWM_CTRL.CMP_CTRL & 0b00100000){
				TCA1.SINGLE.CTRLB |= TCA1_SINGLE_CMP1BUF;
			}
			if(PWM_CTRL.CMP_CTRL & 0b01000000){
				TCA1.SINGLE.CTRLB |= TCA1_SINGLE_CMP2BUF;
			}
			
			PORTC.DIR = 0xFF;								//Sets port as output so the waveform function can be output
			TCA1_SINGLE_INIT();								//Calls main initialization function
		}
		
		if(PWM_CTRL.MAIN_CTRL & 0b00001000){				//TCB ENABLE			
			TCB_INIT();
			PORTA.IN = 0xFF;								//Enables input on pins for TCB Timer
			PORTA.PINCONFIG = PORT_PULLUPEN_bm;
		}
		
		if((PWM_CTRL.MAIN_CTRL & 0b00010000)){				//PORTMUX OVERRIDE ENABLE
			
			//OVERRIDES pins on PORTD		and			PORTC, sets them to work as waveform output for pins W0n
			//					TCA0					TCA1
			PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc | PORTMUX_TCA1_PORTC_gc;
		}
		
		if(PWM_CTRL.MAIN_CTRL & 0b00100000){				//INTERRUPT ENABLE
			//Enables TCA0 interrupt flag.
			TCA0.SINGLE.INTCTRL  = 0b00010000;
			//Enables TCA1 interrupt flag.
			TCA1.SINGLE.INTCTRL  = 0b00010000;
			} else {										//INTERRUPT DISABLE
			//Disables TCA0 interrupt flag.
			TCA0.SINGLE.INTCTRL  = 0b00000000;
			//Disables TCA1 interrupt flag.
			TCA1.SINGLE.INTCTRL  = 0b00000000;
		}
	}
}

//Which operations are to be executed continuously
void PWM_RUN(){
	
	if(PWM_CTRL.MAIN_CTRL & 0b00001000){ //Timer needs to be disabled and re-enabled for changes on CHANNEL0 to take effect.
		TCB0.CTRLA ^= TCB_ENABLE_bm;
		EVSYS.CHANNEL0 = PWM_CTRL.READ_SELECT;
		TCB0.CTRLA ^= TCB_ENABLE_bm;
	}
	
	
	//SINCE WE CAN USE FUNCION POINTERS THIS HAS BEEN WRITTEN AS A TEMPERATURE CONVERTER FOR THE PROJECT
	
	if(PWM_CTRL.MAIN_CTRL & 0b01000000){			//IF AUTMODE ENABLE
		uint8_t reworkedTemp[6];
		uint8_t temp;
		
		for(int i = 0; i < 6;i++){					//CALCULATE EQUIVALENT DUTY CYCLE
			temp = PWM_CTRL.AUTOSCALER[i]-20;
			if(temp >= 0 && temp <= 80){
				reworkedTemp[i] = temp;
			} else if(temp > 80){
				reworkedTemp[i] = 0x50;
			} else {								//IF NOT WITHIN RANGE
				reworkedTemp[i] = 0;
			}
		}
		
		//LOAD PROCESSED VALUES TO MEMORY
		
		TCA0.SINGLE.CMP0BUF = reworkedTemp[0];
		TCA0.SINGLE.CMP1BUF = reworkedTemp[1];
		TCA0.SINGLE.CMP2BUF = reworkedTemp[2];
		
		TCA1.SINGLE.CMP0BUF = reworkedTemp[3];
		TCA1.SINGLE.CMP1BUF = reworkedTemp[4];
		TCA1.SINGLE.CMP2BUF = reworkedTemp[5];
		
	} else {
		
		//IF AUTOMODE DISABLED
		
		TCA0.SINGLE.CMP0BUF = PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[0];
		TCA0.SINGLE.CMP1BUF = PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[1];
		TCA0.SINGLE.CMP2BUF = PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[2];
		
		TCA1.SINGLE.CMP0BUF = PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[3];
		TCA1.SINGLE.CMP1BUF = PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[4];
		TCA1.SINGLE.CMP2BUF = PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[5];
	}
}

//Enables Timer A0 and sets it to operate as a Pulse Width Modulation (PWM) generator.
void TCA0_SINGLE_INIT(){
	
	TCA0.SINGLE.EVCTRL = 0b00000000;														//Event Control Disable, disabled by default, redisables in case something loads wrong
	
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_DSBOTTOM_gc;										//Set the timer operate in Dual Slope mode
	
	TCA0.SINGLE.PERBUF = PWM_CTRL.TCA0_PERBUF_VAL;											//Loads user defined PERBUF value for the PWM Period

	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;					//Set PRESCALER to 1 and Enables TCA0
	
}

//Enables Timer A1 and sets it to operate as a Pulse Width Modulation (PWM) generator.
void TCA1_SINGLE_INIT(){
	
	TCA1.SINGLE.EVCTRL = 0b00000000;														//Event Control Disable, disabled by default, redisables in case something loads wrong
	
	TCA1.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_DSBOTTOM_gc;										//Set the timer operate in Dual Slope mode
	
	TCA1.SINGLE.PERBUF = PWM_CTRL.TCA1_PERBUF_VAL;											//Loads user defined PERBUF value for the PWM Period

	TCA1.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;					//Set PRESCALER to 1 and Enables TCA0
	
}

//Enables Timer/Counter B0 as a Counter on Event Capture
void TCB_INIT(){
	//Using Port A as a Event Generator
	
	TCB0.CCMP = PWM_CTRL.TCA0_PERBUF_VAL; //TO REWORK -> Sets max value of timer
	
	TCB0.INTCTRL = 0b00000001;																//Enable Interrupt
	
	TCB0.CTRLB = TCB_CNTMODE_PW_gc;															//Sets the timer mode to measure PWM signal on input
	
	TCB0.EVCTRL = TCB_CAPTEI_bm;															//Eanbles EVENT CAPTURE and Enables Edge Event, reffer to page 292 for more info

	EVSYS.CHANNEL0 = 0x43;																	//sets event generator to generate an event on value change on pin PA0, using Channel0 (Channel 0 can access PORTA & PORTB)
																							//VARIABLE BETWEEN 0X40-0X47 <-> PA0-PA7
	EVSYS.USERTCB0CAPT = EVSYS_CHANNEL00_bm;												//Connects TCB0 CAPTURE EVENT to channel 0

	TCB0.CTRLA = TCB_ENABLE_bm | TCB_CLKSEL_TCA0_gc;										//Enables Timer and sets it to match TCA0 Timer
}

//Returns value of dutycycle at selected index
uint8_t TCA_GET_DUTYCYCLE(uint8_t fanNr){
	return PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[fanNr];
}

//sets value of dutycycle
void TCA_SET_DUTYCYCLE(uint8_t fanNr,uint8_t newDutyCycle){
	PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[fanNr] = newDutyCycle;
}

//FUNCITON POINTERS INIT
void _PWM_FUNCTION_POINTER_INIT(){
	PWM_CTRL.SET_DUTYCYCLE = &TCA_SET_DUTYCYCLE;
	PWM_CTRL.GET_DUTYCYCLE = &TCA_GET_DUTYCYCLE;
	PWM_CTRL.RUN = &PWM_RUN;
	PWM_CTRL.PWM_INIT_F = &PWM_INIT;
	PWM_CTRL.PWM_PRELOAD = &PWM_Preload;
}

//stock function to load as recommended default settings
void PWM_Preload(){
	PWM_CTRL.TCA0_PERBUF_VAL = 0x50;
	PWM_CTRL.TCA1_PERBUF_VAL = 0x50;
	
	PWM_CTRL.MAIN_CTRL = 0b00011111;
	PWM_CTRL.CMP_CTRL =	 0b11111111;
	PWM_CTRL.READ_CTRL = 0b11111111;
	
	PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[0] = 0x10;
	PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[1] = 0x10;
	PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[2] = 0x10;
	PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[3] = 0x10;
	PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[4] = 0x10;
	PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[5] = 0x10;

	PWM_CTRL.READ_SELECT = 0x40;
}
