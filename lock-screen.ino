#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

char password[5] = "1234";

int rotaryPin = 0;
int buttonPin = 6;
int currentIndex = 0;
int digits[4] = {-1, -1, -1, -1};
char displayString[9] = "_ _ _ _ ";
int buttonState = LOW;
bool heldDown = false;
int buzzerPin = 5;
bool loggedIn = false;

int incorrectAttempts = 0;
bool inLockdown = false;
unsigned long lockdownStartTime = 0;
unsigned long lockdownFor = 0;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  pinMode(rotaryPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // The address is typically 0x3C, but can be 0x3D for some modules
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(2);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,0);              // Start at top-left corner
  display.println(F("_ _ _ _"));
  display.display();
}

void loop() {
  if (loggedIn == true){
    
  } else {
    if (inLockdown == true){
      unsigned long elapsed = millis() - lockdownStartTime;
      long secondsLeft = (long)((lockdownFor - elapsed) / 1000);

      Serial.print(secondsLeft);
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,0);
      display.println(F("Lockdown."));
      display.print(F("Will unlock in "));
      display.print(secondsLeft);
      display.print(F(" seconds"));
      display.display();
      
      if (elapsed >= lockdownFor) {
        inLockdown = false;
      }
    } else {
      int rotaryState = analogRead(rotaryPin);
      buttonState = digitalRead(buttonPin);

      int number = fmin(rotaryState / 100, 9);

      char newDisplayString[9];

      for (int i = 0; i < 4; i++){
        if (currentIndex == i){
          newDisplayString[i*2] = '0' + number;
        } else if (digits[i] == -1){
          newDisplayString[i*2] = '_';
        } else {
          newDisplayString[i*2] = '0' + digits[i];
        }
        newDisplayString[i*2+1] = ' ';
      }
      newDisplayString[8] = '\0';

      if (strcmp(newDisplayString, displayString) != 0){
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println(newDisplayString);
        display.display();
        strcpy(displayString, newDisplayString);
      }

      if (buttonState == HIGH && heldDown == false){
        if (currentIndex == 3){
          digits[currentIndex] = number;
          Serial.print(digits[0] == password[0] - '0' && 
            digits[1] == password[1] - '0' &&
            digits[2] == password[2] - '0' &&
            digits[3] == password[3] - '0');
          if (digits[0] == password[0] - '0' && 
            digits[1] == password[1] - '0' &&
            digits[2] == password[2] - '0' &&
            digits[3] == password[3] - '0'){
            tone(buzzerPin, 1000);
            delay(500);
            noTone(buzzerPin);
            loggedIn = true;
            incorrectAttempts = 0;

            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0,0);
            display.println(F("Welcome"));
            display.display();
          } else {
            digits[0] = -1;
            digits[1] = -1;
            digits[2] = -1;
            digits[3] = -1;
            currentIndex = 0;
            tone(buzzerPin, 200);
            delay(100);
            noTone(buzzerPin);
            delay(50);
            tone(buzzerPin, 200);
            delay(100);
            noTone(buzzerPin);
            delay(50);
            tone(buzzerPin, 200);
            delay(100);
            noTone(buzzerPin);
            incorrectAttempts += 1;
            if (incorrectAttempts >= 3){
              Serial.print("too much");
              inLockdown = true;
              lockdownFor = (incorrectAttempts - 2) * 60000;
              Serial.print(lockdownFor);
              lockdownStartTime = millis();
            }
          }
        }
        else {
          digits[currentIndex] = number;
          currentIndex++;
          heldDown = true;
          tone(buzzerPin, 500);
          delay(100);
          noTone(buzzerPin);
        }
      }
      if (buttonState == LOW && heldDown == true){
        heldDown = false;
      }
    }
  }
}
