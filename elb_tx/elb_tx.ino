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

// import required libraries
#include <SPI.h>   // Comes with Arduino IDE
#include "RF24.h"  // Download and Install (See above)
#include "printf.h" // Needed for "printDetails" Takes up some memory

// define constants
#define  CE_PIN  7   // The pins to be used for CE and SN
#define  CSN_PIN 8

#define DEADMANS_SWITCH A0  // the dead man's switch for arming the board
#define THROTTLE_POT A1 // the throttle pot connected to analog inputs

// initialize objects
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus (usually) pins 7 & 8 (Can be changed) */
RF24 radio(CE_PIN, CSN_PIN);

// variables declaration
byte addresses[][6] = {"1Node", "2Node"}; // These will be the names of the "Pipes"

unsigned long timeNow;  // Used to grab the current time, calculate delays
unsigned long started_waiting_at;
boolean timeout;       // Timeout? True or False

// initialize a data strucuture for the transmitted packets
// initialize a data strucuture for the received packets
struct dataStruct {
  unsigned long _micros;  // to save response times
  
  bool deadmansSwitch;    // The momentary ON switch
  
  int throttlePosition;   // The pot position values
  
} myData;                 // This can be accessed in the form:  myData.throttlePosition  etc.


void setup()
{
  Serial.begin(115200);  // MUST reset the Serial Monitor to 115200 (lower right of window )

  printf_begin(); // Needed for "printDetails" Takes up some memory
  pinMode(DEADMANS_SWITCH, INPUT_PULLUP);  // this pin will be used as a digital input

  radio.begin();          // Initialize the nRF24L01 Radio
  radio.setChannel(108);  // Above most WiFi frequencies
  radio.setDataRate(RF24_2MBPS); // fast at the expense of range
  
  // Set the Power Amplifier Level of the NRF24 module
  // PALevelcan be one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setPALevel(RF24_PA_MAX);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);

  // Start the radio listening for data
  radio.startListening();

//  radio.printDetails(); //Uncomment to show LOTS of debugging information

}


void loop()
{
  radio.stopListening(); // First, stop listening so we can talk.
  
  // read the position of the switch and pot
  myData.deadmansSwitch  = !digitalRead(DEADMANS_SWITCH);  // Invert the pulldown switch
  myData.throttlePosition = analogRead(THROTTLE_POT);

  myData._micros = micros();  // Send back for timing
  
  Serial.print(F("Now sending  -  "));

  // Send data, checking for error ("!" means NOT)
  if (!radio.write( &myData, sizeof(myData) )) {
    Serial.println(F("Transmit failed "));
  }

  radio.startListening(); // Now, continue listening

  started_waiting_at = micros();              // timeout period, get the current microseconds
  timeout = false;                            //  variable to indicate if a response was received or not

  while ( ! radio.available() ) {                            // While nothing is received
    if (micros() - started_waiting_at > 200000 ) {           // If waited longer than 200ms, indicate timeout and exit while loop
      timeout = true;
      break;
    }
  }

  if ( timeout )
  { // Describe the results
    Serial.println(F("Response timed out -  no Acknowledge."));
  }
  else
  {
    // Grab the response, compare, and send to Serial Monitor
    radio.read( &myData, sizeof(myData) );
    timeNow = micros();

    // Show it
    Serial.print(F("Sent "));
    Serial.print(timeNow);
    Serial.print(F(", Got response "));
    Serial.print(myData._micros);
    Serial.print(F(", Round-trip delay "));
    Serial.print(timeNow - myData._micros);
    Serial.println(F(" microseconds "));

  }

  // Send again after delay. When working OK, change to something like 100
  delay(100);

}
