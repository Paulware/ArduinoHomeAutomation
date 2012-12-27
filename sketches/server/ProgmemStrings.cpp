#include "ProgmemStrings.h"
#include <EEPROM.h>

// Print a string from program memory
// Example: ProgmemStrings::printPSTR ( PSTR ( "Hello World" ) );
void ProgmemStrings::printPSTR ( PGM_P s)
{
  char c;
  while ((c=pgm_read_byte(s++)) != 0)
    Serial.print (c);
} 

// s = pointer to PSTR string
void ProgmemStrings::addString ( const prog_char * s )
{ 
  CommandStringType * str;
  prog_char * ptr;
  int index;
  
  if (numStrings == 0) // This is the first one
  {
    strings = (CommandStringType *) malloc ( sizeof (CommandStringType) * numberOfStrings);
  }
  
  if (numStrings < numberOfStrings) // There is memory allocated for the string
  { 
    str = &strings [numStrings++];  
  
    // str->len = progLen (str->ptr);
    str->index = 0;
    str->len = 0;
    str->matched = 0;
    str->ptr = (prog_char *)s;
    ptr = (prog_char *)s;
    while (pgm_read_byte (ptr++))
      str->len = str->len + 1;
  }  
}

ProgmemStrings::ProgmemStrings(int _numberOfStrings )
{
  strings = 0;
  numStrings = 0;
  numberOfStrings = _numberOfStrings;
}

char ProgmemStrings::charAt ( int i, int j)
{
  CommandStringType * str = &strings[i];
  int len = str->len;
  char ch = 0;
  char * ptr; 
    
  if ((i < numStrings) && (i > -1) && (j < len) && (j>-1)) 
  {
    str = &strings [i];  
    ptr = str->ptr + j;      
    ch = pgm_read_byte (ptr);
  } 
  return ch;
}

int ProgmemStrings::stringLen ( int which )
{
  CommandStringType * str;  
  str = &strings[which];
  return str->len;
}

int ProgmemStrings::findString(char * matchStr)
{
  int found = -1;
  int len;
  CommandStringType * str;  
  
  for (int i=0; i<numStrings; i++)
  {
    str = &strings[i];
    len = str->len;
    found = i;
    for (int j=0; j<len; j++)
    {
      if (charAt(i,j) != matchStr[j]) 
      {        
        found = -1;
        break;
      }
    }
    
    if (found > -1)
      break;
  }
  return found;
}

void ProgmemStrings::show ( int startValue, int stopValue ) 
{ 
  if (startValue < 0)
    startValue = 0;
  if (stopValue >= numStrings)
    stopValue = numStrings;
  
  for (int i=startValue; i<stopValue; i++)
  {
    Serial.print ( i );
    printPSTR ( PSTR ( ")" ) );
    printString(i);
    printPSTR ( PSTR ( "\n" ) );
  }
}

void ProgmemStrings::showAll ( )
{
  show (0, numStrings );
}

void ProgmemStrings::printString ( int which )
{
  char c;
  int len = 0;
  
  if ((which < numStrings) && (which >= 0))
  {    
    while (c=charAt(which,len++))
      Serial.print (c);
  }  
}

int ProgmemStrings::matchCharPointer ( char * ch, char * &newPointer ) 
{
  int matched = -1;
  int len;
  CommandStringType * str;  

  for (int i=0; i<numStrings; i++)  
  {
    str = &strings[i];
    len = str->len;
    for (int j=0; j<len; j++)
    {
      if (*(ch+j) != charAt (i,j))
      {      
        //Serial.print ( c );
        //printPSTR ( PSTR ( " != " ) );
        //Serial.println ( (char) *(ch+j));
        break;
      }  

      if ((j+1) == len)
      {
        newPointer = ch+j+1;
        str->matched = 1;
        matched = i;
        //printPSTR ( PSTR  ( "Got a match on word ") );
        //Serial.println ( i );
        break;
      }
    }  
    if (matched > -1)
      break;
  }
  return matched;    
}

void ProgmemStrings::clearIndexes()
{
  CommandStringType * str;
  for (int i=0; i<numStrings; i++)  
    strings[i].index = 0;
}    


// Check all strings for matches using the supplied ch.
int ProgmemStrings::matchString ( char ch, boolean doDebug ) 
{
  int matched = -1;
  char c;  
  CommandStringType * str;
  int watching = 5;
     
  for (int i=0; i<numStrings; i++)  
  {
    str = &strings [i]; 
    c = charAt (i,str->index);
    if (ch == c)
    {
      if (doDebug && (i==watching))
      {
        Serial.print ( ch );
        printPSTR ( PSTR  ( " == " ));
        Serial.println ( c );
      }  
      str->index++;
    }  
    else
    {
      if (doDebug && (i==watching))
      {
        Serial.print ( ch );
        printPSTR ( PSTR  ( " != ") );
        if ((int)c < 33) 
          Serial.println ( (int) c );
        else          
          Serial.println (c);
      }  
      str->index = 0;
    }  
    if (str->index == str->len)
    {
      matched = i;
      clearIndexes();
      str->matched = 1;
      if (doDebug)
      {
        printPSTR ( PSTR  ( "Got a match on") );
        Serial.println ( i );
      }
      break;
    }
  }
  return matched;    
}

