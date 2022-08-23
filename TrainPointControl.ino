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

// Define button inputs (using Analog Inputs) 
#define _btn1_pin 14 /*A0*/
#define _btn2_pin 15 /*A1*/
#define _btn3_pin 16 /*A2*/
#define _btn4_pin 17 /*A3*/

boolean _btn1;
boolean _btn2;
boolean _btn3;
boolean _btn4;

#define pulsetime 50 /*ms*/

int incomingByte = 0;
String strCommand;          // for current command data
String strCommandBuffer;    // for all incoming command data


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
}


void loop() {
  if (Serial.available() > 0) {
    strCommandBuffer += Serial.readString();
       
    Serial.println(strCommandBuffer);
  }

  // Read all the digital pins
  _btn1 = digitalRead(_btn1_pin);
  _btn2 = digitalRead(_btn2_pin);
  _btn3 = digitalRead(_btn3_pin);
  _btn4 = digitalRead(_btn4_pin);

  if (_btn1 == LOW) strCommandBuffer += "p1t\np2t\n";
  if (_btn2 == LOW) strCommandBuffer += "p1d\np2d\n";
  if (_btn3 == LOW) strCommandBuffer += "p3t\np4t\n";
  if (_btn4 == LOW) strCommandBuffer += "p3d\np4d\n";

  // Ensure that the buffer is terminated with \n
  if (strCommandBuffer.length() > 0 && strCommandBuffer.charAt(strCommandBuffer.length()-1) != '\n') {
    strCommandBuffer += '\n';
  }

  // If there is a new command
  if (strCommandBuffer.length() > 0) {
    // Get the first command delimited by '\n'
    strCommand = strCommandBuffer.substring(0, strCommandBuffer.indexOf('\n'));       // Exclude the found newline
    strCommandBuffer = strCommandBuffer.substring(strCommandBuffer.indexOf('\n')+1);  // Skip the newline
    
    if (strCommand=="p1t") {
      digitalWrite(_p1t_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p1t_pin, LOW);            delay(pulsetime);
    } else if (strCommand=="p1d") {
      digitalWrite(_p1d_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p1d_pin, LOW);            delay(pulsetime);
    }
    
    if (strCommand=="p2t") {
      digitalWrite(_p2t_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p2t_pin, LOW);            delay(pulsetime);
    } else if (strCommand=="p2d") {
      digitalWrite(_p2d_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p2d_pin, LOW);            delay(pulsetime);
    }
    
    if (strCommand=="p3t") {
      digitalWrite(_p3t_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p3t_pin, LOW);            delay(pulsetime);
    } else if (strCommand=="p3d") {
      digitalWrite(_p3d_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p3d_pin, LOW);            delay(pulsetime);
    }

    if (strCommand=="p4t") {
      digitalWrite(_p4t_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p4t_pin, LOW);            delay(pulsetime);
    } else if (strCommand=="p4d") {
      digitalWrite(_p4d_pin, HIGH);           delay(pulsetime);
      digitalWrite(_p4d_pin, LOW);            delay(pulsetime);
    }

    Serial.println(strCommand.length());
    Serial.println(strCommand);
    Serial.println(strCommandBuffer.length());
    Serial.println(strCommandBuffer);

    // Must reset it here
    strCommand = "";

    // Pulse the LED to indicate that command received
    // Force a 1 second delay
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
  
}
