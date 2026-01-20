#include <Keypad.h>

const int buzzerPin = 11;
const int ledPin = 10;  
const int potPin = A0;
const int buttonPin = 12;
const int X_pin = A4;
const int Y_pin = A5;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

int keyFreqs[16] = {
  262, 277, 294, 311,
  330, 349, 370, 392, 
  415, 440, 466, 494,
  523, 554, 587, 622
};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int centerX = 512;
int centerY = 512;
int deadzone = 50;
bool octaveHigh = false;
bool lastButtonState = HIGH;
int lastFreq = 0;

char mode = ' ';

int calibrateAxis(int pin) {
  long sum = 0;
  const int samples = 20;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(10);
  }
  return sum / samples;
}

void setup() {
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);  // Initialize LED pin
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(9600);
  delay(500);

  Serial.println("Select mode:");
  Serial.println("Type 'K' for Keypad");
  Serial.println("Type 'J' for Joystick");

  while (mode != 'K' && mode != 'J') {
    if (Serial.available()) {
      char input = Serial.read();
      input = toupper(input);
      if (input == 'K'  input == 'J') {
        mode = input;
        Serial.print("Mode selected: ");
        Serial.println(mode == 'K' ? "KEYPAD" : "JOYSTICK");
        delay(500);
      }
    }
  }

  if (mode == 'J') {
    centerX = calibrateAxis(X_pin);
    centerY = calibrateAxis(Y_pin);
  }
}

void loop() {
  if (mode == 'K') {
    runKeypadMode();
  } else if (mode == 'J') {
    runJoystickMode();
  }
}

void runKeypadMode() {
  keypad.getKeys();

  for (int i = 0; i < LIST_MAX; i++) {
    if (keypad.key[i].kstate == PRESSED  keypad.key[i].kstate == HOLD) {
      char key = keypad.key[i].kchar;

      int keyIndex = -1;
      for (int k = 0; k < 16; k++) {
        if (key == keys[k / 4][k % 4]) {
          keyIndex = k;
          break;
        }
      }

      if (keyIndex != -1) {
        float baseFreq = keyFreqs[keyIndex];
        float nextFreq = (keyIndex < 15) ? keyFreqs[keyIndex + 1] - 1 : baseFreq + 50;
        float freqRange = nextFreq - baseFreq;

        int potVal = analogRead(potPin);
        float ratio = potVal / 1023.0;
        float freqF = baseFreq + ratio * freqRange;
        int freq = (int)(freqF + 0.5);

        tone(buzzerPin, freq);
        digitalWrite(ledPin, HIGH);  // Turn LED on
        Serial.print("Key: ");
        Serial.print(key);
        Serial.print(" | Frequency: ");
        Serial.println(freq);
        return;
      }
    }
  }
  noTone(buzzerPin);
  digitalWrite(ledPin, LOW);  // Turn LED off when no sound
}

void runJoystickMode() {
  int xVal = analogRead(X_pin);
  int yVal = analogRead(Y_pin);
  int potVal = analogRead(potPin);

  bool buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    octaveHigh = !octaveHigh;
    delay(200);
  }
  lastButtonState = buttonState;

  int xDiff = xVal - centerX;
  int yDiff = yVal - centerY;

  int freq = 0;

  if (abs(xDiff) < deadzone && abs(yDiff) < deadzone) {
    freq = 0;
  } else {
    if (xDiff < -deadzone && abs(yDiff) <= deadzone) freq = 262;
    else if (xDiff > deadzone && abs(yDiff) <= deadzone) freq = 294;
    else if (yDiff < -deadzone && abs(xDiff) <= deadzone) freq = 330;
    else if (yDiff > deadzone && abs(xDiff) <= deadzone) freq = 349;
    else if (xDiff < -deadzone && yDiff < -deadzone) freq = 392;
    else if (xDiff > deadzone && yDiff < -deadzone) freq = 440;
    else if (xDiff < -deadzone && yDiff > deadzone) freq = 494;
    else if (xDiff > deadzone && yDiff > deadzone) freq = 523;
  }

  if (freq != 0) {
    int fineTune = map(potVal, 0, 1023, -20, 20);
    freq += fineTune;
    if (octaveHigh) freq *= 2;
  }

Leul, [8/20/2025 11:44 AM]
if (freq != lastFreq) {
    Serial.print("freq: ");
    Serial.print(freq);
    Serial.print("  x: ");
    Serial.print(xVal);
    Serial.print("  y: ");
    Serial.println(yVal);
    lastFreq = freq;
  }

  if (freq != 0) {
    tone(buzzerPin, freq);
    digitalWrite(ledPin, HIGH);  // Turn LED on
  } else {
    noTone(buzzerPin);
    digitalWrite(ledPin, LOW);   // Turn LED off
  }

  delay(10);
}