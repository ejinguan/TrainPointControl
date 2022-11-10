#include <Wire.h>

// Pin A4 reserved for SDA
// Pin A5 reserved for SCL

// Define outputs for actuating the point motors
// Pair of through and divert
#define _p1t_pin 5
#define _p1d_pin 6
#define _p2t_pin 7
#define _p2d_pin 8
#define _p3t_pin 9
#define _p3d_pin 10
#define _p4t_pin 11
#define _p4d_pin 12

#define _pulsetime 50 /*ms*/

// Define button inputs (using Analog Inputs) 
#define _btn1_pin 14 /*A0*/
#define _btn2_pin 15 /*A1*/
#define _btn3_pin 16 /*A2*/
#define _btn4_pin 17 /*A3*/

boolean _btn1;
boolean _btn2;
boolean _btn3;
boolean _btn4;
unsigned long lngBtn1Millis = 0;
unsigned long lngBtn2Millis = 0;
unsigned long lngBtn3Millis = 0;
unsigned long lngBtn4Millis = 0;

unsigned long lngCurrentMillis = 0;
#define _debounce 200 /*ms*/

// Define interlock to through
boolean bp1234interlocked = false;

// For incoming data processing
String strCommand;          // for current command data
String strCommandBuffer;    // for all incoming command data

// For LED on indication
boolean bLEDOn = false;
unsigned long lngLEDOnMillis = 0;




// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  char c;
  
  while(0 < Wire.available()) { // loop through all but the last
    c = Wire.read();            // receive byte as a character
    strCommandBuffer += c;      // append character
    Serial.print(c);            // print the character
  }
  Serial.println('\n');         // print new line

  // if last character is not a newline, append a new line
  if (c != '\n') strCommandBuffer += '\n';  
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Set all point motor pins to OUTPUT
  pinMode(_p1t_pin, OUTPUT);
  pinMode(_p1d_pin, OUTPUT);
  pinMode(_p2t_pin, OUTPUT);
  pinMode(_p2d_pin, OUTPUT);
  pinMode(_p3t_pin, OUTPUT);
  pinMode(_p3d_pin, OUTPUT);
  pinMode(_p4t_pin, OUTPUT);
  pinMode(_p4d_pin, OUTPUT);

  // Set all button inputs to INPUT PULLUP
  pinMode(_btn1_pin, INPUT_PULLUP);
  pinMode(_btn2_pin, INPUT_PULLUP);
  pinMode(_btn3_pin, INPUT_PULLUP);
  pinMode(_btn4_pin, INPUT_PULLUP);

  Serial.begin(9600);
  Wire.begin(2);
  Wire.onReceive(receiveEvent); // register event
}


void loop() {
  // Save current millis
  lngCurrentMillis = millis();
  
  if (Serial.available() > 0) {
    strCommandBuffer += Serial.readString();
       
    Serial.println(strCommandBuffer);
  }

  // Read all the digital pins, check for debounce before appending command
  _btn1 = digitalRead(_btn1_pin);
  _btn2 = digitalRead(_btn2_pin);
  _btn3 = digitalRead(_btn3_pin);
  _btn4 = digitalRead(_btn4_pin);

  if (_btn1 == LOW && lngBtn1Millis < lngCurrentMillis + _debounce) { strCommandBuffer += "p1t\np2t\n"; lngBtn1Millis = lngCurrentMillis; }
  if (_btn2 == LOW && lngBtn2Millis < lngCurrentMillis + _debounce) { strCommandBuffer += "p1d\np2d\n"; lngBtn2Millis = lngCurrentMillis; }
  if (_btn3 == LOW && lngBtn3Millis < lngCurrentMillis + _debounce) { strCommandBuffer += "p3t\np4t\n"; lngBtn3Millis = lngCurrentMillis; }
  if (_btn4 == LOW && lngBtn4Millis < lngCurrentMillis + _debounce) { strCommandBuffer += "p3d\np4d\n"; lngBtn4Millis = lngCurrentMillis; }


  // Ensure that the buffer is terminated with \n
  if (strCommandBuffer.length() > 0 && strCommandBuffer.charAt(strCommandBuffer.length()-1) != '\n') {
    strCommandBuffer += '\n';
  }


  // If there is a new command
  if (strCommandBuffer.length() > 0) {
    // Get the first command delimited by '\n'
    strCommand = strCommandBuffer.substring(0, strCommandBuffer.indexOf('\n'));       // Exclude the found newline
    strCommandBuffer = strCommandBuffer.substring(strCommandBuffer.indexOf('\n')+1);  // Skip the newline
  }


  // Process the commands
  if (strCommand.length() > 0) {
    // Process speed broadcasts
    if (strCommand=="s1on")   bp1234interlocked = true;
    if (strCommand=="s1off")  bp1234interlocked = false;
    if (strCommand=="s2on")   bp1234interlocked = true;
    if (strCommand=="s2off")  bp1234interlocked = false;

    // If interlocked, discard the incoming command
    if (bp1234interlocked) {
      if (strCommand.charAt(0) == 'p') strCommand = "";      
    }

    // Process point movement
    if (strCommand=="p1t") {
      digitalWrite(_p1t_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p1t_pin, LOW);            delay(_pulsetime);
    } else if (strCommand=="p1d") {
      digitalWrite(_p1d_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p1d_pin, LOW);            delay(_pulsetime);
    }
    
    if (strCommand=="p2t") {
      digitalWrite(_p2t_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p2t_pin, LOW);            delay(_pulsetime);
    } else if (strCommand=="p2d") {
      digitalWrite(_p2d_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p2d_pin, LOW);            delay(_pulsetime);
    }
    
    if (strCommand=="p3t") {
      digitalWrite(_p3t_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p3t_pin, LOW);            delay(_pulsetime);
    } else if (strCommand=="p3d") {
      digitalWrite(_p3d_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p3d_pin, LOW);            delay(_pulsetime);
    }

    if (strCommand=="p4t") {
      digitalWrite(_p4t_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p4t_pin, LOW);            delay(_pulsetime);
    } else if (strCommand=="p4d") {
      digitalWrite(_p4d_pin, HIGH);           delay(_pulsetime);
      digitalWrite(_p4d_pin, LOW);            delay(_pulsetime);
    }

    Serial.println(strCommand.length());
    Serial.println(strCommand);
    Serial.println(strCommandBuffer.length());
    Serial.println(strCommandBuffer);

    // Turn on the LED to indicate that command received
    lngLEDOnMillis = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    bLEDOn = true;    
  }


  // Broadcast
  if (strCommand.length() > 0) {
    Wire.beginTransmission(1);        // Transmit to device number 1 (0x2C)
    Wire.write(strCommand.c_str());   // Send the C string array
    Wire.endTransmission();

    // Must reset it here
    strCommand = "";
  }
  

  // Turn off the LED after a 1 second delay
  if (bLEDOn && millis() > lngLEDOnMillis + 1000) {
    lngLEDOnMillis = 0;
    digitalWrite(LED_BUILTIN, LOW);
    bLEDOn = false;
  }
  
}
