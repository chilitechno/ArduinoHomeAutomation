#include <SoftwareSerial.h>
#include <SerialLCD.h>
#include <SPI.h>
#include <Ethernet.h>

SerialLCD slcd(6,7);//this is a must, assign soft serial pins
EthernetClient client;

const int pumpPin =  3; // digital pin 3,4
const String remoteWebHost = "";
const String urlToken = "";
const IPAddress remoteAddr(0,0,0,0);
const String deviceID = ""; //

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
// byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
byte rip[4];
int seconds = 0;
int oldSeconds = 0;
int secondsSinceLastPoll = 0;
int p = 1;
int oldp = 0;
int sensorPin1 = 0;
int sensorPin2 = 2;
int lastMoistureReading = 0;
int lastMoistureReading2 = 0;
int pollInterval = 14400; // 3600; // 14400;
int moisturePumpThreshold = 500;
int moisturePumpThreshold2 = 500;
boolean pumpRelayClosed = false;
int pumpRelayClosedSeconds = 0;
int pumpSecondsInterval = 7;
int XTotalCount = 0;
boolean intializeSuccess = false;
boolean updateSuccess = false;
int currentReading = 0;
String localAddress = "";
String requestUrl = "";

char buf[6];

IPAddress ip(192,168,1, 11);
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

void sendUpdateToServer(String url, boolean initialize) {
  if (initialize == 1) {
    localAddress = "";
    // localAddress = String(getIPAddressFromStruct(Ethernet.localIP()));    
    // delay(10000);
  }
  Serial.println(url);
  requestUrl = "GET " + url + "&U=" + urlToken + "&I=" + localAddress + " HTTP/1.1";
  Serial.println(requestUrl);
  Serial.println("*BEGIN*");
  Serial.println(requestUrl);
  Serial.println("Host: " + remoteWebHost);

  if (initialize) {  
    slcd.setCursor(8,0);
  } 
  else {
    slcd.setCursor(10,0); 
  }
  slcd.print("TX"); 

  if (client.connect(remoteAddr, 80)) {
    client.println(requestUrl);
    client.println("Host: " + remoteWebHost);
    client.println();    
    client.stop();
    Serial.println("Connection Success - Data Transmitted");    
    if (initialize) {
      intializeSuccess = true;     
    } 
    else {
      updateSuccess = true;
    }
  } 
  else {
    Serial.println("Connection Failed");
    if (initialize) {
      intializeSuccess = false;     
    } 
    else {
      updateSuccess = false;
    }
  }
  delay(1000);
  Serial.println("*END*");
  if (initialize) {  
    slcd.setCursor(8,0);
    if (intializeSuccess) {
      slcd.print("OK"); 
    } 
    else {
      slcd.print("ER"); 
    }       
  } 
  else {
    slcd.setCursor(10,0); 
    if (updateSuccess) {
      slcd.print("OK"); 
    } 
    else {
      slcd.print("ER"); 
    }   

  }
}

void setupEthernetConnection() {
  if (!Ethernet.begin(mac)) {
    Serial.println("DHCP failed - using manual IP");
    // if DHCP fails, start with a hard-coded address:
    Ethernet.begin(mac, ip, gw, subnet);
  } 
  else {
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
  lastMoistureReading = analogRead(sensorPin1);
  if (sensorPin2 >= 0) {
    lastMoistureReading2 = analogRead(sensorPin2);
  }
  if (sensorPin2 >= 0) {
    currentReading =  ((lastMoistureReading*2)+(lastMoistureReading2)) / 3;
    sendUpdateToServer("/i/?ID=" + deviceID + "&p=0&m1_n=" + String(lastMoistureReading) + "&m2_n=" + String(lastMoistureReading2) + "&man=" + String(currentReading),true);  
  } 
  else {
    sendUpdateToServer("/i/?ID=" + deviceID + "&p=0&m1_n=" + String(lastMoistureReading),true);
  }
}

void loop()
{

  seconds = millis() / 1000;
  int secdiff = seconds - oldSeconds;
  secondsSinceLastPoll += secdiff;  

  // update every 5 minutes the current sensor measurements
  if ((secdiff > 0) && ((secondsSinceLastPoll % 300) == 0)) {

    lastMoistureReading = analogRead(sensorPin1);    
    slcd.setCursor(0, 1); 
    itoa(lastMoistureReading,buf,10);
    slcd.print(buf);

    currentReading = lastMoistureReading;

    if (sensorPin2 >= 0) {
      lastMoistureReading2 = analogRead(sensorPin2);
      currentReading =  ((lastMoistureReading*2)+(lastMoistureReading2)) / 3;

      slcd.setCursor(3, 1); 
      slcd.print("/");
      slcd.setCursor(4, 1); 
      itoa(lastMoistureReading2,buf,10);
      slcd.print(buf);       
      slcd.setCursor(7, 1); 
      slcd.print("/");
      slcd.setCursor(8, 1); 
      itoa(currentReading,buf,10);
      slcd.print(buf);       
    }    
  }

  if ((secdiff > 0) && ((secondsSinceLastPoll % pollInterval) == 0)) {
    p += 1;
    secondsSinceLastPoll = 0;
    slcd.clear();
    slcd.setCursor(0, 0);
    slcd.print(">");

    itoa(p,buf,10);
    slcd.setCursor((16-(strlen(buf))),0);  
    slcd.print(buf);    

    slcd.setCursor(0, 1); 
    itoa(lastMoistureReading,buf,10);
    slcd.print(buf);


    if (sensorPin2 >= 0) {
      slcd.setCursor(3, 1); 
      slcd.print("/");
      slcd.setCursor(4, 1); 
      itoa(lastMoistureReading2,buf,10);
      slcd.print(buf);       
      slcd.setCursor(7, 1); 
      slcd.print("/");
      slcd.setCursor(8, 1); 
      itoa(currentReading,buf,10);
      slcd.print(buf);       
    }

    itoa(XTotalCount,buf,10);
    slcd.setCursor((16-(strlen(buf))),1);  
    slcd.print(buf);    

    slcd.setCursor(8,0);
    if (intializeSuccess) {
      slcd.print("OK"); 
    } 
    else {
      slcd.print("ER"); 
    }

    slcd.setCursor(10,0);
    if (updateSuccess) {
      slcd.print("OK"); 
    } 
    else {
      slcd.print("ER"); 
    }          
  }

  if (p != oldp) {
    slcd.setCursor(8,0);
    if (intializeSuccess) {
      slcd.print("OK"); 
    } 
    else {
      slcd.print("ER"); 
    }
    slcd.setCursor(10,0);
    if (updateSuccess) {
      slcd.print("OK"); 
    } 
    else {
      slcd.print("ER"); 
    }      

    oldp = p;

    itoa(p,buf,10);
    slcd.setCursor((16-(strlen(buf))),0);  
    slcd.print(buf);    

    // poll the sensor
    lastMoistureReading = analogRead(sensorPin1);    
    if (sensorPin2 >= 0) {
      lastMoistureReading2 = analogRead(sensorPin2);
    }
    slcd.setCursor(0, 1); 
    itoa(lastMoistureReading,buf,10);
    slcd.print(buf);

    boolean moistureThresholdReaching = (lastMoistureReading <= moisturePumpThreshold);

    currentReading = lastMoistureReading;
    if (sensorPin2 >= 0) {
      currentReading =  ((lastMoistureReading*2)+(lastMoistureReading2)) / 3;
      moistureThresholdReaching = ((lastMoistureReading <= moisturePumpThreshold) && (lastMoistureReading <= moisturePumpThreshold2));
    }

    if (sensorPin2 >= 0) {
      slcd.setCursor(3, 1); 
      slcd.print("/");
      slcd.setCursor(4, 1); 
      itoa(lastMoistureReading2,buf,10);
      slcd.print(buf);       
      slcd.setCursor(7, 1); 
      slcd.print("/");
      slcd.setCursor(8, 1); 
      itoa(currentReading,buf,10);
      slcd.print(buf);       
    }

    if ((p > 1) && (pumpRelayClosed) && (moistureThresholdReaching)) {
      Serial.println("Pumping Water");
      // turn on relay for a few seconds to pump water
      pumpRelayClosed = true;
      pumpRelayClosedSeconds = 0;
      digitalWrite(pumpPin, HIGH);  
      XTotalCount++; 

      itoa(XTotalCount,buf,10);
      slcd.setCursor((16-(strlen(buf))),1);  
      slcd.print(buf);      

      slcd.setCursor((16-(strlen(buf)))-1,1);  
      slcd.print("*");        
    } 
    else {
      if (sensorPin2 > 0) {
        sendUpdateToServer("/u/?ID=" + deviceID + "&p=" + String(p) + "&m1_n=" + String(lastMoistureReading) + "&m2_n=" + String(lastMoistureReading2) + "&man=" + String(currentReading), false);              
      } 
      else {
        sendUpdateToServer("/u/?ID=" + deviceID + "&p=" + String(p) + "&m1_n=" + String(lastMoistureReading), false);      
      }
    }
  }

  if (pumpRelayClosed) {
    pumpRelayClosedSeconds += secdiff;    
  }

  if ((pumpRelayClosed) && (pumpRelayClosedSeconds >= pumpSecondsInterval)) {
    // turn the pump off
    pumpRelayClosed = false;    
    digitalWrite(pumpPin, LOW);         

    itoa(XTotalCount,buf,10);
    slcd.setCursor((16-(strlen(buf))),1);  
    slcd.print(buf);      

    slcd.setCursor((16-(strlen(buf)))-1,1);  
    slcd.print(" ");

    int oldMoistureReading = lastMoistureReading;
    int oldMoistureReading2 = lastMoistureReading2;
    // poll the sensor
    lastMoistureReading = analogRead(sensorPin1);    
    if (sensorPin2 >= 0) {
      lastMoistureReading2 = analogRead(sensorPin2);
    }

    slcd.setCursor(0, 1); 
    itoa(lastMoistureReading,buf,10);
    slcd.print(buf);

    if (sensorPin2 >= 0) {
      slcd.setCursor(3, 1); 
      slcd.print("/");
      slcd.setCursor(4, 1); 
      itoa(lastMoistureReading2,buf,10);
      slcd.print(buf);     
    }
    if (sensorPin2 >= 0) {
      int oldReading  =  ((oldMoistureReading*2)+(oldMoistureReading2)) / 3;
      currentReading =  ((lastMoistureReading*2)+(lastMoistureReading2)) / 3;
      sendUpdateToServer("/u/?ID=" + deviceID + "&p=" + String(p) + "&m1_o=" + String(oldMoistureReading) + "&m1_n=" + String(lastMoistureReading) + "&m2_o=" + String(oldMoistureReading2) + "&m2_n=" + String(lastMoistureReading2) + "&mao=" + String(oldReading) + "&man=" + String(currentReading) + "&X=1", false);      
    } 
    else {
      sendUpdateToServer("/u/?ID=" + deviceID + "&p=" + String(p) + "&m1_o=" + String(oldMoistureReading) + "&m1_n=" + String(lastMoistureReading) + "&X=1", false);
    }

  }

  oldSeconds = seconds;

  if (secdiff > 0) {
    int remainingSeconds = pollInterval - secondsSinceLastPoll;
    slcd.setCursor(1,0);  
    itoa(secondsSinceLastPoll,buf,10);
    slcd.print(buf);    
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
      t[0] = c; 
      t[1] = '\0';
      strcat(dest, t);
    } 
    else {
      char t[4];
      snprintf(t, sizeof(t), "%%%02x", c & 0xff);
      strcat(dest, t);
    }
  }
}


