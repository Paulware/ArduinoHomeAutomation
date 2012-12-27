#include <EtherCard.h>
#include <MemoryFree.h>
#include <EEPROM.h>
#include "ProgmemStrings.h"
#include "EEPROMStructs.h"
#define SERVERIP "192.168.0.50"

static int const MAXBUFFER = 900;

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 
  0x74,0x69,0x69,0x2D,0x30,0x31 };
static byte myip[4];
byte Ethernet::buffer[MAXBUFFER];
BufferFiller bfill;
int numDevices = 0;
ProgmemStrings progStrings(6);
EEPROMStructs structures = EEPROMStructs();

struct DeviceType
{
  unsigned long lastPing;  // last millis() that device sent a ping
  char * name;             // Unique text name of device
};
DeviceType devices[25];

void SetupByteArray (byte * myip )
{
  int len = strlen (SERVERIP);
  int count = 0;
  char serverIp[] = SERVERIP;
  char addr[4];
  int index = 0;

  for (int i=0; i<len; i++)
  {
    if (serverIp[i] == '.')
    {
      addr[count] = 0;
      myip[index++] = (byte) atoi (addr);
      count = 0;
    }  
    else
      addr[count++] = serverIp[i];      
  }

  addr[count] = 0;
  myip[index++] = (byte) atoi (addr);

  ProgmemStrings::printPSTR ( PSTR ( "myip:" ));
  for (int i=0; i<4; i++)
  {
    if (i==0)
      Serial.print ( "{");
    else  
      Serial.print ( ",");
    Serial.print ( myip [i]);    
  }
  Serial.println ("}");     
}

void setup () {
  int which;
  Serial.begin (115200);
  ProgmemStrings::printPSTR ( PSTR ( "rbbb_server_progstrings" ) ); 
  SetupByteArray (myip);
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    ProgmemStrings::printPSTR ( PSTR ( "Failed to access Ethernet controller"));
  ether.staticSetup(myip);
  
  progStrings.addString ( PSTR ( "discover" ));
  progStrings.addString ( PSTR ( "show" ));
  progStrings.addString ( PSTR ( "page" ));
  progStrings.addString ( PSTR ( "ping" ));
  progStrings.addString ( PSTR ( "write" ));
  progStrings.addString ( PSTR ( "read" ));
  progStrings.showAll();
  ProgmemStrings::printPSTR ( PSTR ( "Free Memory: " ) );
  Serial.println ( freeMemory() );  
}

// str1 is the actual string
bool strEqual ( char * str1, char * str2 )
{
  bool match = true;
  int len = strlen ( str1 );

  if (strlen(str2) < len) 
    match = false;
  else 
    for (int i=0; i<len; i++)
    {
      if (str1[i] != str2[i])
      {
        match = false;
        break;
      }
    }

  return match;
}

int findDevice (char * name)
{
  int found = -1;
  for (int i=0; i<numDevices; i++)
  {
    if (strEqual (devices[i].name, name) )
    {
      found = i;
      break;
    }  
  }  
  return found;
}

char * findParameterStart ()
{
  char ch;
  int index = 0;
  int ind = 0;
  int state = 0;
  byte * startParam;

  while (state != 2)
  {
    ch = ether.buffer[index++];
    if (ch == ' ') 
      state = 1;
    else if ((state == 1) && (ch == '/'))
      state = 2;
    else 
      state = 0;
  }  
  startParam = &ether.buffer[index];
  return (char *)startParam;
}


void findCommand ( char * &line, int & cmd )
{
  int len;
  cmd = progStrings.findString ( line );
  if (cmd > -1)
  {
    len = progStrings.stringLen (cmd );
    line = line + len;
  }
}

char * copyParam ( char * parameter )
{
  int len = 0;
  int offset = 0;
  char ch;
  char * param;
  int state = 0;
  // find out how long is the parameter 
  while (true)
  {
    ch = parameter[len];
    if (ch == ' ') 
      break;
    else 
    {
      Serial.print ( ch ); 
      Serial.print ( " = " );
      Serial.println ( (int) ch );
    }  
    switch (state)
    {
      case 0:
        if (ch == '%')
          state = 1;
      break;
      case 1:
        if (ch == '2')
          state = 2;
        else
          state = 0;
      break;
      case 2:
        if (ch == '0')
          offset -= 2;          
        state = 0;
      break;
    } 
    len++;
  }
  param = (char *) calloc (len+1+offset,1); // allocate the string
  len = 0;
  offset = 0;
  // Copy the parameter
  while (true)
  {
    ch = parameter [len];
    if (ch == ' ')
      break;
    if ((parameter[len] == '%') && (parameter[len+1]=='2') && (parameter[len+2]=='0'))
    {
      ch= ' ';
      len = len + 2;
      offset = offset - 2;
    }  
    param[len+offset] = ch;
    len++;
  }
  ProgmemStrings::printPSTR ( PSTR ( "param: " ));
  Serial.print ( param );
  Serial.println ( "." );
  
  return param;  
}

void loop () {
  word len = ether.packetReceive();
  char * parameter;
  word pos = ether.packetLoop(len);
  char ch;
  bool debugIt = false;
  int pageNumber = -1;
  int command; 
  char * paramCopy;

  if (pos)  // check if valid tcp data is received
  {
    parameter = findParameterStart ();
    findCommand (parameter, command);
    paramCopy = copyParam ( parameter );
    ProgmemStrings::printPSTR ( PSTR ( "paramCopy len:" ));
    Serial.print ( strlen ( paramCopy ) );
    Serial.print ( ":" );
    Serial.println ( paramCopy );
    ProgmemStrings::printPSTR ( PSTR ( "paramCopy: " )); 
    Serial.println ( (int) paramCopy );
    
    ether.httpServerReply(serverPage (command, paramCopy)); // send web page data
    delete (paramCopy);
  }
}

// Do not consume the terminating 0
boolean readDecimal (char * &charPointer, int &total)
{
  char ch;
  boolean ok = false;
  total = 0;
  
  while (true)
  {
    ch = *charPointer;
    // Exit if number if not a decimal
    if ((ch < '0') || (ch > '9'))
      break;
    else
    {
      ok = true;
      charPointer++;
      total *= 10;
      total += ch - '0';
    }    
  }    
  return ok;
}

void handleShow ()
{
  long t = millis() / 1000;
  word h = t / 3600;
  byte m = (t / 60) % 60;
  byte s = t % 60;
  float secondsAgo;
  
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "<title>Show Devices</title>" 
    "<h1>Connected devices:<br>"
    "<table border=\"2px solid\">"
    "<tr><th>Device</th><th>Last Ping</th></tr>"));

  for (int i=0; i<numDevices; i++)
  {
    Serial.print ( i );
    Serial.print ( "):" );
    Serial.println ( devices[i].name );
    secondsAgo = millis() - devices[i].lastPing;
    secondsAgo /= 1000.0; 
    m = secondsAgo / 60.0;
    if (m > 0) 
      bfill.emit_p (PSTR ( "<tr><td>$S</td><td>$D minutes $D seconds ago</td></tr>"),devices[i].name, m, (int) secondsAgo);
    else  
      bfill.emit_p (PSTR ( "<tr><td>$S</td><td>$D seconds ago</td></tr>"),devices[i].name, (int) secondsAgo);
  }  
  bfill.emit_p (PSTR (
    "</table>"
    " $D$D:$D$D:$D$D</h1><br>buf[$D] of $D<br>"),
    h/10, h%10, m/10, m%10, s/10, s%10, (int) bfill.ptr, MAXBUFFER);
    
  bfill.emit_p (PSTR ( " freeMemory(): $D<br>"), freeMemory());
}

void handleWrite (char * parameter)
{
  int len;
  int device;
  int port;
  int value;

  device = findDevice (parameter); 
  len = strlen (devices[device].name);    
  if (device > -1) 
  {
    parameter += len;
      
    while (readDecimal (++parameter, port ) && readDecimal (++parameter, value))
      structures.updateValue ( device, port, value );
      
    bfill.emit_p(PSTR(
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<title>ping RBBB server page </title>"
      "<h1>write detected $S device found<br>"), parameter);
  }
  else
  {
    bfill.emit_p(PSTR(
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<title>ping RBBB server page </title>"
      "<h1>$S<br>"), "Write detected, device not found");
  }
}

void handleRead ( char * parameter )
{
  int len;
  int device;
  int port;
  int value;
  
  device = findDevice (parameter); 
  if (device > -1) 
  {
    len = strlen (devices[device].name);   
    parameter += len;
    ProgmemStrings::printPSTR ( PSTR ( "parameter in read: " ));
    Serial.println ( parameter );
    value = -1; 
    if (readDecimal (++parameter, port ))
    {
      value = structures.readValue ( device, port );
      bfill.emit_p(PSTR(
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Pragma: no-cache\r\n"
        "\r\n"
        "<h1>read detected for $S port: $D value: $D<br>"), 
         devices[device].name, port, value);
    }  
    else
    {
      ProgmemStrings::printPSTR ( PSTR ( "No port provided, read all values" ));  
      bfill.emit_p(PSTR(
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Pragma: no-cache\r\n"
        "\r\n"));
      
      for (int i=0; i<MAXARDUINOPORTS; i++)
      {
        value = structures.readValue ( device, i);
        if (!i)
          bfill.emit_p (PSTR ( 
          "$D"),
          value );
        else
          bfill.emit_p (PSTR ( 
          ":$D"),
          value );
        
      }
    }               
  }
  else
    bfill.emit_p(PSTR(
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<title>ping RBBB server page </title>"
      "<h1>$S<br>"), "read detected, device not found");
}

static word serverPage(int command, char * parameter) {
  int len;
  int device;
  char discovered[3] = "";
  int port;
  int value;
    
  bfill = ether.tcpOffset();
  
  ProgmemStrings::printPSTR ( PSTR (  "Command: " ) ) ;
  progStrings.show ( command, command+1 );

  switch ( command )
  {
  case 0: // discover
    len = strlen (parameter); 
    ProgmemStrings::printPSTR ( PSTR (  "Device detected:") );
    Serial.print (parameter);
    Serial.println();
    device = findDevice (parameter); 
    if (device > -1)
      devices[device].lastPing = millis();
    else
    { 
      devices[numDevices].lastPing = millis();
      devices[numDevices].name = (char *) calloc (len+1,1);
      strcpy (devices[numDevices].name, parameter);
      structures.clearDevice (numDevices++);
    }  

    if (device > -1)
      strcpy ( discovered, "re");
    
    bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<title border=\"2px solid\">Device Discovery</title>" 
      "<h1>A device was $S discovered: $S</h1><br>"),
    discovered, parameter); 
    break;

  case 1: // show
    handleShow();
    break;
  
  case 2: // page
    bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      //"<meta http-equiv='refresh' content='1'/>"
    "<title>RBBB server page </title>"
      "<h1>$S<br>"), parameter);
    break;
    
  case 3: // ping
    device = findDevice (parameter); 
    if (device > -1) 
    {
      bfill.emit_p(PSTR(
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Pragma: no-cache\r\n"
        "\r\n"
        "<title>ping RBBB server page </title>"
        "<h1>$S<br>"), "Ping detected, device found");
      devices[device].lastPing = millis();
    }        
    else
      bfill.emit_p(PSTR(
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Pragma: no-cache\r\n"
        "\r\n"
        "<title>ping RBBB server page </title>"
        "<h1>$S<br>"), "Ping detected, device not found");
    
    break;  

  case 4: // write to a port on the device
    handleWrite (parameter);
    break;
    
  case 5: // read from a port on the device
    handleRead ( parameter );
    break;

  default: // atoi will return 0 if no page detected
    bfill.emit_p (PSTR ( 
    "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "<html><head>\r\n"
      "<title>Unknown page $D</title>"
      "<body>"
      "Oops Unknown page: $D requested sorry\r\n"
      "</body></html>"), parameter);
    break;
  }  

  return bfill.position();
}


