KOSMOS.sf2 – SoundFont Overview (English)

## 1. What is KOSMOS.sf2?

KOSMOS.sf2 is a lightweight SoundFont used during development and testing of the  
KOSMOS generative MIDI sequencer.  
It provides a clean, neutral timbre suitable for verifying melodic output,  
Euclidean patterns, and USB‑MIDI timing.

This SoundFont is included so users can immediately hear KOSMOS’s behavior  
without requiring external synthesizers or commercial plugins.

---

## 2. Purpose

KOSMOS.sf2 was designed for:

- **MIDI output testing**  
- **Scale and mode verification** (Hirajoshi, Insen, etc.)  
- **Arpeggio and phrase‑direction debugging**  
- **Clock and timing validation**  
- **Quick standalone playback on mobile/desktop SoundFont players**

It is intentionally simple and CPU‑light, making it ideal for mobile apps such as:

- bs‑16i  
- FluidSynth  
- SFZ/SF2 compatible soft synths  
- Android/iOS SoundFont players

---

## 3. Sound Characteristics

- Clean, stable tone  
- Fast attack for rhythmic clarity  
- Neutral harmonic content  
- Works well for:
  - Euclidean pulses  
  - Arpeggiated lines  
  - Long‑form generative phrasing  
  - Bass + lead separation  

The goal is **clarity**, not realism—perfect for debugging generative logic.

---

## 4. File Information

| Property | Value |
|---------|-------|
| Format | SoundFont 2 (.sf2) |
| Channels | Mono |
| Velocity response | Linear |
| Looping | Enabled (where applicable) |
| Size | Small footprint for mobile use |

---

## 5. Usage

### **Desktop**
Load into any SF2‑compatible synthesizer:

- FluidSynth  
- Polyphone  
- Sforzando (via SFZ wrapper)  
- MuseScore / LMMS / Qsynth  

### **Mobile**
Recommended apps:

- **bs‑16i** (iOS / Android)  
- **Audio Evolution Mobile**  
- **SoundFont Pro**  

### **Hardware**
Can be used with:

- Zynthian  
- MOD Devices  
- Any SF2‑compatible sampler

---

## 6. License

KOSMOS.sf2 is distributed under:

### **Creative Commons Attribution 4.0 (CC BY 4.0)**

You may:

- Use it in music  
- Modify it  
- Redistribute it  
- Use it commercially  

**Requirement:**  
Include attribution such as:

> “KOSMOS.sf2 by Sugimoto — licensed under CC BY 4.0”

---

## 7. Folder Structure

```
presets/
└─ KOSMOS.sf2
└─ README.md (this document)
```

---

## 8. Notes for Developers

- The SoundFont is intentionally minimal to expose MIDI behavior clearly  
- Ideal for testing:
  - NoteOn/NoteOff timing  
  - Probability‑based step triggering  
  - Phrase direction logic  
  - Transpose behavior  
  - USB‑MIDI throughput  
- Works well with KOSMOS’s internal pitch range (C2–C6)

---
