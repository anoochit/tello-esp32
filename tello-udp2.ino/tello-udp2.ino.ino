#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi network name and password:
const char * networkName = "TELLO-C569FC";
const char * networkPswd = "";

//IP address to send UDP data to:
// either use the ip address of the server or
// a network broadcast address
const char * udpAddress = "192.168.10.1";
const int udpPort = 8889;

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;

void setup() {
  // Initilize hardware serial:
  Serial.begin(115200);

  //Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);
}

bool takeOff = false;

void loop() {

  if (takeOff) {
    // up 60 cm
    TelloCommand("up 60");
    delay(3000);
    // down 50 cm
    TelloCommand("down 50");
    delay(3000);  
    // left 30 cm
    TelloCommand("left 30");
    delay(3000);
    // right 30 cm
    TelloCommand("right 30");
    delay(3000);
    // turn cw 360 degrees
    TelloCommand("cw 360");
    delay(3000);
    // turn ccw 360 degrees
    TelloCommand("ccw 360");
    delay(3000);
    // flip back    
    TelloCommand("flip b");
    delay(3000);
    // flip front
    TelloCommand("flip f");
    delay(3000);
    // land
    TelloCommand("land");
    delay(5000);
    takeOff = false;
  }

}


//void TelloCommand(char *cmd)
//{
//  uint8_t buffer[100];
//  Serial.printf("Send [%s] to Tello.\n", cmd);
//  udp.beginPacket(udpAddress, udpPort);
//  udp.printf(cmd);
//  udp.endPacket();
//  memset(buffer, 0, 100);
//  while (udp.parsePacket() == 0) {
//    delay(1);
//  }
//  if (udp.read(buffer, 100) > 0) {
//    Serial.print("Message from Tello: ");
//    Serial.println((char * )buffer);
//  }
//}

void TelloCommand(char *cmd) {
  //only send data when connected
  if (connected) {
    //Send a packet
    udp.beginPacket(udpAddress, udpPort);
    udp.printf(cmd);
    udp.endPacket();
    Serial.printf("Send [%s] to Tello.\n", cmd);
  }
}

void connectToWiFi(const char * ssid, const char * pwd) {
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);

  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
      connected = true;

      //only send data when connected
      TelloCommand("command");
      delay(2000);

      TelloCommand("speed 50");
      delay(2000);

      TelloCommand("takeoff");
      delay(3000);

      takeOff = true;

      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;

      break;
  }
}
