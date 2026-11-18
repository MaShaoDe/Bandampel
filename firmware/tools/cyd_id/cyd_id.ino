// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 FerienBSD aka Marcel Sauder
// CYD Display-ID Test (ohne Library, Bitbang-SPI)
// Board: ESP32 Dev Module, Serial Monitor 115200
// Pins ESP32-2432S028: SCLK 14, MOSI 13, MISO 12, CS 15, DC 2, BL 21

#define PIN_SCLK 14
#define PIN_MOSI 13
#define PIN_MISO 12
#define PIN_CS   15
#define PIN_DC   2
#define PIN_BL   21

void sendByte(uint8_t b) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_MOSI, (b >> i) & 1);
    digitalWrite(PIN_SCLK, HIGH);
    digitalWrite(PIN_SCLK, LOW);
  }
}

uint8_t readByte() {
  uint8_t v = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(PIN_SCLK, HIGH);
    v = (v << 1) | digitalRead(PIN_MISO);
    digitalWrite(PIN_SCLK, LOW);
  }
  return v;
}

void readId(uint8_t cmd, uint8_t *out, int n) {
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_DC, LOW);
  sendByte(cmd);
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_SCLK, HIGH);   // 1 Dummy-Takt
  digitalWrite(PIN_SCLK, LOW);
  for (int i = 0; i < n; i++) out[i] = readByte();
  digitalWrite(PIN_CS, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PIN_SCLK, OUTPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);
  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_SCLK, LOW);
}

void loop() {
  uint8_t a[3], b[3];
  readId(0x04, a, 3);   // ST7789: 85 85 52
  readId(0xD3, b, 3);   // ILI9341: 00 93 41
  Serial.printf("CMD 0x04: %02X %02X %02X\n", a[0], a[1], a[2]);
  Serial.printf("CMD 0xD3: %02X %02X %02X\n", b[0], b[1], b[2]);
  Serial.println("ILI9341 = D3 liefert 00 93 41 | ST7789 = 04 liefert 85 85 52");
  Serial.println("---");
  delay(3000);
}
