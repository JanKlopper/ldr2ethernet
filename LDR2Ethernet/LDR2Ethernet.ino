#include "avr/interrupt.h"
#include <EtherCard.h>
#include <EEPROM.h>

const byte
  chipSelect = 8,
  frameHeader = 54,
  LDRInterruptPin = 0,
  ver = 1,
  ldrpin = 2;

// ethernet interface mac address, must be unique on the LAN
const byte etherMac[6] = {0x74, 0x69, 0x69, 0x2D, 0x31, 0x08};

const char http_OK[] PROGMEM = "HTTP/1.0 200 OK\r\n";
const char http_NOK[] PROGMEM = "HTTP/1.0 404 NOT FOUND\r\n";
const char http_JSON[] PROGMEM = "Content-Type: application/json\r\n";
const char http_HTML[] PROGMEM = "Content-Type: text/html\r\n";
const char http_NOCACHE[] PROGMEM = "Pragma: no-cache\r\n\r\n";

// offset addresses for the IP, MAC in the eeprom
const byte eepromIP = 0;
const byte eepromMAC = 4;

volatile int ldrhighcount = 0; // count of hits on the ldr
int interval = 100; // timeout for each ldr after a hit
volatile long lastHitTime = 0; // keep track of last hit

// Ethernet buffer receives just the get request
byte Ethernet::buffer[300];
static BufferFiller bfill;

void setup () {
  Serial.begin(57600);
  Serial.println("[ldrserver]");
  
  Serial.print("ldr on pin: ");
  Serial.println(ldrpin);

  // Set up the ethernet module with our MAC and static IP.
  byte mac[6];
  readMAC(mac);
  if(mac[0] == 0 || mac[0] == 255){
    if (ether.begin(sizeof Ethernet::buffer, etherMac, chipSelect) == 0)
      Serial.println("No eth?");  
    Serial.println("using default mac");
  } else { 
    if (ether.begin(sizeof Ethernet::buffer, mac, chipSelect) == 0)
      Serial.println("No eth?");
    Serial.println("using eeprom mac");
  }
  
  byte etherIP[4];
  readIP(etherIP);
  bool online = false;
  if (etherIP[0] == 0 || etherIP[0] == 255){
    while(!online){
      Serial.println("Setting up DHCP");
      if (!ether.dhcpSetup()){
        Serial.println( "DHCP failed");
      } else {
        Serial.print("Online at DHCP:");
        ether.printIp("IP: ", ether.myip);
        online = true;
      }
    }
  }
  if(!online){
    Serial.print("Online at STATIC:");
    ether.staticSetup(etherIP); 
    ether.printIp("IP: ", ether.myip);
  }
  
    // make the LDR's pin an input:
  pinMode(ldrpin, INPUT); 
  //sei(); //Enable global interrupts
  //EIMSK |= (1 << INT0); //Enable external interrupt INT0
  //EICRA |= (1 << ISC01); //Trigger INT0 on falling edge
  attachInterrupt(LDRInterruptPin, LDRInterrupt, RISING);//FALLING);
}

void loop () {
  // Process incoming packages
  
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  if (pos) {
    bfill = ether.tcpOffset();
    char* data = (char *) Ethernet::buffer + pos;
    if (strncmp("GET / ", data, 6) == 0){
      // Return home page
      Serial.println("homepage");
      homePage(bfill);      
      ether.httpServerReply(bfill.position());
    } else if (strncmp("GET /json", data, 9) == 0){
      Serial.println("json output");
      Json(bfill);
      ether.httpServerReply(bfill.position());
    } else if (strncmp("GET /c?", data, 7) == 0){
      Serial.println("Config saved");
      saveConfigPage(data, bfill);  
      ether.httpServerReply(bfill.position());
    } else {
      Serial.println("404");
      NotOK(bfill);      
      ether.httpServerReply(bfill.position());
    }
  }
}

void LDRInterrupt(){
//RISINGISR(INT0_vect){
  unsigned long hitMillis = millis();
  Serial.println("interrupt");
  if ((hitMillis - lastHitTime) > interval){
    lastHitTime = hitMillis;
    ldrhighcount = ldrhighcount + 1;
  }
}

// eeprom reading and writing
void setIP(byte ip[]){
  Serial.print("IP stored as: ");
  for(int i=0; i<4; ++i){
    EEPROM.write(i+eepromIP, ip[i]);
    Serial.print(ip[i], DEC);
    if (i < 3)
      Serial.print('.');
  }
  Serial.println();
}

void setMAC(byte mac[]){
  Serial.print("MAC stored as: ");
  for(int i=0; i<6; ++i){
    EEPROM.write(i+eepromMAC, mac[i]);
    Serial.print(mac[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();
}

void readIP(byte ip[]){
  for(int i=0; i<4; ++i){
    ip[i] = EEPROM.read(i+eepromIP);
  }
}

void readMAC(byte mac[]){
  for(int i=0; i<6; ++i){
    mac[i] = EEPROM.read(i+eepromMAC);
  }
}


// webpages//
static void homePage(BufferFiller& buf){
  byte etherIP[4];
  readIP(etherIP);
  byte mac[6];
  readMAC(mac);
  buf.emit_p(PSTR("$F$F$F"
                  "<form action='/c'>"),
             http_OK, http_HTML, http_NOCACHE);
  
  buf.emit_p(PSTR("Ip:"));
  // output ipv address octets
  for(int i=0; i<4; i++){
    buf.emit_p(PSTR("<input name='i$D' value='$D'>"), i, etherIP[i]);    
  }
  buf.emit_p(PSTR("<br>MAC:"));
  // output ipv address octets
  for(int i=0; i<6; i++){
    buf.emit_p(PSTR("<input name='m$D' value='$D'>"), i, mac[i]);    
  }
  buf.emit_p(PSTR("<br><input type='submit'></form>"));
  buf.emit_p(PSTR("<br><a href='/json'>Json</a>"));
  // show dhcp as enabled based on eeprom value
  //buf.emit_p(PSTR("<br>Use DHCP: <input type='checkbox' name='dhcp' value='true' $F>"
  //                "<br><input type='submit'></form>"),
  //                (etherIP[0] == 0 || etherIP[0] == 255?"checked":""));
}

static void NotOK(BufferFiller& buf){
  buf.emit_p(PSTR("$F$F$F"          
                  "404"),
             http_NOK, http_HTML, http_NOCACHE);
}

static void Json(BufferFiller& buf){
  // return a json string containing all ldr hitcounts and current values  
  
  buf.emit_p(PSTR("$F$F$F"
                  "{\"version\": $D, \"ldrcount\": $D}"), 
             http_OK, http_JSON, http_NOCACHE, ver, ldrhighcount); 
  
}
static int getIntArg(const char* data, const char* key, int value=255) {
  char temp[10];
  if (ether.findKeyVal(data + 5, temp, sizeof temp, key) > 0){
    value = atoi(temp);
  }
  return value;
}

static void saveConfigPage(const char* data, BufferFiller& buf){
  byte newIP[4];
  newIP[0] = getIntArg(data, "i0", -1);
  newIP[1] = getIntArg(data, "i1", -1);
  newIP[2] = getIntArg(data, "i2", -1);
  newIP[3] = getIntArg(data, "i3", -1);      
  setIP(newIP);

  byte newmac[6];
  newmac[0] = getIntArg(data, "m0");
  newmac[1] = getIntArg(data, "m1");
  newmac[2] = getIntArg(data, "m2");
  newmac[3] = getIntArg(data, "m3");
  newmac[4] = getIntArg(data, "m4");
  newmac[5] = getIntArg(data, "m5");  
  setMAC(newmac);

  buf.emit_p(PSTR("$F$F$F"
                  "Config saved"), 
             http_OK, http_HTML, http_NOCACHE);
}
