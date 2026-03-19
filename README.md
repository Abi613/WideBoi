# 🎮 Wideboi — ESP32 Xbox BLE Telescopic Controller

> **Wideboi** is a custom Bluetooth Low Energy Xbox-compatible gamepad built on an ESP32, using a modified **Alpakka open-source controller** form factor redesigned as a **telescopic controller**, assembled on a custom **ProtoPCB**, and powered by the **BleCompositeHID** library.

---

## 📸 Overview

**Wideboi** turns an ESP32 microcontroller into a fully functional **Xbox Series X–style BLE HID gamepad**, recognized natively by Windows, Android, and other Xbox-compatible hosts — no drivers needed.

The name comes from the telescopic grip design: collapsed it's pocket-sized, extended it's wide and comfortable. The physical build is based on a **modified Alpakka open-source controller layout**, adapted into a **telescopic (extendable grip) form factor**. The PCB was designed and assembled on **ProtoPCB** prototyping board.

---

## ✨ Features

- 🎮 **Full Xbox Series X layout** — A, B, X, Y, LB, RB, LT, RT, Start, Select, L3, R3, D-Pad
- 🕹️ **Dual analog joysticks** via ESP32 ADC (4 axes, 8-sample averaged for smooth reads)
- 📡 **Bluetooth Low Energy (BLE HID)** — pairs like a real Xbox controller
- 🔁 **Interactive button mapping wizard** over Serial — assign any physical button to any Xbox input
- 💾 **EEPROM-backed mapping** — survives power cycles, no re-mapping needed after reboot
- 🔍 **Live debug mode** — print all GPIO pin states over Serial at any time
- 🧩 **Built on ProtoPCB** — hand-soldered prototype with clean routing
- 📐 **Telescopic grip design** — slides open for gaming, collapses for portability

---

## 🛠️ Hardware

### Microcontroller
| Component | Details |
|-----------|---------|
| MCU | ESP32 (38-pin DevKit or equivalent) |
| PCB | ProtoPCB prototype board |
| Power | USB or LiPo battery (with appropriate regulation) |

### Button GPIOs
| Index | GPIO | Suggested Mapping |
|-------|------|-------------------|
| 0  | GPIO4  | A |
| 1  | GPIO5  | B |
| 2  | GPIO16 | X |
| 3  | GPIO17 | Y |
| 4  | GPIO18 | LB |
| 5  | GPIO19 | RB |
| 6  | GPIO21 | Start |
| 7  | GPIO22 | Select |
| 8  | GPIO23 | L3 |
| 9  | GPIO25 | R3 |
| 10 | GPIO26 | D-Pad Up |
| 11 | GPIO27 | D-Pad Down |
| 12 | GPIO13 | D-Pad Left |
| 13 | GPIO14 | D-Pad Right |
| 14 | GPIO15 | Left Trigger |
| 15 | GPIO0  | Right Trigger |

> All button pins use **internal pull-up resistors**. Wire buttons between GPIO and **GND**.

### Analog Joystick ADC Pins
| Axis | GPIO | Notes |
|------|------|-------|
| Left Stick X  | GPIO35 | ADC1_CH7 |
| Left Stick Y  | GPIO34 | ADC1_CH6 |
| Right Stick X | GPIO33 | ADC1_CH5 |
| Right Stick Y | GPIO32 | ADC1_CH4 |

> All analog pins use **11dB attenuation** for full 0–3.3V range (0–4095 ADC counts).  
> Y axes are automatically inverted in firmware to match Xbox conventions.

---

## 📦 Dependencies

Install these libraries via Arduino Library Manager or PlatformIO:

| Library | Source |
|---------|--------|
| [BleCompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) | GitHub — Mystfit |
| Arduino ESP32 core | [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) |
| EEPROM (built-in) | ESP32 Arduino core |

### PlatformIO (`platformio.ini`)
```ini
[env:esp32doit-devkit-v1]
platform = espressif32 @ 6.8.1
board = esp32doit-devkit-v1
framework = arduino
lib_deps = 
        Callback
        h2zero/NimBLE-Arduino@~2.1.2
```

---

## 🚀 Getting Started

### 1. Clone the repo
```bash
git clone https://github.com/Abi613/wide.git
cd wide
```

### 2. Install dependencies
Using PlatformIO (recommended) or Arduino IDE with the BleCompositeHID library installed.

### 3. Flash the firmware
```bash
pio run --target upload
```
Or use Arduino IDE: open `main.cpp` (rename to `.ino` if needed), select your ESP32 board, and upload.

### 4. Open Serial Monitor
Set baud rate to **115200**. On first boot, the **mapping wizard** will launch automatically.

### 5. Map your buttons
Follow the on-screen wizard:
- Press the physical button you want assigned to each Xbox input
- Type `SKIP` + Enter to leave an input unmapped
- Type `DEBUG` + Enter at any time to print all GPIO states

### 6. Pair via Bluetooth
After mapping is saved, open Bluetooth settings on your host device and pair **"ESP32 SeriesX Controller"**.

---

## 🖥️ Serial Commands

Once mapped and running, the following commands are available at any time over Serial (115200 baud):

| Command | Action |
|---------|--------|
| `REMAP` | Re-run the full button mapping wizard |
| `SHOW`  | Print the current button mapping table |
| `DEBUG` | Print live state of all GPIO pins |

---

## 🔧 How It Works

### BLE HID
The firmware uses the **BleCompositeHID** library to present the ESP32 as a composite HID device with an Xbox Series X gamepad profile. This means the host OS sees it as a genuine Xbox controller — complete with correct button IDs, analog stick axes, triggers, and D-pad.

### Button Mapping
Mappings are stored as a `uint8_t btnMapping[20]` array in EEPROM. Each entry holds the `btnPins[]` index for that Xbox input, or `0xFF` if unmapped. This lets you physically rearrange buttons on the PCB without reflashing.

### Analog Smoothing
Each joystick axis is sampled 8 times per loop and averaged (`readSmooth()`) to reduce ADC noise — important for the ESP32's somewhat noisy ADC.

### D-Pad
D-pad directions are read as standard digital buttons and combined into an `XboxDpadFlags` bitmask, supporting diagonal inputs natively.

---

## 🧩 Physical Build — Wideboi (Alpakka Telescopic Modification)

This controller is a **hardware modification of the [Alpakka open-source controller](https://inputlabs.io/alpakka)** by Input Labs.

### Modifications Made
- **Telescopic grip mechanism** — The controller grips extend and retract, allowing the device to collapse to a compact size and expand to a comfortable full-size grip. Designed for portability and travel.
- **ESP32 substitution** — The original Alpakka uses an RP2040. This build replaces it with an ESP32 for native BLE support without additional hardware.
- **ProtoPCB assembly** — Rather than ordering a custom PCB, all connections are hand-routed on ProtoPCB prototyping board.
- **Analog joystick integration** — Two analog thumbsticks wired directly to ESP32 ADC pins.

> Credit to **[Input Labs](https://inputlabs.io/)** for the original open-source Alpakka controller design and files.

---

## 📁 Project Structure

```
wide/
├── src/
│   └── main.cpp          # Main firmware
├── platformio.ini         # PlatformIO config
├── LICENSE
└── README.md
```

---

## ⚠️ Known Limitations

- **ESP32 ADC non-linearity** — The ESP32's ADC has known non-linearity near the rails (0V and 3.3V). Joystick dead zones may need software compensation for precision use.
- **GPIO0 boot behavior** — GPIO0 is used as a button pin but is also the ESP32 boot-mode pin. Avoid holding it LOW during power-on/reset.
- **Single profile** — Only one button mapping profile is stored at a time. Extend the EEPROM layout to support multiple profiles if needed.
- **No rumble** — BLE HID rumble (force feedback) is not currently implemented.

---

## 🗺️ Roadmap / Ideas

- [ ] Analog trigger support (replace digital LT/RT with ADC-based analog triggers)
- [ ] Multiple saved mapping profiles
- [ ] Battery level reporting over BLE
- [ ] Rumble support via BLE HID output reports
- [ ] 3D-printed telescopic housing design files

---

## 📄 License

This project is released under the **MIT License** — see [`LICENSE`](LICENSE) for details.

The Alpakka controller design this is based on is released under the **CERN Open Hardware Licence v2** by Input Labs. See [inputlabs.io/alpakka](https://inputlabs.io/alpakka) for their licensing terms.

---

## 🙏 Credits

| Credit | Link |
|--------|------|
| **BleCompositeHID library** | [Mystfit/ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) |
| **Alpakka open-source controller** | [inputlabs.io/alpakka](https://inputlabs.io/alpakka) |
| **ESP32 Arduino core** | [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) |

---

*Wideboi — built with an ESP32, a soldering iron, and too much free time.* 🔧
