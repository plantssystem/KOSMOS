# KOSMOS v1.3.4  
### *Generative  Instrument for RP2040 + Waveshare Pico-Audio + Pico-LCD-1.3*

KOSMOS is a self‑contained **generative  instrument** built on the RP2040.  
It combines:

- **PRA32-U** dual‑channel synthesizer (via Waveshare Pico-Audio)  
- **Pico-LCD-1.3** for real‑time visual feedback  
- A custom generative engine that produces **organic, breathing musical motion**

Version **v1.3.4** introduces a major upgrade to the **step bar visualization**, greatly improving clarity and musical awareness during performance.

---

# 🎬 Demo Video

[![YouTube Shorts](https://img.youtube.com/vi/SoKy0JxPQFU/maxresdefault.jpg)](https://youtube.com/shorts/SoKy0JxPQFU)

---

# ✨ Features

## 🎹 Generative  Engine
- Scale‑dependent behavior (HEI / MIYA / INSEN)
- Smooth, jumping, or deep expressive movement
- Randomized duration extension for “breathing” phrasing
- Dual‑channel PRA32-U synthesis (Main + Sub)
- USB MIDI output + internal MIDI bridge

## 🎨 Visual Feedback (v1.3.4)
The step bar now uses **color-coded full-fill rendering** to show pattern state, playhead position, and expressive timing.

### Step Colors
| Color | Meaning |
|-------|---------|
| **G (Green)** | Pattern ON (`mainPattern[i] == 1`) |
| **W (Gray)** | Pattern OFF (`mainPattern[i] == 0`) |
| **R (Red)** | Current step (playhead) |
| **O (Orange)** | Extended-duration note (A-part only) |

### Example
```
[W][G][W][G][W][G][W][G][W][G][W][G][W][G][W][G]
                               [R] ← current step
                               [O] ← extended note
```

---

# 🆕 Improvements in v1.3.4

## ✔ 1. Full-Fill Step Rendering
All 16 steps are now drawn as **solid blocks**, eliminating visibility issues and ensuring stable rendering even during fast BPM.

## ✔ 2. Immediate Pattern Reflection
Whenever `mainPattern[]` changes, all steps are redrawn instantly, making pattern transitions visually obvious.

## ✔ 3. Expressive Timing Visualization (O = Orange)
When the A-part randomly extends a note’s duration, the current step briefly turns **O (Orange)**.  
This makes expressive timing changes visible and intuitive.

## ✔ 4. Smooth Behavior Without Visual “Hiccups”
The timing of R/O transitions has been refined to avoid conflicts between:

- step progression  
- noteOn / noteOff  
- duration extension  

The result is a smooth, natural visual flow.

---

# 🧠 Internal Architecture

## Core1 (RP2040)
- PRA32-U synthesizer (Main + Sub)
- I2S audio output
- MIDI event queue processing

## Core0
- KOSMOS generative engine
- Step sequencer
- Pattern generation
- LCD rendering
- USB MIDI output

---

# 🎛 Controls

| Button | Function |
|--------|----------|
| **A** | Program change (ch1) |
| **B** | Program change (ch2) |
| **A + B** | MIDI Start / Stop |
| **X** | Mute toggle (A / B / ALL) |
| **Y** | BPM cycle (20 / 80 / 140 / 200 / 260) |
| **Joystick** | Scale switching & future expansion |

---

# 🎼 Scales

| Mode | Character |
|------|-----------|
| **HEI** | Bright, fast, jumping |
| **MIYA** | Emotional, slower |
| **INSEN** | Deep, traditional, sticky |

Each scale changes:

-  speed  
- Movement width  
- Direction behavior  
- Duration extension probability  

---

# 📦 Hardware Requirements

- Raspberry Pi Pico  
- Waveshare Pico-Audio  
- Waveshare Pico-LCD-1.3  
- USB MIDI (optional)  
- 3.5mm audio output  

---

# 📁 Source Structure

```
/src
  ├── kosmos_main.cpp
  ├── pra32-u-common.h
  ├── pra32-u-synth.h
  └── ...
```

---

# 📝 License
MIT License

---

# 🙌 Author
**Sugimoto — KOSMOS Project**

---

# 🌟 Special Thanks
- MATRIXSYNTH
- Powerd by ISGK Instruments PRA32-U
