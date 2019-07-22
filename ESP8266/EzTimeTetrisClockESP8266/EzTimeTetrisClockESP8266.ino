/*******************************************************************
    Tetris clock that fetches its time Using the EzTimeLibrary

    NOTE: THIS IS CURRENTLY CRASHING!

    For use with an ESP8266
 *                                                                 *
    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// ----------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ----------------------------

#include <Ticker.h>
#include <ESP8266WiFi.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------


#include <PxMatrix.h>
// The library for controlling the LED Matrix
// Can be installed from the library manager
// https://github.com/2dom/PxMatrix

// Adafruit GFX library is a dependancy for the PxMatrix Library
// Can be installed from the library manager
// https://github.com/adafruit/Adafruit-GFX-Library

#include <TetrisMatrixDraw.h>
// This library :)
// https://github.com/toblum/TetrisAnimation

#include <ezTime.h>
// Library used for getting the time and adjusting for DST
// Search for "ezTime" in the Arduino Library manager
// https://github.com/ropg/ezTime

#include <ArduinoOTA.h>

// ---- Stuff to configure ----

// Uncomment the line below to connect to an Android Hotspot
//#define AndroidHotSpot

#ifdef AndroidHotSpot
// Initialize Wifi connection to Android HotSpot
const char* ssid = "AndroidAP";
const char* password = "abba0ea30d11";
#else
// Initialize Wifi connection to the router
const char* ssid = "radar";
const char* password = "0613311164659195";
#define EnableMQTT
#endif

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

// Initialize MQTT
#ifdef EnableMQTT
#include<PubSubClient.h>
const char* mqttServer = "192.168.1.49";
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char* listenTopic = "study/clock/#";
const char* brightnessTopic = "study/clock/brightness";
const char* speedTopic = "study/clock/speed";
#endif


// Set a timezone using the following list
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
#define MYTIMEZONE "Australia/Sydney"
Timezone myTZ;

// Sets whether the clock should be 12 hour format or not.
bool twelveHourFormat = true;

// If this is set to false, the number will only change if the value behind it changes
// e.g. the digit representing the least significant minute will be replaced every minute,
// but the most significant number will only be replaced every 10 minutes.
// When true, all digits will be replaced every minute.
bool forceRefresh = false;
// -----------------------------

// ----- Wiring -------
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_OE 2
#define P_D 12
#define P_E 0
// ---------------------

// ----- Matrix Parameters -----
#define matrixWidth 64
#define matrixHeight 64
#define matrixRotate false
#define matrixScale 2

#define scaledHeight matrixWidth / matrixScale
// -----------------------------

Ticker display_ticker;
Ticker timer_ticker;
Ticker time_ticker;

// PxMATRIX display(matrixWidth, matrixHeight, P_LAT, P_OE,P_A,P_B,P_C);
//PxMATRIX display(matrixWidth,matrixHeight, P_LAT, P_OE,P_A,P_B,P_C,P_D);
PxMATRIX display(matrixWidth, matrixHeight, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

TetrisMatrixDraw tetris(display); // Main clock
TetrisMatrixDraw tetris2(display); // The "M" of AM/PM
TetrisMatrixDraw tetris3(display); // The "P" or "A" of AM/PM

bool showColon = true;
bool finishedAnimating = false;
bool displayIntro = true;

String lastDisplayedTime = "";
String lastDisplayedAmPm = "";


// This method is needed for driving the display
void display_updater() {
  display.display(30);
}

// This method is for controlling the tetris library draw calls
void animationHandler()
{ 
  if (!finishedAnimating) {
    display.clearDisplay();
    if (displayIntro) {
      finishedAnimating = tetris.drawText(1, 21);
    } else {
      if (twelveHourFormat) {
        // Place holders for checking are any of the tetris objects
        // currently still animating.
        bool tetris1Done = false;
        bool tetris2Done = false;
        bool tetris3Done = false;

        tetris1Done = tetris.drawNumbers(-6, scaledHeight, showColon);
        tetris2Done = tetris2.drawText(56, matrixHeight - 1);

        // Only draw the top letter once the bottom letter is finished.
        if (tetris2Done) {
          tetris3Done = tetris3.drawText(56, matrixHeight - 11);
        }

        finishedAnimating = tetris1Done && tetris2Done && tetris3Done;

      } else {
        finishedAnimating = tetris.drawNumbers(2, scaledHeight, showColon);
      }
    }

  }
}

void drawIntro(int x = 0, int y = 0)
{
  tetris.drawChar("W", x, y, tetris.tetrisCYAN);
  tetris.drawChar("r", x += 5, y, tetris.tetrisMAGENTA);
  tetris.drawChar("i", x += 4, y, tetris.tetrisYELLOW);
  tetris.drawChar("t", x += 4, y, tetris.tetrisGREEN);
  tetris.drawChar("t", x += 5, y, tetris.tetrisBLUE);
  tetris.drawChar("e", x += 5, y, tetris.tetrisRED);
  tetris.drawChar("n", x += 5, y, tetris.tetrisWHITE);
  tetris.drawChar(" ", x += 5, y, tetris.tetrisMAGENTA);
  tetris.drawChar("b", x += 5, y, tetris.tetrisYELLOW);
  tetris.drawChar("y", x += 5, y, tetris.tetrisGREEN);
}

void drawConnecting(int x = 0, int y = 0)
{
  tetris.drawChar("C", x, y, tetris.tetrisCYAN);
  tetris.drawChar("o", x += 5, y, tetris.tetrisMAGENTA);
  tetris.drawChar("n", x += 5, y, tetris.tetrisYELLOW);
  tetris.drawChar("n", x += 5, y, tetris.tetrisGREEN);
  tetris.drawChar("e", x += 5, y, tetris.tetrisBLUE);
  tetris.drawChar("c", x += 5, y, tetris.tetrisRED);
  tetris.drawChar("t", x += 5, y, tetris.tetrisWHITE);
  tetris.drawChar("i", x += 4, y, tetris.tetrisMAGENTA);
  tetris.drawChar("n", x += 4, y, tetris.tetrisYELLOW);
  tetris.drawChar("g", x += 5, y, tetris.tetrisGREEN);
}

void setup() {
  Serial.begin(115200);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  //-------- WIFI RECONNECTION CODE ----------
  /* If not done correctly, reconnecting to WiFi can cause a very noticable blink on the screen
   *  due to the delay in re-establishing the connection. The following code overcomes this issue.
   *  The code was obtained from https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-examples.html
  */
  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
    Serial.print("Station connected, IP: ");
    Serial.println(WiFi.localIP());
  });
  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    Serial.println("Station disconnected");
  });
  //------ END WIFI RECONNECTION CODE ------
  
  
  // Set WiFi to station mode and disconnect from an AP if it was Previously connected  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

//  check_wifi_connection = millis() + check_wifi_delay;

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  #ifdef EnableMQTT
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(callback);
  #endif

  // Do not set up display before WiFi connection
  // as it will crash!
  delay(5000);

  // Intialise display library
  display.begin(32);  //16
  display.setRotate(matrixRotate);
  display.setBrightness(25);  // 0 - 255
  display.setFastUpdate(false);  // true - false
  display.clearDisplay();

  // Setup ticker for driving display
  display_ticker.attach(0.002, display_updater);
  yield();
  display.clearDisplay();

  // "connecting"
  drawConnecting(6, 10);

  // Setup EZ Time
  setDebug(INFO);
  int syncAttemptCount = 0;
  do {
    syncAttemptCount++;
    Serial.print("Syncing to time server .... Attempt ");
    Serial.println(syncAttemptCount);
    } while ((!waitForSync(10)) && (syncAttemptCount < 6));

  Serial.println();
  Serial.println("UTC:             " + UTC.dateTime());

  int zoneAttemptCount = 0;
  do {
    zoneAttemptCount++;
    Serial.print("Setting time zone .... Attempt ");
    Serial.println(zoneAttemptCount);
    } while ((!myTZ.setLocation(MYTIMEZONE)) && (zoneAttemptCount < 6));

  Serial.print(F("Time in your set timezone:         "));
  Serial.println(myTZ.dateTime());
  
  display.clearDisplay();
   //"Powered By"
  drawIntro(6, 12);
  delay(2000);

  // Start the Animation Timer
  tetris.setText("B. LOUGH");
  timer_ticker.attach(0.06, animationHandler);

  // Wait for the animation to finish
  while (!finishedAnimating)
  {
    delay(10); //waiting for intro to finish
  }
  delay(2000);
  //timer_ticker.attach(0.1, setAnimateFlag);
  finishedAnimating = false;
  displayIntro = false;
  tetris.scale = 2;

//  //Check every second for time changes and flash cursor.
//  time_ticker.attach(1, tickTock);

  //Force a call to setMatrixTime to display the initial time.
  setMatrixTime();

  // Set up OTA updates
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("TetrisClock");
  

  // No authentication by default
  //ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setMatrixTime() {
  String timeString = "";
  String AmPmString = "";
  if (twelveHourFormat) {
    // Get the time in format "1:15" or 11:15 (12 hour, no leading 0)
    // Check the EZTime Github page for info on
    // time formatting
    timeString = myTZ.dateTime("g:i");

    //If the length is only 4, pad it with
    // a space at the beginning
    if (timeString.length() == 4) {
      timeString = " " + timeString;
    }

    //Get if its "AM" or "PM"
    AmPmString = myTZ.dateTime("A");
    if (lastDisplayedAmPm != AmPmString) {
      Serial.println(AmPmString);
      lastDisplayedAmPm = AmPmString;
      // Second character is always "M"
      // so need to parse it out
      tetris2.setText("M", forceRefresh);

      // Parse out first letter of String
      tetris3.setText(AmPmString.substring(0, 1), forceRefresh);
    }
  } else {
    // Get time in format "01:15" or "22:15"(24 hour with leading 0)
    timeString = myTZ.dateTime("H:i");
  }
  tetris.setTime(timeString, forceRefresh);
  finishedAnimating = false;
}

void loop() {
  // Check for an OTA update
  ArduinoOTA.handle();
  delay(10);

  // Required by ezTime library
  events();
//  if (hourChanged()) {
//    check_wifi_connection = true;
//    Serial.println("Hour changed");
//  }
  if (minuteChanged()) setMatrixTime();
  if (secondChanged()) flashCursor();

#ifdef EnableMQTT
  if (!mqttClient.connected()) {
    Serial.println("Reconnecting to MQTT Server");
    reconnect();
  }
  
  mqttClient.loop();
#endif
  
}

void setBrightness(uint8_t brightness) {
    Serial.print("Setting display brightness to ");
    Serial.println(brightness);
    if (brightness > 254) brightness = 254;
    display.setBrightness(brightness);
}

void setDropSpeed(float dropSpeed) {
    Serial.print("Setting drop speed to ");
    Serial.println(dropSpeed);
    timer_ticker.detach();
    timer_ticker.attach(dropSpeed, animationHandler);  
}

void flashCursor() {
  now();  //Required by ezTime library to internally track time changes. https://github.com/ropg/ezTime/issues/53
  if (showColon) {
    //tetris.drawColon(2,32, tetris.tetrisWHITE);  //24 Hour format
    tetris.drawColon(-6, scaledHeight, tetris.tetrisWHITE);   //12 Hour format
  }
  else {
    //tetris.drawColon(2,32, tetris.tetrisBLACK);  //24 Hour format
    tetris.drawColon(-6, scaledHeight, tetris.tetrisBLACK);   //12 Hour format
  }
  showColon = !showColon;
}

#ifdef EnableMQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.println("]");
  String payloadStr;
  for (int i=0;i<length;i++) {
    payloadStr += (char)payload[i];
  }

  String topicStr = topic;
  
  if (topicStr == brightnessTopic) {
    
    // ---- Convert payload to uint8_t for the setBrightness() function -----
    // http://github.com/knolleary/pubsubclient/issues/105#issuecomment-168538000
    payload[length] = '\0';
    String s = String((char*)payload);
    uint8_t brightness = s.toInt();
    // ----------------------------
   
    setBrightness(brightness);
  }

  if (topicStr == speedTopic) {
    // set the drop speed to a new value

        // ---- Convert payload to uint8_t for the setBrightness() function -----
    // http://github.com/knolleary/pubsubclient/issues/105#issuecomment-168538000
    payload[length] = '\0';
    String s = String((char*)payload);
    float dropSpeed = s.toFloat()/100;
    // ----------------------------
    
    setDropSpeed(dropSpeed);
  }
}
#endif

#ifdef EnableMQTT
void reconnect() {
  // Loop until a connection is made
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection ...");
    if (mqttClient.connect("Tetris Clock")) {
      Serial.println("connected");
      mqttClient.subscribe(listenTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      Serial.println("Try again in 5 seconds");
      //Wait 5 seconds before retring
      delay(5000);
    }
  }
}
#endif
