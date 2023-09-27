#define F_CPU 4000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

//Library Includes
#include "UART.h"
#include "I2C.h"
#include "PWM.h"

//Function Prototypes
void BOD_Enable();
void MENU();
void MENU_init();
void I2C_gather_and_print();
int pow10(int i);

//Error Control
volatile uint8_t errorLog = 0;
volatile uint16_t errCount = 0;

//Data Log Control
volatile struct PWM_DATA data;
volatile uint8_t memIndex = 0;
volatile uint8_t lastPin = 0;

//Export Values
uint16_t fannr = 0;
volatile uint16_t datast = 0;

//Global Values
volatile uint8_t read_compare = 0;

int main(void){
	I2C_init();
	MENU_init();
	USART3_init();
	BOD_Enable();
	
	_PWM_FUNCTION_POINTER_INIT();
	
	PWM_Preload();
	
	MENU();
	
	PWM_INIT();
	
	sei();
	
	while(1){
		PWM_RUN();
		
		if(!(PORTB.IN & (PIN2_bm))){
			MENU();
		}

		//Change Read Select
		if(PWM_CTRL.READ_SELECT < 0x47){
			PWM_CTRL.READ_SELECT = PWM_CTRL.READ_SELECT + 1;
			read_compare = read_compare << 1;
			
			} else {
			PWM_CTRL.READ_SELECT = 0x40;
			read_compare = 1;
		}
		
		//error control
		if(errorLog != 0){
			uint8_t compare = 0b00000001;
			for(int i = 0; i < 8; i++){
				if(errorLog & compare){
					printf("ERRB: %i \n\r",(errorLog & compare));
				}
				
				compare = (compare << 1);
			}
			errorLog = 0;
		}
		
		if(PWM_CTRL.MAIN_CTRL & 0b10000000){
			printf("FAM%i:", fannr);
			printf(" %i\r\n",lastPin);
			printf("FAD%i:", fannr);
			printf(" %i\r\n", PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[fannr]);
		}


		I2C_gather_and_print();
		_delay_ms(50);
	}
}

void BOD_Enable(){
	BOD.CTRLA = BOD_SAMPFREQ_32HZ_gc | BOD_ACTIVE_SAMPLED_gc;	//Enable BOD in sampled mode, with sampling rate of 32hz
	BOD.CTRLB = BOD_LVL_BODLEVEL3_gc;							//Set BOD reset trigger/threshold to highest possible
	
	BOD.VLMCTRLA = BOD_VLMLVL_5ABOVE_gc;						//Set VML to trigger when difference is 5%
	
	//VML INTER ENABLE	| +- BOD VALUE
	BOD.INTCTRL = BOD_VLMIE_bm		| BOD_VLMCFG_BOTH_gc;
}

ISR(TCB0_INT_vect){
	//printf("Were here\n");
	//COPIES DATA FROM CCMP
	volatile uint16_t DATASTORAGE = TCB0.CCMP;

	volatile uint16_t temp = 0;
	
	//CONVERTS CCMP VALUE TO READABLE DATA
	DATASTORAGE = (DATASTORAGE/2)+1;
	
	//CONVERTS READ SELECT SINCE THERE IS 2 READS PER 1 SIGNAL
	volatile uint8_t fanNR = (PWM_CTRL.READ_SELECT - 0x40)/2;
	
	//CALCUALTES IF ITS WITHIN 5% OF EXPECTED VALUE
	if( DATASTORAGE > (PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[fanNR] - 0x04) && DATASTORAGE < (PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[fanNR] - 0x04)){
		errCount++;
	}
	
	//CONVERTS TO RPMS
	if(DATASTORAGE < 0x14){
		DATASTORAGE = 0;
		} else {
		temp = DATASTORAGE - 0x14;
		DATASTORAGE = (temp * 53) + 800;
	}
	
	//IF DATA SAVING ENABLED, SAVE DATA TO LOCAL STORAGE. PREPARE FOR EXPORT.
	if(PWM_CTRL.MAIN_CTRL & 0b10000000 && read_compare & PWM_CTRL.READ_CTRL){
		data.FAN_ID = fanNR;
		data.PWM_OUTPUT = PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[fanNR];
		data.PWM_READ = DATASTORAGE;
		
		PWM_CTRL.DATALOG[memIndex] = data;
		
		lastPin = fanNR;
		
		if(memIndex < 96){
			memIndex++;
			} else {
			memIndex = 0;
		}
	}
	
	fannr = (PWM_CTRL.READ_SELECT - 0x40)/2;
	datast = DATASTORAGE;
	
	//cli();
	TCB0.INTFLAGS |= 0b00000001;

}

ISR(BOD_VLM_vect){
	
	errorLog |= 0b00000001;			//SETS ERROR FLAG
	
	BOD.INTFLAGS = BOD_VLMIF_bm;	//EXITS INTERRUPT
}

void MENU_init(){
	PORTB.DIRSET = PIN3_bm; // Set the LED on the as OUTPUT
	PORTB.DIRCLR = PIN2_bm; // Set the Button on the AVR as INPUT
	PORTB.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTB.OUTTGL = PIN3_bm;
}

void MENU(){
	cli();
	PORTB.OUTTGL = PIN3_bm;
	char command;
	while(command != 'e'){
		switch(command){
			int local_lenght;
			case '0':
			while(1){
				local_lenght = 8;
				printf("\n....................");
				printf("\n Here you can enable and disable different modes on you fan controller");
				printf("\n 7: ENABLE DATA SAVING \n 6: ENABLE AUTO RUN \n 5: ENABLE INTERRUPTS\n 4: ENABLE PORTMUX OVERRIDE");
				printf("\n 3: ENABLE TCB0 \n 2: ENABLE TCA1\n 1: ENABLE TCA0\n 0: MASTER PWM ENABLE\n e: exit to main menu");
				printf("\n The default values should look like: \n  00011111 \n Your current settings are:\n  ");
				Print_Binary(PWM_CTRL.MAIN_CTRL);
				command = USART3_read();
				if(command == 'e'){
					printf("\n\n Returning to main menu! \n\n");
					command = ' ';
					break;
				}
				command = command - '0';
				
				if(((int)command < local_lenght) && ((int)(command) >=0)){
					printf("\n....................");
					printf("\n This command is recoginsed! %i",command);
					PWM_CTRL.MAIN_CTRL ^= (1<<(int)command);
				}
				else{
					printf("\n....................");
					printf("\n This command: %i is NOT recoginsed, please try again ", command);
				}
				command = command + '0';
			}
			break;
			case '1':
			while(1){
				local_lenght = 6;
				printf("\n....................");
				printf("\n Please toggle your fan pairs: \n ex all on: 01110111 \n Corresponding fan: *543*210");
				printf("\n 0: Toggle fan pair 0");
				printf("\n 1: Toggle fan pair 1");
				printf("\n 2: Toggle fan pair 2");
				printf("\n 3: Toggle fan pair 3");
				printf("\n 4: Toggle fan pair 4");
				printf("\n 5: Toggle fan pair 5");
				printf("\n e: Exit to main menu");
				command = USART3_read();
				
				if(command == 'e'){
					printf("\n\n Returning to main menu! \n\n");
					command = ' ';
					break;
				}
				command = command - '0';
				if(((int)command < local_lenght) && ((int)(command) >=0)){
					printf("\n....................");
					printf("\n\n This command is recoginsed! %i",command);
					if((int)command < 3){
						PWM_CTRL.READ_CTRL ^= (1<<(int)command);
						} else {
						PWM_CTRL.READ_CTRL ^= (1<<(int)command+1);
					}
					
				}
				else{
					printf("\n....................");
					printf("\n\n This command: %i is NOT recoginsed, please try again ", command);
				}
				command = command + '0';
			}
			break;
			
			case '3':
			while(1){
				local_lenght = 6;
				printf("\n....................");
				printf("\n Please select fan pair to set duty cycle for");
				printf("\n 0: Set duty cycle for fan pair 0");
				printf("\n 1: Set duty cycle for fan pair 1");
				printf("\n 2: Set duty cycle for fan pair 2");
				printf("\n 3: Set duty cycle for fan pair 3");
				printf("\n 4: Set duty cycle for fan pair 4");
				printf("\n 5: Set duty cycle for fan pair 5");
				printf("\n e: Exit to main menu");
				command = USART3_read();
				
				if(command == 'e'){
					printf("\n\n Returning to main menu! \n\n");
					command = ' ';
					break;
				}
				command = command - '0';
				if(((int)command < local_lenght) && ((int)(command) >=0)){
					printf("\n....................");
					printf("\n You have chosen to set duty cycle for fan pair: %i\n",command);
					printf("\n Please write a precentage between 0-80%, NOTE: include the '%' at the end");
					uint8_t arrPoint = (int)command;
					uint8_t val = 0;
					
					char num[3] = {'0', '0', '0'};
					uint8_t index = 0;
					while(1){
						command = USART3_read();
						if(command == '%'){
							break;
							} else {
							num[index++] = command;
						}
					}
					val = 0;
					index--;
					for(int i = 0; i < index+1; i++){
						val += ((int)(num[i]-'0')) * pow10(index-i);

					}
					float a = (float)val/100 * 0x50;
					PWM_CTRL.PWM_CMPBUF_DUTYCYCLE[arrPoint] = (int)a;
				}
				else{
					printf("\n....................");
					printf("\n This command: %i is NOT recoginsed, please try again ", command);
				}
				command = command + '0';
			}
			break;
			case '2':
			while(1){
				local_lenght = 8;
				printf("\n ....................");
				printf("\n Please toggle your fan pairs: \n ex all on: 01110111 \n Corresponding fan: *543*210");
				printf("\n 0: Toggle pin read 0");
				printf("\n 1: Toggle pin read 1");
				printf("\n 2: Toggle pin read 2");
				printf("\n 3: Toggle pin read 3");
				printf("\n 4: Toggle pin read 4");
				printf("\n 5: Toggle pin read 5");
				printf("\n 4: Toggle pin read 6");
				printf("\n 5: Toggle pin read 7");
				printf("\n e: Exit to main menu");
				printf("\n The default values should look like: \n  00011111 \nYour current settings are: \n  ");
				Print_Binary(PWM_CTRL.READ_CTRL);
				
				
				command = USART3_read();
				
				if(command == 'e'){
					printf("\n\n Returning to main menu! \n\n");
					command = ' ';
					break;
				}
				command = command - '0';
				if(((int)command < local_lenght) && ((int)(command) >=0)){
					printf("\n ....................");
					printf("\n\n This command is recoginsed! You have chosen to toggle fan pair: %i",command);
					PWM_CTRL.READ_CTRL ^= (1<<(int)command);
					
				}
				else{
					printf("\n....................");
					printf("\n\n This command: %i is NOT recoginsed, please try again ", command);
				}
				command = command + '0';
			}
			break;
			default:
			printf("\n ....................");
			printf("\n Welcome to our fan controller\n Please select your next action  \n 0: Main Control\n 1: Select Fans to run\n 2: Select Fans to Watch over\n 3: Select Duty Cycles for your fan pairs \n e: Exit the inital setup phase \n");
			command = USART3_read();
			break;
			
		}
	}
	printf("\n....................");
	printf("\nThe MENU has been closed");
	printf("\nPlease hold SW0 to reopen it");
	command = ' ';
	PORTB.OUTTGL = PIN3_bm;
	sei();
}

/* This function uses the AHT10 functions from the I2C file to read temperature and humidity data (and print it if SAVE data is enabled)*/
void I2C_gather_and_print(){
	uint8_t transfer[6];
	int storage[2];
	AHT10_Data_Read(transfer);
	AHT10_Data_Conversion(transfer, storage);
 	if(PWM_CTRL.MAIN_CTRL & 0b10000000){
 		printf("FUKT:\n%i\n\r", storage[0]);
 		printf("TEMP:\n%i\n\r", storage[1]);
 	}
	//These values may be adjusted furhter later
	PWM_CTRL.AUTOSCALER[0] = storage[1];
	PWM_CTRL.AUTOSCALER[1] = storage[1];
	PWM_CTRL.AUTOSCALER[2] = storage[1];
	PWM_CTRL.AUTOSCALER[3] = storage[1];
	PWM_CTRL.AUTOSCALER[4] = storage[1];
	PWM_CTRL.AUTOSCALER[5] = storage[1];
}

//THIS FUNCTION IS FOR 10^n
int pow10(int i){
	if(i>0){
		int ret = 1;
		for(int j = 0; j < i; j++){
			ret *= 10;
		}
		return ret;
		} else {
		return 1;
	}
	
}
