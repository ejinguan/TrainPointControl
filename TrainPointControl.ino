#include <Wire.h>
#include <CommandQueue.h>
#include <Button.h>

#define THROUGH 0
#define DIVERT 1

#define RED 1
#define GREEN 0

// Pin A4 reserved for SDA
// Pin A5 reserved for SCL

// Command format:
// xxx = command
// _xxx = status update
// ?xxx = query

unsigned long lngCurrentMillis = 0;



/* I2C Command Processing *******************************/

#define I2C_Broadcast 0
#define I2C_SpeedController 11
#define I2C_RemotePointsController 12
#define I2C_LocalPointsController 13

// For incoming data processing
String strSerialCommand;      // for Incoming Serial command data
String strI2CCommand;         // for Incoming I2C command data

String strCommand;            // Current command for processing retrieved from queue
String strResponse;           // Response to command


/* State Processing *************************************/

CommandQueue queue(10);

boolean _speedCtl1;
boolean _speedCtl2;

boolean _p1state = THROUGH;
boolean _p2state = THROUGH;

// Define interlock to through
boolean bP12interlocked = false;

// Min Wait Time in each execution cycle
#define _looptime 1 /*ms*/

// For LED on indication
boolean bLEDOn = false;
unsigned long lngLEDOnMillis = 0;



// 8 bit LED register to indicate LED status
byte _LEDstate[2];
// 0,1 for lights (0-7, 8-11)

// Shift register ID of first signal light
#define FIRSTSIGNALID 0
// Integer divide Light ID by 8 to get corresponding byte
// Modulo of LightID with 8 to get bit number in the byte
#define setLight(LightID)     bitSet  (_LEDstate[(LightID+FIRSTSIGNALID)/8], ((LightID)%8) ) 
#define clearLight(LightID)   bitClear(_LEDstate[(LightID+FIRSTSIGNALID)/8], ((LightID)%8) ) 

int _LED_brightness = 1;        // out of 10
int _LED_brightness_counter = 0;


/* Output Processing ************************************/

// Define outputs for actuating the point motors
// Pair of through and divert
#define _pin_p1a_t 11
#define _pin_p1a_d 10
#define _pin_p1b_t 9
#define _pin_p1b_d 8
#define _pin_p2a_t 7
#define _pin_p2a_d 6
#define _pin_p2b_t 5
#define _pin_p2b_d 4

#define _pulsetime 50 /*ms*/

#define _shiftDATA 2
#define _shiftCLOCK 3
#define _shiftLATCH 12




// From: https://docs.arduino.cc/learn/communication/wire
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  char c;
  int i;

  for(i=0;i<howMany;i++) {              // loop through all characters
    c = Wire.read();                    // receive byte as a character
    if (c == '\n') {                    // if newline received, add to queue
      if (strI2CCommand.length() > 0) { // Add to queue if length is > 0
        queue.push(strI2CCommand);
        strI2CCommand = "";
      }
    } else {
      strI2CCommand += c;               // append character
    }
  }
  
  if (strI2CCommand.length() > 0) {     // Add to queue if length is > 0
    queue.push(strI2CCommand);
    strI2CCommand = "";                 // Reset to blank
  }
}

void processSerialInput() {
  // Run if Serial is available
  if (Serial.available() > 0) {
    strSerialCommand += Serial.readString();

    if (strSerialCommand.charAt(strSerialCommand.length()-1) == '\n') {                       // If last character is newline
      strSerialCommand = strSerialCommand.substring(0, strSerialCommand.indexOf('\n'));       // Exclude the found newline
    }
    
    queue.push(strSerialCommand);                             // Push the command into the queue
    Serial.println(strSerialCommand);
    strSerialCommand = "";
  }  
}

void setup() {
  // Start communications
  Serial.begin(9600);
  Serial.println("Setting Inputs");
  
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

  // Set all Shift Register pins to OUTPUT
  pinMode(_shiftDATA, OUTPUT);
  pinMode(_shiftCLOCK, OUTPUT);
  pinMode(_shiftLATCH, OUTPUT);

  Serial.println("Resetting Points");
  
  // Reset points to THROUGH
  P1command(DIVERT);
  P2command(DIVERT);
  UpdateLEDs();
  delay(1000); // 1 sec
  P1command(THROUGH);
  P2command(THROUGH);
  UpdateLEDs();
  delay(1000); // 1 sec

  // Start I2C
  Wire.begin(I2C_RemotePointsController);                     // Address
  bitSet(TWAR, TWGCE);                                        // Enable I2C General Call receive (broadcast) in TWI Addr Register
  Wire.onReceive(receiveEvent);                               // Register event handler for I2C

  Serial.println("I2C started");
}




void loop() {
  // Save current millis
  lngCurrentMillis = millis();

  /**********************************************************************************
  /* Step 1: Process incoming commands on Serial and I2C
  /**********************************************************************************/
  // I2C is handled using event handler receiveEvent(int)

  // Serial is handled using void processSerialInput()
  processSerialInput();
  

  /**********************************************************************************
  /* Step 2: Process I/O
  /**********************************************************************************/
  // Process on-board IO inputs
  //processIOInput();
  
  /**********************************************************************************
  /* Step 3: Consume Command Queue
  /**********************************************************************************/

  // If there is a new command
  if (queue.size() > 0) {
    // Get the first command
    strCommand = queue.pop();
    Serial.println("Current command: " + strCommand);
  } else {
    strCommand = "";
  }


  // Process the commands
  if (strCommand.length() > 0) {
    // Process speed broadcasts
    if (strCommand=="_s1on")   _speedCtl1 = true;
    if (strCommand=="_s1off")  _speedCtl1 = false;
    if (strCommand=="_s2on")   _speedCtl2 = true;
    if (strCommand=="_s2off")  _speedCtl2 = false;

    // Interlocked if both speed controls are on
    bP12interlocked = _speedCtl1 && _speedCtl2;

    // If interlocked, discard the incoming command
    if (bP12interlocked) {
      if (strCommand.charAt(0)=='p' && (strCommand.charAt(1)=='1' || strCommand.charAt(1)=='2')) {
        strCommand = "";
        // Todo: flash lights
      }

      P1command(THROUGH);                 // Command both points to through anyway
      P2command(THROUGH);

    } else {
      // Not interlocked, free to move
      
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
  }

  
  /**********************************************************************************
  /* Step 4: Update LEDs
  /**********************************************************************************/
  
  UpdateLEDs();

  
  /**********************************************************************************
  /* Step 5: Update Serial, LED, I2C
  /**********************************************************************************/

  // Serial write, update onboard LED
  if (strCommand.length() > 0) {

    Serial.println("Cmd (len): " + strCommand + " (" + strCommand.length() + ")");
    Serial.print("Q len: ");
    Serial.println(queue.size());

    // Turn on the LED to indicate that command received
    lngLEDOnMillis = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    bLEDOn = true;
  }
  
  // Turn off the LED after a 1 second delay
  if (bLEDOn && millis() > (lngLEDOnMillis + 1000)) {
    lngLEDOnMillis = 0;
    digitalWrite(LED_BUILTIN, LOW);
    bLEDOn = false;
  }

  UpdateI2C();



  
  /**********************************************************************************
  /* Step 6: Loop
  /**********************************************************************************/

  // Prevent main loop from running too fast if no more commands to process
  if (queue.size() == 0) {
    delay(_looptime);
  }
  
}


void UpdateI2C() {

  // Broadcast on I2C
  if (strCommand.length() > 0) {

    // Broadcast current state
    if (strCommand.substring(0,2)=="p1" || strCommand.substring(0,2)=="p2") {

      strResponse = "_" + strCommand;

      Wire.beginTransmission(I2C_Broadcast);          // Broadcast device
      Wire.write(strResponse.c_str());                       // Send the C string array
      Wire.endTransmission();
      Serial.println(strResponse);
       
    }

    if (strCommand.substring(0,1)=="?") {
      
      strResponse = "";
      
      if (strCommand.substring(0,3)=="?p1") strResponse = (_p1state == THROUGH)?"p1t":"p1d";
      if (strCommand.substring(0,3)=="?p2") strResponse = (_p2state == THROUGH)?"p2t":"p2d";

      if (strResponse.length() > 0) {

        Wire.beginTransmission(I2C_Broadcast);          // Broadcast device
        Wire.write(strResponse.c_str());                       // Send the C string array
        Wire.endTransmission();
        Serial.println(strResponse);
     
      }
    }
    

    // Must reset it here
    strCommand = "";
  }
  
}



// OUTPUTS: Handle the command processing

void P1command(boolean command) {
  // Left side 
  if (command==DIVERT) {
      // Pulse the DIVERT pins
      digitalWrite(_pin_p1a_d, HIGH);
      digitalWrite(_pin_p1a_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1a_d, LOW);
      digitalWrite(_pin_p1a_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1b_d, HIGH);
      digitalWrite(_pin_p1b_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1b_d, LOW);
      digitalWrite(_pin_p1b_t, LOW);
      delay(_pulsetime);
      _p1state = DIVERT;

      clearLight(3);
      setLight(4);
      clearLight(5);
      
  } else if (command==THROUGH) {    
      // Pulse the THROUGH pins
      digitalWrite(_pin_p1a_t, HIGH);
      digitalWrite(_pin_p1a_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1a_t, LOW);
      digitalWrite(_pin_p1a_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1b_t, HIGH);
      digitalWrite(_pin_p1b_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p1b_t, LOW);
      digitalWrite(_pin_p1b_d, LOW);
      delay(_pulsetime);
      _p1state = THROUGH;

      setLight(3);
      clearLight(4);
      setLight(5);
      
  }
  
  UpdateLEDs();
}
void P2command(boolean command) {
  // Right side
  if (command==DIVERT) {
      // Pulse the DIVERT pins
      digitalWrite(_pin_p2a_d, HIGH);
      digitalWrite(_pin_p2a_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2a_d, LOW);
      digitalWrite(_pin_p2a_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2b_d, HIGH);
      digitalWrite(_pin_p2b_t, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2b_d, LOW);
      digitalWrite(_pin_p2b_t, LOW);
      delay(_pulsetime);
      _p2state = DIVERT;

      clearLight(0);
      setLight(1);
      clearLight(2);
      
  } else if (command==THROUGH) {     
      // Pulse the THROUGH pins
      digitalWrite(_pin_p2a_t, HIGH);
      digitalWrite(_pin_p2a_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2a_t, LOW);
      digitalWrite(_pin_p2a_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2b_t, HIGH);
      digitalWrite(_pin_p2b_d, LOW);
      delay(_pulsetime);
      digitalWrite(_pin_p2b_t, LOW);
      digitalWrite(_pin_p2b_d, LOW);
      delay(_pulsetime);
      _p2state = THROUGH;
      
      setLight(0);
      clearLight(1);
      setLight(2);
      
  }
  
  UpdateLEDs();
}

void UpdateSignals() {

  // Assume THROUGH first
  if (_p1state==THROUGH) {
    UpdateSignalID(1, GREEN);
    UpdateSignalID(2, RED);
    UpdateSignalID(3, GREEN);    
  }
  
  if (_p2state==THROUGH) {
    UpdateSignalID(4, GREEN);
    UpdateSignalID(5, RED);
    UpdateSignalID(6, GREEN);
  }

  // Modify lights 4/3 if DIVERT is present for _p1 and _p2
  if (_p1state==DIVERT) {
    UpdateSignalID(1, RED);
    UpdateSignalID(2, GREEN);
    UpdateSignalID(3, RED);
        
    UpdateSignalID(4, RED);
  }
  if (_p2state==DIVERT) {
    UpdateSignalID(4, RED);
    UpdateSignalID(5, GREEN);
    UpdateSignalID(6, RED);
        
    UpdateSignalID(1, RED);
  }
  
  
  
}

void UpdateSignalID(int SignalID, int RedOrGreen) {
  // Translate SignalID to LED ID
  // LED ID = FIRSTSIGNALID + (SignalID-1) * 2 + RedOrGreen
  // FIRSTSIGNALID is the shift register index for the first signal light
  // if SignalID = 5
  // Red LED =   FIRSTSIGNALID + (5-1)*2 + 1 = 17
  // Green LED = FIRSTSIGNALID + (5-1)*2 + 0 = 16

  if (RedOrGreen == RED) {
    setLight  (FIRSTSIGNALID + (SignalID-1) * 2 + 1);
    clearLight(FIRSTSIGNALID + (SignalID-1) * 2);
  } else {
    clearLight(FIRSTSIGNALID + (SignalID-1) * 2 + 1);
    setLight  (FIRSTSIGNALID + (SignalID-1) * 2);
  }
}


// Update LEDs via shift register
void UpdateLEDs() {
  
  digitalWrite(_shiftCLOCK, LOW); 
  digitalWrite(_shiftLATCH, LOW);

  UpdateSignals();

  //if (_LED_brightness_counter <= _LED_brightness) {
    // Shift out remote lights first (invert as common cathode)
    shiftOut(_shiftDATA, _shiftCLOCK, MSBFIRST, ~(_LEDstate[1]));
    shiftOut(_shiftDATA, _shiftCLOCK, MSBFIRST, ~(_LEDstate[0]));
  //} else {
  //  // Shift out remote lights first (invert as common cathode)
  //  shiftOut(_shiftDATA, _shiftCLOCK, MSBFIRST, 0xFF);
  //  shiftOut(_shiftDATA, _shiftCLOCK, MSBFIRST, 0xFF);
  //}
  //_LED_brightness_counter++;
  //if (_LED_brightness_counter>10) _LED_brightness_counter = 0;
  
  // Shift out MSB (7) first as outputs are 0-5
  //shiftOut(_shiftDATA, _shiftCLOCK, MSBFIRST, _LEDstate[0]);

  digitalWrite(_shiftCLOCK, LOW);
  
  digitalWrite(_shiftLATCH, HIGH);  // Pulse the latch
  digitalWrite(_shiftLATCH, LOW);  

}
