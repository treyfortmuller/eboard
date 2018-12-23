// testing the inputs to the transmitter of the electric longboard
// there is a momentary NO switch for arming and a potentiometer for throttle control

// CONSTANTS
int switchPin = A3;  // select the input pin for the switch
int potPin = A2;  // select the input pin for the potentiometer

// VARIABLES
int switchVal = 0;  // variable to store the value coming from the switch pin
int potVal = 0;  // variable to store the value coming from the pot pin


void setup() {
  Serial.begin(9600);
}


void loop() {
  switchVal = analogRead(switchPin);  // read the value from the switch
  potVal = analogRead(potPin);  // read the value from the pot

  // print to the serial monitor the values read
  Serial.println(F("-------------"));
  Serial.println(switchVal);
  Serial.println(potVal);

  // run this loop at (approximately) 4Hz
  delay(250);
}
 
