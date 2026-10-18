# Crazyflie Firmware  [![CI](https://github.com/bitcraze/crazyflie-firmware/workflows/CI/badge.svg)](https://github.com/bitcraze/crazyflie-firmware/actions?query=workflow%3ACI)

This project contains the source code for the firmware used in the Crazyflie range of platforms, including the Crazyflie 2.x and the Roadrunner.

## BH1750 Light Sensor Integration

This repository contains an experimental integration of a BH1750 ambient light
sensor for the Crazyflie 2.1 firmware. The integration reads the sensor over
I2C and exposes the measured illuminance through the Crazyflie logging system.

### Hardware assumptions

The current implementation assumes the following hardware configuration:

- The BH1750 is connected to the Crazyflie's `I2C1` bus.
- The sensor uses I2C address `0x23`.
- The sensor is powered at a voltage supported by the attached BH1750 breakout
	board and the Crazyflie I/O levels.
- The SDA, SCL, power, and ground connections match the pinout of the specific
	deck or breakout board being used.

BH1750 boards commonly support two addresses. Address `0x23` is normally
selected when the address pin is low, while `0x5C` is normally selected when it
is high. If the sensor is not detected, verify the breakout board's address
selection and update `BH1750_ADDR` in
`src/deck/drivers/src/bh1750_bmo.c` accordingly.

### Firmware implementation

The sensor driver is implemented in
`src/deck/drivers/src/bh1750_bmo.c`. Its startup sequence is:

1. Wait for the Crazyflie system startup with `systemWaitStart()`.
2. Send the BH1750 power-on command (`0x01`).
3. Send the BH1750 reset command (`0x07`).
4. Select continuous high-resolution mode (`0x10`).
5. Wait for the first measurement to become available.
6. Read two bytes from the sensor every 100 ms.

The two-byte sensor result is interpreted as a big-endian raw measurement and
converted to lux using the BH1750 high-resolution conversion factor:

```text
lux = raw_measurement / 1.2
```

The result is stored as a `uint16_t`. If the I2C read fails, the driver stores
`9999` as a diagnostic value. This makes it possible to distinguish a running
sensor task from a failed I2C transaction while testing the hardware.

### Logging the measurement

The driver registers the following log variable:

```text
Log group: bmoSensors
Variable:  lux
Type:      uint16_t
```

The complete log name is therefore `bmoSensors.lux`. It can be selected from a
Crazyflie client or read through the normal Crazyflie logging APIs. A changing
value indicates that the firmware task is running and that the sensor is
responding. A persistent value of `9999` indicates that the I2C read is failing.

### How the driver is included

The driver is added to `src/deck/drivers/src/Kbuild` with:

```make
obj-y += bh1750_bmo.o
```

This means the driver is compiled into the firmware independently of the
standard optional deck configuration symbols. The driver also defines a
`DeckDriver` named `BMO_Sensor` and registers it with `DECK_DRIVER()`.

In addition, `src/deck/core/deck.c` declares `bh1750Init()` and calls
`bh1750Init(NULL)` at the end of `deckInit()`. This explicitly starts the
BH1750 FreeRTOS task during firmware startup, even when no standard deck
identification data is available for the sensor.

### Files changed

The integration consists of these changes:

- `src/deck/drivers/src/bh1750_bmo.c`: BH1750 driver, FreeRTOS task, I2C
	transactions, lux conversion, deck registration, and logging.
- `src/deck/drivers/src/Kbuild`: compiles the BH1750 driver into the firmware.
- `src/deck/core/deck.c`: starts the BH1750 task during deck initialization.
- `src/deck/core/deck_drivers.c`: contains only a formatting change associated
	with the integration.

### Build and flash

Use the repository's normal [building and flashing instructions](https://github.com/bitcraze/crazyflie-firmware/blob/master/docs/building-and-flashing/build.md).
Because the driver is included with `obj-y`, no additional `CONFIG_DECK_*`
option is required for the source file to be compiled.

After flashing the firmware:

1. Power the Crazyflie and connected BH1750 sensor.
2. Connect with a Crazyflie client or another Crazyflie logging application.
3. Add `bmoSensors.lux` to the logging configuration.
4. Place the sensor under different light levels and verify that the value
	 changes.
5. If the value is `9999`, check power, ground, SDA, SCL, the selected I2C bus,
	 and the sensor address.

### Current limitations and follow-up work

This is a direct experimental integration rather than a complete production
deck driver. The current code does not check the return value of the initial
BH1750 command writes, does not expose a configurable address, and uses a
sentinel value for I2C failures. The initialization path should also be
reviewed before production use: the driver is registered with the deck system
and is separately started from `deckInit()`, so it should not be initialized by
both paths at the same time.

The Kbuild line currently uses `-DDECK_FORCE=bh1750_driver`, while the firmware
configuration defines the related setting as `CONFIG_DECK_FORCE`. This line is
documented as part of the current experiment and should be validated or
removed when the driver initialization approach is cleaned up.

### Crazyflie 1.0 support

The 2017.06 release was the last release with Crazyflie 1.0 support. If you want
to play with the Crazyflie 1.0 and modify the code, please clone this repo and
branch off from the 2017.06 tag.

## Building and Flashing
See the [building and flashing instructions](https://github.com/bitcraze/crazyflie-firmware/blob/master/docs/building-and-flashing/build.md) in the github docs folder.


## Official Documentation

Check out the [Bitcraze crazyflie-firmware documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/) on our website.

## Generated documentation

The easiest way to generate the API documentation is to use the [toolbelt](https://github.com/bitcraze/toolbelt)

```tb build-docs```

and to view it in a web page

```tb docs```

## Contribute
Go to the [contribute page](https://www.bitcraze.io/contribute/) on our website to learn more.

### Test code for contribution

To run the tests please have a look at the [unit test documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/development/unit_testing/).

## License

The code is licensed under LGPL-3.0
