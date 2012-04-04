#include <SoftwareSerial.h>
#include <SerialLCD.h>
#include <SPI.h>
#include <Ethernet.h>

SerialLCD slcd(6,7);//this is a must, assign soft serial pins
EthernetClient client;

const int pumpPin = 3;
const String remoteWebHost = "";
const String urlToken = "";
const IPAddress remoteAddr(0,0,0,0);
const String deviceID = ""; // 

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
byte rip[4];
int seconds = 0;
int oldSeconds = 0;
int secondsSinceLastPoll = 0;
int secondsSinceLastUpdate = 0;
int pollCount = 1;
int oldPollCount = 0;
int lastMoistureReading = 0;
int pollInterval = 14400;
int updateInterval = 300;
int moisturePumpThreshold = 575;
int pumpRelayClosed = 0;
int pumpRelayClosedSeconds = 0;
int pumpSecondsInterval = 7;
int pumpedTotalCount = 0;
int intializeSuccess = 0;
int updateSuccess = 0;
int oldMoistureReading = 0;
String localAddress = "";

IPAddress ip(192,168,1,50);
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

void takeMeasurements() {
    oldMoistureReading = lastMoistureReading;
    lastMoistureReading = analogRead(0);
}

void printIntToSerialLCD(int value, int x, int y) {
  char buffer[6];
  itoa(value,buffer,10);
  slcd.setCursor(x, y);
  slcd.print(buffer);  
}

void updateTransmitStatuses () {
    slcd.setCursor(8, 0);
    if (intializeSuccess == 0) {
      slcd.print("-");
    } else {
      slcd.print("+");
    }
    slcd.setCursor(9,0);
    if (updateSuccess == 0) {
      slcd.print("-");
    } else {
      slcd.print("+");
    }    
}

void sendUpdate() {
  slcd.setCursor(9, 0);
  slcd.print("*");
  Serial.println("GET /u/?U=" + urlToken + "&D=" + deviceID + "&p=" + String(pollCount) + "&m1o=" + String(oldMoistureReading) + "&m1n=" + String(lastMoistureReading) + "&X=" + String(pumpedTotalCount) + " HTTP/1.1");
  if (client.connect(remoteAddr, 80)) {
    client.println("GET /u/?U=" + urlToken + "&D=" + deviceID + "&p=" + String(pollCount) + "&m1o=" + String(oldMoistureReading) + "&m1n=" + String(lastMoistureReading) + "&X="  + String(pumpedTotalCount) + " HTTP/1.1");
    client.println("Host: " + remoteWebHost);
    client.println();
    client.stop();
    updateSuccess = 1;
  } 
  else {
    updateSuccess = 0;
  }  
  updateTransmitStatuses();
}

void initializeData() {
  slcd.setCursor(8, 0);
  slcd.print("*");
  Serial.println("GET /i/?U=" + urlToken + "&D=" + deviceID + "&p=0&m1n=" + String(lastMoistureReading) + " HTTP/1.1");
  if (client.connect(remoteAddr, 80)) {
    client.println("GET /i/?U=" + urlToken + "&D=" + deviceID + "&p=0&m1n=" + String(lastMoistureReading) + " HTTP/1.1");
    client.println("Host: " + remoteWebHost);
    client.println();
    client.stop();
    intializeSuccess = 1;
  } 
  else {
    intializeSuccess = 0;
  }
  updateTransmitStatuses();
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
  slcd.clear();
  slcd.setCursor(0, 0);
  slcd.print(">");

  takeMeasurements();
  initializeData();
}

void loop()
{

  seconds = millis() / 1000;
  int secdiff = seconds - oldSeconds;
  secondsSinceLastPoll += secdiff;
  secondsSinceLastUpdate += secdiff;
  
  if ((secondsSinceLastUpdate >= updateInterval) && (secondsSinceLastPoll < pollInterval))  {
    secondsSinceLastUpdate = 0;
    takeMeasurements();
    printIntToSerialLCD(lastMoistureReading,0,1);
    sendUpdate();
  }

  if (secondsSinceLastPoll >= pollInterval) {
    pollCount += 1;
    secondsSinceLastPoll = 0;
    secondsSinceLastUpdate = 0;
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

      updateTransmitStatuses();
  }

  if (pollCount != oldPollCount) {
    updateTransmitStatuses();

    oldPollCount = pollCount;

    char buf3[3];
    itoa(pollCount,buf3,10);
    slcd.setCursor((16-(strlen(buf3))),0);
    slcd.print(buf3);

    takeMeasurements();
    printIntToSerialLCD(lastMoistureReading,0,1);

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
    } 
    else {
      sendUpdate();
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

    takeMeasurements();
    printIntToSerialLCD(lastMoistureReading,0,1);
    sendUpdate();
  }

  oldSeconds = seconds;

  if (secdiff > 0) {
    printIntToSerialLCD(secondsSinceLastPoll,1,0);
  }
}


