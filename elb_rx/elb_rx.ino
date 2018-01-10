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

#define PWM_MIN 850
#define PWM_MAX 2150

// CONFIGURATION
#define CHANNEL_NUMBER 12  //set the number of channels
#define CHANNEL_DEFAULT_VALUE 1000  //set the default servo value
#define FRAME_LENGTH 22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PULSE_LENGTH 300  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define outputSignalPin 9  //set PPM signal output pin on the arduino

// this array holds the values for the ppm signal
int ppm[CHANNEL_NUMBER];

// initialize objects
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus (usually) pins 7 & 8 (Can be changed) */
RF24 radio(CE_PIN, CSN_PIN);

// variables declaration
byte addresses[][6] = {"1Node", "2Node"}; // These will be the names of the "Pipes"

int armed = 1000;

// initialize a data strucuture for the received packets
struct dataStruct {
  unsigned long _micros;  // to save response times

  bool deadmansSwitch;    // The momentary ON switch
  
  int throttlePosition;   // The pot position values
  
} myData;                 // This can be accessed in the form:  myData.throttlePosition  etc.


void setup()
{
  Serial.begin(115200);   // MUST reset the Serial Monitor to 115200 (lower right of window )

  printf_begin(); // Needed for "printDetails" Takes up some memory
  
  radio.begin();          // Initialize the nRF24L01 Radio
  radio.setChannel(108);  // 2.508 Ghz - Above most Wifi Channels
  radio.setDataRate(RF24_2MPS); // fast at the expense of range
  
  // Set the Power Amplifier Level of the NRF24 module
  // PALevelcan be one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setPALevel(RF24_PA_MAX);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[0]);

  // Start the radio listening for data
  radio.startListening();

  //  radio.printDetails(); //Uncomment to show LOTS of debugging information

  //initialize default ppm values
  for(int i=0; i<CHANNEL_NUMBER; i++){
      ppm[i]= CHANNEL_DEFAULT_VALUE;
  }

  // set up pinMode of the output signal pin (going to the ESC)
  pinMode(outputSignalPin, OUTPUT);
  digitalWrite(outputSignalPin, !onState);  //set the PPM signal pin to the default polarity state

  // wtf is all this shit working directly with registers
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
  
}


void loop()
{

  if ( radio.available())
  {

    while (radio.available()) // While there is data ready to be retrieved from the receive pipe
    {
      radio.read( &myData, sizeof(myData) );             // Get the data
    }

    radio.stopListening();                               // First, stop listening so we can transmit
    radio.write( &myData, sizeof(myData) );              // Send the received data back.
    radio.startListening();                              // Now, resume listening so we catch the next packets.

    // print the received packet data
    Serial.print(F("Packet Received - Sent response "));
    Serial.print(myData._micros);
    
    Serial.print(F("uS pot"));
    Serial.print(myData.throttlePosition);

    if ( myData.deadmansSwitch == 1)
    {
      Serial.println(F(" Switch ON"));
    }
    else
    {
      Serial.println(F(" Switch OFF"));
    }

  }

  // interpolate the throttle value across the desired PWM range
  int throttle = map(myData.throttlePosition, 1023, 0, PWM_MIN, PWM_MAX);

  // TRIMS
  throttle += 0;

  // interpolate the arming switch (deadmans) value across the desired PWM range
  int arm = map(myData.switchOn, 0, 1, 1000, 2000);

  // print out the interpolated pwm values
  Serial.print(" Arm");
  Serial.println(arm);

  Serial.print(" Throttle");
  Serial.print(throttle);
  
  ppm[0] = arm;  
  ppm[1] = throttle;

}

// some kind of interrupt sub-routine that sends our PPM signal along the output
ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;
  
  TCNT1 = 0;
  
  if (state) {  //start pulse
    digitalWrite(outputSignalPin, onState);
    OCR1A = PULSE_LENGTH * 2;
    state = false;
  } else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(outputSignalPin, !onState);
    state = true;

    if(cur_chan_numb >= CHANNEL_NUMBER){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PULSE_LENGTH;// 
      OCR1A = (FRAME_LENGTH - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PULSE_LENGTH) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
