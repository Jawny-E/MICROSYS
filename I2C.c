/*
* Inpired by: Blackboardresource LF Øving 7, C. R. Fosse 18.04.2023
* This library handles communication with I2C to an AHT10 unit
* Group 2(Ellen I. Johnsen; Eryk Siejka ; Michal Stakiewicz )
*/
#include "I2C.h"
#include "UART.h"

//Prototypes
void I2C_init(void);
void I2C0_M_start(uint8_t addr, uint8_t dir);
void I2C_M_write (uint8_t addr,uint8_t data);
static void I2C_M_read( uint8_t addr , uint8_t * data , uint8_t len );
void Print_Binary(uint8_t num);
void AHT10_Data_Conversion(uint8_t *data, int storage[]);
void AHT10_Data_Read(uint8_t data[]);

// Definistions related to the AHT10 unit
#define AHTX0_I2CADDR_DEFAULT 0x38   /// Standardadresss for AHT10
#define AHTX0_CMD_TRIGGER 0xAC       /// Commando to trigger the sending of measurement data
#define BITS_IN_MESSAGE 0x08	     /// Number of bits in AHT10 data-packet

// Definitions related to AVR128DB48
#define F_CPU 4000000UL

// Formula for Baud Rate I2C . Chapter 29.3.2.2.1 , chapter 2 in datasheet for AVR128DB48.
# define TWI1_BAUD(F_SCL,T_RISE) (((((( float ) F_CPU / (2*(float )F_SCL))) - 5 - ((float) F_CPU * T_RISE )))/ 2)

//I2C definitions
#define I2C_SCL_FREQ 50
#define DIR_READ 1											
#define DIR_WRITE 0
#define TWI_IS_CLOCKHELD() TWI1.MSTATUS & TWI_CLKHOLD_bm
#define TWI_IS_BUSERR() TWI1.MSTATUS & TWI_BUSERR_bm
#define TWI_IS_ARBLOST() TWI1.MSTATUS & TWI_ARBLOST_bm
#define CLIENT_NACK() TWI1.MSTATUS & TWI_RXACK_bm
#define CLIENT_ACK() !(TWI1.MSTATUS & TWI_RXACK_bm )
#define TWI_IS_BUSBUSY() ((TWI1.MSTATUS & TWI_BUSSTATE_BUSY_gc ) == TWI_BUSSTATE_BUSY_gc )
// TWI_WAIT makes sure the bus is in the correct state! :D
#define TWI_WAIT() while(!((TWI_IS_CLOCKHELD())||(TWI_IS_BUSERR())||(TWI_IS_ARBLOST())||(TWI_IS_BUSBUSY())))

/* Initialise function for I2C */
void I2C_init(void){
	PORTF.DIRSET = PIN2_bm | PIN3_bm;					// Set PORT F, PIN 2 = SDA, PIN 3 = SCL
	PORTF.PINCONFIG =  PORT_PULLUPEN_bm;					// Enable pullup 
	PORTF.PINCTRLUPD = PIN2_bm | PIN3_bm;					//
	TWI1.MCTRLA = TWI_ENABLE_bm;						// Enable Two-Wire-Interface 1
	TWI1.MBAUD = (uint8_t)TWI1_BAUD (I2C_SCL_FREQ ,0) ;			// Set Baud-rate for TWI1
	TWI1.MSTATUS = TWI_BUSSTATE_IDLE_gc;					// Sets bus to IDLE state
}

/*
-> Function starts I2C-transfer with the given adress
@param addr: Adress of secondary unit
@param dir: Set direction for data transfer, 1 is read, 0 is write
*/
void I2C0_M_start(uint8_t addr, uint8_t dir) {
	TWI1.MADDR = ( addr << 1) | dir ;
	TWI_WAIT () ;
}
/*
-> This function writes a BYTE to the given adress
@param addr: Adress of secondary unit
@param data: Byte to be written over I2C
*/
void I2C_M_write (uint8_t addr,uint8_t data) {
	I2C0_M_start(addr, DIR_WRITE);			// Start transfer
	TWI1.MDATA = data;				// Write data to MDATA (TWI1 register)
	TWI_WAIT();					// Wait til bus is ready
	
	if( TWI1.MSTATUS & TWI_RXACK_bm ) {
		printf (" target NACK \n");		// Prints if the target unit NACKED
	}

	TWI1.MCTRLB |= TWI_MCMD_STOP_gc;		// Stopps transfer
}
/*
-> This function reads (len) BYTES form the given adress
@param addr: Adress of secondary unit
@param data: Pointer to the array data is stored to
@param len: Number of bytes to be read
*/
static void I2C_M_read( uint8_t addr, uint8_t* data , uint8_t len){
	I2C0_M_start(addr, DIR_READ);			// Start data transfer
	uint8_t byte_count = 0;				// Counter that keeps control of Bytes read
	while (byte_count < len) {
		TWI_WAIT() ;				// Wait for correct bus-status
		data[byte_count] = TWI1.MDATA ;		// Read the MDATA-byte to the storage array
		byte_count ++;				
		if(byte_count != len) {			// Send ACK if all packages recived
			TWI1.MCTRLB = TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc;		
		}
	}
	TWI1.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc ; // Finish data transfer
}

/*
-> Prints a number in the form of bits
@param num = Number that is to be printed 
Unused but very helpful during debugging :D
*/
void Print_Binary(uint8_t num) {
	int size = sizeof(num) * 8;			//Size of number, lowkey redundant, but lets you switch datatype
	
	for (int i = size - 1; i >= 0; i--) {		// Goes through every bit position from max to min
		int bit = (num >> i) & 1;		//Shifter register to the right and (& 1)
		printf("%d", bit);			//Prints rightmost bit
	}
	printf("\n"); //Newline after finished process
}

/*
-> This function processes data-packets from the AHT10 and gives out readable data for temp and moisture
@ param data: The raw data packet gotten over I2C
@ param storage: Storage array for the 

The AHT10 sends 20bit of data for both sizes(temperature and humidity), however due to the calculation process it has been
deemed adequate to use the 16-MSB as it would be unnecessary to use uint32 for these measurements.
The calculations from the AHT10 datasheet have due to this been multiplied with 2^(4)
*/
void AHT10_Data_Conversion(uint8_t *data, int storage[]){
	
	/*Temperatur*/
	uint8_t high = (data[5]>>4) & 0x0F;				//LSB temp
	uint8_t low = data[3] & 0x0F;					//MSB temp
	uint16_t raaTemperatur = (low<<12| data[4] << 4 |high);		//Sort into 16-bit
	float temperatur = (((float)raaTemperatur/65536)*200)-50;	//Calculation of temperature
	
	/*Relativ fuktigheit*/
	uint16_t raaFuktigheit = ( data[1]<<8 | data[2] );		//Sorts 16MSB for humidity
	float fuktigheit = (float)raaFuktigheit * 100 / 65536;		//Calculation of Relative humidity

	/*Stores the final product! :D*/
	storage[0] = (uint8_t)fuktigheit;
	storage[1] = (uint8_t)temperatur;
}

/*
-> This is the function that reads data from the AHT10
@param data: array that stores data-packets recived in return
*/
void AHT10_Data_Read(uint8_t data[]){
		I2C_M_write(AHTX0_I2CADDR_DEFAULT, AHTX0_CMD_TRIGGER);		//Ask the AHT10 (kindly) to send a data packet
		I2C_M_read(AHTX0_I2CADDR_DEFAULT, data, BITS_IN_MESSAGE);	//Read the response from AHT10
}
