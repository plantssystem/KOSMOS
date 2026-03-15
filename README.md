# KOSMOS v1.1.0  
Generative MIDI Sequencer + Integrated PRA32-U Sound Engine

KOSMOS is a generative music instrument built on the RP2040 (Raspberry Pi Pico).  
While v1.0.0 functioned purely as a **Generative MIDI Sequencer**,  
v1.1.0 introduces a major evolution:  
KOSMOS now integrates the **PRA32-U synthesizer engine** (by Ishigaki Instruments) on Core1,  
allowing the device to **generate sound directly without any external synthesizer**.

This release transforms KOSMOS into a fully self‑contained **generative synthesizer**.

---

## 🌟 New in v1.1.0

### 🔊 PRA32-U Sound Engine Integration (Core1)
- Powered by ISGK Instruments PRA32-U
- The PRA32-U synthesizer engine is now embedded directly into KOSMOS.  
- Core0 runs the KOSMOS generative sequencer.  
- Core1 runs the PRA32-U sound engine.  
- KOSMOS can now output audio on its own, without relying on external MIDI devices.

This is the largest functional upgrade since the project began.

---

## 🖥️ LCD Disabled in This Version

The Waveshare Pico-LCD-1.3 is **not used in v1.1.0** due to pin conflicts:

- The LCD’s SPI pins  
- The Pimoroni Pico Audio Pack’s I2S pins  

These overlap on the RP2040, making simultaneous use unstable.  
To ensure reliable audio output, the LCD is disabled for this release.

---

## 🛠️ Recommended Hardware Configuration

KOSMOS v1.1.0 is designed for the following three‑module setup:

- **Raspberry Pi Pico (RP2040)**  
- **Omnibus Dual Expander**  
- **Pimoroni Pico Audio Pack (I2S DAC)**  

This configuration provides:

- Stable I2S audio output  
- PRA32-U running on Core1  
- Clean separation between sequencing and synthesis tasks  

---

## 🌀 Evolution: v1.0.0 → v1.1.0

| Version | Description |
|--------|-------------|
| **v1.0.0** | Pure Generative MIDI Sequencer (external synth required) |
| **v1.1.0** | Integrated PRA32-U engine → KOSMOS outputs sound directly |

KOSMOS is no longer just a sequencer.  
It is now a **self-contained generative synthesizer**.

---

KOSMOS v1.1.0 separates sequencing and synthesis across the RP2040’s dual cores:

- **Core0** runs the KOSMOS generative sequencer  
- **Core1** runs the PRA32-U synthesizer engine  
- Audio is output via **Pimoroni Pico Audio Pack (I2S DAC)**  
- All modules are mounted on the **Omnibus Dual Expander**  
- LCD is disabled in this version due to SPI/I2S pin conflicts

## 🏗️ System Architecture (KOSMOS v1.1.0)

                   ┌──────────────────────────────────────┐
                   │            Raspberry Pi Pico         │
                   │                (RP2040)              │
                   ├──────────────────────────────────────┤
                   │                                      │
                   │   Core0 (KOSMOS Sequencer Engine)    │
                   │   - Generative phrase logic          │
                   │   - MIDI event generation            │
                   │   - Timing / clock management        │
                   │                                      │
                   ├──────────────────────────────────────┤
                   │                                      │
                   │   Core1 (PRA32-U Synth Engine)       │
                   │   - Synth engine                     │
                   │   - Envelope / modulation            │
                   │   - Voice rendering                  │
                   │                                      │
                   └──────────────────────────────────────┘
                                   │
                                   │ I2S Audio
                                   ▼
                   ┌──────────────────────────────────────┐
                   │      Pimoroni Pico Audio Pack        │
                   │      - I2S DAC output                │
                   │      - Line out / headphone out      │
                   └──────────────────────────────────────┘
                                   │
                                   │ Mounted on
                                   ▼
                   ┌──────────────────────────────────────┐
                   │        Omnibus Dual Expander         │
                   │  - Provides stable mounting & power  │
                   │  - Supports multi‑module stacking    │
                   └──────────────────────────────────────┘

Notes:
- Waveshare Pico-LCD-1.3 is **disabled** in v1.1.0 due to SPI/I2S pin conflict.
- KOSMOS now functions as a standalone generative synthesizer.

---

## 🚀 Roadmap

- Reintroduce LCD support (alternative pin mapping or display module)
- Real‑time PRA32-U parameter control
- Expanded generative phrasing algorithms
- Optional UI layer when hardware permits

---

## 📜 License

- **KOSMOS**: MIT License  
- **PRA32-U**: Follows Ishigaki Instruments licensing  

---

## ✨ Special Thanks

- **Ishigaki Instruments** — for providing the PRA32-U Core1 implementation  
- The generative music & DIY synth community  
