 /*
 *  ESP8266_PIR Version 2
 *  Version 2 includes DeepSleep
 *  
 *  Motion Sensor using Adafruit HUZZAH ESP8266 and PIR Sensor
 *  Sends events to IFTTT for EMAIL or SMS script
 *  Project Documentation & files at 
 *  https://github.com/rgrokett/ESP8266_PIRv2
 *  
 *  IFTTT requires HTTPS SSL. You can elect to verify their cert or just accept it by setting verifyCert variable.
 *  Default is "false" for initial use.
 *  If needed, update IFTTT SHA1 Fingerprint used for cert verification:
 *  $ openssl s_client -servername maker.ifttt.com -connect maker.ifttt.com:443 | openssl x509 -fingerprint -noout
 *  (Replace colons with spaces in result)
 *  
 *  Create a IFTTT Recipe using the Maker Channel to trigger a Send Email or Send SMS Action.
 *  
 *  HTTPSRedirect is available from GitHub at:
 *  https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect
 *  
 *  The ESP8266WiFi library should include WiFiClientSecure already in it
 *  
 *  This version also contains deep sleep power saving features, turning off ESP between detects
 *  It will NOT wake unless PIR detects motion.
 *  Note Use LiPO 5v or 4 AA batteries (4-6v) for power
 *
 *  version 2.0 2017-01-25 R.Grokett
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "HTTPSRedirect.h"

extern "C" {
  #include "gpio.h"
}

extern "C" {
#include "user_interface.h" 
}

const char* ssid     = "{YOUR_WIFI_SSID}";  // Your WiFi SSID
const char* password = "{YOUR_WIFI_PWD}";   // Your WiFi Password
const char* api_key  = "{YOUR_IFTTT_API_KEY}"; // Your API KEY from https://ifttt.com/maker

// Uncomment & update if you want to use a Static IP 
// (Static IP connection is much faster than DHCP)
//#define IP
//IPAddress ip(192, 168, 1, 6);
//IPAddress gateway(192, 168, 1, 254);
//IPAddress subnet(255, 255, 255, 0);

const char* host     = "maker.ifttt.com";
const int port       = 443;
const char* event    = "pirtrigger";             // Your IFTTT Event Name

const char* SHA1Fingerprint="A9 81 E1 35 B3 7F 81 B9 87 9D 11 DD 48 55 43 2C 8F C3 EC 87";  // See SHA1 comment above 
bool verifyCert = false; // set to true if you want SSL certificate validation.

int LED = 0;          // GPIO 0 (built-in LED)
int MOTION_DELAY = 15; // Delay in seconds between events to keep from flooding IFTTT & emails


//////////////////////////////////
// CONNECT TO WIFI AND SEND MESSAGE
void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare onboard LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Set Static IP, if defined above
  #if defined(IP)
      WiFi.config(ip, gateway, subnet);
  #endif

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Blink onboard LED to signify its connected
  blink();
  blink();
  blink();
  blink(); 

  // Send a Message to IFTTT
  sendEvent(); 

  // Sleep until PIR motion detection
  // See diagram for PIR wiring
  ESP.deepSleep(0);
  
}


// SEND HTTPS GET TO IFTTT
void sendEvent()
{
    Serial.print("connecting to ");
    Serial.println(host);
  
    // Use WiFiClient class to create TCP connections
    // IFTTT does a 302 Redirect, so we need HTTPSRedirect 
    HTTPSRedirect client(port);
    if (!client.connect(host, port)) {
      Serial.println("connection failed");
      return;
    }

	if (verifyCert) {
	  if (!client.verify(SHA1Fingerprint, host)) {
        Serial.println("certificate doesn't match. will not send message.");
        return;
	  } 
	}
  
    // We now create a URI for the request
    String url = "/trigger/"+String(event)+"/with/key/"+String(api_key);
    Serial.print("Requesting URL: https://");
    Serial.println(host+url);
  
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
    delay(10); 
  
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  
    Serial.println();
    Serial.println("closing connection");

    // Keep from flooding IFTTT and emails from quick triggers
    delay(MOTION_DELAY * 1000);
    
}

// Blink the LED 
void blink() {
  digitalWrite(LED, LOW);
  delay(100); 
  digitalWrite(LED, HIGH);
  delay(100);
}


// Empty LOOP never executes
void loop() {

}


