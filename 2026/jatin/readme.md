# Jatin — hardware test sketches

Bring-up sketches for the modules going into the **TADMAK 1.0** robotics learning kit.
Each folder is one module, one sketch, one thing proved. Everything here targets an
**Arduino Mega 2560** unless the sketch says otherwise.

The panel these parts mount on is drawn in [`../layout/`](../layout/) — open
`index.html` for the colour sheet or `panel-mono.html` for the printable one.

---

## What has been tested

| # | Module | Sketch | Interface | Mega pins | What the sketch proves |
|---|---|---|---|---|---|
| 1 | ST7735 1.8″ TFT, 128×160 | [`128_160_TFT_MODULE`](128_160_TFT_MODULE) | SPI | CS 10 · DC 9 · RST 8 | Full-screen colour fills, border, text at two sizes, `FreeSans9pt7b` / `FreeMono9pt7b` fonts, portrait + landscape rotation, and it renders the 10-item kit test menu |
| 2 | BH1750 light / lux | [`BH1750_Light_lux_Sensor`](BH1750_Light_lux_Sensor) | I²C `0x23` | SDA 20 · SCL 21 | Continuous lux readings to serial |
| 3 | DHT11 temp + humidity | [`DHT11_Tem._Humidity`](DHT11_Tem._Humidity) | 1-wire | 13 | °C and %RH on a 2 s cycle |
| 4 | HTU21D (HW-220) temp + humidity | [`HW220_Humidity_Temp`](HW220_Humidity_Temp) | I²C `0x40` | SDA 20 · SCL 21 | °C and %RH on a 2 s cycle — the higher-accuracy alternative to the DHT11 |
| 5 | INA219 current / voltage | [`INA219_current_voltage`](INA219_current_voltage) | I²C `0x40` | SDA 20 · SCL 21 | Bus voltage, shunt mV, load voltage, mA and mW — the part that measures what the rest of the kit draws |
| 6 | L298N dual H-bridge | [`L298N_MOTOR_DRIVER`](L298N_MOTOR_DRIVER) | digital + PWM | EN 3 · IN 4 · IN 7 · pot A0 | Direction control plus live speed from a potentiometer, mapped to a 70–255 duty floor so the motor still turns at the bottom of the range |
| 7 | MQ-2 gas | [`MQ_GAS_SENSOR`](MQ_GAS_SENSOR) | analogue | A0 | Raw analogue gas readings |
| 8 | MAX6675 + K-type probe | [`Max6675_thermocouple`](Max6675_thermocouple) | soft SPI | SO 2 · CS 5 · CLK 6 | °C and °F, with the 250 ms settling the part needs between reads |
| 9 | Rain / water sensor | [`Rain_Water_Sensor`](Rain_Water_Sensor) | analogue | A0 · alarm 13 | Threshold detection — above 100 counts raises the alarm output |
| 10 | TTP223 capacitive touch | [`TOUCH_TTP223`](TOUCH_TTP223) | analogue | A0 · LED 13 | Latching toggle: one touch on, next touch off, debounced by a state flag rather than a blocking wait |
| 11 | Reed switch module | [`read_switch_module`](read_switch_module) | digital | 12 · LED 13 | Active-low magnetic read driving an LED |
| 12 | SG90 servo | [`servo_motor`](servo_motor) | PWM | 10 | Lock position, 90° step, and a smooth degree-by-degree 0–90 sweep |
| 13 | HC-SR04 + servo gate | [`ultrasonic_servo`](ultrasonic_servo) | digital + PWM | TRIG 9 · ECHO 8 · servo 10 | Distance from `pulseIn`, converted at 0.034 cm/µs, driving a gate with **hysteresis** — opens under 10 cm, closes only past 15 cm, so it cannot chatter at the threshold |

### Integration tests

These two are the ones that matter — they prove the parts co-exist, not just that each works alone.

| Module | Sketch | Mega pins | What it proves |
|---|---|---|---|
| Three I²C devices, one bus | [`Practical_I2c_comunication`](Practical_I2c_comunication) | SDA 20 · SCL 21 | BH1750 `0x23`, SSD1306 OLED `0x3C` and PCA9685 `0x40` sharing one SDA/SCL pair. Lux is read, mapped to a servo pulse (150–600) and to degrees, the servo moves, and both numbers print to the OLED. One bus, three addresses, no collisions. |
| Mega testing jig | [`Testing_Jig_ArduinoMega`](Testing_Jig_ArduinoMega) | L298N 38–45 · MAX6675 46/47/48 · PCA9685 SDA 20 / SCL 21 | A serial menu that exercises three subsystems on demand: `1` runs four L298N motors (pins 39–45), `2` reads the MAX6675 (46/47/48), `3` sweeps all 16 PCA9685 channels through their full 150–600 travel. |

---

## Watch out: three parts all default to `0x40`

**PCA9685**, **HTU21D** and **INA219** ship on the same I²C address. Any two of them on
one bus will not both answer until one is moved.

| Part | Default | Can move to |
|---|---|---|
| HTU21D (HW-220) | `0x40` | **fixed** — no address pins |
| INA219 | `0x40` | `0x41`, `0x44`, `0x45` via the A0/A1 pads |
| PCA9685 | `0x40` | `0x41`–`0x7F` via the A0–A5 solder jumpers |

Since the HTU21D cannot move, leave it on `0x40` and strap the other two. The panel sheet
assumes the PCA9685 keeps `0x40` because the HTU21D is not on it — bring both onto one
bus and that has to be resolved first.

---

## Parked, not finished

Live code in these sketches works; the commented-out sections are the parts not taken through.

- **`ultrasonic_servo`** — the RGB distance indicator (LEDs on 2/3/4) and the proximity
  buzzer (pin 5) are commented out. Only the servo gate runs.
- **`MQ_GAS_SENSOR`** — reads the analogue output only; the digital `DO` threshold branch
  on pin 13 is commented out.
- **`L298N_MOTOR_DRIVER`** — the fixed-speed steps and the automatic dimmer ramp are
  commented out in favour of the potentiometer path.

**Not yet covered:** the TFT test menu lists a **motion sensor** as item 9, but there is no
PIR sketch in this folder yet. That is the gap between the menu and the folder.

---

## Libraries

Install from the Arduino IDE library manager:

```
Adafruit GFX Library
Adafruit ST7735 and ST7789 Library
Adafruit SSD1306
Adafruit PWM Servo Driver Library     # PCA9685
Adafruit HTU21DF Library              # HW-220
Adafruit INA219
Adafruit Unified Sensor + DHT sensor library
BH1750                                # Christopher Laws
MAX6675 library
```

`Wire`, `SPI` and `Servo` ship with the IDE.

---

## A note on pin numbers

Every sketch here picks its own pins, so the same physical pin means different things
between folders — `13` is the DHT11 in one sketch and the rain alarm in another. That is
fine for bring-up, but they cannot all be loaded at once.

The panel drawing in [`../layout/`](../layout/) resolves this: one conflict-free Mega map
covering every part on the panel, with `0`/`1` kept free for Serial and `44`–`46` kept
clear because the Servo library claims Timer5 and disables `analogWrite` there.
