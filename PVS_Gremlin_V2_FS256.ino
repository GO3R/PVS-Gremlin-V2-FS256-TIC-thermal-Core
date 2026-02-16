#include <Arduino.h>

#define ROTARY_A_PIN   2
#define ROTARY_B_PIN   3
#define ROTARY_SW_PIN  6
#define THUMB_BUTTON   7
#define LED_BLUE       8

#define RX_PIN         20
#define TX_PIN         21

#define VALUE_MIN 0
#define VALUE_MAX 100

// =========================
// Globale Variablen
// =========================

int brightness = 50;
int contrast   = 50;
int palette    = 0;

int lastA = HIGH;

// =========================
// UART Sendefunktion
// =========================

void sendFSCommand(uint8_t classAddr, uint8_t subAddr, uint8_t flag, uint8_t *data, uint8_t len)
{
  uint8_t size = len + 4;
  uint8_t chk  = 0x36 + classAddr + subAddr + flag;

  for (int i = 0; i < len; i++)
    chk += data[i];

  chk &= 0xFF;

  Serial1.write(0xF0);
  Serial1.write(size);
  Serial1.write(0x36);
  Serial1.write(classAddr);
  Serial1.write(subAddr);
  Serial1.write(flag);

  for (int i = 0; i < len; i++)
    Serial1.write(data[i]);

  Serial1.write(chk);
  Serial1.write(0xFF);
}

// =========================
// Kamera Funktionen
// =========================

void camSetBrightness(int value)
{
  uint8_t data[1] = { (uint8_t)value };
  sendFSCommand(0x78, 0x02, 0x00, data, 1);
}

void camSetContrast(int value)
{
  uint8_t data[1] = { (uint8_t)value };
  sendFSCommand(0x78, 0x03, 0x00, data, 1);
}

void camSetPalette(int value)
{
  uint8_t data[1] = { (uint8_t)value };
  sendFSCommand(0x78, 0x20, 0x00, data, 1);
}

void camSetAGCOff()
{
  uint8_t data[1] = { 0x00 };
  sendFSCommand(0x78, 0x0A, 0x00, data, 1);
}

void camNUC()
{
  uint8_t data[1] = { 0x00 };
  sendFSCommand(0x7C, 0x02, 0x00, data, 1);
}

// =========================
// Setup
// =========================

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(ROTARY_A_PIN, INPUT_PULLUP);
  pinMode(ROTARY_B_PIN, INPUT_PULLUP);
  pinMode(ROTARY_SW_PIN, INPUT_PULLUP);
  pinMode(THUMB_BUTTON, INPUT_PULLUP);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_BLUE, HIGH);

  lastA = digitalRead(ROTARY_A_PIN);

  delay(4000);  // Kamera Bootzeit

  camSetAGCOff();
  delay(200);

  camSetBrightness(brightness);
  delay(200);

  camSetContrast(contrast);
  delay(200);

  camSetPalette(palette);
}

// =========================
// Loop
// =========================

void loop()
{
  int currentA = digitalRead(ROTARY_A_PIN);

  if (currentA != lastA)
  {
    bool clockwise = (digitalRead(ROTARY_B_PIN) == currentA);

    bool thumbPressed   = (digitalRead(THUMB_BUTTON) == LOW);
    bool encoderPressed = (digitalRead(ROTARY_SW_PIN) == LOW);

    // ===== KONTRAST =====
    if (encoderPressed)
    {
      if (clockwise)
        contrast += 2;
      else
        contrast -= 2;

      contrast = constrain(contrast, VALUE_MIN, VALUE_MAX);
      camSetContrast(contrast);

      Serial.print("Contrast: ");
      Serial.println(contrast);
    }

    // ===== PALETTE =====
    else if (thumbPressed)
    {
      if (clockwise)
        palette++;
      else
        palette--;

      if (palette > 13) palette = 0;
      if (palette < 0)  palette = 13;

      camSetPalette(palette);

      Serial.print("Palette: ");
      Serial.println(palette);
    }

    // ===== HELLIGKEIT =====
    else
    {
      if (clockwise)
        brightness += 2;
      else
        brightness -= 2;

      brightness = constrain(brightness, VALUE_MIN, VALUE_MAX);
      camSetBrightness(brightness);

      Serial.print("Brightness: ");
      Serial.println(brightness);
    }

    digitalWrite(LED_BLUE, LOW);
    delay(10);
    digitalWrite(LED_BLUE, HIGH);
  }

  lastA = currentA;

  // ===== NUC nur bei Encoder + Thumb =====
  static bool lastNUCState = false;

  bool encoderPressed = (digitalRead(ROTARY_SW_PIN) == LOW);
  bool thumbPressed   = (digitalRead(THUMB_BUTTON) == LOW);

  bool nucNow = encoderPressed && thumbPressed;

  if (nucNow && !lastNUCState)
  {
    camNUC();
    Serial.println("NUC TRIGGERED");

    digitalWrite(LED_BLUE, LOW);
    delay(100);
    digitalWrite(LED_BLUE, HIGH);
  }

  lastNUCState = nucNow;
}
