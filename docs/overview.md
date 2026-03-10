KOSMOS – Technical Overview (English)  
A compact generative MIDI sequencer for RP2040

---

## 1. Overview

KOSMOS is a standalone generative MIDI sequencer running on the **Raspberry Pi Pico (RP2040)**.  
It features a **240×240 LCD UI**, **Euclidean rhythm engine**, **melodic generators**,  
and **USB‑MIDI output**.  
Once powered, it immediately begins producing evolving musical patterns.

The system is fully self‑contained:  
no external clock, DAW, or configuration is required.

---

## 2. Hardware Requirements

- **Raspberry Pi Pico (RP2040)**
- **Waveshare Pico‑LCD‑1.3** (ST7789, 240×240)
- **Pico Omnibus / dual expander** (optional)
- **Analog joystick** (X/Y + push switch)
- **4 digital buttons** (A/B/X/Y)

### Pin Assignments (from code)

| Function | Pin |
|---------|-----|
| LCD DC  | GP8 |
| LCD CS  | GP9 |
| LCD RST | GP12 |
| LCD BL  | GP13 |
| LCD SCK | GP10 (SPI1) |
| LCD MOSI | GP11 (SPI1) |
| Buttons A/B/X/Y | GP15 / GP17 / GP19 / GP21 |
| Joystick X/Y | GP27 / GP26 |
| Joystick SW | GP3 |

---

## 3. Display System

The LCD is driven via **SPI1** at 24 MHz.  
The firmware includes:

- Low‑level ST7789 initialization  
- Pixel, rectangle, and text drawing routines  
- A built‑in **5×7 ASCII font**  
- UI layers for:
  - Euclidean parameters  
  - Probability bars  
  - Step indicators  
  - Note visualization  
  - Random mode indicator  
  - CC value display  

The UI is redrawn incrementally for performance.

---

## 4. Input System

### Buttons  
Four digital buttons (A/B/X/Y) are used for:

- Step selection  
- Probability editing  
- Euclid parameter editing  
- Random mode switching  

---

## 5. Sequencer Engine

KOSMOS contains **two independent tracks**:

### Track A – Melodic Generator
- Uses **Hirajoshi‑like in‑scale movement**
- Long‑form phrase direction (12–24 notes)
- Upward motion is expressive; downward motion is softened
- Occasional octave jumps at phrase peaks/valleys
- Transpose parameter affects both tracks

---

## 6. Euclidean Rhythm Engine

The Euclid generator creates a 16‑step pattern:

```
steps = 16
hits = 5
rotation = 0
```

The user can modify:

- **steps** (1–16)  
- **hits**  
- **rotation**  

A probability table (0–100%) is applied per step.

Random modes:

| Mode | Meaning |
|------|---------|
| 0 | Euclid randomization |
| 1 | Step randomization |
| 2 | Both |

Modes can change automatically every 8 seconds.

---

## 7. MIDI Output

KOSMOS uses **Adafruit TinyUSB** for USB‑MIDI.

Supported messages:

- **Note On / Note Off**  
- **CC input** (for external controllers like Korg nanoKONTROL)

Example:

```cpp
uint8_t msg[3] = {0x90, note, velocity};
usb_midi.write(msg, 3);
```

The device appears as a **USB‑MIDI class‑compliant device**.

---

## 8. Timing

- Internal BPM (default 120)
- Step BPM for Euclid progression
- Randomized note durations for natural phrasing
- Clock messages sent at standard MIDI timing

---

## 9. Visualizers

The firmware includes several visual elements:

- **Step bars** (active step highlighted)
- **Probability bars**
- **Note dots** (scrolling pitch visualization)
- **Generative background** (mode‑dependent)
- **LFO block animation**
- **Screen flash** for accents

---

## 10. File Structure (recommended)

```
KOSMOS/
├─ src/                 # Main firmware
├─ hardware/            # Schematics / PCB
├─ presets/             # Patterns / patches
└─ docs/                # Build & usage docs
```

---

## 11. Build Environment

- Arduino IDE  
- RP2040 board package  
- TinyUSB enabled  
- Compile as standard Pico sketch  
- Export UF2 via “Export Compiled Binary”

---

## 12. License Summary

- **Code** → MIT  
- **Hardware** → CERN‑OHL‑S  
- **Presets** → CC BY 4.0  

---
