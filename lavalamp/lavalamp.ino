#include <ArduinoOTA.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #include <Fonts/FreeSans9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#define WIFI_SSID "LavaLamp"
#define WIFI_PASSWORD "lavaislove"


// WebServer
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <uri/UriBraces.h>

#define DEBUG
//#define DEBUG_C
// #define DEBUG_MYDELAY
#define DEBUG_FIRECLICK
#define DEBUG_CHANGEMODE

#define SERIAL_SPEED 9600

/**
 * Lavalamp 3.0 
 * 
 * 2019 by Norival Toniato
 * 
 * Based on Mario Keller's work (http://www.youtube.com/watch?v=6jfAZXOzzec)
 * 
 * Dependencies:
 *  MD_REncoder
 *  FastLED
 * 
 * Hardware: 
 *    - NodeMCU (ESP8266)
 *    - 20 x APA102 RGB LEDs
 *    - 1 Rotary Encoder
 *    - 5V / 2A Powersupply
 *    
 *    NODEMCU PINS
 *    D1 = APA102 - Data
 *    D2 = APA102 - Clock
 *    D5 = Rotary Encoder SW
 *    D6 = Rotary Encoder DT
 *    D7 = Rotary Encoder CLK
 *    
 **/

 
/**** 
 *  Encoder
 */  
#include <MD_REncoder.h>
#define ENCODER_A   D6
#define ENCODER_B   D7
#define ENCODER_SW  D3
#define ENCODER_UP 1
#define ENCODER_DOWN 0
#define ACTION_CLICK 1
#define ACTION_DOUBLECLICK 2
#define ACTION_HOLD 3
#define ACTION_LONGHOLD 4
MD_REncoder R = MD_REncoder(ENCODER_A, ENCODER_B);
#define VALUE_MIN 2
#define VALUE_MAX 254
#define BRIGHTNESS_MIN 2
#define BRIGHTNESS_MAX 250
#define COLOR_MIN 2
#define COLOR_MAX 255
#define ANIMATION_MIN 0
#define ANIMATION_MAX 9

#define ANIMATION_COLOR 0
#define ANIMATION_RAINBOW_ALL 1
#define ANIMATION_RAINBOW_UP 2
#define ANIMATION_RAINBOW_SPIRAL 3
#define ANIMATION_RAINBOW_CLOCKWISE 4
#define ANIMATION_RAINBOW_COUNTERCLOCKWISE 5
#define ANIMATION_LAVALAMP 6 
#define ANIMATION_STROBO 7
#define ANIMATION_STROBO_COLOR 8
#define ANIMATION_WHITE 9

#define SPEED_MIN 1
#define SPEED_MAX 10
#define SPEED_STEP 24
#define ANIMATION_START ANIMATION_RAINBOW_SPIRAL
#define SPEED_START 5
#define BRIGHTNESS_START 160


#define MODE_NONE 0
#define MODE_BRIGHTNESS 1
#define MODE_COLOR 2
#define MODE_ANIMATION 3
#define MODE_SPEED 4

int encoderStep=3;
void ICACHE_RAM_ATTR loopEncoder();



/**** 
 *  Init
 */  
bool isOTASet = false;
int encoderPos = 0;
int encoderPinALast = LOW;
unsigned long lastMillis = 0;
int doubleClickLength = 500;
int debounceLimit = 100;
unsigned long debounceMilis = 0;
int pos = 0;
// Button timing variables
int debounce = 20; // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 250; // max ms between clicks for a double click event
int holdTime = 2000; // ms hold period: how long to wait for press+hold event
int longHoldTime = 5000; // ms long hold period: how long to wait for press+hold event

// Other button variables
boolean buttonVal = HIGH; // value read from button
boolean buttonLast = HIGH; // buffered value of the button's previous state
boolean DCwaiting = false; // whether we're waiting for a double click (down)
boolean DConUp = false; // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true; // whether it's OK to do a single click
long downTime = -1; // time the button was pressed down
long upTime = -1; // time the button was released
boolean ignoreUp = false; // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false; // when held, whether to wait for the up event
boolean holdEventPast = false; // whether or not the hold event happened already
boolean longHoldEventPast = false;// whether or not the long hold event happened already
boolean resetEncoderOnEnd = false;
boolean changeFunction = false;
String displayLine1;
String displayLine2;



#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

#define MODE_MIN 0
#define MODE_MAX 4

//pin 4 is the output pin 
#define DATA_PIN    D4
#define CLOCK_PIN   D5
#define NUM_LEDS    50
#define LED_TYPE    APA102
#define COLOR_ORDER BGR

#define A1 1
#define A2 2

#define KNOB_MODE_VALUE 0
#define KNOB_MODE_BRIGHT 1
#define KNOB_MAX 1

CRGB leds[NUM_LEDS];

int brightness = BRIGHTNESS_START;

int value = 1;
int animation = ANIMATION_START;
int color = COLOR_MIN;
int speed = SPEED_START;
int mode = 0;
bool configuration = false;
String animationName[ANIMATION_MAX+1] = { "Color", "Rainbow", "Up", "Spiral", "Clockwise", "Ctr.Clockwise", "Lava", "Strobo", "Color Strobo", "White" };

WiFiManager wm;

void setupWifi() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
#ifdef DEBUG
  Serial.println ( "setupWifi iniciado" );
#endif  
    bool res;
    res = wm.autoConnect(WIFI_SSID,WIFI_PASSWORD); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        delay( 1000 ); // power-up safety delay
        ESP.restart();
    }   
    IPAddress ip;
    ip = WiFi.localIP();
    displayLine1 = "IP:" +String(ip[0])+'.'+String(ip[1])+'.'+String(ip[2])+'.'+String(ip[3]);
    displayWrite();

#ifdef DEBUG
  Serial.println ( "setupWifi concluido" );
#endif  
}


void setupEnvironment() {
//  pinMode(ENCODER_SW, INPUT);
  pinMode(DATA_PIN, OUTPUT);
  Serial.begin (SERIAL_SPEED);
  delay( 1000 ); // power-up safety delay
#ifdef DEBUG
  Serial.println ( "setupEnvironment concluido" );
#endif  
}

void setBrightness() {
  FastLED.setBrightness( brightness );
  FastLED.show();
#ifdef DEBUG
  Serial.println( "SHOW-setBrightness()" );
  Serial.print ( "Brightness: " );
  Serial.println ( brightness );
#endif  
}

void setupLeds() {
#ifdef DEBUG_V
  Serial.println ( "SetupLeds" );
#endif  
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  setBrightness();
#ifdef DEBUG
  Serial.println ( "setupLeds concluido" );
#endif    
}


void changeAnimation(int an=animation+1) {
  Serial.println ( "changeAnimation" );
  animation = constrainWithinLimits(an, ANIMATION_MIN, ANIMATION_MAX, true);
  changeFunction = true;
  displayWrite();
}

void handleModeChange() {
  changeMode(server.pathArg(0).toInt());
  server.send(200, "text/html", updateWebpage());    
}

void handleAnimationChange() {
  changeAnimation(server.pathArg(0).toInt());
  server.send(200, "text/html", updateWebpage());    
}

void handleBrightnessChange() {
  brightness = constrainWithinLimits(server.pathArg(0).toInt(), BRIGHTNESS_MIN, BRIGHTNESS_MAX, false);
  setBrightness();
  server.send(200, "text/html", updateWebpage());    
}

void handleColorChange() {
  color = constrainWithinLimits(server.pathArg(0).toInt(), COLOR_MIN, COLOR_MAX, false);
  changeFunction = true;
  server.send(200, "text/html", updateWebpage());    
}

void handleSpeedChange() {
  int arg = constrainWithinLimits(server.pathArg(0).toInt(), SPEED_MIN, SPEED_MAX, false);
  speed = map(arg,SPEED_MIN,SPEED_MAX,SPEED_MAX,SPEED_MIN);   

  Serial.print ( "Speed: " );  
  Serial.println ( speed );  
  changeFunction = true;
  server.send(200, "text/html", updateWebpage());    
}

void handleOTA() {
  displayWriteBig("OTA Update");
  configuration=true;
  changeFunction = true;
  server.send(200, "text/html", updateWebpage());    
}

void setupWebServer() {
  server.sendHeader("Pragma", "no-cache");
  server.on("/", handle_OnConnect);
  server.on(UriBraces("/mode/{}"), handleModeChange);
  server.on(UriBraces("/animation/{}"), handleAnimationChange);
  server.on(UriBraces("/brightness/{}"), handleBrightnessChange);
  server.on(UriBraces("/color/{}"), handleColorChange);
  server.on(UriBraces("/speed/{}"), handleSpeedChange);
  server.on(UriBraces("/flash"), handleOTA);
  server.onNotFound(handle_NotFound);
  server.begin();
}

void loopEncoder() {
  uint8_t x = R.read();
  if ((x == DIR_CW) || (x == DIR_CCW)) {
    bool isCW = (x == DIR_CW);
    if (isCW) {
#ifdef DEBUG_V
      Serial.println ( "CW" );
#endif  
      updateParameter(1);
    } else {
#ifdef DEBUG_V
      Serial.println ( "CCW" );
#endif  
      updateParameter(-1);
    }

    if (mode==KNOB_MODE_VALUE) {
      if (resetEncoderOnEnd) {
        if (value > VALUE_MAX) {
          value = VALUE_MIN;
        } else if (value < VALUE_MIN) {
          value = VALUE_MAX;
        }
      } else {
        if (value > VALUE_MAX) {
          value = VALUE_MAX;
        } else if (value < VALUE_MIN) {
          value = VALUE_MIN;
        }
      }
#ifdef DEBUG
      Serial.print ( "Value: " );
      Serial.println ( value );
#endif  
    } else if (mode==KNOB_MODE_BRIGHT) {
      if (brightness > BRIGHTNESS_MAX) {
        brightness = BRIGHTNESS_MAX;
      } else if (brightness < BRIGHTNESS_MIN) {
        brightness = BRIGHTNESS_MIN;
      }
      setBrightness();
    }
  }
}


void updateParameter(int step) {
    switch(mode) {
      case MODE_BRIGHTNESS:
        brightness = constrainWithinLimits(brightness + (step*encoderStep), BRIGHTNESS_MIN, BRIGHTNESS_MAX, false);
        setBrightness();
        break;
      case MODE_COLOR:
        color = constrainWithinLimits(color + step, COLOR_MIN, COLOR_MAX, true);
        changeFunction = true;
#ifdef DEBUG
      Serial.print ( "Color: " );
      Serial.println ( color );
#endif  
        break;
      case MODE_ANIMATION:
        animation = constrainWithinLimits(animation + step, ANIMATION_MIN, ANIMATION_MAX, true);
        changeFunction = true;
#ifdef DEBUG
      Serial.print ( "Animation: " );
      Serial.println ( animation );
#endif  
        break;

      case MODE_SPEED:
        speed = constrainWithinLimits(speed + step, SPEED_MIN, SPEED_MAX, false);
#ifdef DEBUG
      Serial.print ( "Speed: " );
      Serial.println ( speed );
#endif  
        break;
    }

  
}


int constrainWithinLimits(int value, int lowerLimit, int upperLimit, bool cycle) {
  if (value > upperLimit) {
    if (cycle) {
      return lowerLimit;
    }
    return upperLimit;
  }

  if (value < lowerLimit) {
    if (cycle) {
      return upperLimit;
    }
    return lowerLimit;
  }

  return value;
  
}



void setupEncoder() {
  R.begin();
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  pinMode(ENCODER_SW, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), loopEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), loopEncoder, CHANGE);
//  attachInterrupt(digitalPinToInterrupt(ENCODER_SW), checkButton, FALLING);
#ifdef DEBUG
  Serial.println ( "setupEncoder concluido" );
#endif 
}


void setupOTA() {
#ifdef DEBUG
  Serial.println ( "setupOTA iniciado" );
#endif 
  ArduinoOTA.setHostname("OTA-Lava");
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    configuration=false;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();  
#ifdef DEBUG
  Serial.println ( "setupOTA concluido" );
#endif 
}


void setupDisplay() 
{
#ifdef DEBUG
  Serial.println ( "setupDisplay iniciado" );
#endif   
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  // display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.setTextColor(WHITE);
#ifdef DEBUG
  Serial.println ( "setupDisplay concluido" );
#endif   
}


void setup() {
  setupEnvironment();
  setupDisplay();
  setupLeds();
  setupEncoder();
  setupWifi();
  setupWebServer();
#ifdef DEBUG
  Serial.println ( "Setup concluido" );
#endif  
}



void changeMode(int newMode) {
  mode = constrainWithinLimits(newMode, MODE_MIN, MODE_MAX, true);
#if defined(DEBUG_V) || defined(DEBUG_CHANGEMODE)
    Serial.println( "changeMode() IN" );
#endif  
    changeFunction=true;
#if defined(DEBUG_V) || defined(DEBUG_CHANGEMODE)
    Serial.print ( "Mode: " );
    Serial.println ( mode );
#endif  
}

void annimationLoop() {
#ifdef DEBUG_V
    Serial.println ( "annimationLoop()" );
#endif  
  changeFunction = false;
  char msg[2];
  snprintf (msg, 2, "%ld", animation);
  switch(animation) {
    case ANIMATION_COLOR:
        animationColor();
        break;
      case ANIMATION_RAINBOW_ALL:
        rainbowAll();
        break; 
      case ANIMATION_RAINBOW_UP:
        rainbowUp();
        break;
      case ANIMATION_RAINBOW_SPIRAL:
        rainbowSpiral();
        break;
      case ANIMATION_RAINBOW_CLOCKWISE:
        rainbowClockwise();
        break; 
      case ANIMATION_RAINBOW_COUNTERCLOCKWISE:
        rainbowCounterClockwise();
        break;
      case ANIMATION_LAVALAMP:   
        bubbles(); 
        break;
      case ANIMATION_STROBO:
        animationStrobo();     
        break;    
      case ANIMATION_STROBO_COLOR:
        animationStroboColor();     
        break;    
      case ANIMATION_WHITE:
        white();     
        break;    
  }
  // client.loop();
}



void displayWrite() 
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(displayLine1);
  display.setCursor(0,16);
  display.println("Anim:" +animationName[animation]);
  display.display();
}


void displayWriteBig(String message) 
{
  display.setTextSize(2);
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(message);
  display.display();
}




void configutationLoop(){
  // Serial.println( "OTA UPDATE" );
  if (!isOTASet) {
    isOTASet = true;
    setupOTA();
  }

  ArduinoOTA.handle();
  // configuration=false;
  // changeFunction = false;
}

/**
 * just check the current mode and switch to according function
 *
 **/
void loop(){
//  Serial.println( "LOOP" );
  server.handleClient();
  
  if (configuration) {
    configutationLoop();
  } else {
    annimationLoop();
  }
}




void fireClick() {
#if defined(DEBUG_V) || defined(DEBUG_FIRECLICK)
  Serial.println( "fireClick() IN" );
#endif  
  changeAnimation();
  // changeMode(mode+1);
#if defined(DEBUG_V) || defined(DEBUG_FIRECLICK)
  Serial.println( "fireClick() OUT" );
#endif  
}

void fireDoubleClick() {
//  mode++; 
//  if (mode>KNOB_MAX) {
//    mode = 0;
//  } 
//#ifdef DEBUG_V
//  if (mode==KNOB_MODE_VALUE) {
//    Serial.print ( "[2] Value mode" );
//  } else if (mode==KNOB_MODE_BRIGHT) {
//    Serial.print ( "[2] Brightness mode " );
//  }  
//#endif  
//  
}

void fireHold() {
  // configuration=true;
  // changeFunction = true;
#if defined(DEBUG)
  Serial.println( "FireHold" );
#endif  
}

void fireLongHold() {
#if defined(DEBUG)
  Serial.println( "FireLongHold" );
#endif  
  displayWriteBig("WiFi Setup");
  wm.resetSettings();
  delay(1000);
  wm.startConfigPortal();
}

int revertValue() {
  return map(color,0,255,255,2);   
}


void myDelay(int delayLength) {
  server.handleClient();
  
  unsigned long targetMilis;
  targetMilis = millis() + delayLength;
  int event;
  while (targetMilis > millis()) {
    server.handleClient();
    event = checkButton();
    if (event!=0) {
      switch (event) {
      case 1: 
        fireClick();
        break;
      case 2: 
        fireDoubleClick();
        break;
      case 3: 
        fireHold();
        break;
      case 4:
        fireLongHold();
        break;
      }
    }
    delay(1);
  } 
  targetMilis=0;
}



int checkButton() {
  int event = 0;
  // Read the state of the button
  buttonVal = digitalRead(ENCODER_SW);
#if defined(DEBUG_C)
  Serial.print( "buttonVal : " );
  Serial.println( buttonVal );
#endif  

  // Button pressed down
  if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce) {
    downTime = millis();
    ignoreUp = false;
    waitForUp = false;
    singleOK = true;
    holdEventPast = false;
    longHoldEventPast = false;
    if ((millis() - upTime) < DCgap && DConUp == false && DCwaiting == true) DConUp = true;
    else DConUp = false;
    DCwaiting = false;
  }
  // Button released
  else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce) {
    if (not ignoreUp) {
      upTime = millis();
      if (DConUp == false) DCwaiting = true;
      else {
        event = 2;
        DConUp = false;
        DCwaiting = false;
        singleOK = false;
      }
    }
  }
  // Test for normal click event: DCgap expired
  if ( buttonVal == HIGH && (millis() - upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true) {
    event = 1;
    DCwaiting = false;
  }
  // Test for hold
  if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
    // Trigger "normal" hold
    if (not holdEventPast) {
      event = 3;
      waitForUp = true;
      ignoreUp = true;
      DConUp = false;
      DCwaiting = false;
      //downTime = millis();
      holdEventPast = true;
    }
    // Trigger "long" hold
    if ((millis() - downTime) >= longHoldTime) {
      if (not longHoldEventPast) {
        event = 4;
        longHoldEventPast = true;
      }
    }
  }
  buttonLast = buttonVal;
  
  return event;
}

/**
 * function to set a specific LED in the chain to a given value of the HVS color-circle
 *
 * this is simplified version of HVS to RGB comversion
 * see: http://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
 *
 * in all cases saturation and value are "1" so we do not have to calculate all parts
 *
 * we also use a range of 0 to 255 instead of 0 to 360 for HUE
 **/
void setHue2(int h, int pin) {
  //this is the algorithm to convert from RGB to HSV
  double r=0; 
  double g=0; 
  double b=0;

  double hf=h/42.5;

  //calculate with 255 instead of 360° for the whole color-circle so we have to divide by 42.5 instead of 60°
  int i=(int)floor(h/42.5);
  double f = h/42.5 - i;
  double qv = 1 - f;
  double tv = f;

  switch (i)
  {
  case 0: 
    r = 1;
    g = tv;
    break;
  case 1: 
    r = qv;
    g = 1;
    break;
  case 2: 
    g = 1;
    b = tv;
    break;
  case 3: 
    g = qv;
    b = 1;
    break;
  case 4:
    r = tv;
    b = 1;
    break;
  case 5: 
    r = 1;
    b = qv;
    break;
  }

  //set each component to a integer value between 0 and 255
  leds[pin].r = constrain((int)255*r,0,255);
  leds[pin].g = constrain((int)255*g,0,255);
  leds[pin].b = constrain((int)255*b,0,255);

}

/**
 * function to set a specific LED in the chain to concrete R, G and B values
 *
 **/
void setRGB(int r, int g, int b, int pin) {
  //set each component to a integer value between 0 and 255
  leds[pin].r = r;
  leds[pin].g = g;
  leds[pin].b = b;

}

/**
 * set all LEDs to the same color
 **/
void animationColorChangeSet() {
#ifdef DEBUG
  Serial.println ( "animationColor()" );
#endif  
    for(int j = 0; j < NUM_LEDS; j++ ){
      setHue2(color,j); 
    }
    FastLED.show();
#ifdef DEBUG
  Serial.println( "SHOW-animationColor()" );
#endif  
    myDelay(value);
  while(!changeFunction) {
    //only reset LEDs if value has changed
    myDelay(value);
#ifdef DEBUG_V
    Serial.println ( "animationColor()" );
#endif  
  }
}

/**
 * set all LEDs to "white" (same value for all colors)
 **/
void animationWhite() {
#ifdef DEBUG
  Serial.println ( "animationWhite()" );
#endif  
  FastLED.setTemperature( CoolWhiteFluorescent ); // first temperature
  int myValue;
  //run infinite until Animation changes
  for(int j = 0; j < NUM_LEDS; j++ ){
    leds[j] = CRGB::White;  
  }
  FastLED.show();
#ifdef DEBUG
  Serial.println( "SHOW-animationWhite()" );
#endif  
  while(!changeFunction) {
    //only reset LEDs if value has changed
    myDelay(value);
#ifdef DEBUG_V
    Serial.println ( "animationWhite()" );
#endif  
  }
}




// /**
//  * run through the color-circle (HUE value) 
//  * all LEDs are set to the same color value
//  * delay() ist set by the value of the second potentiometer
//  **/
// void rainbowAll()
// { 
// #ifdef DEBUG
//   Serial.println ( "rainbowAll()" );
// #endif  
//   resetEncoderOnEnd = false;
//   while(!changeFunction) {
//       myDelay(revertValue());
// #ifdef DEBUG_V
//       Serial.println ( "rainbowAll()" );
// #endif  
//   }
// }

/**
 * run through the color-circle (HUE value) 
 * every row is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void animationColorChangeAuto(){
#ifdef DEBUG
  Serial.println ( "rainbowUp()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for (int j = 0; j < 255; j++) {
      if (changeFunction) { continue; }
      for (int i = 0; i < NUM_LEDS; i++) {
        if (changeFunction) { continue; }
        setHue2(j,i);        
//        yield();
      }
      FastLED.show();
#ifdef DEBUG
  Serial.println( "SHOW-animationRainbow()" );
#endif  
      myDelay(25*speed);
    }
    myDelay(revertValue()/3);
    if( changeFunction ) continue;
#ifdef DEBUG_V
      Serial.println ( "rainbowUp()" );
#endif  
  }
}

int invertSpeed(int value) {
  return map(value, SPEED_MIN, SPEED_MAX, SPEED_MAX, SPEED_MIN);
}


int roundLimit(int value) {
  if (value >= 255) {
    value = roundLimit(value-255);
  }
  return value;
}


/**
 * run through the color-circle (HUE value) 
 * every row is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void animationRainbow(){
#ifdef DEBUG
  Serial.println ( "animationRainbowWave()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for (int j = 0; j < 255; j++) {
      if (changeFunction) continue;
      for (int i = 0; i < NUM_LEDS; i++) {
        if (changeFunction) {continue;}
        setHue2(roundLimit(j+(i*5)),i);        
      }
      FastLED.show();
#ifdef DEBUG
  Serial.println( "SHOW-animationRainbowWave()" );
#endif  
      myDelay(SPEED_MAX-speed);
    }
//    myDelay(revertValue()/3);
    if( changeFunction ) continue;
#ifdef DEBUG_V
      Serial.println ( "rainbowUp()" );
#endif  
  }
}

/**
 * run through the color-circle (HUE value) 
 * every row is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void animationRainbowWave(){
#ifdef DEBUG
  Serial.println ( "animationRainbowWave()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for (int j = 0; j < 255; j++) {
      if (changeFunction) {continue;}
      for (int i = 0; i < NUM_LEDS; i++) {
      if (changeFunction) {continue;}
        int v = (i * j) % 255;
        setHue2(v,i);        
//          yield();
      }
      FastLED.show();
#ifdef DEBUG
  Serial.println( "SHOW-animationRainbowWave()" );
#endif  
      myDelay(10+speed);
    }
    
    myDelay(revertValue()/3);
    if( changeFunction ) continue;
#ifdef DEBUG_V
      Serial.println ( "rainbowUp()" );
#endif  
  }
}

void handle_OnConnect() {
  server.send(200, "text/html", updateWebpage());   
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String updateWebpage(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #3498db;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>LavaLamp</h1>\n";
  char buffer[40];
  // sprintf(buffer, "<h3>Mode: %i</h3>\n", mode);
  // ptr += buffer;
  // ptr +="<p><a href='/mode/0'>0 - Default</a><p>\n";
  // ptr +="<p><a href='/mode/1'>1 - Brightness</a><p>\n";
  // ptr +="<p><a href='/mode/2'>2 - Color</a><p>\n";
  // ptr +="<p><a href='/mode/3'>3 - Animation</a><p>\n";
  // ptr +="<p><a href='/mode/4'>4 - Speed</a><p>\n";
  sprintf(buffer, "<h3>Animation: %i</h3>\n", animation);
  ptr += buffer;
  ptr +="<p><a href='/animation/0'>0 - Color</a></p>\n";
  ptr +="<p><a href='/animation/1'>1 - Rainbow All</a></p>\n";
  ptr +="<p><a href='/animation/2'>2 - HRainbou Up</a></p>\n";
  ptr +="<p><a href='/animation/3'>3 - Spiral Rainbow</a></p>\n";
  ptr +="<p><a href='/animation/4'>4 - VRainbow Clockwise</a></p>\n";
  ptr +="<p><a href='/animation/5'>5 - VRainbow CounterClockwise</a></p>\n";
  ptr +="<p><a href='/animation/6'>6 - Lava</a></p>\n";
  ptr +="<p><a href='/animation/7'>7 - Strobo</a></p>\n";
  ptr +="<p><a href='/animation/8'>8 - Strobo Color</a></p>\n";
  ptr +="<p><a href='/animation/9'>9 - White</a></p>\n";
  ptr +="<h3>Updates:</h3>\n";
  ptr +="<p><a href='/flash'>Firmware Flash</a></p>\n";

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}


/**
 * set all LEDs to the same color
 **/
void animationColor() {
#ifdef DEBUG
  Serial.println ( "color()" );
#endif  
  resetEncoderOnEnd = true;
  while(!changeFunction) {
    for(int j = 0; j < NUM_LEDS; j++ ){
      setHue2(revertValue(),j); 
    }
    FastLED.show();
    myDelay(speed);
#ifdef DEBUG_V
    Serial.println ( "color()" );
#endif  
  }
}

/**
 * run through the color-circle (HUE value) 
 * all LEDs are set to the same color value
 * delay() ist set by the value of the second potentiometer
 **/
void rainbowAll()
{ 
#ifdef DEBUG
  Serial.println ( "rainbowAll()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for(int i = 0; i < 255; i++) {
      if( changeFunction ) continue;
      for(int j=0;j< NUM_LEDS; j++) {
        if( changeFunction ) continue;
        setHue2(i,j);
        }
      FastLED.show();
      myDelay(speed);
#ifdef DEBUG_V
      Serial.println ( "rainbowAll()" );
#endif  
    }
  }
}

/**
 * run through the color-circle (HUE value) 
 * every row is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbowUp(){
#ifdef DEBUG
  Serial.println ( "rainbowUp()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for(int i = 255; i >= 0; i--) {
      if( changeFunction ) continue;
      for (int col = 0; col < 4; col++) {
        if( changeFunction ) continue;
        for(int row = 0; row < 5; row++) {
          if( changeFunction ) continue;
          int v = ((10*row) + i) % 255;
          setHue2(v,(5*col + row));        
          yield();
        } 
        yield();
      }
      FastLED.show();
      myDelay(speed*10);
      // myDelay(revertValue()/3);
      if( changeFunction ) continue;
#ifdef DEBUG_V
      Serial.println ( "rainbowUp()" );
#endif  
    }
  }
}

/**
 * run through the color-circle (HUE value) 
 * every LED in a row is set to the next color
 * this creates a spiral effect of colors climbing up the lamp
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbowSpiral(){
#ifdef DEBUG
  Serial.println ( "rainbowSpiral()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for(int i = 255; i >= 0; i--) {
      if( changeFunction ) continue;
      int v = i;
      for (int row = 0; row < 5; row++) {
        if( changeFunction ) continue;
        for(int col = 0; col < 4; col++) {
          if( changeFunction ) continue;
          v = (v + 12) % 255;
          setHue2(v,(5*col + row));        
        } 
        FastLED.show();
        myDelay(speed);
        // myDelay(revertValue()/10);
#ifdef DEBUG_V
        Serial.println ( "rainbowSpiral()" );
#endif  
      }
    }
  }
}

/**
 * run through the color-circle (HUE value) 
 * every column is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbowClockwise(){
#ifdef DEBUG
  Serial.println ( "rainbowClockwise()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    //counting down creates a clockwise rotation
    for(int i = 255; i >= 0; i--) {
      if( changeFunction ) continue;
      for (int row = 0; row < 5; row++) {
        if( changeFunction ) continue;
        for(int col = 0; col < 4; col++) {
          if( changeFunction ) continue;
          int v = ((10*col) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }
      FastLED.show();
      myDelay(speed*10);
      // myDelay(revertValue()/3);
#ifdef DEBUG_V
      Serial.println ( "rainbowClockwise()" );
#endif  
    }
  }
}

/**
 * run through the color-circle (HUE value) 
 * every column is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbowCounterClockwise() {
#ifdef DEBUG
  Serial.println ( "rainbowCounterClockwise()" );
#endif  
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    //counting up creates a counter clockwise rotation
    for(int i = 0; i < 256; i++) {
      if( changeFunction ) continue;
      for (int row = 0; row < 5; row++) {
        if( changeFunction ) continue;
        for(int col = 0; col < 4; col++) {
          if( changeFunction ) continue;
          int v = ((10*col) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }
      FastLED.show();
      myDelay(speed*10);
      // myDelay(revertValue()/3);
#ifdef DEBUG_V
      Serial.println ( "rainbowCounterClockwise()" );
#endif  
    }
  }
}

/**
 * create a array of "bubbles" in different colors on all columns
 * the climb up the lamp with different speed
 * speed is controlled by the value of the second potentiometer
 **/
void bubbles() {
#ifdef DEBUG
  Serial.println ( "bubles()" );
#endif  
  resetEncoderOnEnd = false;
  
  //how many bubbles do we need
  #define NUM_BUBBLES 12

  //prepare arrays with column, row, color and speed of each bubble
  byte bubblecol[NUM_BUBBLES];
  byte bubblerow[NUM_BUBBLES];
  byte bubblespeed[NUM_BUBBLES];
  byte bubblehue[NUM_BUBBLES];

  //initialize the bubble-positions
  byte numbubbles = 0;
  for(int i=0;i<NUM_BUBBLES;i++) {
    //columns number of "0" means "inactive" bubble
    bubblecol[i] = 0;
    //starting position is "under" the first row
    bubblerow[i] = -1;
  }
 
  int stepcounter =0;

  //run infinite until mode has changed
  while(!changeFunction) {
    //first check if all bubbles are "active"
    //if not, create a new bubble
    if(numbubbles < NUM_BUBBLES) {
      //find fist "incactive bubble"
      for(int i=0;i<NUM_BUBBLES;i++) {
        if( changeFunction ) continue;
        if(bubblecol[i]==0) {
          //count up number of active bubbles
          numbubbles++;
          //set columns of the new bubble
          bubblecol[i]=random(1,5);
          //start "under" the first row
          bubblerow[i] = -1;
          //set speed
          bubblespeed[i]=random(1,11);
          //set color
          bubblehue[i]=random(0,256);
          break;
        }
      }
    }
    stepcounter++;
    
    //now run to all "active" bubbles an let them climb up one row
    //but only if the should, according to their speed
    for(int i=0;i<NUM_BUBBLES;i++) {
      if( changeFunction ) continue;
      if(bubblecol[i]!=0) {
        if(stepcounter % bubblespeed[i] == 0) {
          bubblerow[i]++;
          //check if bubble has reached upper border and has to be deleted
          if(bubblerow[i] > 4) {
            bubblecol[i] = 0;
            bubblerow[i] = 0;
            numbubbles--;
            setRGB(0,0,0,(5*(bubblecol[i]-1) + 4));
          } 
          else {
            if(bubblerow[i] > 0) setRGB(0,0,0,(5*(bubblecol[i]-1) + bubblerow[i]-1));
            setHue2(bubblehue[i],(5*(bubblecol[i]-1) + bubblerow[i]));
            FastLED.show();
          }
        }
      }
    }
    myDelay(speed*50);
#ifdef DEBUG_V
    Serial.println ( "bubles()" );
#endif  
  }  
}

/**
 * set all LEDs to "white" (same value for all colors)
 **/
void white() {
  // brightness = BRIGHTNESS_START
#ifdef DEBUG
  Serial.println ( "white()" );
#endif  
  FastLED.setTemperature( Candle ); // first temperature
  int myValue;

  
  //run infinite until mode changes
  for(int j = 0; j < NUM_LEDS; j++ ){
    leds[j] = CRGB::White;  
  }
  FastLED.show();
  while(!changeFunction) {
    //only reset LEDs if value has changed
    myDelay(1000);
#ifdef DEBUG_V
    Serial.println ( "white()" );
#endif  
  }
}


/**
 * set all LEDs to "white" (same value for all colors)
 **/
void animationStrobo() {
#ifdef DEBUG
  Serial.println ( "randomWhite()" );
#endif  
  int initialBrigtness;
  initialBrigtness = brightness;
  brightness = BRIGHTNESS_MAX;
  setBrightness( );
  while(!changeFunction) {
    for(int j = 0; j < NUM_LEDS; j++ ){
      leds[j] = CRGB::Black;
    }
    FastLED.show();
    leds[random(0, NUM_LEDS-1)] = CRGB::White;
    FastLED.show();
    myDelay(speed*20);
#ifdef DEBUG_V
    Serial.println ( "randomWhite()" );
#endif  
  }
  brightness = initialBrigtness;
  setBrightness( );
  
}

/**
 * set all LEDs to "white" (same value for all colors)
 **/
void animationStroboColor() {
#ifdef DEBUG
  Serial.println ( "randomWhite()" );
#endif  
  int initialBrigtness;
  initialBrigtness = brightness;
  brightness = BRIGHTNESS_MAX;
  setBrightness( );
  while(!changeFunction) {
    for(int j = 0; j < NUM_LEDS; j++ ){
      leds[j] = CRGB::Black;
    }
    FastLED.show();
    setHue2(random(COLOR_MIN, COLOR_MAX),random(0, NUM_LEDS-1));    
    FastLED.show();
    myDelay(speed*20);

#ifdef DEBUG_V
    Serial.println ( "randomWhite()" );
#endif  
  }
  brightness = initialBrigtness;
  setBrightness( );
  
}


