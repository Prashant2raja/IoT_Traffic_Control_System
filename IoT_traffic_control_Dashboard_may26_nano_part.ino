/*
=========================================================
 SMART TRAFFIC SIGNAL CONTROL SYSTEM
 Arduino Nano Code
=========================================================

FEATURES:
✓ 8 IR Sensors
✓ Auto Traffic Density Control
✓ Manual Mode Support
✓ Serial Communication with ESP32
✓ Shared Yellow LED Pin
✓ Opposite Direction Control
✓ Dynamic Green Timing

=========================================================
PIN CONFIGURATION
=========================================================

IR Sensors:
N1 -> D7
N2 -> D8

S1 -> D9
S2 -> D10

E1 -> D11
E2 -> D12

W1 -> A0
W2 -> A1

Traffic LEDs:
NS_GREEN -> D2
NS_RED   -> D3

EW_GREEN -> D4
EW_RED   -> D5

ALL_YELLOW -> D6

=========================================================
SERIAL COMMANDS FROM ESP32
=========================================================

AUTO
MANUAL

NS_GREEN
EW_GREEN

ALL_RED

=========================================================
*/
#include <SoftwareSerial.h>
SoftwareSerial espSerial(8, 9); // RX, TX
#define NS_GREEN 6
#define NS_RED 5

#define EW_GREEN 5
#define EW_RED 6

#define YELLOW_PIN 4

// IR Sensors
#define N1 A0
#define N2 A4

#define S1 A3
#define S2 2

#define E1 A1
#define E2 A2

#define W1 A5
#define W2 3

// Modes
bool autoMode = true;

// Density Variables
int northDensity = 0;
int southDensity = 0;
int eastDensity = 0;
int westDensity = 0;

// Timing
unsigned long greenTime = 10000;

unsigned long signalTimer = 0;

bool currentNS = true;

bool yellowState = false;

unsigned long yellowTimer = 0;

int yellowBlinkCount = 0;

bool inYellowMode = false;

String manualCommand = "ALL_RED";

bool manualYellow = false;

unsigned long manualYellowTimer = 0;

bool yellowBlinkState = false;
const unsigned long GREEN_DURATION = 30000;

const unsigned long YELLOW_DURATION = 5000;

unsigned long countdownTimer = 0;

int remainingSeconds = 30;

//////////////////////////////////////////////////////////
// VEHICLE COUNTERS
//////////////////////////////////////////////////////////

int northCount = 0;
int southCount = 0;
int eastCount  = 0;
int westCount  = 0;

bool prevNIn  = HIGH;
bool prevNOut = HIGH;

bool prevSIn  = HIGH;
bool prevSOut = HIGH;

bool prevEIn  = HIGH;
bool prevEOut = HIGH;

bool prevWIn  = HIGH;
bool prevWOut = HIGH;

void setup(){
  Serial.begin(9600);

  espSerial.begin(4800);

  // LED Pins
  pinMode(NS_GREEN, OUTPUT);
  pinMode(NS_RED, OUTPUT);

  pinMode(EW_GREEN, OUTPUT);
  pinMode(EW_RED, OUTPUT);

  pinMode(YELLOW_PIN, OUTPUT);

  // Sensor Pins
  pinMode(N1, INPUT);
  pinMode(N2, INPUT);

  pinMode(S1, INPUT);
  pinMode(S2, INPUT);

  pinMode(E1, INPUT);
  pinMode(E2, INPUT);

  pinMode(W1, INPUT);
  pinMode(W2, INPUT);

  allRed();

  Serial.println("SMART TRAFFIC SYSTEM STARTED");
  signalTimer = millis();
}

void loop()
{
  updateCountdown();
  readSensors();

  serialCommands();

  sendDataToESP32();

  //////////////////////////////////////////////////////
  // AUTO MODE
  //////////////////////////////////////////////////////

  if (autoMode)
  {
    autoTrafficControl();
  }

  //////////////////////////////////////////////////////
  // MANUAL MODE
  //////////////////////////////////////////////////////

  else
  {
    manualTrafficControl();
  }
}

//////////////////////////////////////////////////////////
// READ SENSOR DENSITY
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// READ TRAFFIC DENSITY
//////////////////////////////////////////////////////////

void readSensors()
{
  //////////////////////////////////////////////////////
  // NORTH LANE
  //////////////////////////////////////////////////////

  bool nIn  = digitalRead(N1);
  bool nOut = digitalRead(S1);

  // Vehicle Enter
  if (prevNIn == HIGH && nIn == LOW)
  {
    northCount++;
  }

  // Vehicle Exit
  if (prevNOut == HIGH && nOut == LOW)
  {
    if (northCount > 0)
      northCount--;
  }

  prevNIn = nIn;
  prevNOut = nOut;

  //////////////////////////////////////////////////////
  // SOUTH LANE
  //////////////////////////////////////////////////////

  bool sIn  = digitalRead(S2);
  bool sOut = digitalRead(N2);

  if (prevSIn == HIGH && sIn == LOW)
  {
    southCount++;
  }

  if (prevSOut == HIGH && sOut == LOW)
  {
    if (southCount > 0)
      southCount--;
  }

  prevSIn = sIn;
  prevSOut = sOut;

  //////////////////////////////////////////////////////
  // EAST LANE
  //////////////////////////////////////////////////////

  bool eIn  = digitalRead(E2);
  bool eOut = digitalRead(W2);

  if (prevEIn == HIGH && eIn == LOW)
  {
    eastCount++;
  }

  if (prevEOut == HIGH && eOut == LOW)
  {
    if (eastCount > 0)
      eastCount--;
  }

  prevEIn = eIn;
  prevEOut = eOut;

  //////////////////////////////////////////////////////
  // WEST LANE
  //////////////////////////////////////////////////////

  bool wIn  = digitalRead(W1);
  bool wOut = digitalRead(E1);

  if (prevWIn == HIGH && wIn == LOW)
  {
    westCount++;
  }

  if (prevWOut == HIGH && wOut == LOW)
  {
    if (westCount > 0)
      westCount--;
  }

  prevWIn = wIn;
  prevWOut = wOut;

  //////////////////////////////////////////////////////
  // UPDATE GLOBAL DENSITY
  //////////////////////////////////////////////////////

  northDensity = northCount;
  southDensity = southCount;
  eastDensity  = eastCount;
  westDensity  = westCount;
}

//////////////////////////////////////////////////////////
// DENSITY CALCULATION
//////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////
// AUTO TRAFFIC CONTROL
//////////////////////////////////////////////////////////

void autoTrafficControl()
{
  //////////////////////////////////////////////////////
  // YELLOW MODE
  //////////////////////////////////////////////////////

  if (inYellowMode)
  {
    handleYellowBlink();
    return;
  }

  //////////////////////////////////////////////////////
  // RUN CURRENT SIGNAL
  //////////////////////////////////////////////////////

  static bool lastDirection = false;

if (currentNS)
{
    northSouthGreen();

    if (lastDirection != currentNS)
    {
        espSerial.println("<STATUS:NS_GREEN_ACTIVE>");
        lastDirection = currentNS;
    }
}
else
{
    eastWestGreen();

    if (lastDirection != currentNS)
    {
        espSerial.println("<STATUS:EW_GREEN_ACTIVE>");
        lastDirection = currentNS;
    }
}

  //////////////////////////////////////////////////////
  // AFTER 30 SECONDS START YELLOW
  //////////////////////////////////////////////////////

  if (millis() - signalTimer >= GREEN_DURATION)
  {
    signalTimer = millis();

    startYellowMode();
  }
}

//////////////////////////////////////////////////////////
// NORTH SOUTH GREEN
//////////////////////////////////////////////////////////

void northSouthGreen(){
  digitalWrite(NS_GREEN, HIGH);
  digitalWrite(NS_RED, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(EW_GREEN, LOW);
  digitalWrite(EW_RED, HIGH);
}

//////////////////////////////////////////////////////////
// EAST WEST GREEN
//////////////////////////////////////////////////////////

void eastWestGreen(){
  digitalWrite(NS_GREEN, LOW);
  digitalWrite(NS_RED, HIGH);
    digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(EW_GREEN, HIGH);
  digitalWrite(EW_RED, LOW);
}

//////////////////////////////////////////////////////////
// ALL RED
//////////////////////////////////////////////////////////

void allRed(){
  digitalWrite(NS_GREEN, LOW);
  digitalWrite(NS_RED, HIGH);

  digitalWrite(EW_GREEN, LOW);
  digitalWrite(EW_RED, HIGH);
}



//////////////////////////////////////////////////////////
// SERIAL COMMANDS FROM ESP32
//////////////////////////////////////////////////////////

void serialCommands(){
  if (espSerial.available())
  {
    String cmd = espSerial.readStringUntil('>');
cmd.replace("<", "");
cmd.replace(">", "");
cmd.trim();

    cmd.trim();

    ////////////////////////////////////////////////////
    // AUTO MODE
    ////////////////////////////////////////////////////

    if (cmd == "AUTO")
    {
      autoMode = true;

      manualCommand = "";

      espSerial.println("<MODE:AUTO>");
    }

    ////////////////////////////////////////////////////
    // MANUAL MODE
    ////////////////////////////////////////////////////

    else if (cmd == "MANUAL")
    {
      autoMode = false;

      manualCommand = "ALL_RED";

      allRed();

      espSerial.println("<MODE:MANUAL>");
    }

    ////////////////////////////////////////////////////
    // MANUAL NS GREEN
    ////////////////////////////////////////////////////

    else if (cmd == "NS_GREEN")
    {
      autoMode = false;

      manualCommand = "NS_GREEN";

      northSouthGreen();
    }

    ////////////////////////////////////////////////////
    // MANUAL EW GREEN
    ////////////////////////////////////////////////////

    else if (cmd == "EW_GREEN")
    {
      autoMode = false;

      manualCommand = "EW_GREEN";

      eastWestGreen();
    }

    ////////////////////////////////////////////////////
    // MANUAL YELLOW
    ////////////////////////////////////////////////////

    else if (cmd == "YELLOW")
    {
      autoMode = false;

      manualCommand = "YELLOW";

      manualYellow = true;

      espSerial.println("<STATUS:YELLOW>");
    }

    ////////////////////////////////////////////////////
    // ALL RED
    ////////////////////////////////////////////////////

    else if (cmd == "ALL_RED")
    {
      autoMode = false;

      manualCommand = "ALL_RED";

      manualYellow = false;

      allRed();

      espSerial.println("<STATUS:ALL_RED>");
    }
  }
}

//////////////////////////////////////////////////////////
// SEND SENSOR DATA TO ESP32
//////////////////////////////////////////////////////////

void sendDataToESP32(){
  static unsigned long lastSend = 0;

  if (millis() - lastSend > 2000)
  {
    lastSend = millis();

    espSerial.print("<");

    espSerial.print("N:");
    espSerial.print(northDensity);

    espSerial.print(",S:");
    espSerial.print(southDensity);

    espSerial.print(",E:");
    espSerial.print(eastDensity);

    espSerial.print(",W:");
    espSerial.print(westDensity);

    espSerial.print(",MODE:");

    if (autoMode)
      espSerial.print("AUTO");
    else
      espSerial.print("MANUAL");
    espSerial.print(",T:");
    espSerial.print(remainingSeconds);

    espSerial.println(">");
  }
}

void startYellowMode(){
  inYellowMode = true;

  yellowTimer = millis();
  remainingSeconds = 5;

signalTimer = millis();

  allRed();

  espSerial.println("<STATUS:YELLOW>");
}


void handleYellowBlink(){

unsigned long yellowElapsed =
millis() - signalTimer;

if(yellowElapsed >= YELLOW_DURATION)
{
    remainingSeconds = 0;
}
else
{
    remainingSeconds =
    (YELLOW_DURATION -
     yellowElapsed) / 1000;
}

  if (millis() - yellowTimer >= 500)
  {
    yellowTimer = millis();

    yellowState = !yellowState;

    digitalWrite(YELLOW_PIN, yellowState);
  }

  //////////////////////////////////////////////////////
  // COMPLETE YELLOW AFTER 5 SECONDS
  //////////////////////////////////////////////////////

  if (millis() - signalTimer >= YELLOW_DURATION)
  {
    digitalWrite(YELLOW_PIN, LOW);
    remainingSeconds = 30;

    inYellowMode = false;

    ////////////////////////////////////////////////////
    // SWITCH SIDE
    ////////////////////////////////////////////////////

    currentNS = !currentNS;

    signalTimer = millis();

    ////////////////////////////////////////////////////
    // UPDATE STATUS
    ////////////////////////////////////////////////////

    if (currentNS)
    {
      espSerial.println("<STATUS:NS_GREEN_ACTIVE>");
    }
    else
    {
      espSerial.println("<STATUS:EW_GREEN_ACTIVE>");
    }
  }
}


void manualTrafficControl()
{

    unsigned long elapsed =
millis() - signalTimer;

if(elapsed >= GREEN_DURATION)
{
    remainingSeconds = 0;
}
else
{
    remainingSeconds =
    (GREEN_DURATION - elapsed) / 1000;
}

  static String lastManualStatus = "";

  //////////////////////////////////////////////////////
  // NS GREEN
  //////////////////////////////////////////////////////

  if (manualCommand == "NS_GREEN")
  {
    northSouthGreen();

    if (lastManualStatus != "NS")
    {
      espSerial.println("<STATUS:NS_GREEN_ACTIVE>");
      lastManualStatus = "NS";
    }
  }

  //////////////////////////////////////////////////////
  // EW GREEN
  //////////////////////////////////////////////////////

  else if (manualCommand == "EW_GREEN")
  {
    eastWestGreen();

    if (lastManualStatus != "EW")
    {
      espSerial.println("<STATUS:EW_GREEN_ACTIVE>");
      lastManualStatus = "EW";
    }
  }

  //////////////////////////////////////////////////////
  // ALL RED
  //////////////////////////////////////////////////////

  else if (manualCommand == "ALL_RED")
  {
    allRed();

    if (lastManualStatus != "RED")
    {
      espSerial.println("<STATUS:ALL_RED>");
      lastManualStatus = "RED";
    }
  }

//////////////////////////////////////////////////////////
// YELLOW BLINK
//////////////////////////////////////////////////////////

else if (manualCommand == "YELLOW")
{
    if (lastManualStatus != "YELLOW")
    {
        espSerial.println("<STATUS:YELLOW>");
        lastManualStatus = "YELLOW";
    }

    ////////////////////////////////////////////////////
    // TURN OFF RED & GREEN
    ////////////////////////////////////////////////////

    digitalWrite(NS_GREEN, LOW);
    digitalWrite(NS_RED, LOW);

    digitalWrite(EW_GREEN, LOW);
    digitalWrite(EW_RED, LOW);

    ////////////////////////////////////////////////////
    // BLINK YELLOW
    ////////////////////////////////////////////////////

    if (millis() - manualYellowTimer >= 500)
    {
        manualYellowTimer = millis();

        yellowBlinkState = !yellowBlinkState;

        digitalWrite(YELLOW_PIN,
                     yellowBlinkState);
    }
}
}

void updateCountdown()
{
    static unsigned long lastSecond = 0;

    if (millis() - lastSecond >= 1000)
    {
        lastSecond = millis();

        if (remainingSeconds > 0)
        {
            remainingSeconds--;
        }
    }
}