// Arduino pin numbers
const int dead_switch = 3; // digital pin connected to switch output
const int press_pin = 2; // digital pin connected to joystick press output
const int X_pin = 0; // analog pin connected to X output
const int Y_pin = 1; // analog pin connected to Y output

// values of our inputs
int dead_switch_val = 0;
int press_pin_val = 0;
int X_val = 0;
int Y_val = 0;

// smoothing of the throttle command averaging
const int numReadings = 20;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int throttle_avg = 0;                // the average

void setup() {
  pinMode(dead_switch, INPUT);
  pinMode(press_pin, INPUT);
  digitalWrite(press_pin, HIGH); // why's this here?
  Serial.begin(115200);

  // initialize the throttle smoothing array to 0
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

}

void loop() {
  // Read from switches and pots
  dead_switch_val = digitalRead(dead_switch);
  press_pin_val  = digitalRead(press_pin);
  X_val = analogRead(X_pin);
  Y_val = analogRead(Y_pin);

  // smoothly ramp the throttle according to some time constant
  // subtract the last reading
  total = total - readings[readIndex];
  // read from the sensor
  readings[readIndex] = X_val;
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

  // Serial output of all the controller inputs
  Serial.print("Switch:  ");
  Serial.print(digitalRead(dead_switch));
  Serial.print("\n");
  
  Serial.print("Press:  ");
  Serial.print(digitalRead(press_pin));
  Serial.print("\n");
  
  Serial.print("X-axis: ");
  Serial.print(analogRead(X_pin));
  Serial.print("\n");

  Serial.print("Smoothed throttle: ");
  Serial.print(throttle_avg);
  Serial.print("\n");
  
  Serial.print("Y-axis: ");
  Serial.println(analogRead(Y_pin));
  Serial.print("\n\n");

  delay(100); // milliseconds, 10Hz main loop
}
