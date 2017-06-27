# ESP8266_PIRv2
ESP8266 Huzzah Deep Sleep Mode PIR Motion Alarm using IFTTT

Version 2 – This version changes the ESP8266 to use Deep Sleep mode to significantly save battery power. Requires extra hardware. See original version for simpler hardware (and shorter battery life).  
http://www.instructables.com/id/The-Cat-Has-Left-the-Building-ESP8266-PIR-Monitor/
https://github.com/rgrokett/ESP8266_PIR

This version is only needed if you wish to run the unit using AA batteries. 
After 4 months and hundreds of triggers, still running strong on the same set of batteries. 

We just installed a Cat Door in our garage and I wanted to see how many times per day (actually night) our cat went in and out the door. We could tell the cat was using the door as we would find it outside sometimes and inside sometimes. 
So, breaking out my parts box, I found a PIR motion sensor and ESP8266. 

I used the Adafruit HUZZAH ESP8266 as it has a regulator for powering the 3.3v ESP as well as good tutorials for initially getting it set up. I also used the Arduino IDE with the ESP8266 library, since I was already quite familiar with using it with the Huzzah ESP8266. 
I decided to interface this to IFTTT (www.ifttt.com) to trigger any number of types of events. Initially, just an email each time motion was detected. 

SEE: ESP8266_PIRv2.pdf for full details.
