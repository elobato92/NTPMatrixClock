/*
 Link Sprite LED Matrix, Chained NTP Display
 By Eric Lobato 08/17/2017
 Gets the time from a Network Time Protocol (NTP) time server
 and display it across 4 LED Matrices.
 Contains modified code originally from 
 tumaku, Michael Margolis, Tom Igoe, OIvan Grokhotkov
  
 This code is in the public domain.
 This version only displays hours and minutes
 */
#define NBR_MTX 4
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "LedControlMS.h"

char ssid[] = "**********";  //  your network SSID (name)
char pass[] = "**********";       // your network password

unsigned int localPort = 2390;      // local port to listen for UDP packets
LedControl lc=LedControl(16,12,13, NBR_MTX);
String scrollString= "00:00    ";
int stringLength=scrollString.length();
char ch0, ch1, ch2, ch3;
int nextCharIndex=0;
//
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
    //from scroll code
  for (int i=0; i< NBR_MTX; i++){
  lc.shutdown(i,false);
  /* Set the brightness to a low value */
  lc.setIntensity(i,1);
  /* and clear the display */
  lc.clearDisplay(i);}
}

void loop()
{
  lc.clearAll();
  String scrollString = GetTime();
  Serial.println(scrollString);
  for (int i =0;i<45;i++){
    lc.displayChar(3, lc.getCharArrayPosition(ch0));
    lc.displayChar(2, lc.getCharArrayPosition(ch1));
    lc.displayChar(1, lc.getCharArrayPosition(ch2));
    lc.displayChar(0, lc.getCharArrayPosition(ch3));
    ch0=ch1;
    ch1=ch2;
    ch2=ch3;
    ch3=scrollString[nextCharIndex++];
    if (nextCharIndex>=stringLength) nextCharIndex=0;
    delay(400);//controls speed of scroll
    lc.clearAll();
    delay(25);
  }
}
String GetTime(){
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    epoch = epoch - 21600; //25200(MST) for GMT -7. 21600 for GMT -8(MDT)
    unsigned long new_epoch = (epoch % 86400L / 3600);
    String current_time = String(new_epoch);
    current_time += ":";
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
      current_time += 0;
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    current_time +=String((epoch  % 3600) / 60);
    current_time += "    ";
    Serial.println("Current_time var is:" + current_time);
    return current_time;
  }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
