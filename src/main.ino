// ============================================================
// ESP32 Xbox BLE Controller — Physical Button Mapping
// v4 — with full debug output so you can see pin states live
// ============================================================

#include <Arduino.h>
#include <EEPROM.h>
#include <BleCompositeHID.h>
#include <XboxGamepadDevice.h>

// ---- EEPROM ----
#define EEPROM_SIZE       64
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_MAGIC_VAL  0xF1   // bumped — forces fresh remap on first flash
#define EEPROM_MAP_ADDR   2

// ---- Button pins ----
#define BTN_COUNT 16
const int btnPins[BTN_COUNT] = {
    4,  5,  16, 17,
    18, 19, 21, 12,
    23, 25, 26, 27,
    13, 14, 15, 0
};
const char* btnPinNames[BTN_COUNT] = {
    "GPIO4",  "GPIO5",  "GPIO16", "GPIO17",
    "GPIO18", "GPIO19", "GPIO21", "GPIO22",
    "GPIO23", "GPIO25", "GPIO26", "GPIO27",
    "GPIO13", "GPIO14", "GPIO15", "GPIO0"
};

// ---- Joystick ADC pins ----
#define JOY_L_X_PIN 35
#define JOY_L_Y_PIN 34
#define JOY_R_X_PIN 33
#define JOY_R_Y_PIN 32

// ---- Xbox input list ----
#define XBOX_INPUT_COUNT 20
#define AXIS_START_IDX   16

const char* xboxInputNames[XBOX_INPUT_COUNT] = {
    "A", "B", "X", "Y",
    "LB", "RB",
    "START", "SELECT",
    "LS (L3)", "RS (R3)",
    "DPAD UP", "DPAD DOWN", "DPAD LEFT", "DPAD RIGHT",
    "left_trigger", "right_trigger",
    "Left Stick X  (GPIO32)",
    "Left Stick Y  (GPIO33)",
    "Right Stick X (GPIO34)",
    "Right Stick Y (GPIO35)"
};

// btnMapping[xbox_input_index] = btnPins[] index (0-15), 0xFF = unmapped
uint8_t btnMapping[XBOX_INPUT_COUNT];


// ---- BLE ----
XboxGamepadDevice *gamepad;
BleCompositeHID compositeHID("ESP32 SeriesX Controller", "Mystfit", 100);

// ============================================================
//  Serial line reader — strips \r, works with ANY line ending
// ============================================================
String _serialBuf = "";

bool readSerialLine(String &out) {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            out = _serialBuf;
            _serialBuf = "";
            out.trim();
            out.toUpperCase();
            return true;
        }
        _serialBuf += c;
    }
    return false;
}

String readSerialLineBlocking() {
    String s;
    while (!readSerialLine(s)) delay(2);
    return s;
}

int readSmooth(int pin) {
  int sum = 0;
  for(int i = 0; i < 8; i++) {
    sum += analogRead(pin);
  }
  return sum / 8;
}

void printDivider() { Serial.println(F("================================================")); }

// ============================================================
//  DEBUG: print all pin states — call this to see what's LOW
// ============================================================
void debugPinStates() {
    Serial.println(F("\n--- Pin states (LOW means pressed) ---"));
    for (int i = 0; i < BTN_COUNT; i++) {
        Serial.print(btnPinNames[i]);
        Serial.print(F(" = "));
        Serial.println(digitalRead(btnPins[i]) == LOW ? F("LOW  <-- PRESSED") : F("HIGH"));
    }
    Serial.println(F("--------------------------------------\n"));
}

// ============================================================
//  Wait for a physical button press OR "SKIP" in serial.
//  Returns btnPins[] index (0-15), or -1 for SKIP.
// ============================================================
int waitForButtonPress() {
    // Step 1: wait for all pins to be HIGH (released)
    Serial.println(F("  [waiting for all buttons to be released...]"));
    unsigned long releaseStart = millis();
    while (true) {
        bool anyLow = false;
        for (int i = 0; i < BTN_COUNT; i++) {
            if (digitalRead(btnPins[i]) == LOW) {
                anyLow = true;
                break;
            }
        }
        if (!anyLow) break;
        // If stuck for >3s, print debug
        if (millis() - releaseStart > 3000) {
            Serial.println(F("  [still detecting held pins — showing states:]"));
            debugPinStates();
            releaseStart = millis();
        }
        String dummy; readSerialLine(dummy);
        delay(10);
    }
    delay(60); // settle
    _serialBuf = "";
    Serial.println(F("  [ready — press your button now]"));

    // Step 2: wait for any pin LOW or serial SKIP
    unsigned long waitStart = millis();
    while (true) {
        String line;
        if (readSerialLine(line)) {
            if (line == "SKIP") {
                Serial.println(F("  [SKIP received]"));
                return -1;
            }
            if (line == "DEBUG") {
                debugPinStates();
            }
        }

        for (int i = 0; i < BTN_COUNT; i++) {
            if (digitalRead(btnPins[i]) == LOW) {
                delay(30); // debounce
                if (digitalRead(btnPins[i]) == LOW) {
                    Serial.print(F("  [detected LOW on "));
                    Serial.print(btnPinNames[i]);
                    Serial.println(F("]"));
                    // wait for release
                    unsigned long pressStart = millis();
                    while (digitalRead(btnPins[i]) == LOW) {
                        delay(5);
                        if (millis() - pressStart > 5000) {
                            Serial.println(F("  [pin held too long — ignoring]"));
                            break;
                        }
                    }
                    delay(60);
                    return i;
                }
            }
        }

        // Every 5 seconds, print pin states automatically so you can see what's happening
        if (millis() - waitStart > 5000) {
            Serial.println(F("  [still waiting... current pin states:]"));
            debugPinStates();
            waitStart = millis();
        }
        delay(5);
    }
}

// ============================================================
//  EEPROM
// ============================================================
void saveMapping() {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);
    for (int i = 0; i < XBOX_INPUT_COUNT; i++)
        EEPROM.write(EEPROM_MAP_ADDR + i, btnMapping[i]);
    EEPROM.commit();
    Serial.println(F("  [EEPROM written OK]"));
}

bool loadMapping() {
    uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);
    Serial.print(F("  [EEPROM magic byte: 0x"));
    Serial.print(magic, HEX);
    Serial.print(F(", expected: 0x"));
    Serial.print(EEPROM_MAGIC_VAL, HEX);
    Serial.println(F("]"));
    if (magic != EEPROM_MAGIC_VAL) return false;
    for (int i = 0; i < XBOX_INPUT_COUNT; i++)
        btnMapping[i] = EEPROM.read(EEPROM_MAP_ADDR + i);
    return true;
}

void clearMapping() {
    for (int i = 0; i < XBOX_INPUT_COUNT; i++) btnMapping[i] = 0xFF;
}

// ============================================================
//  Print current mapping
// ============================================================
void printCurrentMapping() {
    printDivider();
    Serial.println(F("Current mapping:"));
    const int joyPins[4] = {JOY_L_X_PIN, JOY_L_Y_PIN, JOY_R_X_PIN, JOY_R_Y_PIN};
    for (int i = 0; i < XBOX_INPUT_COUNT; i++) {
        Serial.print(F("  "));
        Serial.print(xboxInputNames[i]);
        Serial.print(F("  =>  "));
        if (i < AXIS_START_IDX) {
            if (btnMapping[i] == 0xFF) Serial.println(F("(unmapped)"));
            else {
                Serial.print(btnPinNames[btnMapping[i]]);
                Serial.println();
            }
        } else {
            if (btnMapping[i] == 0xFF) Serial.println(F("(disabled)"));
            else { Serial.print(F("GPIO")); Serial.println(joyPins[i - AXIS_START_IDX]); }
        }
    }
    printDivider();
}

// ============================================================
//  Mapping wizard
// ============================================================
void runMappingWizard() {
    clearMapping();
    _serialBuf = "";

    printDivider();
    Serial.println(F("  Xbox Controller — Button Mapping Wizard"));
    printDivider();
    Serial.println(F("  For each Xbox input: physically PRESS the"));
    Serial.println(F("  button you want assigned to it."));
    Serial.println(F("  Type SKIP + Enter  to leave it unmapped."));
    Serial.println(F("  Type DEBUG + Enter to print all pin states."));
    printDivider();
    Serial.println();

    bool pinUsed[BTN_COUNT];
    memset(pinUsed, 0, sizeof(pinUsed));

    for (int i = 0; i < AXIS_START_IDX; i++) {
        while (true) {
            Serial.println();
            Serial.print(F(">>> Map  [ "));
            Serial.print(xboxInputNames[i]);
            Serial.println(F(" ]"));

            int pinIdx = waitForButtonPress();

            if (pinIdx == -1) {
                btnMapping[i] = 0xFF;
                Serial.print(F("  [ "));
                Serial.print(xboxInputNames[i]);
                Serial.println(F(" ]  =>  SKIPPED"));
                break;
            }

            if (pinUsed[pinIdx]) {
                Serial.print(F("  ! "));
                Serial.print(btnPinNames[pinIdx]);
                Serial.println(F(" already used. Use anyway? (Y/N): "));
                String ans = readSerialLineBlocking();
                if (ans != "Y") {
                    Serial.println(F("  OK, try again..."));
                    continue;
                }
            }

            btnMapping[i] = (uint8_t)pinIdx;
            pinUsed[pinIdx] = true;
            Serial.print(F("  [ "));
            Serial.print(xboxInputNames[i]);
            Serial.print(F(" ]  =>  "));
            Serial.println(btnPinNames[pinIdx]);
            break;
        }
    }

    // Joystick axes
    Serial.println();
    printDivider();
    Serial.println(F("  Joystick axes — type Y or N + Enter for each"));
    printDivider();

    const char* axisLabels[4] = {
        "Left Stick X  (GPIO32)",
        "Left Stick Y  (GPIO33)",
        "Right Stick X (GPIO34)",
        "Right Stick Y (GPIO35)"
    };
    for (int i = 0; i < 4; i++) {
        Serial.print(F("  Enable  "));
        Serial.print(axisLabels[i]);
        Serial.print(F(" ? (Y/N): "));
        String ans = readSerialLineBlocking();
        bool yes = (ans == "Y");
        btnMapping[AXIS_START_IDX + i] = yes ? 0 : 0xFF;
        Serial.println(yes ? F("  => Enabled") : F("  => Skipped"));
    }

    saveMapping();
    Serial.println();
    printDivider();
    Serial.println(F("  Done! Mapping saved."));
    Serial.println(F("  Commands: REMAP | SHOW | DEBUG"));
    printDivider();
    printCurrentMapping();
}

// ============================================================
//  Apply button
// ============================================================
void applyButton(int xi, bool pressed) {
    switch (xi) {
        case 0:  pressed ? gamepad->press(XBOX_BUTTON_A)      : gamepad->release(XBOX_BUTTON_A);      break;
        case 1:  pressed ? gamepad->press(XBOX_BUTTON_B)      : gamepad->release(XBOX_BUTTON_B);      break;
        case 2:  pressed ? gamepad->press(XBOX_BUTTON_X)      : gamepad->release(XBOX_BUTTON_X);      break;
        case 3:  pressed ? gamepad->press(XBOX_BUTTON_Y)      : gamepad->release(XBOX_BUTTON_Y);      break;
        case 4:  pressed ? gamepad->press(XBOX_BUTTON_LB)     : gamepad->release(XBOX_BUTTON_LB);     break;
        case 5:  pressed ? gamepad->press(XBOX_BUTTON_RB)     : gamepad->release(XBOX_BUTTON_RB);     break;
        case 6:  pressed ? gamepad->press(XBOX_BUTTON_START)  : gamepad->release(XBOX_BUTTON_START);  break;
        case 7:  pressed ? gamepad->press(XBOX_BUTTON_SELECT) : gamepad->release(XBOX_BUTTON_SELECT); break;
        case 8:  pressed ? gamepad->press(XBOX_BUTTON_LS)     : gamepad->release(XBOX_BUTTON_LS);     break;
        case 9:  pressed ? gamepad->press(XBOX_BUTTON_RS)     : gamepad->release(XBOX_BUTTON_RS);     break;
        // case 14: pressed ? gamepad->pressShare()              : gamepad->releaseShare();               break;
        // case 15: pressed ? gamepad->press(XBOX_BUTTON_HOME)   : gamepad->release(XBOX_BUTTON_HOME);   break;
    }
}

// ============================================================
//  Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1500); // wait for Serial Monitor to connect

    Serial.println(F("\n\nESP32 Xbox BLE Controller booting..."));

    EEPROM.begin(EEPROM_SIZE);

    // Set all button pins INPUT_PULLUP
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(btnPins[i], INPUT_PULLUP);
    }
    analogSetPinAttenuation(32, ADC_11db);
    analogSetPinAttenuation(33, ADC_11db);
    analogSetPinAttenuation(34, ADC_11db);
    analogSetPinAttenuation(35, ADC_11db);

    // Print initial pin states so you can verify wiring immediately
    Serial.println(F("Initial pin states at boot:"));
    debugPinStates();

    bool hasSaved = loadMapping();

    if (!hasSaved) {
        Serial.println(F("No saved mapping — starting wizard..."));
        runMappingWizard();
    } else {
        printCurrentMapping();
        Serial.println(F("Commands: REMAP | SHOW | DEBUG"));
    }

    XboxSeriesXControllerDeviceConfiguration* config =
        new XboxSeriesXControllerDeviceConfiguration();
    BLEHostConfiguration hostConfig = config->getIdealHostConfiguration();
    gamepad = new XboxGamepadDevice(config);
    compositeHID.addDevice(gamepad);
    compositeHID.begin(hostConfig);
    Serial.println(F("BLE started. Pair via Bluetooth."));
}

// ============================================================
//  Loop
// ============================================================

unsigned long lastDebugPrint = 0;

void loop() {
    String cmd;
    if (readSerialLine(cmd)) {
        if      (cmd == "REMAP") runMappingWizard();
        else if (cmd == "SHOW")  printCurrentMapping();
        else if (cmd == "DEBUG") debugPinStates();
        else {
            Serial.print(F("Unknown command: "));
            Serial.println(cmd);
            Serial.println(F("Commands: REMAP | SHOW | DEBUG"));
        }
    }

    if (!compositeHID.isConnected()) return;

    bool dpadUp=false, dpadDown=false, dpadLeft=false, dpadRight=false;

    for (int xi = 0; xi < AXIS_START_IDX; xi++) {
        if (btnMapping[xi] == 0xFF) continue;
        bool pressed = (digitalRead(btnPins[btnMapping[xi]]) == LOW);
        if (xi == 10) { dpadUp    = pressed; continue; }
        if (xi == 11) { dpadDown  = pressed; continue; }
        if (xi == 12) { dpadLeft  = pressed; continue; }
        if (xi == 13) { dpadRight = pressed; continue; }
        applyButton(xi, pressed);
    }

    uint8_t dpadFlags = 0;
    if (dpadUp)    dpadFlags |= (uint8_t)XboxDpadFlags::NORTH;
    if (dpadDown)  dpadFlags |= (uint8_t)XboxDpadFlags::SOUTH;
    if (dpadLeft)  dpadFlags |= (uint8_t)XboxDpadFlags::WEST;
    if (dpadRight) dpadFlags |= (uint8_t)XboxDpadFlags::EAST;
    if (dpadFlags) gamepad->pressDPadDirectionFlag(XboxDpadFlags(dpadFlags));
    else           gamepad->releaseDPad();

    int16_t lx=0, ly=0, rx=0, ry=0;
    if (btnMapping[16] != 0xFF) lx = map(readSmooth(JOY_L_X_PIN), 4095, 0, XBOX_STICK_MIN, XBOX_STICK_MAX);
    if (btnMapping[17] != 0xFF) ly = map(readSmooth(JOY_L_Y_PIN), 0, 4095, XBOX_STICK_MAX, XBOX_STICK_MIN);
    if (btnMapping[18] != 0xFF) rx = map(readSmooth(JOY_R_X_PIN), 4095, 0, XBOX_STICK_MIN, XBOX_STICK_MAX);
    if (btnMapping[19] != 0xFF) ry = map(readSmooth(JOY_R_Y_PIN), 0, 4095, XBOX_STICK_MAX, XBOX_STICK_MIN);
    uint16_t lt = digitalRead(btnPins[btnMapping[14]]) == LOW ? 1023 : 0;
    uint16_t rt = digitalRead(btnPins[btnMapping[15]]) == LOW ? 1023 : 0;

    gamepad->setLeftTrigger(lt);
    gamepad->setRightTrigger(rt);

    gamepad->setLeftThumb(lx, ly);
    gamepad->setRightThumb(rx, ry);
    gamepad->sendGamepadReport();
    delay(8);
}
