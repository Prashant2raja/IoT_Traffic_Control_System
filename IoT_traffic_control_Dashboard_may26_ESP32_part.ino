/*
=========================================================
ESP32 FIREBASE TRAFFIC SIGNAL DASHBOARD
=========================================================

FEATURES:
✓ Firebase Realtime Database
✓ Auto / Manual Mode
✓ Live Traffic Density Upload
✓ Dashboard Control
✓ Serial Communication with Arduino Nano
✓ Web Dashboard Support

=========================================================
REQUIRED LIBRARIES
=========================================================

1. Firebase ESP Client
   By Mobizt

2. WiFi.h

=========================================================
FIREBASE DATABASE STRUCTURE
=========================================================

TrafficSystem
|
|-- mode
|     AUTO / MANUAL
|
|-- command
|     NS_GREEN
|     EW_GREEN
|     ALL_RED
|
|-- density
|      |-- north
|      |-- south
|      |-- east
|      |-- west
|
|-- status
       NS_GREEN_ACTIVE
       EW_GREEN_ACTIVE

=========================================================
UART CONNECTION
=========================================================

ESP32 TX2 -> Arduino Nano RX
ESP32 RX2 -> Arduino Nano TX

GPIO17 -> TX2
GPIO16 -> RX2

IMPORTANT:
Use voltage divider for Nano TX -> ESP32 RX

=========================================================
*/

#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Firebase Helper
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

//////////////////////////////////////////////////////////
// WIFI CREDENTIALS
//////////////////////////////////////////////////////////

#define WIFI_SSID "Free Internet"
#define WIFI_PASSWORD "Pay@369.dlx"

//////////////////////////////////////////////////////////
// FIREBASE CONFIG
//////////////////////////////////////////////////////////

#define API_KEY "AIzaSyA2dmUCogmK_XpAGRpGzIk0WAMLOdi3-mQ"

#define DATABASE_URL "https://traffic-control-26-default-rtdb.firebaseio.com/"

#define USER_EMAIL "aboutprincekumar@gmail.com"
#define USER_PASSWORD "12345678"

//////////////////////////////////////////////////////////
// FIREBASE OBJECTS
//////////////////////////////////////////////////////////

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

//////////////////////////////////////////////////////////
// SERIAL2 FOR ARDUINO NANO
//////////////////////////////////////////////////////////

HardwareSerial NanoSerial(2);

//////////////////////////////////////////////////////////
// VARIABLES
//////////////////////////////////////////////////////////

String mode = "AUTO";
String command = "";

unsigned long sendDataPrevMillis = 0;
unsigned long readControlMillis = 0;

int lastNorth = -1;
int lastSouth = -1;
int lastEast = -1;
int lastWest = -1;

String lastFirebaseMode = "";
String lastStatus = "";

//////////////////////////////////////////////////////////
// SETUP
//////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);

  // RX = 16
  // TX = 17
  NanoSerial.begin(4800, SERIAL_8N1, 16, 17);

  //////////////////////////////////////////////////////
  // WIFI CONNECT
  //////////////////////////////////////////////////////

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.println("WiFi Connected");

  //////////////////////////////////////////////////////
  // FIREBASE CONFIG
  //////////////////////////////////////////////////////

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Connected");

  //////////////////////////////////////////////////////
  // DEFAULT VALUES
  //////////////////////////////////////////////////////

  Firebase.RTDB.setString(&fbdo,
                          "/TrafficSystem/mode",
                          "AUTO");

  Firebase.RTDB.setString(&fbdo,
                          "/TrafficSystem/command",
                          "NONE");
}

//////////////////////////////////////////////////////////
// LOOP
//////////////////////////////////////////////////////////

void loop()
{
  readNanoData();

  readFirebaseControls();

}

//////////////////////////////////////////////////////////
// READ DATA FROM ARDUINO NANO
//////////////////////////////////////////////////////////

void readNanoData()
{
  static String receivedData = "";

  while (NanoSerial.available())
  {
    char c = NanoSerial.read();

    ////////////////////////////////////////////////////
    // START PACKET
    ////////////////////////////////////////////////////

    if (c == '<')
    {
      receivedData = "";
    }

    ////////////////////////////////////////////////////
    // END PACKET
    ////////////////////////////////////////////////////

    else if (c == '>')
    {
      receivedData.trim();

      //////////////////////////////////////////////////
      // VALIDATE DATA
      //////////////////////////////////////////////////

      if (receivedData.length() > 3)
      {
        Serial.println(
        "VALID DATA: " + receivedData);

        processData(receivedData);
      }

      receivedData = "";
    }

    ////////////////////////////////////////////////////
    // STORE CHARACTER
    ////////////////////////////////////////////////////

    else
    {
      //////////////////////////////////////////////////
      // AVOID BUFFER OVERFLOW
      //////////////////////////////////////////////////

      if (receivedData.length() < 100)
      {
        receivedData += c;
      }
    }
  }
}
//////////////////////////////////////////////////////////
// READ FIREBASE CONTROLS
//////////////////////////////////////////////////////////

void readFirebaseControls(){
  if (millis() - readControlMillis > 1000)
  {
    readControlMillis = millis();

    ////////////////////////////////////////////////////
    // READ MODE
    ////////////////////////////////////////////////////

    if (Firebase.RTDB.getString(
          &fbdo,
          "/TrafficSystem/mode"))
    {
      String newMode = fbdo.stringData();

      if (newMode != mode)
      {
        mode = newMode;

        NanoSerial.println(mode);

        Serial.println("MODE SENT: " + mode);
      }
    }

    ////////////////////////////////////////////////////
    // READ COMMAND
    ////////////////////////////////////////////////////

    if (Firebase.RTDB.getString(
          &fbdo,
          "/TrafficSystem/command"))
    {
      String newCommand = fbdo.stringData();

      if (newCommand != command)
      {
        command = newCommand;

        NanoSerial.println(command);

        Serial.println("COMMAND SENT: " + command);
      }
    }
  }
}

//////////////////////////////////////////////////////////
// EXTRACT SENSOR VALUES
//////////////////////////////////////////////////////////

int getValue(String data, char key){
  int startIndex = data.indexOf(key);

  if (startIndex < 0)
    return 0;

  startIndex += 2;

  int endIndex = data.indexOf(",", startIndex);

  if (endIndex < 0)
    endIndex = data.length();

  String value =
    data.substring(startIndex, endIndex);

  return value.toInt();
}

//////////////////////////////////////////////////////////
// EXTRACT MODE
//////////////////////////////////////////////////////////

String getMode(String data){
  int startIndex = data.indexOf("MODE:");

  if (startIndex < 0)
    return "UNKNOWN";

  startIndex += 5;

  return data.substring(startIndex);
}


void processData(String data){
  Serial.println("VALID DATA: " + data);

  int n = getValue(data, 'N');
  int s = getValue(data, 'S');
  int e = getValue(data, 'E');
  int w = getValue(data, 'W');

  int timerValue =
getTimerValue(data);

Firebase.RTDB.setInt(
  &fbdo,
  "/TrafficSystem/timer",
  timerValue
);

if (data.startsWith("STATUS:"))
{
    String status =
    data.substring(7);

    status.trim();

    if (status != lastStatus)
    {
        lastStatus = status;

        Serial.println(status);

        Firebase.RTDB.setString(
          &fbdo,
          "/TrafficSystem/status",
          status
        );
    }

    return;
}

 if (n != lastNorth)
{
    lastNorth = n;

    Firebase.RTDB.setInt(
      &fbdo,
      "/TrafficSystem/density/north",
      n
    );
}

if (s != lastSouth)
{
    lastSouth = s;

    Firebase.RTDB.setInt(
      &fbdo,
      "/TrafficSystem/density/south",
      s
    );
}

if (e != lastEast)
{
    lastEast = e;

    Firebase.RTDB.setInt(
      &fbdo,
      "/TrafficSystem/density/east",
      e
    );
}

if (w != lastWest)
{
    lastWest = w;

    Firebase.RTDB.setInt(
      &fbdo,
      "/TrafficSystem/density/west",
      w
    );
}

     String mode = "";

if (data.indexOf("MODE:AUTO") >= 0)
{
    mode = "AUTO";
}
else if (data.indexOf("MODE:MANUAL") >= 0)
{
    mode = "MANUAL";
}

if (mode != "" &&
    mode != lastFirebaseMode)
{
    lastFirebaseMode = mode;

    Firebase.RTDB.setString(
      &fbdo,
      "/TrafficSystem/currentMode",
      mode
    );
}
}     

int getTimerValue(String data)
{
    int startIndex =
    data.indexOf("T:");

    if(startIndex < 0)
        return 0;

    startIndex += 2;

    int endIndex =
    data.indexOf(",", startIndex);

    if(endIndex < 0)
        endIndex = data.length();

    return data.substring(
      startIndex,
      endIndex
    ).toInt();
}