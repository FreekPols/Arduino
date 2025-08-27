// MKR Zero + MPL3115A2 temperature logger to SD
// Creates TEMP000.CSV, TEMP001.CSV, ... and logs: ms_since_boot, temp_C, temp_F

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MPL3115A2.h>

Adafruit_MPL3115A2 mpl = Adafruit_MPL3115A2();
File logFile;

const uint32_t LOG_INTERVAL_MS = 5000;     // change to taste
const uint8_t  MAX_FILES        = 200;     // TEMP000.CSV ... TEMP199.CSV
const char*    FILE_PREFIX      = "TEMP";
const char*    FILE_EXT         = ".CSV";

// For MKR Zero the SD CS pin is defined as SDCARD_SS_PIN by the core
#ifndef SDCARD_SS_PIN
#define SDCARD_SS_PIN 4
#endif

uint32_t lastLog = 0;

String makeFilename(uint16_t idx) {
  char name[13]; // 8.3 format max
  snprintf(name, sizeof(name), "%s%03u%s", FILE_PREFIX, idx, FILE_EXT);
  return String(name);
}

bool createNewLogFile() {
  for (uint16_t i = 0; i < MAX_FILES; i++) {
    String fname = makeFilename(i);
    if (!SD.exists(fname.c_str())) {
      logFile = SD.open(fname.c_str(), FILE_WRITE);
      if (logFile) {
        // CSV header
        logFile.println(F("ms_since_boot,temp_C"));
        logFile.flush();
        Serial.print(F("Logging to: "));
        Serial.println(fname);
        return true;
      } else {
        Serial.print(F("Failed to open new file: "));
        Serial.println(fname);
        return false;
      }
    }
  }
  Serial.println(F("No free TEMPxxx.CSV filename slots left."));
  return false;
}

bool initSD() {
  Serial.print(F("Initializing SD... "));
  if (!SD.begin(SDCARD_SS_PIN)) {
    Serial.println(F("SD.begin() failed!"));
    return false;
  }
  Serial.println(F("OK"));
  return true;
}

bool initSensor() {
  Serial.print(F("Initializing MPL3115A2... "));
  if (!mpl.begin()) {
    Serial.println(F("not found (check wiring, power, I2C addr 0x60)."));
    return false;
  }
  // The Adafruit driver sets a reasonable default; ensure we're in active mode
  // and ready to read temperature (always available).
  Serial.println(F("OK"));
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) { /* wait for USB */ }

  if (!initSD()) {
    // Keep trying so user can insert a card after boot
    while (!initSD()) {
      Serial.println(F("Insert/format a microSD card and press reset if needed."));
      delay(2000);
    }
  }

  if (!initSensor()) {
    Serial.println(F("Sensor init failed; halting."));
    while (1) { delay(100); }
  }

  if (!createNewLogFile()) {
    Serial.println(F("Could not create log file; halting."));
    while (1) { delay(100); }
  }

  lastLog = millis() - LOG_INTERVAL_MS; // so we log immediately
}

void loop() {
  uint32_t now = millis();
  if (now - lastLog >= LOG_INTERVAL_MS) {
    lastLog = now;

    // Read temperature (°C) from MPL3115A2
    float tempC = mpl.getTemperature();  // Adafruit lib returns Celsius


    // Print to serial
    Serial.print(now);
    Serial.print(F(", "));
    Serial.print(tempC, 2);
    Serial.print(F(" C, "));


    // Write to SD
    if (logFile) {
      logFile.print(now);
      logFile.print(',');
      logFile.print(tempC, 2);
      logFile.flush(); // ensure data hits the card
    } else {
      // Try to recover by reopening (append to last file if possible)
      Serial.println(F("logFile closed unexpectedly; attempting to recover..."));
      // This simple example does not track the exact filename after reopen,
      // so you could store it in a global if you want to be able to re-open cleanly.
    }
  }

  // Low-power friendly delay between checks
  delay(5);
}
