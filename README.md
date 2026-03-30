# KOSMOS  
A generative MIDI sequencer for RP2040  
Version **v1.2.1**

KOSMOS is a standalone generative MIDI sequencer built on the RP2040.  
It blends Euclidean rhythms, step sequencing, and expressive parameter mapping into a compact, minimal, and musical device.

KOSMOS includes a built‑in **multi‑timbral digital synth engine based on ISGK Instruments' PRA32‑U**, running on Core1.  
This allows KOSMOS to function not only as a sequencer but also as a self‑contained sound module when desired.

This version **v1.2.1** focuses on refining the sequencer engine for more stable, expressive, and predictable generative behavior.

---

## 🎬 Demo Video

[![KOSMOS Demo (YouTube Shorts)](https://img.youtube.com/vi/pYgWGcJLjj8/0.jpg)](https://youtube.com/shorts/pYgWGcJLjj8)

---

## 🌌 Features

### 🎛 Generative Sequencer
- Hybrid engine combining **Euclidean**, **Step**, and **Both** modes  
- Scale‑aware arpeggio generation  
- Scene‑based transitions with speed, width, density, direction, and duration mapping  
- Improved timing stability and event scheduling (v1.2.1)

### 🎹 Built‑in Digital Synth Engine (PRA32‑U Multi‑Timbral)
- Powered by **ISGK Instruments PRA32‑U**  
- Multi‑timbral digital synthesis  
- Runs on RP2040 Core1 for stable audio performance  
- Supports CC‑based parameter control  
- Clean, low‑noise output via PCM5102 DAC or Waveshare PicoAudio

### 🖥 LCD User Interface (240×240)
- Real‑time step bar visualization  
- Parameter display with live updates  
- Mode indicators (E / S / B)  
- Smooth rendering on Core0 for stable UI performance  

### 🔌 Connectivity
- Standard MIDI OUT  
- USB‑MIDI support  
- Runs on RP2040 (Raspberry Pi Pico / Waveshare PicoAudio recommended)

---

## 🆕 What’s New in v1.2.1

### 🔧 Sequencer Engine Improvements
- Refined event scheduling for tighter rhythmic accuracy  
- Improved step/Euclid hybrid behavior in **Both** mode  
- Reduced micro‑timing drift during long generative sessions  
- More predictable arpeggio width & direction transitions  
- Enhanced internal state recovery after rapid parameter changes  

### 🖥 UI Enhancements
- Step bar update timing improved for smoother animation  
- Reduced flicker and unnecessary redraws  
- More responsive parameter display

### 🎼 Musicality Improvements
- Better distribution of notes in high‑density Euclid patterns  
- More natural phrasing when switching scales or scenes  
- Improved consistency between visual steps and generated events

---

## 📦 Installation

### 1. Download the Firmware
Grab the latest UF2 from the **Releases** page:  
`KOSMOS_v1.2.1.uf2`

### 2. Flash to RP2040
1. Hold **BOOTSEL** on your RP2040  
2. Connect via USB  
3. Copy the UF2 file to the mounted drive  

### 3. Connect Hardware
- RP2040 board (Pico or Waveshare PicoAudio)  
- PCM5102 DAC (if not using PicoAudio)  
- MIDI OUT or USB‑MIDI to your DAW/synth  

---

## 🎹 Usage

### Modes
- **E** – Euclidean rhythm generator  
- **S** – Step sequencer  
- **B** – Hybrid mode (Euclid × Step)

### Parameters
- **Speed** – Arpeggio rate  
- **Width** – Interval spread  
- **Density** – Event probability / Euclid fill  
- **Direction** – Up / Down / Ping‑pong / Random  
- **Duration** – Note length  

All parameters update in real time and are reflected on the LCD.

---

## 🛠 Hardware

### Recommended
- PCM5102  
- 240×240 SPI LCD (ST7789)  
- RP2040 (Raspberry Pi Pico)

## 📄 License
KOSMOS is released under the **MIT License**.  
The built‑in sound engine is based on **PRA32‑U (CC0 License)** by ISGK Instruments.

---

## 🌠 Special Thanks
**Powered by ISGK Instruments PRA32‑U**  

KOSMOS is designed as a minimal, expressive generative sequencer—  
a device that breathes, reacts, and evolves with musical intention.
