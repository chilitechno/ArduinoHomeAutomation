#include <SoftwareSerial.h>
#include <SerialLCD.h>
#include <SPI.h>
#include <Ethernet.h>

SerialLCD slcd(6,7);//this is a must, assign soft serial pins
EthernetClient client;

const int pumpPin =  3; 
const String remoteWebHost = "";
const String urlToken = "";
const IPAddress remoteAddr(0,0,0,0);
const String deviceID = "000001"; // 000002

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
byte rip[4];
int seconds = 0;
int oldSeconds = 0;
int secondsSinceLastPoll = 0;
int pollCount = 1;
int oldPollCount = 0;
int lastMoistureReading = 0;
int pollInterval = 3600; // 3600; // 14400;
int moisturePumpThreshold = 575;
int pumpRelayClosed = 0;
int pumpRelayClosedSeconds = 0;
int pumpSecondsInterval = 7;
int pumpedTotalCount = 0;
int intializeSuccess = 0;
int updateSuccess = 0;
String localAddress = "";

IPAddress ip(192,168,1, 10); 
IPAddress gw(192,168,1, 1);
IPAddress subnet(255,255,255,0);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):

String getIPAddressFromStruct(IPAddress ip) {
  String addr = "";
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    addr = addr + String(ip[thisByte], DEC);
    if (thisByte < 3) {
      addr = addr + ".";
    }
  }
  return addr;  
}

void sendUpdateToServer(String url, int initialize) {
  if (initialize == 1) {
    localAddress = String(getIPAddressFromStruct(Ethernet.localIP()));    
  }
  Serial.println(url);
  String requestUrl = "GET " + url + "&urlToken=" + urlToken + "&clientIP=" + localAddress + " HTTP/1.1";
  Serial.println(requestUrl);
  Serial.println("****** Begin Network Transmission *****");
  Serial.println(requestUrl);
  Serial.println("Host: " + remoteWebHost);
    
  if (client.connect(remoteAddr, 80)) {
    client.println(requestUrl);
    client.println("Host: " + remoteWebHost);
    client.println();    
    client.stop();
    Serial.println("Connection Success - Data Transmitted");    
    if (initialize == 1) {
        intializeSuccess = 1;     
    } else {
       updateSuccess = 1;
    }
  } else {
    Serial.println("Connection Failed");
    if (initialize == 1) {
       intializeSuccess = 0;     
    } else {
       updateSuccess = 0;
    }
  }
  Serial.println("****** End Network Transmission *****");
    slcd.setCursor(5,1);
    if (updateSuccess == 1) {
      slcd.print("OK"); 
    } else {
      slcd.print("XX"); 
    }    
}

void setupEthernetConnection() {
  if (!Ethernet.begin(mac)) {
    Serial.println("DHCP failed - using manual IP");
    // if DHCP fails, start with a hard-coded address:
    Ethernet.begin(mac, ip, gw, subnet);
  } else {
     Serial.println("DHCP success"); 
  }
  // wait for the ethernet connection to initialize
  delay(10000);
}

void setup()
{
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);

  Serial.begin(9600);
  Serial.println("Initializing");

  setupEthernetConnection();
  
  slcd.begin();
  // slcd.backlight();
  
  slcd.clear();

  
  slcd.setCursor(0, 0);
  slcd.print(">");

    // poll the sensor
  lastMoistureReading = analogRead(0);
  sendUpdateToServer("/initialize/?ID=" + deviceID + "&pollCount=0&moisture1_new=" + String(lastMoistureReading),1);

}

void loop()
{
  
  seconds = millis() / 1000;
  int secdiff = seconds - oldSeconds;
  secondsSinceLastPoll += secdiff;  

  if ((secdiff > 0) && ((secondsSinceLastPoll % pollInterval) == 0)) {
      pollCount += 1;
      secondsSinceLastPoll = 0;
      slcd.clear();
      slcd.setCursor(0, 0);
      slcd.print(">");
      
      char buf3[3];
      itoa(pollCount,buf3,10);
      slcd.setCursor((16-(strlen(buf3))),0);  
      slcd.print(buf3);    

      slcd.setCursor(0, 1); 
      char buf[5];  
      itoa(lastMoistureReading,buf,10);
      slcd.print(buf);
      
      char buf4[3];
      itoa(pumpedTotalCount,buf4,10);
      slcd.setCursor((16-(strlen(buf4))),1);  
      slcd.print(buf4);    
      
      slcd.setCursor(8,1);
      if (intializeSuccess == 1) {
        slcd.print("OK"); 
      } else {
        slcd.print("XX"); 
      }
      
     slcd.setCursor(5,1);
      if (updateSuccess == 1) {
        slcd.print("OK"); 
      } else {
        slcd.print("XX"); 
      }          
  }
  
  if (pollCount != oldPollCount) {
     slcd.setCursor(8,1);
      if (intializeSuccess == 1) {
        slcd.print("OK"); 
      } else {
        slcd.print("XX"); 
      }
     slcd.setCursor(5,1);
      if (updateSuccess == 1) {
        slcd.print("OK"); 
      } else {
        slcd.print("XX"); 
      }      
      
    oldPollCount = pollCount;
    
    char buf3[3];
    itoa(pollCount,buf3,10);
    slcd.setCursor((16-(strlen(buf3))),0);  
    slcd.print(buf3);    
  
    // poll the sensor
    lastMoistureReading = analogRead(0);    
    slcd.setCursor(0, 1); 
    char buf[5];  
    itoa(lastMoistureReading,buf,10);
    slcd.print(buf);

    if ((pollCount > 1) && (pumpRelayClosed == 0) && (lastMoistureReading < moisturePumpThreshold)) {
        Serial.println("Pumping Water");
        // turn on relay for a few seconds to pump water
        pumpRelayClosed = 1;
        pumpRelayClosedSeconds = 0;
        digitalWrite(pumpPin, HIGH);  
        pumpedTotalCount++; 
        
        char buf4[3];
        itoa(pumpedTotalCount,buf4,10);
        slcd.setCursor((16-(strlen(buf4))),1);  
        slcd.print(buf4);      
        
        slcd.setCursor((16-(strlen(buf4)))-1,1);  
        slcd.print("*");        
    } else {
      sendUpdateToServer("/update/?ID=" + deviceID + "&pollCount=" + String(pollCount) + "&moisture1_new=" + String(lastMoistureReading), 0);      
    }
  }
  
  if (pumpRelayClosed == 1) {
    pumpRelayClosedSeconds += secdiff;
    
  }
  
  if ((pumpRelayClosed == 1) && (pumpRelayClosedSeconds >= pumpSecondsInterval)) {
    // turn the pump off
    pumpRelayClosed = 0;    
    digitalWrite(pumpPin, LOW);         
 
    char buf4[3];
    itoa(pumpedTotalCount,buf4,10);
    slcd.setCursor((16-(strlen(buf4))),1);  
    slcd.print(buf4);      
    
    slcd.setCursor((16-(strlen(buf4)))-1,1);  
    slcd.print(" ");
        
    int oldMoistureReading = lastMoistureReading;
    // poll the sensor
    lastMoistureReading = analogRead(0);    
    slcd.setCursor(0, 1); 
    char buf[5];  
    itoa(lastMoistureReading,buf,10);
    slcd.print(buf);
    sendUpdateToServer("/update/?ID=" + deviceID + "&pollCount=" + String(pollCount) + "&moisture1_old=" + String(oldMoistureReading) + "&moisture1_new=" + String(lastMoistureReading) + "&pumped=1", 0);


  }
  
  
  
  oldSeconds = seconds;

  if (secdiff > 0) {
    char buf2[4];  
    int remainingSeconds = pollInterval - secondsSinceLastPoll;
    slcd.setCursor(1,0);  
    itoa(secondsSinceLastPoll,buf2,10);
    slcd.print(buf2);    
  }
}

// help functions
// from: https://gist.github.com/744925

void urlencode(char* dest, char* src) {
  int i;
  for(i = 0; i < strlen(src); i++) {
    char c = src[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~') {
      char t[2];
      t[0] = c; t[1] = '\0';
      strcat(dest, t);
    } else {
      char t[4];
      snprintf(t, sizeof(t), "%%%02x", c & 0xff);
      strcat(dest, t);
    }
  }
}
