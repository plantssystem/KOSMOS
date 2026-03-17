***

# KOSMOS v1.2.0

KOSMOS is a **compact Generative MIDI Sequencer** project based on the Raspberry Pi Pico / RP2040.  
In v1.2.0, while building on the existing core functionality, we significantly enhance the **user interface and audio output**, aiming to bring the hardware one step closer to a more complete and refined form.

---

## Key Enhancements in v1.2.0

### ? Official Introduction of an LCD-Based User Interface

In v1.2.0, an LCD-centered user interface has been fully implemented.

- Step states and parameters can be **visually monitored at a glance**
- A layout designed for intuitive operation during live performance
- Careful tuning of displayed content and refresh timing to ensure clarity even on a small screen

Rather than being a simple debugging display,  
the UI is designed with the goal of becoming  
**“an interface you can physically play as an instrument.”**

---

### ? Audio Output via PCM5102 (I2S DAC)

In addition to traditional MIDI output,  
v1.2.0 introduces **audio output using a PCM5102 I2S DAC**.

- Stable audio output utilizing the RP2040’s I2S capabilities
- A simple analog circuit design to minimize noise
- Practical sound quality achieved within a compact, low-power configuration

With this addition, KOSMOS moves beyond being just a MIDI generator,  
bringing it closer to a **standalone sequencer capable of producing sound on its own**.

---

## Design and Fine-Tuning Toward a “Finished Form”

In v1.2.0, the focus is not only on adding new features, but on refining the overall balance of the system.

- Re-evaluating the role allocation between Core0 and Core1
- Timing adjustments to prevent interference between LCD rendering and audio processing
- Structural cleanup with future expansion in mind (alternative DACs, external modules)

Rather than simply increasing functionality, the highest priorities are:

- **Stability over long periods of operation**
- **A natural, comfortable feel when interacting with the physical device**

Careful and deliberate tuning continues throughout the system.

---

## Positioning of v1.2.0

KOSMOS v1.2.0 is not the final version, but with the combination of:

- User Interface
- Audio Output
- Hardware Integration

it represents a key transition point?from a  
**“concept prototype” to a “practical musical instrument.”**

Through continued testing and refinement on real hardware,  
KOSMOS will keep evolving into a generative sequencer that is genuinely enjoyable to pick up and play.

---

## Development Status

- Target MCU: RP2040 (Raspberry Pi Pico compatible)
- LCD: SPI-connected display
- Audio DAC: PCM5102 (I2S)
- Development Environment: Arduino / C++

Detailed wiring examples and sketch structure will be added to the repository progressively.
```

***

# System Overview (Role Separation)

| Subsystem  | Description        |
| ---------- | ------------------ |
| Display    | Waveshare SPI LCD  |
| Audio      | PCM5102 (I2S DAC)  |
| Processing | Pico Core0 / Core1 |
| Power      | Supplied from Pico |

**Design Policy:**

*   **LCD uses fixed SPI**
*   **Audio is fully separated via I2S**
*   **Leave GPIOs available for future expansion**

***

# Recommended GPIO Assignment (Final)

## Waveshare LCD (SPI)

| LCD Signal | Pico GPIO  | Notes                   |
| ---------- | ---------- | ----------------------- |
| SCK        | **GPIO18** | SPI0 SCK                |
| MOSI       | **GPIO19** | SPI0 TX                 |
| CS         | **GPIO17** | Arbitrary               |
| DC         | **GPIO16** | Data / Command          |
| RST        | **GPIO20** | Reset                   |
| BL         | **GPIO21** | Backlight (PWM capable) |
| VCC        | 3.3V       |                         |
| GND        | GND        |                         |

**Official Waveshare configuration, verified stable on real hardware**

***

## PCM5102 (I2S DAC)

### Complete Non?interference with LCD, USB, and Expanders

| PCM5102 Signal | Pico GPIO  | Notes            |
| -------------- | ---------- | ---------------- |
| BCK            | **GPIO2**  | I2S Bit Clock    |
| LRCK           | **GPIO3**  | I2S Word Select  |
| DIN            | **GPIO4**  | I2S Data         |
| VCC            | 5V or 3.3V | Module dependent |
| GND            | GND        |                  |

**GPIO2?4 do not conflict with LCD or USB**

***

## Power Wiring (Important)

    Pico 5V (VSYS) ── PCM5102 VCC
    Pico GND       ── PCM5102 GND

*   Low noise, reduced audio distortion
*   Modules supporting 3.3V operation may use **3V3**

***

# Physical Wiring Diagram (ASCII)

    ┌────────────────┐
    │ Raspberry Pi   │
    │ Pico           │
    │                │
    │ GPIO18 ────┐   │  SPI SCK → LCD
    │ GPIO19 ────┼───┘  SPI MOSI → LCD
    │ GPIO17 ────┘       CS → LCD
    │ GPIO16 ────────── DC → LCD
    │ GPIO20 ────────── RST → LCD
    │ GPIO21 ────────── BL → LCD
    │                │
    │ GPIO2  ───────── BCK → PCM5102
    │ GPIO3  ───────── LRCK → PCM5102
    │ GPIO4  ───────── DIN → PCM5102
    │                │
    │ 5V (VSYS) ────── VCC → PCM5102
    │ GND       ────── GND → PCM5102
    └────────────────┘

***

# Why This Layout Is Optimal

### Zero Pin Conflicts

*   LCD: GPIO16?21
*   Audio: GPIO2?4  
    → **Completely independent**

### Easy Core Separation

*   Core1 → Audio (I2S)
*   Core0 → LCD / UI / MIDI

### Easy Expansion

*   GPIO6?15 fully available
*   Room for Omnibus Expander, clock input, LEDs, etc.

***

# Notes (Mandatory Checks)

### PCM5102 MUTE / FLT Pins

*   Most modules work **with pins left unconnected**
*   If no audio output is observed:

<!---->

    MUTE → GND

### Wiring Length

*   Keep **BCK / LRCK / DIN as short as possible**
*   Jumper wires: **5?10 cm maximum recommended**
***
