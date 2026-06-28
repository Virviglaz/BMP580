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

class BMP580_Base
{
public:
    explicit BMP580_Base(BMP580_InterfaceBase& ifs) : ifs_(ifs) {}
    virtual ~BMP580_Base() = default;

    void Reset();
    int Init();

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

    void SetOversampling(Oversampling osr_p, Oversampling osr_t);

    void SetDataRate(uint8_t odr);

    struct Result {
        Result(int32_t raw_temp, int32_t raw_press);
        float pressure;
        float temperature;
        float GetmmHg() const { return pressure * 0.00750062f; } // Convert Pa to mmHg
        float GetinHg() const { return pressure * 0.0002953f; } // Convert Pa to inHg
        float GetkPa() const { return pressure * 0.001f; } // Convert Pa to kPa
        float GetCelsius() const { return temperature; } // Temperature in Celsius
        float GetFahrenheit() const { return (temperature * 9.0f / 5.0f) + 32.0f; } // Convert Celsius to Fahrenheit
        float GetKelvin() const { return temperature + 273.15f; } // Convert Celsius to Kelvin
        float GetAltitude(float sea_level_pressure = 101325.0f) const;
    };

    Result ReadData();

    virtual Result WaitForData();

    virtual bool IsDataReady();
protected:
    BMP580_InterfaceBase& ifs_;
};

#endif /* BMP580_H */
