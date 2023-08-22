#include <SoftwareSerial.h>
//AT+SLEEP
//to wake up:
// way 1: Click the button on the board.
// way 2: Give "EN" PIN a high level (over 100ms).

struct Time {
  unsigned long millis;
  unsigned long second;
  unsigned long minute;
  unsigned long hour;

  void update(unsigned long _millis) {
    millis += _millis;
    second = (millis / 1000) % 60;
    minute = (millis / 60000) % 60;
    hour = (millis / 3600000) % 24;
  }
  void set(unsigned long _millis) {
    millis = _millis;
    second = (millis / 1000) % 60;
    minute = (millis / 60000) % 60;
    hour = (millis / 3600000) % 24;
  }
};


//PINS---------------------------------------------------------------
#define WATER_PUMP_PIN 9
#define LED_R_PIN 6
#define LED_G_PIN 5
#define LED_B_PIN 3
//CHAR---------------------------------------------------------------
#define END_CHAR '\n'
#define SPLIT_CHAR ';'
//MODES---------------------------------------------------------------
#define COLOR_MODE_STATIC 0
#define COLOR_MODE_RAINBOW 1

#define SET_LIGHT 0
#define SET_COLOR_MODE 1
#define SET_COLOR 2
#define SET_WATER 3
#define SET_WATER_SPEED 4
#define SET_TIMER 5
#define GET_STATE 6
#define GET_COLOR 7

//TXD = 10 | RXD = 11
SoftwareSerial bluetoothSerial(10, 11);


String message = "";


boolean lightsOn = false;
int currColorMode = 0;
int red = 0;
int green = 0;
int blue = 0;

int rainbowColorDelay = 200;
int rainbowColorHue = 0;

boolean waterpumpOn = false;
int waterpumSpeed = 0;

//millis diff between one loop
unsigned long deltaMillis = 0;
//needed to calc the deltaMillis
unsigned long lastMillis = 0;
unsigned long lastColorModeUpdate = 0;

boolean shutdowmTimerOn = false;
long shutdownMillis = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Los geht's");
  bluetoothSerial.begin(9600);

  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);

  lightsOn = true;
  setColor(127, 127, 127);
}
void calcDeltaMillis() {
  deltaMillis = millis() - lastMillis;
  lastMillis = millis();
}
void updateShutdownTimer() {
  if (!shutdowmTimerOn) {
    return;
  }

  shutdownMillis -= deltaMillis;
  // Serial.println(shutdownMillis);
  if (shutdownMillis <= 0) {
    Serial.println("Shutdown...");

    shutdownMillis = 0;
    shutdowmTimerOn = false;
    setLight(false);
    setWater(false);
    setColorMode(0);

    delay(150);
    getState();
  }

  // delay(500);
  // String setTimer = (String)SET_TIMER + SPLIT_CHAR + (String)shutdownMillis;
  // bluetoothSerial.println(setTimer);
}
void loop() {
  calcDeltaMillis();
  // currTime.update(deltaMillis);
  readBluetooth();
  updateShutdownTimer();
  updateColorMode();
}
void updateColorMode() {
  if (currColorMode != COLOR_MODE_RAINBOW) {
    return;
  }
  if (millis() - lastColorModeUpdate < rainbowColorDelay) {
    return;
  }
  lastColorModeUpdate = millis();
  rainbowColorHue++;
  if (rainbowColorHue >= 256) {
    rainbowColorHue = 0;
  }
  int r, g, b;
  hueToRGB(rainbowColorHue, r, g, b);
  setColor(r, g, b);

  // String setColor = (String)SET_COLOR + SPLIT_CHAR + (String)red + SPLIT_CHAR + (String)green + SPLIT_CHAR + (String)blue;
  // bluetoothSerial.println(setColor);
}
void readBluetooth() {
  if (bluetoothSerial.available()) {
    char c = bluetoothSerial.read();

    if (c == END_CHAR) {
      //end of message
      message = message.substring(0, message.length());
      computeMessage();
      message = "";
      bluetoothSerial.flush();
      return;
    }
    message += c;
  }
}
/*
  * COMMAND + SPLITCHAR + PARAMETER + SPLITCHAR + PARAMETER + END_CHAR
  * Example:
  * SetColor to red
  * "0;255;0;0\n"
  * 0 = setColor; R; G, B \ncommand ended
*/
void computeMessage() {
  Serial.println("Message:");
  Serial.println(message);
  // bluetoothSerial.println("response to: " + message);
  // return;

  int MAX_BT_INPUT_COUNT = 20;
  String substrings[MAX_BT_INPUT_COUNT];
  int substringCount = splitString(message, SPLIT_CHAR, substrings, MAX_BT_INPUT_COUNT);
  int intInputs[substringCount];

  for (int i = 0; i < substringCount; i++) {
    // Serial.println(substrings[i]);
    if (isDigit(substrings[i])) {
      intInputs[i] = substrings[i].toInt();
      // Serial.println(intInputs[i]);
    } else {
      Serial.print("Not a number: ");
      // Serial.println(substrings[i]);
    }
  }

  if (substringCount < 1) {
    // Serial.println("substringCount < 2;");
    return;
  }

  int command = intInputs[0];
  // Serial.print(command);
  // Serial.print(" : ");
  // Serial.println(GET_STATE);
  if (command == SET_LIGHT) {
    if (substringCount != 2) return;
    setLight(intInputs[1]);
    Serial.println("Set Light");
  } else if (command == SET_COLOR_MODE) {
    if (substringCount != 2) return;
    setColorMode(intInputs[1]);
    Serial.println("Set Color Mode");
  } else if (command == SET_COLOR) {
    if (substringCount != 4) return;
    if (currColorMode == COLOR_MODE_RAINBOW) {
      setColorMode(COLOR_MODE_STATIC);
      String setColorMode = (String)SET_COLOR_MODE + SPLIT_CHAR + (String) + currColorMode;
      bluetoothSerial.println(setColorMode);
      delay(150);
    }
    setColor(intInputs[1], intInputs[2], intInputs[3]);
    Serial.println("Set Color");
  } else if (command == SET_WATER) {
    if (substringCount != 2) return;
    setWater(intInputs[1]);
    Serial.println("Set Water");
  } else if (command == SET_WATER_SPEED) {
    if (substringCount != 2) return;
    setWaterSpeed(intInputs[1]);
    Serial.println("Set Water Speed");
  } else if (command == SET_TIMER) {
    if (substringCount != 2) return;
    //weil intInputs[1] falsch ist müssen wir es neu berechnen
    //es ist falsch weil wir einen int parsen und keinen long der int kann nicht die Zahl speichern
    unsigned long value = strtoul(substrings[1].c_str(), NULL, 10);

    setTimer(value);
    Serial.println("Set Timer");
  } else if (command == GET_STATE) {
    if (substringCount != 1) return;
    delay(100);
    getState();
    Serial.println("getState");
  } else if (command == GET_COLOR) {
    if (substringCount != 1) return;
    String setColor = (String)SET_COLOR + SPLIT_CHAR + (String)red + SPLIT_CHAR + (String)green + SPLIT_CHAR + (String)blue;
    bluetoothSerial.println(setColor);
    Serial.println("getColor");
  }

  // Serial.println("Splited String:");
  // for (int i = 0; i < substringCount; i++) {
  //   Serial.println(substrings[i]);
  //   if (isDigit(substrings[i])) {
  //     intInputs[i] = substrings[i].toInt();
  //   } else {
  //     Serial.print("Not a number: ");
  //     Serial.println(substrings[i]);
  //   }
  // }
}
void getState() {
  String setLight = (String)SET_LIGHT + SPLIT_CHAR + (String)lightsOn;
  bluetoothSerial.println(setLight);
  delay(150);
  String setColorMode = (String)SET_COLOR_MODE + SPLIT_CHAR + (String) + currColorMode;
  bluetoothSerial.println(setColorMode);
  delay(150);
  String setColor = (String)SET_COLOR + SPLIT_CHAR + (String)red + SPLIT_CHAR + (String)green + SPLIT_CHAR + (String)blue;
  bluetoothSerial.println(setColor);
  delay(150);
  String setWater = (String)SET_WATER + SPLIT_CHAR + (String)waterpumpOn;
  bluetoothSerial.println(setWater);
  delay(150);
  String setWaterSpeed = (String)SET_WATER_SPEED + SPLIT_CHAR + (String)waterpumSpeed;
  bluetoothSerial.println(setWaterSpeed);
  delay(150);
  String setTimer = (String)SET_TIMER + SPLIT_CHAR + (String)shutdownMillis;
  bluetoothSerial.println(setTimer);
  delay(150);
  bluetoothSerial.println("");
  bluetoothSerial.flush();
}
void setColor(int r, int g, int b) {
  // if (currColorMode == COLOR_MODE_RAINBOW) {
  //   currColorMode = COLOR_MODE_STATIC;
  // }BUG!!
  red = r;
  green = g;
  blue = b;
  analogWrite(LED_R_PIN, red);
  analogWrite(LED_G_PIN, green);
  analogWrite(LED_B_PIN, blue);
  // Serial.println((String)r + SPLIT_CHAR + (String)g + SPLIT_CHAR + (String)b);
  // bluetoothSerial.println((String)r + SPLIT_CHAR + (String)g + SPLIT_CHAR + (String)b);
}

void setColorMode(int mode) {
  currColorMode = mode;
  String setColorMode = (String)SET_COLOR_MODE + SPLIT_CHAR + (String) + currColorMode;
  bluetoothSerial.println(setColorMode);
  delay(150);
}

void setLight(bool state) {
  lightsOn = state;
  if(currColorMode == COLOR_MODE_RAINBOW){
    currColorMode = COLOR_MODE_STATIC;
  }
  if (lightsOn) {
    analogWrite(LED_R_PIN, red);
    analogWrite(LED_G_PIN, green);
    analogWrite(LED_B_PIN, blue);
  } else {
    analogWrite(LED_R_PIN, 0);
    analogWrite(LED_G_PIN, 0);
    analogWrite(LED_B_PIN, 0);
  }
}

void setWater(bool state) {
  waterpumpOn = state;
  if (waterpumpOn) {
    analogWrite(WATER_PUMP_PIN, waterpumSpeed);
  } else {
    analogWrite(WATER_PUMP_PIN, 0);
  }
}

void setWaterSpeed(int value) {
  waterpumSpeed = value;
  analogWrite(WATER_PUMP_PIN, value);
}

void setTimer(unsigned long millis) {
  shutdownMillis = millis;
  shutdowmTimerOn = millis != 0;
  String setTimer = (String)SET_TIMER + SPLIT_CHAR + (String)shutdownMillis;
  bluetoothSerial.println(setTimer);
  delay(150);
}

int splitString(const String& input, char delimiter, String* output, int maxCount) {
  int substringCount = 0;
  int startIndex = 0;
  int endIndex = 0;

  while (endIndex >= 0 && substringCount < maxCount - 1) {
    endIndex = input.indexOf(delimiter, startIndex);

    if (endIndex >= 0) {
      output[substringCount] = input.substring(startIndex, endIndex);
      startIndex = endIndex + 1;
      substringCount++;
    }
  }

  // Handle the last substring
  if (substringCount < maxCount) {
    output[substringCount] = input.substring(startIndex);
    substringCount++;
  }

  return substringCount;
}
bool isDigit(String text) {
  for (size_t i = 0; i < text.length(); i++) {
    if (!isdigit(text.charAt(i))) {
      return false;
    }
  }
  return true;
}
void hueToRGB(int hue, int& r, int& g, int& b) {
  float angle = 2 * PI * hue / 255.0;
  r = int((sin(angle + 2 * PI / 3) + 1) * 127);
  g = int((sin(angle) + 1) * 127);
  b = int((sin(angle - 2 * PI / 3) + 1) * 127);
}
