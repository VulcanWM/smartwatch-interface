#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT20.h"
#include <EEPROM.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// inputs
int rotaryPin = 0;
int buttonPin = 6;
DHT20 DHT;
const int soundPin = A2;

// outputs
int buzzerPin = 5;
int ledPin = 4;

// full program global variables
bool loggedIn = false;
int buttonState = LOW;
bool heldDown = false; // to prevent it acting multiple times on the same button press

// initialising lock screen
char password[5] = "0000";
int currentIndex = 0;
int digits[4] = {-1, -1, -1, -1};

// to prevent it redisplaying content multiple times
char screenDisplay[50] = "_ _ _ _ ";
int previousTemperature = -1000;
int previousHumidity = -1000;

// lockdown global variables
int incorrectAttempts = 0;
bool inLockdown = false;
unsigned long lockdownStartTime = 0;
unsigned long lockdownFor = 0;

// global variables for the sound level
int maxSoundLevel;
const int soundThresholdAddress = 0;
int previousSoundLevel = -1000;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void default_screen_setup(int textSize){
  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(textSize);        // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,0);              // Start at top-left corner
}

void buzz_multiple_times(int times, int frequency, int time_during, int time_between){
  for (int i = 0; i < times; i++){
    tone(buzzerPin, frequency);
    delay(time_during);
    noTone(buzzerPin);
    if (i + 1 != times){
      delay(time_between);
    }
  }
}

void setup() {
  // setting up inputs and outputs
  pinMode(rotaryPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(soundPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);

  // gets sound level from EEPROM
  EEPROM.get(soundThresholdAddress, maxSoundLevel);

  if (maxSoundLevel < 0 || maxSoundLevel > 1023) {
    maxSoundLevel = 500;
    EEPROM.put(soundThresholdAddress, maxSoundLevel);
  }
  

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // default screen settings
  default_screen_setup(2);
  display.println(F("_ _ _ _"));
  display.display();
}

void loop() {
  if (loggedIn == true){
    // main program once logged in
    int rotaryState = analogRead(rotaryPin);

    int number = fmin(rotaryState / 200, 5);
    if (number == 0){
      // clock face

      // calculates the time based on when it started (start time is 12:00)
      unsigned long totalSeconds = millis() / 1000;
      int hours = 12 + totalSeconds / 3600;
      int minutes = (totalSeconds % 3600) / 60;
      hours %= 24;

      // generates time display screen
      char timeDisplay[6];
      snprintf(timeDisplay, sizeof(timeDisplay), "%02d:%02d", hours, minutes);
      char fullDisplay[20];
      snprintf(fullDisplay, sizeof(fullDisplay), "Hi! %s", timeDisplay);

      if (strcmp(fullDisplay, screenDisplay) != 0){
        // shows new time if it has changed since last display
        default_screen_setup(3);
        display.println("Hi!");
        display.print(timeDisplay);
        display.display();
        strcpy(screenDisplay, fullDisplay);
      }
    } else if (number == 1) {
      // stopwatch
      if (strcmp(screenDisplay, "Stopwatch") != 0){
        default_screen_setup(2);
        display.println(F("Stopwatch"));
        display.display();
        strcpy(screenDisplay, "Stopwatch");
      }
    } else if (number == 2) {
      // timer
      if (strcmp(screenDisplay, "Timer") != 0){
        default_screen_setup(2);
        display.println(F("Timer"));
        display.display();
        strcpy(screenDisplay, "Timer");
      }
    } else if (number == 3) {
      if (DHT.read() == DHT20_OK) {
        // if the reading is sucessful
        float temperature = DHT.getTemperature();
        float humidity = DHT.getHumidity();

        if (temperature != previousTemperature ||
          humidity != previousHumidity ||
          strcmp(screenDisplay, "Temperature") != 0) {
          // if the temperature and humidity are different from before

          default_screen_setup(2);
          display.print(F("Temperature: "));
          display.print(temperature);
          display.println(F(" C"));

          display.print(F("Humidity: "));
          display.print(humidity);
          display.println(F(" %"));

          display.display();

          previousTemperature = temperature;
          previousHumidity = humidity;
          strcpy(screenDisplay, "Temperature");
        }
      }
      delay(100);
    }
    else if (number == 4){
      // sound monitor
      int soundLevel = analogRead(soundPin);

      if (soundLevel != previousSoundLevel ||
          strcmp(screenDisplay, "Sound") != 0) {
        // if the sound level is different from before
        default_screen_setup(2);
        display.print(F("Sound Level: "));
        display.println(soundLevel);

        if (soundLevel > maxSoundLevel){
          display.println("Stop shouting!");
        }

        display.display();

        previousSoundLevel = soundLevel;
        strcpy(screenDisplay, "Sound");
      }
      delay(100);
    } else if (number == 5){
      // settings
      if (strcmp(screenDisplay, "Settings") != 0){
        default_screen_setup(2);
        display.println(F("Settings"));
        display.display();
        strcpy(screenDisplay, "Settings");
      }
    }
  } else {
    if (inLockdown == true){
      // calculating time left
      unsigned long elapsed = millis() - lockdownStartTime;
      long secondsLeft = (long)((lockdownFor - elapsed) / 1000);

      char lockdownDisplayText[50];
      snprintf(lockdownDisplayText, sizeof(lockdownDisplayText),
         "Lockdown. Will unlock in %ld seconds", secondsLeft);

      if (strcmp(screenDisplay, lockdownDisplayText) != 0){
        // displaying lockdown screen if seconds have changed
        default_screen_setup(2);
        display.println(F("Lockdown."));
        display.print(F("Will unlock in "));
        display.print(secondsLeft);
        display.print(F(" seconds"));
        display.display();
        strcpy(screenDisplay, lockdownDisplayText);

        // removes lockdown once time has ended
        if (elapsed >= lockdownFor) {
          inLockdown = false;
        }
      }
    } else {
      // needing to enter pin
      int rotaryState = analogRead(rotaryPin);
      buttonState = digitalRead(buttonPin);

      // getting pin number from rotary
      int number = fmin(rotaryState / 100, 9);

      // generating display string for screen
      char displayString[9];
      for (int i = 0; i < 4; i++){
        if (currentIndex == i){
          displayString[i*2] = '0' + number;
        } else if (digits[i] == -1){
          displayString[i*2] = '_';
        } else {
          displayString[i*2] = '0' + digits[i];
        }
        displayString[i*2+1] = ' ';
      }
      displayString[8] = '\0';

      // displays need display string if has changed since last time
      if (strcmp(displayString, screenDisplay) != 0){
        default_screen_setup(2);
        display.println(displayString);
        display.display();
        strcpy(screenDisplay, displayString);
      }

      if (buttonState == HIGH && heldDown == false){
        // if button has just been held down
        if (currentIndex == 3){
          // if last digit has been entered
          digits[currentIndex] = number;
          heldDown = true;
          if (digits[0] == password[0] - '0' && 
            digits[1] == password[1] - '0' &&
            digits[2] == password[2] - '0' &&
            digits[3] == password[3] - '0'){
            buzz_multiple_times(1, 1000, 500, 500);
            // if correct password
            loggedIn = true;
            incorrectAttempts = 0;
          } else {
            // if password is incorrect
            for (int i = 0; i < 4; i++) digits[i] = -1;
            currentIndex = 0;
            buzz_multiple_times(3, 200, 100, 50);
            incorrectAttempts += 1;
            if (incorrectAttempts >= 3){
              // if has entered incorrect password too many times
              inLockdown = true;
              lockdownFor = (incorrectAttempts - 2) * 60000;
              lockdownStartTime = millis();
            }
          }
        } else {
          // if entered digit is not last digit to enter
          digits[currentIndex] = number;
          currentIndex++;
          heldDown = true;
          buzz_multiple_times(1, 500, 100, 100);
        }
      }
      if (buttonState == LOW && heldDown == true){
        // has stopped holding button
        heldDown = false;
      }
    }
  }
}
