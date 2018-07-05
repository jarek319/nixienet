#include <Time.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const byte sinArray[60] = {83, 101, 119, 136, 153, 169, 184, 198, 211, 222, 232, 240, 247, 251, 254, 255, 254, 251, 247, 240, 232, 222, 211, 198, 184, 169, 153, 136, 119, 101, 83, 65, 47, 30, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 30, 47, 65};
const int timeZone = -4;  // Eastern Standard Time (USA)

int nixieBulbDelayTime = 500;
int ledFadeTime = 8;

String ZIPCODE = "*";
String APPID = "*";

char ssid[] = "*";
char pass[] = "*";
char host[] = "api.openweathermap.org";

int GEIGER_PIN = 5;
int GEIGER_LED = 4;
int TIME_DATA_IN_PIN = 12;
int TIME_CLK_PIN = 13;
int TIME_LE_PIN = 0;
int TIME_BL_PIN = 2;
int STATUS_A_PIN = 14;
int STATUS_B_PIN = 15;
int STATUS_C_PIN = 16;

struct nixieStruct {
  byte hour1;
  byte hour2;
  byte minute1;
  byte minute2;
  byte bulbs;
};

time_t prevDisplay = 0; // when the digital clock was displayed

nixieStruct nixies = {2, 9, 9, 9, 15}; // initialize nixie tubes

unsigned int localPort = 2390;        // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets
WiFiUDP udp;

void drawNixies() {
  if (now() != prevDisplay) { //update the display only if time has changed
    prevDisplay = now();
    nixies.hour1 = hour() / 10;
    nixies.hour2 = hour() % 10;
    nixies.minute1 = minute() / 10;
    nixies.minute2 = minute() % 10;
  }

  int c = 0;                            //initialize counter
  digitalWrite(TIME_LE_PIN, LOW);       //open shift register latch
  digitalWrite(TIME_DATA_IN_PIN, LOW);  //set next bit low
  for (c = 0; c < 32; c++) {            //push low bit 32 times to clear the register
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
  }
  digitalWrite(TIME_LE_PIN, HIGH);      //push latch
  digitalWrite(TIME_LE_PIN, LOW);       //open latch
  digitalWrite(TIME_DATA_IN_PIN, LOW);  //set next bit low
  for (c = 9; c > nixies.minute2; c--) {
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
  }
  digitalWrite(TIME_DATA_IN_PIN, HIGH);
  digitalWrite(TIME_CLK_PIN, HIGH);
  digitalWrite(TIME_CLK_PIN, LOW);
  digitalWrite(TIME_DATA_IN_PIN, LOW);
  for (c; c > 0; c--) {
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
  }
  digitalWrite(TIME_DATA_IN_PIN, LOW);
  for (c = 9; c > nixies.minute1; c--) {
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
  }
  digitalWrite(TIME_DATA_IN_PIN, HIGH);
  digitalWrite(TIME_CLK_PIN, HIGH);
  digitalWrite(TIME_CLK_PIN, LOW);
  digitalWrite(TIME_DATA_IN_PIN, LOW);
  for (c; c > 0; c--) {
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
  }
  digitalWrite(TIME_DATA_IN_PIN, LOW);
  for (c = 9; c > nixies.hour2; c--) {
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
  }
  digitalWrite(TIME_DATA_IN_PIN, HIGH);
  digitalWrite(TIME_CLK_PIN, HIGH);
  digitalWrite(TIME_CLK_PIN, LOW);
  digitalWrite(TIME_DATA_IN_PIN, LOW);
  for (c; c > 0; c--) {
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
  }
  digitalWrite(TIME_DATA_IN_PIN, LOW);
  if (nixies.hour1 != 0) {
    for (c = 2; c > nixies.hour1; c--) {
      digitalWrite(TIME_CLK_PIN, HIGH);
      digitalWrite(TIME_CLK_PIN, LOW);
    }
    digitalWrite(TIME_DATA_IN_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, HIGH);
    digitalWrite(TIME_CLK_PIN, LOW);
    digitalWrite(TIME_DATA_IN_PIN, LOW);
    for (c; c > 1; c--) {
      digitalWrite(TIME_CLK_PIN, HIGH);
      digitalWrite(TIME_CLK_PIN, LOW);
    }
  }
  else {
    for (int i = 0; i < 2; i++) {
      digitalWrite(TIME_CLK_PIN, HIGH);
      digitalWrite(TIME_CLK_PIN, LOW);
    }
  }
  digitalWrite(TIME_LE_PIN, HIGH);

  if (nixies.bulbs & 0x01) { // Top Colon
    digitalWrite(STATUS_A_PIN, LOW);
    digitalWrite(STATUS_B_PIN, LOW);
    digitalWrite(STATUS_C_PIN, LOW);
    delayMicroseconds(nixieBulbDelayTime);
  }
  if (nixies.bulbs & 0x02) { // Botton Colon
    digitalWrite(STATUS_A_PIN, HIGH);
    digitalWrite(STATUS_B_PIN, LOW);
    digitalWrite(STATUS_C_PIN, LOW);
    delayMicroseconds(nixieBulbDelayTime);
  }
  if (nixies.bulbs & 0x04) { // Sun
    digitalWrite(STATUS_A_PIN, LOW);
    digitalWrite(STATUS_B_PIN, HIGH);
    digitalWrite(STATUS_C_PIN, HIGH);
    delayMicroseconds(nixieBulbDelayTime);
  }
  if (nixies.bulbs & 0x08) { // Cloud
    digitalWrite(STATUS_A_PIN, LOW);
    digitalWrite(STATUS_B_PIN, HIGH);
    digitalWrite(STATUS_C_PIN, LOW);
    delayMicroseconds(nixieBulbDelayTime);
  }
  if (nixies.bulbs & 0x10) { // Rain
    digitalWrite(STATUS_A_PIN, LOW);
    digitalWrite(STATUS_B_PIN, LOW);
    digitalWrite(STATUS_C_PIN, HIGH);
    delayMicroseconds(nixieBulbDelayTime);
  }
  if (nixies.bulbs & 0x20) { // Snow
    digitalWrite(STATUS_A_PIN, HIGH);
    digitalWrite(STATUS_B_PIN, LOW);
    digitalWrite(STATUS_C_PIN, HIGH);
    delayMicroseconds(nixieBulbDelayTime);
  }
}

time_t getNtpTime() {
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
  }
  String url = "/data/2.5/forecast?zip=";
  url += ZIPCODE;
  url += ",us&APPID=";
  url += APPID;
  url += "&cnt=3";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(1000);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String storedRequestString = client.readStringUntil('\r');
    if (storedRequestString.charAt(1) == '{') {
      storedRequestString.remove(0, 1);
      storedRequestString.remove(storedRequestString.length() - 1);
      Serial.print("String: ");
      Serial.println(storedRequestString);
      int index1 = storedRequestString.indexOf("\"dt\":");
      int index2 = storedRequestString.indexOf("\"dt\":", index1 + 1);
      int index3 = storedRequestString.indexOf("\"dt\":", index2 + 1);
      int weatherIndex1 = storedRequestString.indexOf("\"weather\":[{\"id\":", index1);
      int weatherIndex2 = storedRequestString.indexOf("\"weather\":[{\"id\":", index2);
      int weatherIndex3 = storedRequestString.indexOf("\"weather\":[{\"id\":", index3);
      String forecastString1 = storedRequestString.substring(weatherIndex1 + 17, weatherIndex1 + 20);
      String forecastString2 = storedRequestString.substring(weatherIndex2 + 17, weatherIndex2 + 20);
      String forecastString3 = storedRequestString.substring(weatherIndex3 + 17, weatherIndex3 + 20);
      Serial.println(forecastString1);
      Serial.println(forecastString2);
      Serial.println(forecastString3);
      nixies.bulbs &= 3;
      if (forecastString1.charAt(0) == '6' || forecastString2.charAt(0) == '6' || forecastString3.charAt(0) == '6') {
        nixies.bulbs |= 0x20; //SNOW
        Serial.println("Snow");
      }
      else if (forecastString1.charAt(0) == '5' || forecastString2.charAt(0) == '5' || forecastString3.charAt(0) == '5' || forecastString1.charAt(0) == '3' || forecastString2.charAt(0) == '3' || forecastString3.charAt(0) == '3' || forecastString1.charAt(0) == '2' || forecastString2.charAt(0) == '2' || forecastString3.charAt(0) == '2') {
        nixies.bulbs |= 0x10; //RAIN
        Serial.println("Rain");
      }
      else if ((forecastString1.charAt(0) == '8' || forecastString2.charAt(0) == '8' || forecastString3.charAt(0) == '8') && (forecastString1.charAt(2) != '0' || forecastString2.charAt(2) != '0' || forecastString3.charAt(2) != '0')) {
        nixies.bulbs |= 0x08; //CLOUDS
        Serial.println("Clouds");
      }
      else {
        nixies.bulbs |= 0x04; //CLEAR
        Serial.println("Clear");
      }

      Serial.println(forecastString1);
      Serial.println(forecastString2);
      Serial.println(forecastString3);
      /*
      const char* sensor = root["coord"];
      const char* time = root["weather"];
      Serial.println(sensor);
      Serial.println(time);
      */
    }
  }
  Serial.println();
  Serial.println("closing connection");

  while (udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    if (!udp.parsePacket()) {}
    else {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

void setup() {
  pinMode(GEIGER_PIN, INPUT);
  pinMode(GEIGER_LED, OUTPUT);
  digitalWrite(GEIGER_LED, LOW);
  pinMode(TIME_DATA_IN_PIN, OUTPUT);
  digitalWrite(TIME_DATA_IN_PIN, LOW);
  pinMode(TIME_CLK_PIN, OUTPUT);
  digitalWrite(TIME_CLK_PIN, LOW);
  pinMode(TIME_LE_PIN, OUTPUT);
  digitalWrite(TIME_LE_PIN, HIGH);
  pinMode(TIME_BL_PIN, OUTPUT);
  digitalWrite(TIME_BL_PIN, HIGH);
  pinMode(STATUS_A_PIN, OUTPUT);
  digitalWrite(STATUS_A_PIN, HIGH);
  pinMode(STATUS_B_PIN, OUTPUT);
  digitalWrite(STATUS_B_PIN, HIGH);
  pinMode(STATUS_C_PIN, OUTPUT);
  digitalWrite(STATUS_C_PIN, HIGH);
  drawNixies();

  Serial.begin(115200);

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

  setSyncProvider(getNtpTime);
}

void loop() {
  analogWrite(GEIGER_LED, constrain((sin((((millis() / ledFadeTime) % 1024) - 0) * (6.28 - 0.00) / (1024 - 0) + 0.00) + 1.00) * 512.00, 5, 1015));
  drawNixies();
}

unsigned long sendNTPpacket(IPAddress& address) { // send an NTP request to the time server at the given address
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
  //  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
