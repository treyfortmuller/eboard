#include <millisDelay.h>
#include <printf.h>
#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24.h>

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

// Arduino pin numbers
#define PIN_DEAD_SWITCH 3 // digital pin connected to momentary switch output
#define PIN_JS_CLICK    2 // digital pin connected to joystick click output
#define PIN_JS_X        0 // analog pin connected to joystick X-axis output
#define PIN_JS_Y        1 // analog pin connected to joystick Y-axis output

#define DELAY_READ_INPUT      100 // (ms) 100ms = 10Hz
#define DELAY_PRINT_RX_OUTPUT 100 

/***************************** Constants *********************************/

// Smoothing of the throttle command averaging
const int numReadings = 20;

/***************************** Variables *********************************/

// Stored values to keep track of
int readings[numReadings]; // the readings from the analog input
int readIndex = 0;         // the index of the current reading
int total = 0;             // the running total
int throttle_avg = 0;      // the average

// Delays
millisDelay input_delay;
millisDelay print_rx_delay;

unsigned long timeNow;  // Used to grab the current time, calculate delays
unsigned long started_waiting_at;
boolean timeout;       // Timeout? True or False

byte addresses[][6] = {"1Node", "2Node"}; // These will be the names of the "Pipes"


// Initialize objects
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus (usually) pins 7 & 8 (Can be changed) */
RF24 radio(PIN_RF24_CE, PIN_RF24_CSN);


/***************************** Structs *********************************/

// initialize a data strucuture for the tx packets ???
struct dataStruct {
  bool dead_switch;    // The momentary ON switch
  int js_x; // The pot position values for the x axis
  int throttle_pwm; // throttle averaged and formated as pwm
  int js_y; // The pot position values for the y axis
  bool js_click; // The joystick press value 
  unsigned long _micros;  // to save response times
} myData;                 // This can be accessed in the form:  myData.throttlePosition  etc.



/***************************** Functions *********************************/

void setup()
{
  // Arduino config
  Serial.begin(115200);  // MUST reset the Serial Monitor to 115200 (lower right of window )
  printf_begin(); // Needed for "printDetails" Takes up some memory


  // Radio config
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


  // Inputs config
  pinMode(PIN_DEAD_SWITCH, INPUT);
  pinMode(PIN_JS_CLICK, INPUT);

  input_delay.start(DELAY_READ_INPUT); // starts the input read delay
  print_rx_delay.start(DELAY_PRINT_RX_OUTPUT); // starts the rx print delay

  // Variables config
  // initialize the throttle smoothing array to 0
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

void read_inputs() 
{
  // Read from switches and pots
  myData.dead_switch = digitalRead(PIN_DEAD_SWITCH);
  myData.js_click  = digitalRead(PIN_JS_CLICK);
  myData.js_x = analogRead(PIN_JS_X);
  myData.js_y = analogRead(PIN_JS_Y);
}

void ramp_throttle()
{
  // smoothly ramp the throttle according to some time constant
  // subtract the last reading
  total = total - readings[readIndex];
  // read from the sensor
  readings[readIndex] = myData.js_x;
  // add the reading to the total
  total = total + readings[readIndex];
  // advance to the next position in the array
  readIndex = readIndex + 1;

  // if we're at the end of the array wrap around to the beginning
  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  // calculate the throttle average
  throttle_avg = total / numReadings;

  // map throttle_avg to pwm
  myData.throttle_pwm = map(throttle_avg, 0, 1023, 1300, 1700);
}

void print_inputs() {
  // Serial output of all the controller inputs
  Serial.print("Switch:  ");
  Serial.print(digitalRead(myData.dead_switch));
  Serial.print("\n");
  
  Serial.print("Press:  ");
  Serial.print(digitalRead(myData.js_click));
  Serial.print("\n");
  
  Serial.print("X-axis: ");
  Serial.print(analogRead(myData.js_x));
  Serial.print("\n");

  Serial.print("Smoothed PWM throttle: ");
  Serial.print(myData.throttle_pwm);
  Serial.print("\n");
  
  Serial.print("Y-axis: ");
  Serial.print(analogRead(myData.js_y));
  Serial.print("\n\n");
}

void process_throttle(bool verbose_output)
{
  read_inputs();
  ramp_throttle();
  if (verbose_output)
    print_inputs();
}

void loop()
{
  radio.stopListening(); // First, stop listening so we can talk.
  if (input_delay.isFinished()) {
    process_throttle(true);
    input_delay.start(DELAY_READ_INPUT);
  }
  
  myData._micros = micros();  // Send back for timing
  if (print_rx_delay.remaining() == 0) {
    Serial.print(F("Now sending  -  "));
  }

  // Send data, checking for error ("!" means NOT)
  if (!radio.write( &myData, sizeof(myData) )) {
    if (print_rx_delay.remaining() == 0) {
      Serial.println(F("Transmit failed "));
    }
  }

  radio.startListening(); // Now, continue listening

  started_waiting_at = micros(); // timeout period, get the current microseconds
  timeout = false; //  variable to indicate if a response was received or not

  while ( ! radio.available() ) { // While nothing is received
    if (micros() - started_waiting_at > 200000 ) { // If waited longer than 200ms, indicate timeout and exit while loop
      timeout = true;
      break;
    }
  }

  if (print_rx_delay.remaining() == 0) {
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
  }

 
  if (print_rx_delay.remaining() == 0) {
    print_rx_delay.start(DELAY_PRINT_RX_OUTPUT);
  }

}
