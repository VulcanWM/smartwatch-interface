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
unsigned long buttonHeldDownAt = -1000; // to differentiate between short and long press

// initialising lock screen
char password[5] = "0000";
int currentIndex = 0;
int digits[4] = {-1, -1, -1, -1};

// to prevent it redisplaying content multiple times
char screenDisplay[20] = "_ _ _ _ ";
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
unsigned long timeOfStart = 0;
unsigned long differenceAtStop = 0;
bool stopwatchStarted = false;
int stopwatchMenuItem = 0;

// global variables for timer
bool timerStarted = false;
int timerMenuItem = 0;
unsigned long timerTimeOfStart = 0;
unsigned long timerTimeLeftAtStop = 0; // in seconds
int timerDuration = 0; // in seconds

// global variables for settings
bool inSetting = false;
int settingMenuItem = 0;
int inSettingMenuItem = 0;

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
      // convert to clock format depending on user settings
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

      // TODO: only change text when buttons pressed or time changed

      default_screen_setup(2);
      display.println(F("Stopwatch"));

      // generate the time to display
      int minutes;
      int seconds;
      unsigned long totalSeconds;

      char stopwatchTime[10];

      if (stopwatchStarted == true){
        totalSeconds = (millis() - timeOfStart) / 1000;
      } else {
        totalSeconds = differenceAtStop / 1000;
      }

      minutes = totalSeconds / 60;
      seconds = totalSeconds - (minutes * 60);

      snprintf(stopwatchTime, sizeof(stopwatchTime), "%02d:%02d", minutes, seconds);

      display.println(stopwatchTime);

      // display the 2 action items
      if (stopwatchMenuItem == 0){
        display.print("> ");
      } else {
        display.print("  ");
      }
      if (stopwatchStarted == true){
        display.println("Stop");
      } else {
        display.println("Start");
      }
      if (stopwatchMenuItem == 1){
        display.print("> ");
      } else {
        display.print("  ");
      }
      display.println("Reset");
      display.display();

      // use the button to do actions
      buttonState = digitalRead(buttonPin);
      if (buttonState == HIGH && heldDown == false){
        heldDown = true;
        buttonHeldDownAt = millis();
      }
      if (buttonState == LOW && heldDown == true){
        if ((millis() - buttonHeldDownAt) / 1000 > 0){
          if (stopwatchMenuItem == 0){
            if (stopwatchStarted == true){
              differenceAtStop = millis() - timeOfStart;
              stopwatchStarted = false;
            } else {
              timeOfStart = millis() - differenceAtStop;
              stopwatchStarted = true;
            }
          } else {
            stopwatchStarted = false;
            differenceAtStop = 0;
          }
        } else {
          stopwatchMenuItem = 1 - stopwatchMenuItem;
        }
        heldDown = false;
      }
    } else if (number == 2) {
      // timer

      default_screen_setup(2);
      display.println(F("Timer"));
      if (timerStarted == true){
        // generate the time to display
        int minutes;
        int seconds;
        unsigned long totalSeconds = timerDuration - ((millis() - timerTimeOfStart) / 1000);

        char stopwatchTime[10];

        minutes = totalSeconds / 60;
        seconds = totalSeconds - (minutes * 60);

        snprintf(stopwatchTime, sizeof(stopwatchTime), "%02d:%02d", minutes, seconds);

        display.println(stopwatchTime);

        if (timerMenuItem == 0){
          display.print("> ");
        } else {
          display.print("  ");
        }
        display.println("Stop");
        if (timerMenuItem == 1){
          display.print("> ");
        } else {
          display.print("  ");
        }
        display.println("Cancel");
      } else {
        if (timerTimeLeftAtStop == 0){
          display.println(F("1 5 10"));
          int spaces = timerMenuItem * 2;
          for (int i = 0; i < spaces; i++){
            display.print(F(" "));
          }
          display.println("^");
        } else {
          int minutes;
          int seconds;

          char stopwatchTime[10];

          minutes = timerTimeLeftAtStop / 60;
          seconds = timerTimeLeftAtStop - (minutes * 60);

          snprintf(stopwatchTime, sizeof(stopwatchTime), "%02d:%02d", minutes, seconds);

          display.println(stopwatchTime);

          if (timerMenuItem == 0){
            display.print("> ");
          } else {
            display.print("  ");
          }
          display.println("Start");
          if (timerMenuItem == 1){
            display.print("> ");
          } else {
            display.print("  ");
          }
          display.println("Cancel");
        }
      }
      display.display();

      buttonState = digitalRead(buttonPin);
      if (buttonState == HIGH && heldDown == false){
        heldDown = true;
        buttonHeldDownAt = millis();
      }
      if (buttonState == LOW && heldDown == true){
        if ((millis() - buttonHeldDownAt) / 1000 > 0){
          if (timerStarted == false){
            if (timerTimeLeftAtStop == 0){
              // start timer
              timerStarted = true;
              timerTimeOfStart = millis();
              if (timerMenuItem == 0){
                timerDuration = 60;
              } else if (timerMenuItem == 1){
                timerDuration = 300;
              } else {
                timerDuration = 600;
              }
              timerMenuItem = 0;
            } else {
              if (timerMenuItem == 0){
                timerDuration = timerTimeLeftAtStop;
                timerTimeOfStart = millis();
                timerStarted = true;
              } else {
                timerStarted = false;
                timerTimeLeftAtStop = 0;
              }
            }
          } else {
            if (timerMenuItem == 0){
              timerTimeLeftAtStop = timerDuration - ((millis() - timerTimeOfStart) / 1000);
              timerStarted = false;
            } else {
              timerStarted = false;
              timerTimeLeftAtStop = 0;
            }
          }
        } else {
          if (timerStarted == false){
            if (timerTimeLeftAtStop == 0){
              timerMenuItem = (timerMenuItem + 1) % 3;
            } else {
              timerMenuItem = 1 - timerMenuItem;
            }
          } else {
            timerMenuItem = 1 - timerMenuItem;
          }
        }
        heldDown = false;
      }
    } else if (number == 3) {
      if (DHT.read() == DHT20_OK) {
        // if the reading is sucessful
        float temperature = DHT.getTemperature();
        float humidity = DHT.getHumidity();

        // convert to fahrenheit if needed
        if (temperatureUnit == 1){
          temperature = (temperature * (9.0/5.0)) + 32;
        }

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
      default_screen_setup(2);
      if (inSetting == false){
        display.println(F("Settings"));
        if (settingMenuItem == 0){
          display.print(F("> "));
        } else {
          display.print(F("  "));
        }
        display.println(F("Temp"));
        if (settingMenuItem == 1){
          display.print(F("> "));
        } else {
          display.print(F("  "));
        }
        display.println(F("Sound"));
        if (settingMenuItem == 2){
          display.print(F("> "));
        } else {
          display.print(F("  "));
        }
        display.println(F("Time"));
        // show the 3 menu items
      } else {
        if (settingMenuItem == 0){
          display.println(F("Temp Unit"));
          if (inSettingMenuItem == 0){
            display.print(F("> "));
          } else {
            display.print(F("  "));
          }
          display.println(F("C"));
          if (inSettingMenuItem == 1){
            display.print(F("> "));
          } else {
            display.print(F("  "));
          }
          display.println(F("F"));
        } else if (settingMenuItem == 1){
          display.println(F("Sound Lim"));
          if (inSettingMenuItem == 300){
            display.print(F("> "));
          } else {
            display.print(F("  "));
          }
          display.println(F("300"));
          if (inSettingMenuItem == 500){
            display.print(F("> "));
          } else {
            display.print(F("  "));
          }
          display.println(F("500"));
          if (inSettingMenuItem == 700){
            display.print(F("> "));
          } else {
            display.print(F("  "));
          }
          display.println(F("700"));
        } else {
          display.println(F("Time Form"));
          if (inSettingMenuItem == 0){
            display.print(F("> "));
          } else {
            display.print(F("  "));
          }
          display.println(F("24 hour"));
          if (inSettingMenuItem == 1){
            display.print(F("> "));
          } else {
            display.print(F("  "));
          }
          display.println(F("12 hour"));
        }
      }
      display.display();

      buttonState = digitalRead(buttonPin);
      if (buttonState == HIGH && heldDown == false){
        heldDown = true;
        buttonHeldDownAt = millis();
      }
      if (buttonState == LOW && heldDown == true){
        if ((millis() - buttonHeldDownAt) / 1000 > 0){
          if (inSetting == false){
            inSetting = true;
          } else {
            if (settingMenuItem == 0){
              EEPROM.write(temperatureUnitAddress, inSettingMenuItem);
              temperatureUnit = settingMenuItem;
            } else if (settingMenuItem == 1){
              EEPROM.put(soundThresholdAddress, inSettingMenuItem);
              maxSoundLevel = inSettingMenuItem;
            } else {
              EEPROM.write(clockFormatAddress, inSettingMenuItem);
              clockFormat = inSettingMenuItem;
            }
            inSetting = false;
          }
        } else {
          if (inSetting == false){
            settingMenuItem = (settingMenuItem + 1) % 3;
          } else {
            if (settingMenuItem == 0 || settingMenuItem == 2){
              inSettingMenuItem = 1 - inSettingMenuItem;
            } else {
              if (inSettingMenuItem == 300){
                inSettingMenuItem = 500;
              }
              if (inSettingMenuItem == 500){
                inSettingMenuItem = 700;
              }
              if (inSettingMenuItem == 700){
                inSettingMenuItem = 300;
              }
            }
          }
        }
        heldDown = false;
      }
    }

    if (timerStarted == true){
      unsigned long totalSeconds = timerDuration - ((millis() - timerTimeOfStart) / 1000);
      if (totalSeconds <= 0){
        timerStarted = false;
        timerTimeOfStart = 0;
        timerTimeLeftAtStop = 0;
        timerDuration = 0; 
        buzz_multiple_times(5, 300, 500, 100);

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
            heldDown = false;
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
