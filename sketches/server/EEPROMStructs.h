#ifndef EEPROMSTRUCTS_H
#define EEPROMSTRUCTS_H
#include <Arduino.h>

static int const MAXARDUINOPORTS = 20;
struct ArduinoDataType
{
  //Note:
  //      Analogs A0..A5 can also be used as digital inputs or digital outputs 
  //      when they are used as digitals they are referenced starting as D14..D19 
  // int is used for port values because value range = 0..1023 (0..1 digital, 0..180 servos, 0..255 pwm, 0..1023 analog inputs)
  int values[MAXARDUINOPORTS];  // D0..D13 + Analogs D14..D19, ports 0..19, size = 40 bytes            
  uint8_t days;    // ports [20] Number of days for timer (1) to wait
  uint8_t hours;   // ports [21] Number of hours for timer (2) to wait
  uint8_t seconds; // ports [22] Number of seconds for timer (3) to wait
  // startTimes is not accessible from the writeValue command 
  unsigned long startTimes[3]; // on setup client arduino will set timer and this, size = 4*3 bytes
  // Total bytes = 40 + 3 + 12 = 55
};

/*
   Since the size of the structure is 55 bytes and the UNO has 1024 bytes of EEPROM, an Arduino UNO can store 
   information for 18 clients.  An Arduino Mega has more EEPROM (8K) so you can store 
   information for more clients with an Arduino Mega.   
*/

class EEPROMStructs
{
  public:          
    // Constructor
    EEPROMStructs();
    boolean validDevicePort ( int device, int port );
    void clear ( int which );
    void updateValue ( int device, int port, int value );
    int  readValue (int device, int port );
    void clearDevice (int device);
    void writeLong ( int index, unsigned long value);
    unsigned long readLong ( int index );
    
    // Todo add functions to interface with timer
  
};
#endif


