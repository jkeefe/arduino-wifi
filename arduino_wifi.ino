/*************************************************** 
  This is an example for the Adafruit CC3000 Wifi Breakout & Shield

  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
 
 /*

  --- --- --- --- ---
  
  Original source for this code is at:
  https://github.com/adafruit/Adafruit_CC3000_Library
  
  Modified from the example to transmit data to the data.sparkfun.com 
  data-collection service.
  
  by John Keefe
  March 2015
  http://johnkeefe.net
 
  Details on this project at
  http://johnkeefe.net/make-every-week-arduino-wifi
  
  ---- --- --- --- ---

*/
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "myNetwork"        // cannot be longer than 32 characters!
#define WLAN_PASS       "myPassword"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// Posting to the sparkfun data site.
// Public location for my data stream 
// is at https://data.sparkfun.com/streams/xR0K32a8yGC67Vvdlog1
// Get your own for free at http://data.sparkfun.com
#define WEBSITE      "data.sparkfun.com"


/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/

// temperature reading/sending variables
int sensorPin = A0;    // select the input pin for the thermistor
int sensor_reading = 0; // variable to store the value coming from the sensor
String sensor_reading_str; // will turn reading into string for sending
String sparkfun_data_page; // this will be the URL for the data transmission
char SPARKFUN_PAGE[82]; // a char type version of sparkfun_data_page

 // hard-coding the sparkfun IP address here instead of looking it up
 // It's 54.86.132.254
 // using IP2U32 function to convert ip address to unsigned 32 int
uint32_t ip = cc3000.IP2U32(54, 86, 132, 254);

// Note that you also have to provide the base url, minus the http://data.sparkfun.com part
// below in the TEMPERATURE READING SECTION

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  /* Display the IP address DNS, Gateway, etc. */  
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  
}

void loop(void)
{
  
  
  /// -=-=-=-=-=-=-= TEMPERATURE READING SECTION =-=-=--=-=-=- ///
  
  // reset data page string every time.
  // get this when you set up an input stream at data.sparkfun.com
  sparkfun_data_page = "/input/xR0K32a8yGC67Vvdlog1?private_key=secretkeyhere&temp=";
  
  ///// do the temp-reading
  //sensor_reading = 333;  // test value
  sensor_reading = analogRead(sensorPin);
  
  // print it to the monitor for debugging
  Serial.println(sensor_reading);
  
  // convert reading to a string
  sensor_reading_str = String(sensor_reading);
  
  // append to the sparkfun url
  sparkfun_data_page += sensor_reading_str;
  
  // convert the web page string to a Character Array, which the software is expeting
  sparkfun_data_page.toCharArray(SPARKFUN_PAGE, 82);
  
  
  /// -=-=-=-=-=-=-= END TEMPERATURE READING SECTION =-=-=--=-=-=- ///

  /// Transimit the data over the web

  cc3000.printIPdotsRev(ip);

  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(SPARKFUN_PAGE);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }

  Serial.println(F("-------------------------------------"));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println(F("-------------------------------------"));
  
//  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
//  /* the next time your try to connect ... */
//  Serial.println(F("\n\nDisconnecting"));
//  cc3000.disconnect();
  
 // wait 10 seconds
  delay(10000);
 
}

/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33]; 

  if (!cc3000.startSSIDscan(&index)) {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}