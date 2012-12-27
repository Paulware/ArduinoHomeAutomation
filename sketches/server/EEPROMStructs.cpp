#include "EEPROMStructs.h"
#include <EEPROM.h>

// Maximum port number that can be accessed
#define MAX_STRUCT_PORT 22

void printPSTR1 ( PGM_P s) 
{
  char c;
  while ((c=pgm_read_byte(s++)) != 0)
    Serial.print (c);
} 

void EEPROMStructs::writeLong ( int index, unsigned long value)
{
  uint8_t val;
  int i = index;
  unsigned long v = value;
  for (int i=0; i<4; i++)
  {
    val = value & 0xFF;
    EEPROM.write (index++, val); // mask off lsb
    value >>= 8; // Shift 1 byte
  }
  
  if (v != readLong ( i ) ) 
    Serial.println ( "Got a mismatch" );
  else
    Serial.println ( " Values matched" );
}

unsigned long EEPROMStructs::readLong ( int index )
{
  unsigned long val = 0;
  uint8_t v;
  index = index + 3;
  for (int i=0; i<4; i++)
  {
    val *= 256;
    v = EEPROM.read ( index--);
    val += v;
  }
  Serial.print ( " read: " );
  Serial.print ( val );
  Serial.print ( " from index: " ) ;
  Serial.println ( index + 1);
  return val;
}

EEPROMStructs::EEPROMStructs () 
{
}

boolean EEPROMStructs::validDevicePort ( int device, int port )
{
  boolean ok = true;
  int maxDevices = 1024 / sizeof (ArduinoDataType);
  if ((device <0) || (device > maxDevices))
  {
    Serial.print ( "Illegal device: " );
    Serial.println ( device );
    ok = false;
  }
  else if ((port > MAX_STRUCT_PORT) || (port < 0))
  {
    Serial.print ( "Illegal port " );
    Serial.println ( port );
    ok = false;
  }
  return ok;
}

void EEPROMStructs::clearDevice (int device)
{
  int structSize = sizeof (ArduinoDataType);
  int offset = structSize * device;
  for (int i=0; i<structSize; i++)
    EEPROM.write (offset+i,0);
}

// Return the data that contains the specified device:port value
int EEPROMStructs::readValue (int device, int port )
{
  int index = sizeof (ArduinoDataType) * device;
  int val = 0;
  int which;
  unsigned long triggerTime;
  if (validDevicePort(device,port))
  {
    printPSTR1 ( PSTR ( "r:" ) );
    Serial.print (device);
    Serial.print ( ":" );
    Serial.print ( port );
    Serial.print ( ":" );
    switch (port)
    {
      case 22: // seconds
	index += 23;
	val = EEPROM.read (index); // Read the max value
        // computer triggerTimer
        triggerTime = 0;
        index += 8; // msb seconds, Skip start time for days and hours
        triggerTime = readLong (index);
        if (millis() > triggerTime)
        {
          printPSTR1 ( PSTR ( "Seconds have elapsed" ) );
          Serial.println ( );
          val = 1; // Time elapsed
        }  
        else
        {
          printPSTR1 ( PSTR ( "Seconds remaining: " ) );
          Serial.println ( (triggerTime - millis() ) / 1000 );
          val = 0;        
        }          
	    break;
	  
      case 21: // hours
	    index += 22;
	    val = EEPROM.read (index);
        // computer triggerTimer
        triggerTime = 0;
        index += 5; // Skips seconds and starttime for days
        triggerTime = readLong (index);
        if (millis() > triggerTime)
        {
          printPSTR1 ( PSTR ( "hours have elapsed" ) ) ;
          val = 1; // Time elapsed
        }  
        else
        {
          printPSTR1 ( PSTR ( "hours remaining: " ) );
          Serial.println ( (triggerTime - millis() ) / 3600000 );
          val = 0;        
        }  
	    break;
		
      case 20: // days
	    index += 21; 
	    val = EEPROM.read (index);
        // computer triggerTimer
        triggerTime = 0;
        index += 2; // Skips hours and seconds
        triggerTime = readLong (index);
        if (millis() > triggerTime)
        {
          printPSTR1 ( PSTR ( "days have elapsed" ) );          
          val = 1; // Time elapsed
        }  
        else
        {
          printPSTR1 ( PSTR ( "days remaining: " ) );
          Serial.println ( (triggerTime - millis() ) / 86400000 );
          val = 0;        
        }  
        break;		
		
      // ports 0..19	
      default:	
        index += (port *2); // get an integer rather than byte index
        // Structure in memory = [lsb,msb]
        val = EEPROM.read (index+1);
        val *= 256;
        val += EEPROM.read(index); 
        Serial.println ( val );
        break;  
    }    
  }
  return val;  
}

void EEPROMStructs::updateValue ( int device, int port, int value )
{
  int index = sizeof (ArduinoDataType) * device;
  uint8_t val;
  int which;
  unsigned long stopTime;
  
  if (validDevicePort(device,port))
  {
    Serial.print ( "w:" );
    Serial.print ( device );
    Serial.print ( ":" );
    Serial.print ( port );
    Serial.print ( ":" );
    Serial.println ( value );
    switch (port)
    {
      case 22: // seconds timer
        val = value;
        // Write the timeout maximum
	    index += 23;
	    EEPROM.write (index,val);

        // Write the current time
        stopTime = val; // Seconds to wait
        stopTime *= 1000; // convert to milliseconds
        stopTime += millis(); // add to current Time
        index += 8; // Skip startTime for days and hours
        writeLong ( index, stopTime );
	    break;
		
      case 21: // hours
        val = value;
	    index += 22;
	    EEPROM.write (index,val);
        // Write the current time
        stopTime = val; // hours to wait
        stopTime *= 3600000; // convert to milliseconds
        stopTime += millis(); // add to current Time
        index += 5; // Skip seconds and startTime for days
        writeLong ( index, stopTime);
	    break;
		
      case 20: //days
        val = value;
	    index += 21; 
	    EEPROM.write (index,val);
        // Write the current time
        stopTime = val * 3600000 * 24; // convert to milliseconds
        stopTime += millis(); // add to current Time
        printPSTR1 ( PSTR ( "millis() : " ) ) ;
        Serial.print ( millis() );
        printPSTR1 ( PSTR ( " stop at: " ) ) ;
        Serial.println ( stopTime );
        index += 2; // Skip seconds and hours
        Serial.print ( "index: " );
        Serial.println ( index );
        writeLong ( index, stopTime );
    	break;

      // port 0..19	
      default:
        index += (port*2);
        val = value & 0xFF;
        EEPROM.write ( index, val);
        val = (value & 0xFF00) >> 8;
        EEPROM.write (index+1,val);
		break;
    }
  }
}

void EEPROMStructs::clear ( int which ) 
{
  int index = sizeof ( EEPROMStructs ) * which;
  for (int i=0; i<sizeof ( EEPROMStructs ); i++)
    EEPROM.write ( index++, 0);
}
