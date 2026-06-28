# BMP580 C++ Driver Example

Minimal C++ driver and demo app for Bosch BMP580 pressure sensor.

The project is designed to stay platform-independent at the driver layer and uses external bus abstractions for I2C/SPI communication.

## Features

- BMP580 chip ID check and reset flow
- I2C and SPI interface wrappers
- Oversampling and output data rate setup
- Pressure (Pa) and temperature (deg C) readout
- Convenience conversions: kPa, mmHg, inHg, Kelvin, Fahrenheit, altitude
- Cross-compilation friendly Makefile

## Project Structure

- `BMP580.h` - driver interfaces and public API
- `BMP580.cpp` - driver implementation
- `main.cpp` - demo application using I2C
- `Makefile` - build configuration
- `BuildServices/` - shared make fragments

## Dependencies

This repository expects your external `mylibraries` repository to be available next to it:

- `../mylibraries/Common`
- `../mylibraries/Linux`

The demo uses:

- `i2c.h`
- `devices.h`
- `../mylibraries/Linux/i2c.cpp`

## Supported Bus Address

The demo is currently configured for BMP580 I2C address `0x47`.

## Build

Default Makefile settings:

- Target: `BMP580`
- Cross toolchain prefix: `aarch64-linux-gnu-`
- Link flags: `-static` to remove dependency from glibc

Build command:

```bash
make all
```

Output binary:

```text
build/BMP580
```

## Run

Run with default I2C bus:

```bash
./build/BMP580
```

Run with explicit I2C device path:

```bash
./build/BMP580 /dev/i2c-0
```

The demo reads and prints 10 samples.

Example output format:

```text
Pressure: 100980.25 Pa, Temperature: 24.81 C, Altitude: 28.37 m
```

## Quick Usage Example

```cpp
#include "BMP580.h"
#include "i2c.h"
#include <chrono>
#include <thread>

static I2C_Interface i2c_interface;
static I2C_DeviceBase i2c_device(i2c_interface, 0x47);
static BMP580_I2C_Interface bmp580_i2c(i2c_device);
static BMP580_Base bmp580(bmp580_i2c);

int main(int argc, char *argv[])
{
    i2c_interface.Init(argc > 1 ? argv[1] : "/dev/i2c-0");

    int res = 0;

    try {
        bmp580.Reset(); // Reset the BMP580 to ensure it's in a known state before initialization
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for 100 ms after reset
        res = bmp580.Init();
        if (!res) {
            bmp580.SetOversampling(BMP580_Base::Oversampling::X4, BMP580_Base::Oversampling::X4); // Set oversampling for pressure and temperature
            bmp580.SetDataRate(0x03); // Set data rate to 50 Hz (0x03 corresponds to 50 Hz in BMP580)
        } else {
            fprintf(stderr, "Failed to initialize BMP580: error code %d\n", res);
            return res;
        }
    } catch (const std::exception &e) {
        fprintf(stderr, "Failed to initialize BMP580: %s\n", e.what());
        return 1;
    }

    /* Read and print raw sensor data */
    for (int i = 0; i < 10; ++i) {
        auto real_data = bmp580.WaitForData();
        printf("Pressure: %.2f Pa, Temperature: %.2f °C, Altitude: %.2f m\n", real_data.pressure, real_data.temperature, real_data.GetAltitude());
    }

    return res;
}
```
## Output example
```
Pressure: 101607.47 Pa, Temperature: 26.57 °C, Altitude: -23.48 m
Pressure: 101607.52 Pa, Temperature: 26.58 °C, Altitude: -23.48 m
Pressure: 101607.34 Pa, Temperature: 26.59 °C, Altitude: -23.47 m
Pressure: 101606.89 Pa, Temperature: 26.59 °C, Altitude: -23.43 m
Pressure: 101607.73 Pa, Temperature: 26.60 °C, Altitude: -23.50 m
Pressure: 101606.62 Pa, Temperature: 26.60 °C, Altitude: -23.40 m
Pressure: 101607.66 Pa, Temperature: 26.60 °C, Altitude: -23.49 m
Pressure: 101607.03 Pa, Temperature: 26.60 °C, Altitude: -23.44 m
Pressure: 101606.89 Pa, Temperature: 26.61 °C, Altitude: -23.43 m
Pressure: 101607.64 Pa, Temperature: 26.61 °C, Altitude: -23.49 m
```
## Notes

- Pressure values are reported in Pascal (Pa).
- Temperature values are reported in degrees Celsius.
- Altitude is computed from pressure using a standard barometric formula and configurable sea-level pressure reference.

## Troubleshooting

- `Failed to initialize BMP580: error code -19`:
  chip ID mismatch or sensor not detected on the selected bus/address.
- Unrealistic pressure/altitude:
  verify wiring, I2C bus path, sensor address, and local sea-level pressure reference used for altitude.
- Build errors about missing `mylibraries` headers/sources:
  place `mylibraries` as a sibling directory to this repository.

## License

MIT. See `LICENSE` for details.