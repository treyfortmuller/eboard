#include <printf.h>
#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24.h>
#include <Servo.h>

/* 
CONNECTIONS
nRF24L01 Radio Module: See http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
   1 - GND
   2 - VCC 3.3V !!! NOT 5V
   3 - CE to Arduino pin 7
   4 - CSN to Arduino pin 8
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - UNUSED
*/

/***************************** Defines *********************************/

// The pins to be used for CE and SN
#define PIN_RF24_CE  7
#define PIN_RF24_CSN 8

#define PWM_MIN 850
#define PWM_MAX 2150

// initialize objects
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus (usually) pins 7 & 8 (Can be changed) */
RF24 radio(PIN_RF24_CE, PIN_RF24_CSN);

// variables declaration
byte addresses[][6] = {"1Node", "2Node"}; // These will be the names of the "Pipes"

int DEFAULT_PWM = 1500;

// for outputting PWM to the ESC
Servo VESC; // create servo object to control the Vedder ESC

/***************************** Structs *********************************/

// initialize a data strucuture for the tx packets
struct dataStruct {
  bool dead_switch;    // The momentary ON switch
  int js_x; // The pot position values for the x axis
  int throttle_pwm; // throttle averaged and formated as pwm
  int js_y; // The pot position values for the y axis
  bool js_click; // The joystick press value 
  unsigned long _micros;  // to save response times
} myData;                 // This can be accessed in the form:  myData.throttlePosition  etc.

void setup()
{
  // Arduino config
  Serial.begin(115200);  // MUST reset the Serial Monitor to 115200 (lower right of window )
  printf_begin(); // Needed for "printDetails" Takes up some memory

  VESC.attach(9); // attaches the servo on pin 9 to the servo object

  // Radio config
  radio.begin();          // Initialize the nRF24L01 Radio
  radio.setChannel(108);  // Above most WiFi frequencies
  radio.setDataRate(RF24_2MBPS); // fast at the expense of range
  
  // Set the Power Amplifier Level of the NRF24 module
  // PALevelcan be one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setPALevel(RF24_PA_MAX);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[0]);

  // Start the radio listening for data
  radio.startListening();
}


void loop()
{

  if ( radio.available()) {

    while (radio.available()) // While there is data ready to be retrieved from the receive pipe
    {
      radio.read( &myData, sizeof(myData) );             // Get the data
    }
    radio.stopListening();                               // First, stop listening so we can transmit
    radio.write( &myData, sizeof(myData) );              // Send the received data back.
    radio.startListening();                              // Now, resume listening so we catch the next packets.


    // print the received packet data
    Serial.print(F("Packet Received - Sent response "));
    Serial.println(myData._micros);

    if (myData.dead_switch)
      Serial.println(F("deadman_switch ON"));
    else
      Serial.println(F("deadman_switch OFF"));
      
    Serial.print(F("js_x: "));
    Serial.println(myData.js_x);
    Serial.print(F(", throttle pwm: "));
    Serial.println(myData.throttle_pwm);

    Serial.print(F("js_y: "));
    Serial.println(myData.js_y);

    if (myData.js_click)
      Serial.println(F("js_click ON"));
    else
      Serial.println(F("js_click OFF"));

    // write PWM to the ESC
    if (myData.dead_switch)
      VESC.writeMicroseconds(myData.throttle_pwm);
    else
      VESC.writeMicroseconds(DEFAULT_PWM);
   
  }
}

