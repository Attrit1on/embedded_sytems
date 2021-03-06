/*  Lab 3: Introduction to Watchdog Timers
 *  
 *  File:       serial_echo_with_watchdog.ino
 *  Version:    1.01
 *  Author:     Trey Harrison (CWID: 11368768)
 *  Email:      ntharrison@crimson.ua.edu
 *  Created:    16 February, 2016
 *  Modified:   16 February, 2016
 *  
 *  This is a useful function for echoing a single hex digit received via serial
 *  to both the serial port and a 7-segment display, i.e. the input must be an
 *  integer between 0 and 15 terminated by a line feed.  The program transmits  
 *  an error statement to the serial port whenever an invalid input type is 
 *  received or there is no line feed.  Additionally, the program performs a reset
 *  after 4 seconds of inactivity or 3 consecutive invalid user inputs.  Attempting 
 *  to transmit any data via serial during a reset will cause the processor to lock
 *  up until the watchdog bites 4 seconds later.
 *  
 *  Copyright (C) 2016, Trey Harrison
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Source: <https://github.com/Attrit1on/embedded_systems>
 *  
 */

// Library for utilizing the AVR's integrated watchdog
#include <avr/wdt.h> 

void setup() {
  // Disable the watchdog
  disableWatchdog();
  // Configure all bits of PORTC as outputs for the 7-segment display
  DDRC = 0xFF;
  // Initialize the sevenSegmentDisplay to display a reset
  PORTC = sevenSegmentControl(20);
  // Serial start and initial statements
  Serial.begin(115200);
  Serial.println("\n\n\n\n\n\n\n\n\n\n\n\n\n\n---Serial Echo with Watchdog V1.00---");
  Serial.println("***Program resets after 4 seconds of inactivity***\n");
  Serial.println("Please enter an integer between 0 and 15 terminated by a line feed:");
  // Start the watchdog interrupt with a 4s timeout
  enableWatchdog();
}

// Program State variables
boolean inputComplete = 0, inputError = 0, waitingOnInputCompletion = 0;
uint8_t input = 0;
unsigned long inputStartTime;

void loop() {
  // Check if a valid input was received
  if (inputComplete&!inputError) {
    inputComplete = 0;
    echoInput();
    input = 0;
  }
  // Else determine if there was an error
  else if (((millis()-inputStartTime > 100) & waitingOnInputCompletion) | inputComplete) {
      inputErrorHandler();
  }
  else {
    // Just keep on looping
  }
}

/*  serialEvent()
 *  Arduino function that's called in between each loop() if there
 *  is data available in the serial buffer.  Programmed to accept
 *  only integers or linefeed without triggering an inputError.
 */
void serialEvent() {
  if (Serial.available()) {
    
    // Reset the watchdog timer
    wdt_reset();
    
    // Start input start time
    inputStartTime = millis();
    if (!input) {
      waitingOnInputCompletion = 1;
    }
    
    // Read first byte off the serial buffer
    int temp = Serial.read();
    
    // Check if first byte in buffer was an integer
    if (temp > 47 && temp < 58) {
      input *= 10;
      input += temp-48;
    }
    
    // Check if it was a line feed, this signifies the end of an input
    else if (temp == 10) {
      // Check if the completed input is out of range
      if (input > 15) {
        inputComplete = inputError = !(waitingOnInputCompletion = 0);
        
      } 
      else {
        inputComplete= !(waitingOnInputCompletion = 0);
      }
    }
    
    // Invalid input received
    else {
      inputError = 1;
    }
  }
}

/*  inputErrorHandler()
 *  Signals the user that there was an error in their input and clears the current
 *  data from input along with the error flag(s).  On the third consecutive error,
 *  causes the processor to trigger a reset.
 *  
 */
uint8_t inputErrorCount = 0;
void inputErrorHandler() {
  input = inputComplete = 0;
  PORTC = sevenSegmentControl(30);
  if (inputErrorCount++ < 2) {
    if (waitingOnInputCompletion) {
      Serial.println("\nERROR: No Line Feed! You have " + (String)(3-inputErrorCount) +
                   " chance(s) remaining to enter a valid input.");
      waitingOnInputCompletion = 0;
    } 
    else {
      Serial.println("ERROR: Invalid input type! You have " + (String)(3-inputErrorCount) +
                   " chance(s) remaining to enter a valid input.");
      inputError = 0;
    }
    Serial.println("Please enter an integer between 0 and 15 terminated by a line feed:");

  } else {
    Serial.println("Too many invalid inputs; resetting application...");
    delay(100);
    asm volatile ("jmp 0");
  }
}

/*  echoInput() 
 *  Echos valid inputs to both serial and the 7-segment display
 *  
 */
void echoInput() {
  inputErrorCount = 0;
  Serial.print("Input: 0x");
  Serial.println(input,HEX);
  PORTC = sevenSegmentControl(input);
}

/* sevenSegmentControl(uint8_t displayVal)
 * Returns the byte needed in a register to represent displayVal on 
 * a seven segment display in hex wired with all segments to a single 
 * port as described in the table below.  This code was executed with 
 * the display wired to PORTC of the Arduino Mega platform.
 * 
 * The segment field corresponds to the segment of the display to be
 * controlled(segment map to right of table).  The port_bit refers 
 * to the bits in a single port of the arduino to be used.  In this  
 * case, the pin field corresponds to the appropriate pins needed to 
 * correctly utilize PORTC of the Arduino Mega platform.
 * 
 * ---THIS ONLY WORKS FOR COMMON CATHODE DISPLAYS---
 * 
 *   PORTC[0:6] => Digital Pins 37:31
 *   ---------------------------
 *   |segment    port_bit   pin|
 *   |   o          7       30 |     segments
 *   |   G          6       31 |     |--A--|
 *   |   F          5       32 |     F     B
 *   |   E          4       33 |     |--G--|
 *   |   D          3       34 |     E     C
 *   |   C          2       35 |     |--D--| o
 *   |   B          1       36 |
 *   |   A          0       37 |
 *   ---------------------------
 *
 */
uint8_t sevenSegmentControl(uint8_t displayVal) {
  switch (displayVal) {
    case 0:  return 0x3F; break;
    case 1:  return 0x06; break;
    case 2:  return 0x5B; break;
    case 3:  return 0x4F; break;
    case 4:  return 0x66; break;
    case 5:  return 0x6D; break;
    case 6:  return 0x7D; break;
    case 7:  return 0x07; break;
    case 8:  return 0x7F; break;
    case 9:  return 0x6F; break;
    case 10: return 0x77; break;
    case 11: return 0x7C; break;
    case 12: return 0x39; break;
    case 13: return 0x5E; break;
    case 14: return 0x79; break;
    case 15: return 0x71; break;
    case 20: return 0x80; break;    // Display reset
    case 30: return 0x49; break;    // Display error
    default: return 0x00; break;    // Turn off
  }
}

/*  disableWatchdog() 
 *  Disables the ATmega2560 watchdog and clears relevant flags
 *  
 */
void disableWatchdog() {
  cli();          // Disable interrupts
  MCUSR  &= 0xF7; // Clear the watchdog reset flag
  WDTCSR |= 0x18; // Enable changes to WDTCSR
  WDTCSR  = 0x00; // Clear the watchdog timer control register
  sei();          // Enable interrupts
}

/*  enableWatchdog()
 *  Enables the ATmega2560 watchdog in system reset mode with a 4 second timeout
 *  
 */
void enableWatchdog() {
  cli();          // Disable Interrupts
  MCUSR  &= 0xF7; // Clear the watchdog reset flag
  WDTCSR |= 0x18; // Enable changes to the watchdog
  WDTCSR  = 0x28; // Config watchdog for 4s timeout and enable watchdog reset
  sei();          // Enable interrupts
}
