#include <Wire.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Use the LSM9DS1 class to create an object. [imu] can be
// named anything, we'll refer to that throught the sketch.
LSM9DS1 imu;

// SDO_XM and SDO_G are both pulled high, so our addresses are:
#define LSM9DS1_M 0x1C
#define LSM9DS1_AG 0x6A // Would be 0x6A if SDO_AG is LOW

#define PRINT_CALCULATED
//#define PRINT_RAW
#define PRINT_SPEED 250 // 250 ms between prints
int moveType = 0;
static unsigned long lastPrint = 0;


// Earth's magnetic field varies by location. Add or subtract
// a declination to get a more accurate heading. Calculate
// your's here: http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION -8.58 // Declination (degrees) in Boulder, CO.

// WiFi network name and password:
const char * networkName = "TELLO-C569FC";
const char * networkPswd = "";

//IP address to send UDP data to:
const char * udpAddress = "192.168.10.1";
const int udpPort = 8889;

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;

void setup()
{

  Serial.begin(115200);

  imu.settings.device.commInterface = IMU_MODE_I2C;
  imu.settings.device.mAddress = LSM9DS1_M;
  imu.settings.device.agAddress = LSM9DS1_AG;

  if (!imu.begin()) {
    Serial.println("Failed to communicate with LSM9DS1.");
    Serial.println("Double-check wiring.");
    Serial.println("Default settings in this sketch will " \
                   "work for an out of the box LSM9DS1 " \
                   "Breakout, but may need to be modified " \
                   "if the board jumpers are.");
    while (1)
      ;
  }

  //Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);

}

void loop()
{

  if (imu.accelAvailable()) {
    imu.readAccel();
  }

  if (imu.magAvailable()) {
    imu.readMag();
  }

  if ((lastPrint + PRINT_SPEED) < millis()) {
    printAccel();
    printAttitude(imu.ax, imu.ay, imu.az, -imu.my, -imu.mx, imu.mz);
    Serial.println();
    lastPrint = millis();

  }

}

//void printGyro() {
//  imu.readGyro();
//
//  Serial.print("G: ");
//#ifdef PRINT_CALCULATED
//  Serial.print(imu.calcGyro(imu.gx), 2);
//  Serial.print(", ");
//  Serial.print(imu.calcGyro(imu.gy), 2);
//  Serial.print(", ");
//  Serial.print(imu.calcGyro(imu.gz), 2);
//  Serial.println(" deg/s");
//#elif defined PRINT_RAW
//  Serial.print(imu.gx);
//  Serial.print(", ");
//  Serial.print(imu.gy);
//  Serial.print(", ");
//  Serial.println(imu.gz);
//#endif
//}

void printAccel() {
  Serial.print("A: ");
#ifdef PRINT_CALCULATED
  Serial.print(imu.calcAccel(imu.ax), 1);
  Serial.print(", ");
  Serial.print(imu.calcAccel(imu.ay), 1);
  Serial.print(", ");
  Serial.print(imu.calcAccel(imu.az), 1);
  Serial.println(" g");
#elif defined PRINT_RAW
  Serial.print(imu.ax);
  Serial.print(", ");
  Serial.print(imu.ay);
  Serial.print(", ");
  Serial.println(imu.az);
#endif

}

//void printMag()
//{
//  imu.readMag();
//  Serial.print("M: ");
//#ifdef PRINT_CALCULATED
//  Serial.print(imu.calcMag(imu.mx), 2);
//  Serial.print(", ");
//  Serial.print(imu.calcMag(imu.my), 2);
//  Serial.print(", ");
//  Serial.print(imu.calcMag(imu.mz), 2);
//  Serial.println(" gauss");
//#elif defined PRINT_RAW
//  Serial.print(imu.mx);
//  Serial.print(", ");
//  Serial.print(imu.my);
//  Serial.print(", ");
//  Serial.println(imu.mz);
//#endif
//}

// Calculate pitch, roll, and heading.
// Pitch/roll calculations take from this app note:
// http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf?fpsp=1
// Heading calculations taken from this app note:
// http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/Magnetic__Literature_Application_notes-documents/AN203_Compass_Heading_Using_Magnetometers.pdf
void printAttitude(float ax, float ay, float az, float mx, float my, float mz) {
  float roll = atan2(ay, az);
  float pitch = atan2(-ax, sqrt(ay * ay + az * az));
  float movement  =  sqrt  (ax * ax + ay * ay + az * az);
  float heading;

  if (my == 0)
    heading = (mx < 0) ? 180 : 0;
  else
    heading = atan2(mx, my);

  heading -= DECLINATION * PI / 180;

  if (heading > PI) heading -= (2 * PI);
  else if (heading < -PI) heading += (2 * PI);
  else if (heading < 0) heading += 2 * PI;

  // Convert everything from radians to degrees:
  heading *= 180.0 / PI;
  pitch *= 180.0 / PI;
  roll  *= 180.0 / PI;

  Serial.print("Pitch: ");
  Serial.print(pitch, 1);
  Serial.print(", Roll: ");
  Serial.print(roll, 1);
  Serial.print("Heading: ");
  Serial.println(heading, 0);

  if (pitch > 70) {
    Serial.print(" @@UP@@");
    if (moveType != 1) {
      TelloCommand("takeoff");
      delay(100);
    }
    moveType = 1;
  } else if (pitch < -35) {
    Serial.print(" @DOWN@");
    if (moveType != 2) {
      TelloCommand("land");
      delay(500);
    }
    moveType = 2;
  } else if ((pitch > 5) && (roll < -90)) {
    Serial.print(" @@CW@@");
    if (moveType != 3) {
      TelloCommand("flip l");
      delay(500);
    }
    moveType = 3;
  } else if ((pitch > 5) && (roll > 90)) {
    Serial.print(" @CCW@");
    if (moveType != 4) {
      TelloCommand("flip r");
      delay(500);
    }
    moveType = 4;
  } else {
    moveType = 0;
  }

}

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

      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;

      break;
  }
}
