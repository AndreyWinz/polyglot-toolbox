# wave-gen

A standalone, zero-dependency C++17 signal generator that creates standard 16-bit PCM `.wav` audio files for waveforms, DTMF tones, and Morse code CW signaling.

## Features
- **Standard Waveforms:** Sine, Square, Sawtooth, and Triangle waves with configurable frequency, duration, and sample rate.
- **DTMF Keypad Encoder:** Synthesises standard dual-tone multi-frequency keypress signals for telephone dialling tests.
- **Morse Code CW Generator:** Translates text strings into CW keying at selectable frequencies and Words-Per-Minute (WPM) speeds using standard Paris timing metrics.

## Building

```bash
cd cpp/wave-gen
mkdir build && cd build
cmake ..
make
```

The output binary will be located at `cpp/wave-gen/build/wave-gen`.

## Usage

### 1. Generate a 1000 Hz Sine Wave

```bash
./wave-gen -t sine -f 1000 -d 3 -o 1khz.wav
```

### 2. Generate DTMF Keypad Tones

```bash
./wave-gen -t dtmf --text "1234#" -o dtmf.wav
```

### 3. Generate Morse Code CW

```bash
./wave-gen -t morse --text "CQ CQ DE SOTA" -f 700 --wpm 20 -o morse.wav
```
