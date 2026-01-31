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

### PlatformIO (Recommended)

PlatformIO automatically handles the RP2040 board core and library dependencies.

#### VSCode

1. Install the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
2. Open this folder in VSCode
3. Click the PlatformIO icon in the sidebar
4. Click **Build** or **Upload**

#### Command Line

```bash
# Install PlatformIO Core
pip install platformio

# Build
pio run

# Upload (connect Pico while holding BOOTSEL first time)
pio run --target upload
```

### Arduino IDE (Alternative)

<details>
<summary>Click to expand Arduino IDE instructions</summary>

#### Board Setup

1. Open **File > Preferences**
2. Add the following URL to **Additional Board Manager URLs**:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. Open **Tools > Board > Boards Manager**
4. Search for "pico" and install **Raspberry Pi Pico/RP2040** by Earle F. Philhower

#### Installing Libraries

Install the following libraries via **Sketch > Include Library > Manage Libraries**:

| Library | Purpose |
|---------|---------|
| Adafruit GFX Library | Graphics primitives |
| Adafruit ST7789 | Display driver |

Additionally, copy `lib/BuzzSynth/` to your Arduino libraries folder for audio synthesis.

#### Upload

1. Select **Tools > Board > Raspberry Pi Pico**
2. Connect the Pico while holding BOOTSEL (first time only)
3. Select the appropriate port
4. Click Upload

</details>

## Controls

| Input | Action |
|-------|--------|
| Joystick | Move bee toward target position |
| Push Joystick Down | Boost (1.2x speed multiplier) |
| Click Joystick | Ping radar for navigation |
| Release Joystick | Snap back toward hive center |

## Gameplay

### Objective

Collect pollen from flowers and deliver it to the hive before your survival timer runs out.

### Mechanics

- **Pollen Collection**: Fly over flowers to automatically collect pollen (max 8)
- **Hive Delivery**: Return to the hive center to deposit pollen and gain survival time
- **Radar**: Click joystick to ping—shows nearest flower (empty) or hive direction (carrying pollen)
- **Boost**: Auto-triggered on flower collection or manual via push-down; increases speed (1.2x) and camera zoom (1.22x) with screen shake and trail particles
- **Survival Timer**: Starts at 15 seconds; each pollen delivered adds 0.65s base + 0.55s per pollen carried

### Scoring

Each pollen delivered scores 1 point with a score popup animation. Time gain scales with pollen carried: more pollen per trip yields better returns. Chain deposits for visual feedback via belt item animations and hive pulse effects.

## Audio Design

The custom **BuzzSynth** library drives a piezo buzzer with dynamic audio feedback:

**Sound Modes:**
- `SND_IDLE` — Ambient wing buzz with pitch modulated by velocity, turning, and acceleration
- `SND_CLICK` — Radar ping chirp (3-tone sequence)
- `SND_RADAR` — Navigation tone during radar display
- `SND_POLLEN_CHIRP` — Ascending tones on flower pickup
- `SND_POWERUP` — Boost activation sound

**Features:**
- Dynamic pitch envelope with wingbeat variation
- Vibrato effects responding to movement
- Event tail smoothing between sounds
- Ambient envelope management for seamless transitions

## Technical Details

**Rendering:**
- 120x80 pixel game canvas + 28px HUD, scaled 2x to 240x216 display area
- Offscreen buffer compositing for flicker-free graphics
- Adaptive frame rate: 25 FPS active, 12.5 FPS idle

**Physics:**
- Spring-based movement with configurable stiffness (normal vs boost)
- Velocity damping for smooth deceleration
- Joystick maps to target position within roaming radius

**Procedural Generation:**
- Seeded RNG (xrnd hash) for deterministic world generation
- 7 active flowers with collision-aware spawning
- Infinite world via hash-based cell seeding

## License

This project is licensed under the [Human-Scale Source License v1.2](LICENSE).

## Contributing

Contributions welcome! Please open an issue or pull request.
