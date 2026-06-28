/*
 * This file is provided under a MIT license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * MIT License
 *
 * Copyright (c) 2026 Pavel Nadein
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * BMP580 C++ driver implementation header.
 *
 * Contact Information:
 * Pavel Nadein <pavelnadein@gmail.com>
 */

#ifndef BMP580_H
#define BMP580_H

#ifndef __cplusplus
#error "This header requires C++11 or higher"
#endif

#include "devices.h"
#include <type_traits>

/**
 * @brief Base class for BMP580 sensor interface.
 * This class provides methods to interact with the BMP580 sensor over I2C or SPI.
 */
class BMP580_InterfaceBase
{
public:
    explicit BMP580_InterfaceBase() = default;
    virtual ~BMP580_InterfaceBase() = default;

    /* I2C/SPI interface */
    virtual void Write(const uint8_t *reg_addr,
                       size_t reg_addr_size,
                       const uint8_t *data,
                       size_t length) = 0;
    virtual void Read(const uint8_t *reg_addr,
                      size_t reg_addr_size,
                      uint8_t *data,
                      size_t length) = 0;

    void WriteReg(uint8_t reg, uint8_t value);
    uint8_t ReadReg(uint8_t reg);
};

/**
 * @brief BMP580 I2C interface implementation.
 * This class provides methods to communicate with the BMP580 sensor over I2C.
 */
class BMP580_I2C_Interface : public BMP580_InterfaceBase
{
public:
    explicit BMP580_I2C_Interface(I2C_DeviceBase& device) : device_(device) {}
    ~BMP580_I2C_Interface() override = default;

    void Write(const uint8_t *reg_addr,
               size_t reg_addr_size,
               const uint8_t *data,
               size_t length) override;

    void Read(const uint8_t *reg_addr,
              size_t reg_addr_size,
              uint8_t *data,
              size_t length) override;
private:
    I2C_DeviceBase& device_;
};

/**
 * @brief BMP580 SPI interface implementation.
 * This class provides methods to communicate with the BMP580 sensor over SPI.
 */
class BMP580_SPI_Interface : public BMP580_InterfaceBase
{
public:
    explicit BMP580_SPI_Interface(SPI_DeviceBase& device) : device_(device) {}
    ~BMP580_SPI_Interface() override = default;

    void Write(const uint8_t *reg_addr,
               size_t reg_addr_size,
               const uint8_t *data,
               size_t length) override;

    void Read(const uint8_t *reg_addr,
              size_t reg_addr_size,
              uint8_t *data,
              size_t length) override;
private:
    SPI_DeviceBase& device_;
};

/**
 * @brief BMP580 sensor base class.
 * This class provides methods to initialize the sensor,
 * configure oversampling and data rate,
 * read raw data, and check data readiness.
 */
template <typename T = float, typename P = double>
class BMP580_Base
{
public:
    explicit BMP580_Base(BMP580_InterfaceBase& ifs) : ifs_(ifs) {}
    virtual ~BMP580_Base() = default;

    /**
     * @brief Reset the BMP580 sensor.
     * This method sends a soft reset command to the sensor.
     * @note After calling this method, it is recommended to wait
     * for a short period before re-initializing the sensor.
     */
    void Reset();

    /**
     * @brief Initialize the BMP580 sensor.
     * This method checks the sensor's chip ID and configures
     * the sensor for normal operation.
     *
     * @return 0 on success, or a negative error code on failure.
     */
    int Init();

    /**
     * @brief Oversampling settings for pressure and temperature.
     * This enum defines the available oversampling options for the BMP580 sensor.
     */
    enum class Oversampling : uint8_t
    {
        X1 = 0x00,
        X2 = 0x01,
        X4 = 0x02,
        X8 = 0x03,
        X16 = 0x04,
        X32 = 0x05,
        X64 = 0x06,
        X128 = 0x07
    };

    /**
     * @brief Set the oversampling settings for pressure and temperature.
     * This method configures the oversampling settings for both pressure and temperature measurements.
     *
     * @param osr_p Oversampling setting for pressure.
     * @param osr_t Oversampling setting for temperature.
     */
    void SetOversampling(Oversampling osr_p, Oversampling osr_t);

    void SetDataRate(uint8_t odr);

    /**
     * @brief Result structure for BMP580 sensor readings.
     * This structure holds the raw and converted sensor data.
     */
    struct Result {
        Result(int32_t raw_temp, int32_t raw_press);
        P pressure;
        T temperature;
        P GetmmHg() const { return pressure * static_cast<P>(0.00750062); } // Convert Pa to mmHg
        P GetinHg() const { return pressure * static_cast<P>(0.0002953); } // Convert Pa to inHg
        P GetkPa() const { return pressure * static_cast<P>(0.001); } // Convert Pa to kPa
        T GetCelsius() const { return temperature; } // Temperature in Celsius
        T GetFahrenheit() const { return (temperature * static_cast<T>(9.0) / static_cast<T>(5.0)) + static_cast<T>(32.0); } // Convert Celsius to Fahrenheit
        T GetKelvin() const { return temperature + static_cast<T>(273.15); } // Convert Celsius to Kelvin
        P GetAltitude(P sea_level_pressure = static_cast<P>(101325.0)) const;
    };

    /**
     * @brief Read raw data from the BMP580 sensor.
     * This method reads the raw pressure and temperature data from the sensor.
     *
     * @return Result structure containing the raw and converted sensor data.
     */
    Result ReadData();

    /**
     * @brief Wait for new data to be ready and read it.
     * This method blocks until new data is available, then reads the raw pressure and temperature data.
     *
     * @return Result structure containing the raw and converted sensor data.
     */
    virtual Result WaitForData();

    /**
     * @brief Check if new data is ready.
     * This method checks the sensor's status register to determine if new data is available.
     * Overrides of this method should implement the specific logic for checking data readiness
     * based on the interface used (I2C or SPI).
     *
     * @return true if new data is ready, false otherwise.
     */
    virtual bool IsDataReady();

    static_assert(std::is_floating_point<T>::value,
                  "Template parameter T must be a floating-point type");
    static_assert(std::is_floating_point<P>::value,
                  "Template parameter P must be a floating-point type");
protected:
    BMP580_InterfaceBase& ifs_;
};

#endif /* BMP580_H */
