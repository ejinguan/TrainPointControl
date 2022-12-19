#include <Wire.h>
#include <CommandQueue.h>
#include <Button.h>

#define THROUGH 0
#define DIVERT 1

// Pin A4 reserved for SDA
// Pin A5 reserved for SCL




/* Button Processing ************************************/

#define _debounce 200 /*ms*/
unsigned long lngCurrentMillis = 0;

// Define button inputs (using Analog Inputs) 
#define _pin_btn1 14 /*A0*/
#define _pin_btn2 15 /*A1*/
//#define _pin_btn3 16 /*A2*/
//#define _pin_btn4 17 /*A3*/

// Define and initialize Button objects
Button _btn1(_pin_btn1, _debounce);
Button _btn2(_pin_btn2, _debounce);


/* I2C Command Processing *******************************/

// For incoming data processing
String strCommand;          // for current command data
String strCommandBuffer;    // for all incoming command data


/* State Processing *************************************/

CommandQueue queue();

boolean _p1state = THROUGH;
boolean _p2state = THROUGH;

// Define interlock to through
boolean bP12interlocked = false;

// Min Wait Time in each execution cycle
#define _looptime 1 /*ms*/

// For LED on indication
boolean bLEDOn = false;
unsigned long lngLEDOnMillis = 0;


/* Output Processing ************************************/

// Define outputs for actuating the point motors
// Pair of through and divert
#define _pin_p1a_t 12
#define _pin_p1a_d 11
#define _pin_p1b_t 10
#define _pin_p1b_d 9
#define _pin_p2a_t 8
#define _pin_p2a_d 7
#define _pin_p2b_t 6
#define _pin_p2b_d 5

#define _pulsetime 50 /*ms*/




// From: https://docs.arduino.cc/learn/communication/wire
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  char c;
  
  while(0 < Wire.available()) { // loop through all characters
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
  pinMode(_pin_p1a_t, OUTPUT);
  pinMode(_pin_p1a_d, OUTPUT);
  pinMode(_pin_p1b_t, OUTPUT);
  pinMode(_pin_p1b_d, OUTPUT);
  pinMode(_pin_p2a_t, OUTPUT);
  pinMode(_pin_p2a_d, OUTPUT);
  pinMode(_pin_p2b_t, OUTPUT);
  pinMode(_pin_p2b_d, OUTPUT);

  // Set all button inputs to INPUT PULLUP
  pinMode(_pin_btn1, INPUT_PULLUP);
  pinMode(_pin_btn2, INPUT_PULLUP);
  // Initialized in Button class

  // Reset points to THROUGH
  P1command(DIVERT);
  P2command(DIVERT);
  delay(1000); // 1 sec
  P1command(THROUGH);
  P2command(THROUGH);
  delay(1000); // 1 sec

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
  _btn1.Update();
  _btn2.Update();

  // Action based on state change
  if (_btn1.JustChanged() && _btn1.Status() == LOW) {
    if (_p1state==DIVERT) {
      strCommandBuffer += "p1t\n";
    } else { // is THROUGH
      strCommandBuffer += "p1d\n";
    }
  } 
  if (_btn2.JustChanged() && _btn2.Status() == LOW) {
    if (_p2state==DIVERT) {
      strCommandBuffer += "p2t\n";
    } else { // is THROUGH
      strCommandBuffer += "p2d\n";
    }
  }

  Serial.print(_btn1.Status());
  Serial.print(",");
  Serial.print(_btn1.JustChanged());
  Serial.print(",");
  Serial.print(_btn2.Status());
  Serial.print(",");
  Serial.println(_btn2.JustChanged());


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
    if (strCommand=="s1on")   bP12interlocked = true;
    if (strCommand=="s1off")  bP12interlocked = false;
    if (strCommand=="s2on")   bP12interlocked = true;
    if (strCommand=="s2off")  bP12interlocked = false;

    // If interlocked, discard the incoming command
    if (bP12interlocked) {
      if (strCommand.charAt(0) == 'p') strCommand = "";

      P1command(THROUGH);
      P2command(THROUGH);

    } else {
      // Process point movement
      if (strCommand=="p1t") {
        P1command(THROUGH);
      } else if (strCommand=="p1d") {
        P1command(DIVERT);
      }
      
      if (strCommand=="p2t") {
        P2command(THROUGH);
      } else if (strCommand=="p2d") {
        P2command(DIVERT);
      }

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

  // Prevent main loop from running too fast if no more commands to process
  if (strCommandBuffer.length() == 0) {
    delay(_looptime);
  }
  
}



// OUTPUTS: Handle the command processing

void P1command(boolean command) {
  if (command==DIVERT) {
      // Pulse the DIVERT pins
      digitalWrite(_pin_p1a_d, HIGH);
      digitalWrite(_pin_p1b_d, HIGH);
      digitalWrite(_pin_p1a_t, LOW);
      digitalWrite(_pin_p1b_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1a_d, LOW);
      digitalWrite(_pin_p1b_d, LOW);
      digitalWrite(_pin_p1a_t, LOW);
      digitalWrite(_pin_p1b_t, LOW);
      delay(_pulsetime);
      _p1state = DIVERT;
      
  } else if (command==THROUGH) {    
      // Pulse the THROUGH pins
      digitalWrite(_pin_p1a_t, HIGH);
      digitalWrite(_pin_p1b_t, HIGH);
      digitalWrite(_pin_p1a_d, LOW);
      digitalWrite(_pin_p1b_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1a_t, LOW);
      digitalWrite(_pin_p1b_t, LOW);
      digitalWrite(_pin_p1a_d, LOW);
      digitalWrite(_pin_p1b_d, LOW);
      delay(_pulsetime);
      _p1state = THROUGH;
      
  }
}
void P2command(boolean command) {
  if (command==DIVERT) {
      // Pulse the DIVERT pins
      digitalWrite(_pin_p2a_d, HIGH);
      digitalWrite(_pin_p2b_d, HIGH);
      digitalWrite(_pin_p2a_t, LOW);
      digitalWrite(_pin_p2b_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2a_d, LOW);
      digitalWrite(_pin_p2b_d, LOW);
      digitalWrite(_pin_p2a_t, LOW);
      digitalWrite(_pin_p2b_t, LOW);
      delay(_pulsetime);
      _p2state = DIVERT;
      
  } else if (command==THROUGH) {     
      // Pulse the THROUGH pins
      digitalWrite(_pin_p2a_t, HIGH);
      digitalWrite(_pin_p2b_t, HIGH);
      digitalWrite(_pin_p2a_d, LOW);
      digitalWrite(_pin_p2b_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2a_t, LOW);
      digitalWrite(_pin_p2b_t, LOW);
      digitalWrite(_pin_p2a_d, LOW);
      digitalWrite(_pin_p2b_d, LOW);
      delay(_pulsetime);
      _p2state = THROUGH;
      
  }
}
