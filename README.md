# Pixel Buzz Box

A real-time arcade survival game for the Raspberry Pi Pico featuring a bee collecting pollen in an infinite procedurally-generated world.

<!-- TODO: Add demo video/image here -->
![Pixel Buzz Box Demo](demo.gif)

## Overview

Control a bee navigating an endless world to collect pollen from flowers and return it to the hive. Manage your survival timer by making successful deliveries—each pollen deposited extends your time. Features spring-based physics, a radar system, boost mechanics, and immersive buzzer-driven audio feedback.

## Hardware Requirements

### Components

- Raspberry Pi Pico (RP2040)
- ST7789 TFT LCD Display (240x320, 16-bit color)
- Analog Joystick with button
- Piezo Buzzer

### Pin Configuration

| Component | GPIO | Purpose |
|-----------|------|---------|
| LCD Backlight | GP16 | Display brightness |
| LCD CS | GP17 | SPI chip select |
| LCD SCK | GP18 | SPI clock |
| LCD MOSI | GP19 | SPI data |
| LCD DC | GP20 | Data/Command select |
| LCD Reset | GP21 | Display reset |
| Joystick Button | GP22 | Click input |
| Joystick X | GP26 (ADC0) | Analog X axis |
| Joystick Y | GP27 (ADC1) | Analog Y axis |
| Buzzer | GP15 | PWM audio output |

## Building & Uploading

### Arduino IDE Setup

1. Open **File > Preferences**
2. Add the following URL to **Additional Board Manager URLs**:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. Open **Tools > Board > Boards Manager**
4. Search for "pico" and install **Raspberry Pi Pico/RP2040** by Earle F. Philhower

### Installing Libraries

Install the following libraries via **Sketch > Include Library > Manage Libraries**:

| Library | Purpose |
|---------|---------|
| Adafruit GFX Library | Graphics primitives |
| Adafruit ST7789 | Display driver |

### Upload

1. Select **Tools > Board > Raspberry Pi Pico**
2. Connect the Pico while holding BOOTSEL (first time only)
3. Select the appropriate port
4. Click Upload

## Controls

| Input | Action |
|-------|--------|
| Joystick | Move bee toward target position |
| Joystick Button | Ping radar for navigation |
| Release Joystick | Return to hive |

## Gameplay

### Objective

Collect pollen from flowers and deliver it to the hive before your survival timer runs out.

### Mechanics

- **Pollen Collection**: Fly over flowers to automatically collect pollen (max 8)
- **Hive Delivery**: Return to the hive center to deposit pollen and gain survival time
- **Radar**: Press the joystick button to ping—shows nearest flower (empty) or hive direction (carrying pollen)
- **Boost**: Triggered automatically when collecting flowers; increases speed and camera zoom
- **Survival Timer**: Starts at 15 seconds; replenished by delivering pollen

### Scoring

Each pollen delivered scores 1 point and extends your survival time. Chain deliveries for bonus time multipliers.

## Audio Design

The buzzer provides dynamic audio feedback:

- **Ambient Buzz**: Pitch modulates with wing speed, turning, and acceleration
- **Radar Ping**: 3-tone chirp sequence for navigation
- **Collection Chirp**: Ascending tones on flower pickup
- **Deposit Sequence**: Rising arpeggio during pollen unload

## License

This project is licensed under the [Human-Scale Source License v1.2](LICENSE).

## Contributing

Contributions welcome! Please open an issue or pull request.
