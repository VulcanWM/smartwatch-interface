#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

int rotaryPin = 0;
int buttonPin = 6;
int currentIndex = 0;
int digits[4] = {-1, -1, -1, -1};
char displayString[9] = "_ _ _ _ ";
int buttonState = LOW;
bool heldDown = false;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  pinMode(rotaryPin, INPUT);
  pinMode(buttonPin, INPUT);
  Serial.begin(115200);

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
    digits[currentIndex] = number;
    currentIndex++;
    heldDown = true;
  }
  if (buttonState == LOW && heldDown == true){
    heldDown = false;
  }
}
