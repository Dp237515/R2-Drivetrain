// =======================================================================================
// /////////////////////////RomoBot360 Body Code v1.0 ////////////////////////////////////
// =======================================================================================
/*
by Duncan Price
email: bananawithsprinkles@gmail.com

Based on Padawan360 by Dan Kraus which can be found here (https://github.com/dankraus/padawan360/blob/master/padawan360_body/padawan360_body_uno/padawan360_body_uno.ino).
This code is configured to use the left stick of an Xbox 360 controller and run on an Arduino UNO. There is a list of the hardware
that this code is designed to be run on listed down below. 

v1.0

Hardware:
***Arduino UNO***
USB Host Shield from circuits@home
Microsoft Xbox 360 Controller
Xbox 360 USB Wireless Reciver
Sabertooth Motor Controller

Set Sabertooth 2x25/2x12 Dip Switches 1 and 2 Down, All Others Up

*/

//************************** Set speed and turn speeds here************************************//

//set these 3 to whatever speeds work for you. 0-stop, 127-full speed.
const byte DRIVESPEED1 = 50;
const byte DRIVESPEED2 = 75;
//Set to 0 if you only want 2 speeds.
const byte DRIVESPEED3 = 100;

byte drivespeed = DRIVESPEED1;

// the higher this number the faster the droid will spin in place, lower - easier to control.
const byte TURNSPEED = 40; 

// Ramping- the lower this number the longer R2 will take to speedup or slow down, change this by incriments of 1
const byte RAMPING = 5;

// Compensation is for deadband/deadzone checking. There's a little play in the neutral zone which gets a reading of a value of something other than 0 when you're not moving the stick.
// It may vary a bit across controllers and how broken in they are, sometimex 360 controllers
// develop a little bit of play in the stick at the center position. You can do this with the direct method calls against the Sabertooth library itself but it's not supported in all
// serial modes so just manage and check it in software here, use the lowest number with no drift
// LEFTSTICKDEADZONE for the left stick, RIGHTSTICKDEADZONE for the right stick
const byte LEFTSTICKDEADZONE = 0;
const byte RIGHTSTICKDEADZONE = 0;

#include <Sabertooth.h>
#include <Servo.h>
#include <XBOXRECV.h>
#include <SoftwareSerial.h>

// These are the pins for the Sabertooth
SoftwareSerial STSerial(NOT_A_PIN, 4);

/////////////////////////////////////////////////////////////////
Sabertooth ST(128, STSerial);

// Satisfy IDE, which only needs to see the include statment in the ino. (Not sure what this does)
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

// Set some defaults for start up
// 0 = drive motors off at start
boolean isDriveEnabled = false;

// Automated function variables
// Used as a boolean to turn on/off automated functions like periodic random sounds and periodic dome turns
//boolean isInAutomationMode = false;
//unsigned long automateMillis = 0;
//byte automateDelay = random(5,20);// set this to min and max seconds between sounds
//How much the dome may turn during automation.
//int turnDirection = 20;
// Action number used to randomly choose a sound effect or a dome turn
//byte automateAction = 0;

int driveThrottle = 0;
int throttleStickValue = 0;
int turnThrottle = 0;

boolean firstLoadOnConnect = false;

AnalogHatEnum throttleAxis;
AnalogHatEnum turnAxis;
ButtonEnum speedSelectButton;

USB Usb;
XBOXRECV Xbox(&Usb);

void setup(){
  // 9600 is the default baud rate for Sabertooth packet serial.
  STSerial.begin(9600);
  // Send the autobaud command to the Sabertooth controller(s).
  ST.autobaud();
  /* NOTE: *Not all* Sabertooth controllers need this command.
  It doesn't hurt anything, but V2 controllers use an
  EEPROM setting (changeable with the function setBaudRate) to set
  the baud rate instead of detecting with autobaud.
  */

  ST.setTimeout(950);
  
  // The Sabertooth won't act on mixed mode packet serial commands until
  // it has received power levels for BOTH throttle and turning, since it
  // mixes the two together to get diff-drive power levels for both motors.
  ST.drive(0);
  ST.turn(0);

  //pinMode(EXTINGUISHERPIN, OUTPUT);
  //digitalWrite(EXTINGUISHERPIN, HIGH);

  throttleAxis = LeftHatY;
  turnAxis = LeftHatX;
  speedSelectButton = L3;

  //Serial.begin(115200);
  // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  while (!Serial);
    if (Usb.Init() == -1) {
      //Serial.print(F("\r\nOSC did not start"));
      while (1); //halt
    }
  //Serial.print(F("\r\nXbox Wireless Receiver Library Started"));
}


void loop(){
  Usb.Task();
  // if we're not connected, return so we don't bother doing anything else.
  // set all movement to 0 so if we lose connection we don't have a runaway droid!
  if(!Xbox.XboxReceiverConnected || !Xbox.Xbox360Connected[0]){
    ST.drive(0);
    ST.turn(0);
    firstLoadOnConnect = false;
    return;
  }

  // After the controller connects, Blink all the LEDs so we know drives are disengaged at start
  if(!firstLoadOnConnect){
    firstLoadOnConnect = true;
    Xbox.setLedMode(ROTATING, 0);
  }
  
  if (Xbox.getButtonClick(XBOX, 0)) {
    if(Xbox.getButtonPress(L1, 0) && Xbox.getButtonPress(R1, 0)){ 
      Xbox.disconnect(0);
    }
  }

  // enable / disable left stick (droid movement)
  if(Xbox.getButtonClick(START, 0)) {
    if(isDriveEnabled){
      isDriveEnabled = false;
      Xbox.setLedMode(ROTATING, 0);
    } else {
      isDriveEnabled = true;
      // //When the drive is enabled, set our LED accordingly to indicate speed
      if(drivespeed == DRIVESPEED1){
        Xbox.setLedOn(LED1, 0);
      } else if(drivespeed == DRIVESPEED2 && (DRIVESPEED3!=0)){
        Xbox.setLedOn(LED2, 0);
      } else {
        Xbox.setLedOn(LED3, 0);
      }
    }
  }

  //Toggle automation mode with the BACK button
  /*if(Xbox.getButtonClick(BACK, 0)) {
    if(isInAutomationMode){
      isInAutomationMode = false;
      automateAction = 0;
    } else {
      isInAutomationMode = true;
    }
  }*/

  // Change drivespeed if drive is eabled
  // Press Left Analog Stick (L3)
  // Set LEDs for speed - 1 LED, Low. 2 LED - Med. 3 LED is high
  if(Xbox.getButtonClick(speedSelectButton, 0) && isDriveEnabled) {
    //if in lowest speed
    if(drivespeed == DRIVESPEED1){
      //change to medium speed and play sound 3-tone
      drivespeed = DRIVESPEED2;
      Xbox.setLedOn(LED2, 0);
    } else if(drivespeed == DRIVESPEED2 && (DRIVESPEED3!=0)){
      //change to high speed and play sound scream
      drivespeed = DRIVESPEED3;
      Xbox.setLedOn(LED3, 0);
    } else {
      //we must be in high speed
      //change to low speed and play sound 2-tone
      drivespeed = DRIVESPEED1;
      Xbox.setLedOn(LED1, 0);
    }
  }


  // FOOT DRIVES
  // Xbox 360 analog stick values are signed 16 bit integer value
  // Sabertooth runs at 8 bit signed. -127 to 127 for speed (full speed reverse and  full speed forward)
  // Map the 360 stick values to our min/max current drive speed
  throttleStickValue = (map(Xbox.getAnalogHat(throttleAxis, 0), -32768, 32767, -drivespeed, drivespeed));
  //this is basically saying hey the stick is at this postion accelerate the motors
  if (throttleStickValue > -LEFTSTICKDEADZONE && throttleStickValue < LEFTSTICKDEADZONE) {
    // stick is in dead zone - don't drive
    driveThrottle = 0;
  }//This is saying hey you reached the stick stop accelerating 
  else {
    //this is telling it to move forward
    if (driveThrottle < throttleStickValue) {
      if (throttleStickValue - driveThrottle < (RAMPING + 1) ) {
        driveThrottle += RAMPING;
      } else {
        driveThrottle = throttleStickValue;
      }
    } //this tells it to move backward 
    else if (driveThrottle > throttleStickValue) {
      if (driveThrottle - throttleStickValue < (RAMPING + 1) ) {
        driveThrottle -= RAMPING;
      } else {
        driveThrottle = throttleStickValue;
      }
    }
  }

  turnThrottle = map(Xbox.getAnalogHat(turnAxis, 0), -32768, 32767, -TURNSPEED, TURNSPEED);

  // DRIVE!
  // left stick (drive)
  if (isDriveEnabled) {
    // Only do deadzone check for turning here. Our Drive throttle speed has some math applied
    // for RAMPING and stuff, so just keep it separate here
    if (turnThrottle > -LEFTSTICKDEADZONE && turnThrottle < LEFTSTICKDEADZONE) {
      // stick is in dead zone - don't turn
      turnThrottle = 0;
    }
    ST.turn(-turnThrottle);
    ST.drive(driveThrottle);
  }
} // END loop()
