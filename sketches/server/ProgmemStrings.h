#ifndef PROGMEMSTRINGS_H
#define PROGMEMSTRINGS_H

#include <Arduino.h>
struct CommandStringType
{
  uint8_t index;
  uint8_t len;
  prog_char * ptr;
  uint8_t matched;
};

class ProgmemStrings
{
  public:          
    // Constructor
    ProgmemStrings(int _numberOfStrings);
    int matchString ( char ch, boolean doDebug); 
    void printString ( int which );
    void addString (const prog_char * s);
    void show ( int startValue, int stopValue ); 
    void showAll ();
    int matchCharPointer ( char * ch, char * &newPointer); 
    int numberOfStrings;
    char charAt ( int i, int j);
    int numStrings;
    void showMatches();
    void clearMatches();
    void clearIndexes();
    int findString(char *);
    int stringLen ( int which );
    
    boolean checkMatch (int which);
    static void printPSTR ( PGM_P s);  

  private:  
    CommandStringType * strings;
};
#endif


