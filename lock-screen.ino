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
int previousSoundLevel = -1000;
char previousClockTime[8] = ""; 

// lockdown global variables
int incorrectAttempts = 0;
bool inLockdown = false;
unsigned long lockdownStartTime = 0;
unsigned long lockdownFor = 0;

// global variables for EEPROM
int maxSoundLevel;
const int soundThresholdAddress = 0;
int clockFormat; // 0 will be 24 hour; 1 will be 12pm
const int clockFormatAddress = 1;
int temperatureUnit; // 0 will be celsius; 1 will be fahrenheit
const int temperatureUnitAddress = 2;


// global variables for stopwatch
unsigned long timeSinceStart = 0;
bool stopwatchStarted = false;

// global variables for timer
int timerMinutes = 0;
bool timerStarted = false;

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

  // getting all values from EEPROM
  EEPROM.get(soundThresholdAddress, maxSoundLevel);

  if (maxSoundLevel < 0 || maxSoundLevel > 1023) {
    maxSoundLevel = 500;
    EEPROM.put(soundThresholdAddress, maxSoundLevel);
  }

  clockFormat = EEPROM.read(clockFormatAddress);
  if (clockFormat < 0 || clockFormat > 1){
    EEPROM.write(clockFormatAddress, 0);
  }

  temperatureUnit = EEPROM.read(temperatureUnitAddress);
  if (temperatureUnit < 0 || temperatureUnit > 1){
    EEPROM.write(temperatureUnitAddress, 0);
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
      char timeDisplay[8];
      Serial.println(clockFormat);
      if (clockFormat == 0){
        snprintf(timeDisplay, sizeof(timeDisplay), "%02d:%02d", hours, minutes);
      } else {
        char ending[2] = {'|', 'm'};
        if (hours == 0){
          hours = 12;
          ending[0] = 'a';
        } else if (hours > 0 && hours < 12){
          ending[0] = 'a';
        } else {
          ending[0] = 'p';
          if (hours != 12){
            hours = hours % 12;
          } else {
          }
        }
        snprintf(timeDisplay, sizeof(timeDisplay), "%02d:%02d%s", hours, minutes, ending);

        delay(100);
      }
      // char fullDisplay[20];
      // snprintf(fullDisplay, sizeof(fullDisplay), "Hi! %s", timeDisplay);

      if (strcmp(timeDisplay, previousClockTime) ||
          strcmp(screenDisplay, "Clock") != 0) {
        // shows new time if it has changed since last display
        default_screen_setup(3);
        display.println("Hi!");
        display.print(timeDisplay);
        display.display();
        strcpy(previousClockTime, timeDisplay);
        strcpy(screenDisplay, "Clock");
      }
    } else if (number == 1) {
      // stopwatch

      // TODO: show the count
      // TODO: show play/pause + reset
      // TODO: determine play/pause depending on whether it's on or not
      // TODO: show arrows under the buttons for which one to press
      // TODO: press to switch arrows
      // TODO: hold press to do the action (play/reset)
      if (strcmp(screenDisplay, "Stopwatch") != 0){
        default_screen_setup(2);
        display.println(F("Stopwatch"));
        display.display();
        strcpy(screenDisplay, "Stopwatch");
      }
    } else if (number == 2) {
      // timer
      if (timerStarted == true){
        // TODO: timer countdown
      } else {
        // // dial to start the timer
        // int dialMinutes = fmin(rotaryState / 20, 50) + 1;
        // default_screen_setup(2);
        // display.println(F("Timer for:"));
        // display.print(dialMinutes);
        // display.display();

        // TODO: press button to rotate through the options: 1, 5, 10

        // TODO: long button press to start the timer
        
      }

      // if (strcmp(screenDisplay, "Timer") != 0){
      //   default_screen_setup(2);
      //   display.println(F("Timer"));
      //   display.display();
      //   strcpy(screenDisplay, "Timer");
      // }
    } else if (number == 3) {
      if (DHT.read() == DHT20_OK) {
        // if the reading is sucessful
        float temperature = DHT.getTemperature();
        if (temperatureUnit == 1){
          temperature = (temperature * (9.0/5.0)) + 32;
        }
        float humidity = DHT.getHumidity();

        if (temperature != previousTemperature ||
          humidity != previousHumidity ||
          strcmp(screenDisplay, "Temperature") != 0) {
          // if the temperature and humidity are different from before

          default_screen_setup(2);
          display.print(F("Temperature: "));
          display.print(temperature);
          if (temperatureUnit == 0){
            display.println(F(" C"));
          } else {
            display.println(F(" F"));
          }

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
      // TODO: show all settings: temperature unit, sound monitor limit, maybe clock format
      // TODO: indent the setting it's referring to
      // TODO: pressing button changes indent
      // TODO: holding button goes into the setting page
      // TODO: temperature unit screen:
        // TODO: use slider to indent each option (celcius, fahrenheit), then button to save and go back to menu
      // TODO: sound monitor screen:
        // TODO: use slider to indent each option, then button to save and go back to menu
      // TODO: clock format screen:
        // TODO: use the slider to indent each option (am/24 hours), then button to go back to menu
        // TODO: show format on main page
      if (strcmp(screenDisplay, "Settings") != 0){
        default_screen_setup(2);
        display.println(F("Settings"));
        display.display();
        strcpy(screenDisplay, "Settings");
      }
    }
    // TODO: check if the timer has finished and then buzz until button pressed
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
