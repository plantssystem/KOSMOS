KOSMOS – Hardware Overview (English)  
A compact generative MIDI sequencer built on the RP2040 platform

---

## 1. Hardware Summary

KOSMOS is built around the **Raspberry Pi Pico (RP2040)** and a **240×240 SPI LCD**.  
The design focuses on minimal wiring, low power consumption, and immediate standalone operation.

The system includes:

- Raspberry Pi Pico  
- Waveshare Pico‑LCD‑1.3 (ST7789)  
- 4 digital buttons (A/B/X/Y)  
- 1 analog joystick (X/Y + push switch)  
- Optional: Pico Omnibus / dual‑expander board  
- USB‑MIDI via the Pico’s USB port  

No external clock, storage, or audio hardware is required.

---

## 2. Wiring Diagram (Text Version)

### **LCD (Waveshare Pico‑LCD‑1.3)**  
Connected to **SPI1**:

| LCD Signal | Pico Pin | Notes |
|------------|----------|-------|
| DC         | GP8      | Data/Command |
| CS         | GP9      | Chip Select |
| RST        | GP12     | Reset |
| BL         | GP13     | Backlight |
| SCK        | GP10     | SPI1 SCK |
| MOSI       | GP11     | SPI1 TX |
| MISO       | —        | Not used |

The LCD is powered directly from the Pico’s **3.3V** and **GND** pins.

---

### **Buttons (A/B/X/Y)**  
Each button is wired:

- One side → **Pico GPIO**  
- Other side → **GND**  
- Firmware uses **INPUT_PULLUP**

| Button | Pico Pin |
|--------|----------|
| A      | GP15 |
| B      | GP17 |
| X      | GP19 |
| Y      | GP21 |

---

## 3. Power & USB

- Powered via **USB‑C / USB‑micro** (depending on Pico board)  
- USB connection provides:
  - Power  
  - USB‑MIDI device interface  
- No external 5V regulator is required  
- Typical current draw: **40–70 mA** (LCD backlight on)

---

## 4. Optional Hardware

### **Pico Omnibus / Dual Expander**
Recommended for:

- Easier access to GPIO pins  
- Mechanical stability  
- Cleaner wiring for LCD + joystick + buttons  

### **Enclosure**
KOSMOS can be mounted in:

- 3D‑printed case  
- Laser‑cut acrylic frame  
- Breadboard prototype  
- Eurorack‑style panel (USB‑powered)

---

## 5. Mechanical Layout (Suggested)

A typical layout:

```
 ┌───────────────────────────────┐
 │   240×240 LCD (Main UI)       │
 ├───────────────────────────────┤
 │  A   B   X   Y   (Buttons)    │
 │                               │
 │      Joystick (X/Y + SW)      │
 └───────────────────────────────┘
```

The LCD is the primary interface; buttons and joystick provide parameter control.

---

## 6. Electrical Notes

- All GPIO used are **3.3V‑safe** (RP2040 requirement)  
- No level shifting required  
- LCD backlight is driven directly from GP13  
- SPI1 runs at **24 MHz** for fast UI rendering  
- Buttons use internal pull‑ups; no external resistors needed  
- Joystick requires stable 3.3V reference for accurate analog readings  

---

## 7. Bill of Materials (Minimal)

| Item | Qty | Notes |
|------|-----|-------|
| Raspberry Pi Pico | 1 | RP2040 |
| Waveshare Pico‑LCD‑1.3 | 1 | ST7789 |
| Tactile button | 4 | A/B/X/Y |
| Analog joystick | 1 | X/Y + SW |
| Wires / headers | — | Solder or jumper |
| USB cable | 1 | Power + MIDI |

---

## 8. Assembly Steps (Short)

1. Solder headers to the Pico  
2. Mount Pico on Omnibus (optional)  
3. Connect LCD to SPI1 pins  
4. Connect buttons to GPIO + GND  
5. Connect joystick to GP27/26/3  
6. Power via USB  
7. Flash UF2 → KOSMOS boots immediately  

---
