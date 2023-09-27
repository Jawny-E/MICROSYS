/******************************************
 *
 * This code is inspired by:
 *
 * Jose Garcia, https://github.com/jotathebest/
 *
 * ****************************************/


/****************************************
 * Library Dependencies
 ****************************************/
#include "UbidotsEsp32Mqtt.h"

/****************************************
 * Constants
 ****************************************/
const char* VARIABLES[] = {"FUKT", "TEMP", "FAM0", "FAM1", "FAD0", "FAD1", "ERRB"};
float VALUES[sizeof(VARIABLES)];

const char *UBIDOTS_TOKEN = "XXXXXXXX";  //Ubidots TOKEN
const char *DEVICE_LABEL = "XXXXXXXXX";                                 //Device name Ubidots
const char *WIFI_SSID = "XXXXXXXXXXXX";                                   //Wifi SSID
const char *WIFI_PASS = "XXXXXXXXXXXX";                                 //Wifi-passord


const int PUBLISH_FREQUENCY = 2000; //Time between publications (OBS: check Ubidots limit)
unsigned long timer;

HardwareSerial avrSerial(2);  //Serial 2 is used to communicate with AVR128
Ubidots ubidots(UBIDOTS_TOKEN);

/****************************************
 * Functions
 ****************************************/

/* Can be used later to recive messages from Ubidots */
void callback(char *topic, byte *payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

/* This function recived incoming messages from AVR128 and sends them to */
void readUSART(){
  if (avrSerial.available()){
    String message = avrSerial.readStringUntil('\r');
    Serial.println(message);
    handleCommand(message);
  }
}
/*This function processes the information read over USART and compares the command word with our 
defined command words. If they match the information will be stored in an array with matching index
to the command word*/
void handleCommand(String command){
  command.trim(); //Remove \n and \r

  String name = command.substring(0,4); //Command word should be the first 4 characters of string
  const char* y = name.c_str();         //Converts command word to type const char
  Serial.println(name);
  for (int i = 0; i < sizeof(VARIABLES) / sizeof(VARIABLES[0]); i++) { //Check if it matches a command word
    if (strcmp(VARIABLES[i], y) == 0) {
      String value = command.substring(6);
      float x = value.toFloat();
      if( (0<x) && (x<5000) ){ // We dont expect numbers lower than 0 for any size or numbers greater than 5000
        VALUES[i] = x;
        break;
      }
      else{
        Serial.println("Invalid number");
      } 
    }
  }
}

/*This function handles sending messages to Ubidots*/
void publishUbidots(const char* VARIABLE_NAME, float value){
  ubidots.add(VARIABLE_NAME, value);
  ubidots.publish(DEVICE_LABEL);
}



/****************************************
 * Setup og main
 ****************************************/

void setup()
{
  //For debugging
  Serial.begin(115200);
  //Ubidots setup
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
  //Start communication over RX2 and TX2
  avrSerial.begin(115200, SERIAL_8N1, 16, 17);
  //Set timer 
  timer = millis();
}

void loop()
{
  //Check connection status
  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  readUSART();

  //Publishes every time PUBLISH_FREQUECY has been reached
  if((millis() - timer) > PUBLISH_FREQUENCY){
    //Print every value along with its command name, this may give val = 0 if the avr is not publishing to the given command name
    for(int i = 0; i< sizeof(VARIABLES) / sizeof(VARIABLES[0]); i++){ 
      publishUbidots(VARIABLES[i], VALUES[i]);
      Serial.println("Message published");
    }
    timer = millis();
  }
  // Refresh Ubidots connection
  ubidots.loop();
}
