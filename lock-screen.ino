#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// inputs
int rotaryPin = 0;
int buttonPin = 6;

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
char screenDisplay[100] = "_ _ _ _ ";

// lockdown global variables
int incorrectAttempts = 0;
bool inLockdown = false;
unsigned long lockdownStartTime = 0;
unsigned long lockdownFor = 0;

// clock times


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
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);

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

    int number = fmin(rotaryState / 200, 4);
    if (number == 0){
      // clock face
    } else if (number == 1) {
      // 
    } else if (number == 2) {
      //
    } else if (number == 3){
      //
    } else if (number == 4) {
      //
    }
  } else {
    if (inLockdown == true){
      // calculating time left
      unsigned long elapsed = millis() - lockdownStartTime;
      long secondsLeft = (long)((lockdownFor - elapsed) / 1000);

      char lockdownDisplayText[100];
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
        strcpy(lockdownDisplayText, screenDisplay);

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
        strcpy(displayString, screenDisplay);
      }

      if (buttonState == HIGH && heldDown == false){
        // if button has just been held down
        if (currentIndex == 3){
          // if last digit has been entered
          digits[currentIndex] = number;
          if (digits[0] == password[0] - '0' && 
            digits[1] == password[1] - '0' &&
            digits[2] == password[2] - '0' &&
            digits[3] == password[3] - '0'){
            buzz_multiple_times(1, 1000, 500, 500);
            // if correct password
            loggedIn = true;
            incorrectAttempts = 0;

            default_screen_setup(2);
            display.println(F("Welcome"));
            display.display();
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
