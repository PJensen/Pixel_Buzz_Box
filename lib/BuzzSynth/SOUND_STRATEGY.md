# Buzzer Sound Design Strategy

This document converts the current audio observations into actionable issues with task stubs. Each issue is intended to be tackled in a follow-up change, keeping the system within the existing `tone()`-based buzzer constraints.

## Issue 1: Ambient buzz is harsh and synthetic
**Problem**: The current ambient buzz is a single sine-modulated tone, which reads as harsh and “buzzy” rather than organic.

**Task stub**
- Add a lightweight multi-curve pitch envelope that cycles between two close base frequencies to emulate wingbeat variation.
- Introduce a subtle micro-detune jitter (±2–4 Hz) per update tick to soften the harshness.
- Gate the envelope so it eases in/out with movement start/stop to reduce abruptness.

**Acceptance**
- Ambient buzz feels less like a raw square wave and more like a living wingbeat.
- Idle-to-move transition does not pop.

## Issue 2: Direction changes do not affect timbre
**Problem**: Movement direction and directional change don’t yet influence timbre; only speed does.

**Task stub**
- Track turn rate (delta heading or stick vector change) and map it to a small pitch skew.
- Apply a short “swish” transient (very fast up/down pitch) when turn rate spikes.
- Ensure the modulation is subtle (single-digit Hz changes) to avoid an arcade effect.

**Acceptance**
- Sharp turns sound slightly sharper/brighter than straight flight.
- Straight flight returns to the neutral buzz within ~150 ms.

## Issue 3: No Doppler/proximity cue
**Problem**: The bee “passing you” or accelerating doesn’t change timbre—no sense of Doppler or proximity.

**Task stub**
- Model a simple radial velocity proxy (acceleration along movement vector) to nudge pitch up on approach and down on retreat.
- Use a brief pitch overshoot on acceleration bursts to imply proximity pass-by.
- Clamp effect to a small range so it remains believable on a piezo.

**Acceptance**
- Speed bursts feel like a whoosh without becoming a siren.
- Pitch returns to baseline within 200–300 ms.

## Issue 4: Event sounds feel disconnected
**Problem**: Event sounds (click/radar/pollen/powerup) are separate from the ambient buzz and feel disconnected.

**Task stub**
- Blend event tones into the ambient line by ramping the ambient pitch toward event pitch before a tone trigger.
- Add a short ambient “tail” after event sounds that matches the current movement speed.
- Reuse a shared envelope (attack/decay) for ambient and event tones.

**Acceptance**
- Event sounds feel like natural accents to the ongoing buzz.
- Ambient tone returns smoothly after events.

## Issue 5: Fixed vibrato is static
**Problem**: The current vibrato uses a single fixed LFO; it doesn’t respond to acceleration or turning, so it feels static.

**Task stub**
- Modulate vibrato depth based on acceleration magnitude.
- Modulate vibrato rate based on turn rate, clamped to a narrow range.
- Fade vibrato depth to near-zero when hovering/idle.

**Acceptance**
- Vibrato feels responsive and “alive” during motion.
- Idle buzz is calmer and less warbly.

## Constraint notes
- The buzzer uses `tone()` (single square wave). “Timbre” must be faked via micro-pitch variations and short pitch curves.
- If PWM control is not available, keep pitch modulation subtle to avoid fatigue.
