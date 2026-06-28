#include "BMP580.h"
#include "i2c.h"
#include <exception>
#include <cstdio>
#include <thread>
#include <chrono>

/* I2C interface and device provided by my custom platform-independent implementation */
static I2C_Interface i2c_interface;
static I2C_DeviceBase i2c_device(i2c_interface, 0x47); // BMP580 default I2C address is 0x47
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
