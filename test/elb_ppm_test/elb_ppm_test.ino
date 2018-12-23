/*
 Controlling a servo position using a potentiometer (variable resistor)
 by Michal Rinott <http://people.interaction-ivrea.it/m.rinott>

 modified on 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Knob
*/

#include <Servo.h>

Servo myservo;  // create servo object to control a servo

int command = 1500;    // variable to read the value from the analog pin

void setup() {
  Serial.begin(9600);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
}

void loop() {

  int newCommand = Serial.parseInt(); // read int, returns 0 if you don't input anything
  if (newCommand != 0){
    command = newCommand;
  }

  Serial.print("output: ");
  Serial.print(command);
  Serial.print("\n");
  
  myservo.writeMicroseconds(command);                  // sets the servo position according to the scaled value
  delay(15);                           // waits for the servo to get there
}


