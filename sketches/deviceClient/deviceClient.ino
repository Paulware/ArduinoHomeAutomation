#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2E,0x30,0x31 };
// static byte myip[4];
static byte myip[] = { 192,168,0,188 };
// gateway ip address
static byte gwip[] = { 192,168,0,1 };

byte Ethernet::buffer[700];
unsigned long timeout = 0;

#define MYIP "192.168.0.60"
// Location of web-server
char website[] PROGMEM = "192.168.0.50";

char * findString ( char * source, char * target ) 
{
  char * found = 0;
  int len = strlen ( source );
  char ch;
  char ch1;
  char count=0;
  
  for (int i=0; i<len; i++)
  {
    ch = source[i];
    ch1 = target[count];
    found = &source[i];
    if (ch == ch1)
    {
      count++;
      if (count == strlen(target)) // match
        break;
    }
    else
      count = 0;

  }
  if (count != strlen ( target )) // no match
    found = 0;
    
  return found;
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

// called when the client request is complete
static void my_callback (byte status, word off, word len) 
{
  char ch = ' ';
  int count = 0;  
  int i = 0;
  int state = 0;
  
  while (state != 4) 
  {
    ch = Ethernet::buffer[i++];    
    switch (state)
    {
      case 0:
        if (ch == 13)
          state = 1;
        else
          state = 0;
      break;
      case 1:
        if (ch == 10)
          state = 2;
        else
          state = 0;
      break;
      case 2:
        if (ch == 13)
          state = 3;
        else
          state = 0;
      break;
      case 3:
        if (ch == 10)
          state = 4;
        else
          state = 0;
      break;
    }
  }
  
  Ethernet::buffer[off+300] = 0;
  while (ch)
  {
    ch = Ethernet::buffer[i++];
    Serial.print ( ch );
  }
  Serial.println ();
  //Serial.print((const char*) Ethernet::buffer + off+30);
  //Serial.println("...");
  /*
  char * found;
  int which;
  char ch;
  static int last[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  static int d;
  int offset = 0;
  
  // Serial.print ( "." );
  
  Ethernet::buffer[off+300] = 0;
  Serial.print ( "off: " );
  Serial.print ( off );
  Serial.print ( "len: " );
  Serial.println ( len );
  Serial.println ( Ethernet::buffer[off+62] );
  
  /*
    NOTE: Server puts out a page that has this data embedded:
    parent.sId "dX","checked","false"
    
    Where X is the number of the discrete that is being read
  */  

  //for (int i=0; i<20; i++)
  //{
     /* 
       Note: 62 is to skip the 
       "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"
       which is output be the server on each response
     */
     //ch = Ethernet::buffer[off+62+offset];    
     /* 
     offset++;
     d = 0;
     while ((ch <= '9') && (ch >= '0'))
     {
       d *= 10;
       d += ch - '0';
       ch = Ethernet::buffer[off+62+offset];
       offset++;
     }
     */
     // Serial.print ( Ethernet::buffer[off+62+offset]);
     /*
     if (d != last[i])
     {
       Serial.print ("D");
       Serial.print (i);
       Serial.print ("=");
       Serial.println ( d );
       last[i] = d;
     }  
     */
  //}
  
}
/*
void SetupByteArray (byte * myip )
{
  int len = strlen (MYIP);
  int count = 0;
  char ipAddress[] = MYIP;
  char addr[4];
  int index = 0;

  for (int i=0; i<len; i++)
  {
    if (ipAddress[i] == '.')
    {
      addr[count] = 0;
      myip[index++] = (byte) atoi (addr);
      count = 0;
    }  
    else
      addr[count++] = ipAddress[i];      
  }

  addr[count] = 0;
  myip[index++] = (byte) atoi (addr);

  Serial.print ( "myip:" );
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
*/

void setup () {
  Serial.begin(115200);  
  Serial.println("\n[webClient]");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  else
    Serial.println("access Ethernet controller ok" );  
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip); 
  
  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);

  timeout = millis() + 2000; 
}

void loop ()
{
  ether.packetLoop(ether.packetReceive());
  if (millis() > timeout)
  {
      ether.browseUrl(PSTR("/"), "readKingTiger1", website , my_callback);      
      timeout = millis() + 2000; // 1 time a second
  }
}
