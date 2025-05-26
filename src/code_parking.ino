#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Servo.h>

// RC522
#define SS_PIN   10
#define RST_PIN   8
#define IRQ_PIN   2

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, A3, 6, 5, 4, 3);

// Servo
#define SERVO_PIN 9
Servo bariera;

// Fotorezistori
#define NUM_LOCURI 4
const int pini_ldr[NUM_LOCURI] = { A0, A1, A4, A5 };
const int prag_ocupare = 200;

// RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);
volatile bool card_flag = false;

// UID autorizat (exemplu)
byte uid_autorizat1[] = { 0xEE, 0x79, 0x9D, 0x1D };
byte uid_autorizat2[] = { 0x80, 0xDD, 0xA6, 0x14 };

void cardInterrupt() {
  card_flag = true;
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  // Config fotorezistori
  for (int i = 0; i < NUM_LOCURI; i++) {
    pinMode(pini_ldr[i], INPUT);
  }

  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_AntennaOn();

  lcd.begin(16, 2);
  lcd.print("Apropie card...");

  pinMode(IRQ_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), cardInterrupt, FALLING);

  // Servo
  bariera.attach(SERVO_PIN);
  bariera.write(95); // bariera initial jos

  // Activare IRQ initial
  mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, 0b00100000);
  mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7F);
  mfrc522.PCD_WriteRegister(mfrc522.FIFOLevelReg, 0x80);
  mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x07);

  Serial.println("Sistem pregatit.");
}

void loop() {
  if (!card_flag) goto end;

  card_flag = false;

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Afiseaza UID
  Serial.print("Card UID: ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card detectat:");
  lcd.setCursor(0, 1);

  bool card_valid = true;

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    byte val = mfrc522.uid.uidByte[i];
    if (val < 0x10) {
      Serial.print("0");
      lcd.print("0");
    }
    Serial.print(val, HEX);
    lcd.print(val, HEX);
    Serial.print(" ");

    if (val != uid_autorizat1[i] && val != uid_autorizat2[i]) {
      card_valid = false;
    }
  }
  Serial.println();

  if (card_valid) {
    Serial.println("Acces permis.");
    lcd.clear();
    lcd.print("Acces permis.");
    bariera.write(185);   // ridica bariera
    delay(2000);
    bariera.write(95);    // coboara bariera
  } else {
    Serial.println("Acces respins.");
    lcd.clear();
    lcd.print("Acces respins.");
    delay(2000);
  }

  lcd.clear();
  lcd.print("Apropie card...");

  // Oprire comunicare card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

end:
  // Reinitializare RC522
  mfrc522.PCD_Init();
  mfrc522.PCD_AntennaOn();
  mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, 0b00100000);
  mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7F);
  mfrc522.PCD_WriteRegister(mfrc522.FIFOLevelReg, 0x80);
  mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x07);

  // Serial output pe o singura linie
  Serial.print("LDRs: ");
  for (int i = 0; i < NUM_LOCURI; i++) {
    int valoare = analogRead(pini_ldr[i]);
    Serial.print("L");
    Serial.print(i + 1);
    Serial.print(":");
    Serial.print(valoare);
    Serial.print(" ");
  }
  Serial.println();

  // LCD status: L1:O L2:L ...
  lcd.setCursor(0, 1);
  int ocupat = 0;
  for (int i = 0; i < NUM_LOCURI; i++) {
    int valoare = analogRead(pini_ldr[i]);
    if (valoare < prag_ocupare) {
      ocupat++;
    }
  }
  lcd.print("Free Seats: ");
  lcd.print(NUM_LOCURI - ocupat);
}
