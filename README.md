# ESP32 Multi Level Accident Detection System

A real-time embedded safety prototype that uses **multi-level confirmation** to identify serious motorcycle accidents more reliably before sending an emergency alert.

## Overview

This project connects ESP32-based bike and helmet units to evaluate an impact alongside contextual signals such as helmet status, pulse monitoring, GPS availability, and alcohol detection. Instead of treating a single accelerometer spike as a crash, the system cross-validates multiple inputs to reduce false-positive alerts caused by normal riding conditions.

When the confirmation conditions are met, the system captures location data and sends a GSM emergency notification.

## Key capabilities

- **Impact analysis:** evaluates tri-axis accelerometer readings, distinguishing normal riding conditions (approximately 1.0 g) from potential impact events (3.0–6.0 g).
- **Multi-sensor confirmation:** combines impact information with helmet, pulse, GPS, and alcohol-detection signals before escalation.
- **Paired ESP32 communication:** uses ESP-NOW for communication between the bike and helmet units.
- **Emergency response:** retrieves GPS location and sends a GSM alert after an accident is confirmed.
- **Hardware firmware integration:** demonstrates sensor interfacing, embedded C/C++, and real-time decision logic.

## System flow

1. The bike unit monitors motion and potential impact events.
2. The helmet unit and connected sensors provide contextual status signals.
3. The ESP32 units exchange data over ESP-NOW.
4. The confirmation logic evaluates the combined signals.
5. For a confirmed emergency, the system gathers GPS data and sends a GSM notification.

## Repository structure

| Folder | Contents |
| --- | --- |
| `Source_Code` | ESP32 firmware and project source code |
| `Circuit_Diagram` | Circuit diagrams and wiring references |
| `Documentation` | Project documentation |
| `Libraries` | Required libraries and dependencies |
| `Images` | Project images |
| `Demo` | Demonstration materials |

## Technologies

- ESP32
- ESP-NOW
- GPS and GSM modules
- Accelerometer and pulse sensors
- Embedded C/C++
- Sensor interfacing and hardware-firmware integration

## Project status

Academic embedded-systems project and prototype focused on improving accident-alert reliability through sensor fusion and staged confirmation.

## Author

**Abinavu M** — Electronics and Communication Engineering student focused on embedded systems and IoT.

- [GitHub profile](https://github.com/abiabinavu)
- [Portfolio](https://abiabinavu.github.io/abinavu-portfolio/)
