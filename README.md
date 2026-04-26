# Modular ESP32 Robot Framework

A modular code structure for an ESP32-based robot. Currently includes **OLED eyes** and a **neck servo** module. Easily extensible with additional sensors/actuators.

---

## Wiring

### OLED SSD1306

| OLED | ESP32  |
| ---- | ------ |
| VCC  | 3.3V   |
| GND  | GND    |
| SDA  | GPIO21 |
| SCL  | GPIO22 |

### Neck Servo

| Servo | ESP32   | Movement     |
| ----- | ------- | ------------ |
| Yaw   | GPIO 13 | Right ↔ Left |
| Pitch | GPIO 12 | Up ↔ Down    |

---

## Required Libraries

Install via Arduino Library Manager:

* `Adafruit SSD1306`
* `Adafruit GFX Library`
* `ESP32Servo`

---

## Servo Positions

| Position | Yaw (GPIO13) | Pitch (GPIO12) |
| -------- | ------------ | -------------- |
| Center   | 90°          | 90°            |
| Right    | 45°          | —              |
| Left     | 135°         | —              |
| Up       | —            | 55°            |
| Down     | —            | 120°           |

> If right/left movement is inverted, swap the values of `YAW_RIGHT` and `YAW_LEFT` in `neck_servo.h`.
> Adjust `PITCH_UP` / `PITCH_DOWN` if your pitch servo orientation differs.

---

## Serial Commands

Baud rate: **115200**. Send commands via Serial Monitor (append newline).

### Neck (Servo)

| Command              | Action                                  |
| -------------------- | --------------------------------------- |
| `NECK_RIGHT`         | Turn right (yaw 45°)                    |
| `NECK_LEFT`          | Turn left (yaw 135°)                    |
| `NECK_CENTER`        | Return to center (yaw 90°)              |
| `NECK_UP`            | Look up (pitch 55°)                     |
| `NECK_DOWN`          | Look down (pitch 120°)                  |
| `NECK_MIDDLE`        | Return to center (pitch 90°)            |
| `NECK_YAW:<0-180>`   | Set custom yaw, e.g. `NECK_YAW:70`      |
| `NECK_PITCH:<0-180>` | Set custom pitch, e.g. `NECK_PITCH:100` |

### Eyes (OLED)

| Command          | Expression                    |
| ---------------- | ----------------------------- |
| `EXPR_HAPPY`     | Happy — wide eyes + smile     |
| `EXPR_SURPRISED` | Surprised — very wide eyes    |
| `EXPR_SLEEPY`    | Sleepy — half-closed eyes     |
| `EXPR_ANGRY`     | Angry — narrow eyes + brows   |
| `EXPR_CONFUSED`  | Confused — asymmetrical + "?" |
| `EXPR_FOCUSED`   | Focused — slightly narrowed   |
| `EXPR_WINK`      | Left eye wink                 |
| `EXPR_IDLE`      | Return to default animation   |

### Test & Info

| Command     | Function                                 |
| ----------- | ---------------------------------------- |
| `TEST_NECK` | Run all neck positions sequentially      |
| `TEST_EYES` | Display all eye expressions              |
| `TEST_SYNC` | Test neck movement + eye synchronization |
| `STATUS`    | Show current system state                |
| `HELP`      | Display command list                     |

---

## Eye & Neck Synchronization

Each neck movement automatically adjusts the **OLED gaze offset** and triggers a corresponding expression:

| Neck Movement  | OLED Gaze    | Expression  |
| -------------- | ------------ | ----------- |
| Right          | Shift right  | `FOCUSED`   |
| Left           | Shift left   | `FOCUSED`   |
| Center (yaw)   | Reset X      | —           |
| Up             | Shift up     | `SURPRISED` |
| Down           | Shift down   | `SLEEPY`    |
| Center (pitch) | Reset Y      | —           |
| Custom angle   | Proportional | —           |

Custom `NECK_YAW` and `NECK_PITCH` are also automatically synchronized — gaze offset is calculated proportionally from the servo angle.

---

## Adding a New Module

Example: adding an ultrasonic sensor

1. Create `ultrasonic.h` and `ultrasonic.cpp`
2. Expose functions: `ultrasonicInit()`, `ultrasonicUpdate()`, `ultrasonicGetDistance()`
3. Include in `robot_main.ino`, call init in `setup()` and update in `loop()`
4. Add command handling in `command_handler.cpp` if needed
5. If LLM access via MCP is required, expose functions through Serial commands

---

## Notes for LLM / MCP Integration

Serial commands are designed to be easily callable by LLM tools:

* One command = one line of text ending with `\n`
* Responses always start with `OK` on success, or return an error message
* `STATUS` returns full state in a parse-friendly format
* All commands are case-insensitive (automatically uppercased in handler)

