
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

#include "BMP580.h"
#include <errno.h>
#include <cstring>
#include <stdexcept>

const uint8_t REG_CHIP_ID = 0x01;
const uint8_t REG_STATUS = 0x28;
const uint8_t REG_OSR_CONFIG = 0x36;
const uint8_t REG_ODR_CONFIG = 0x37;
const uint8_t REG_CMD = 0x7E;
const uint8_t BMP580_OSR = 0x36;
const uint8_t BMP580_ODR = 0x37;
const uint8_t REG_INT_SOURCE = 0x15;
const uint8_t REG_INT_STATUS = 0x27;

const uint8_t EXPECTED_CHIP_ID = 0x50;
const uint8_t CMD_SOFT_RESET = 0xB6;

#ifndef BIT
#define BIT(n) (1U << (n))
#endif

void BMP580_InterfaceBase::WriteReg(uint8_t reg, uint8_t value)
{
    Write(&reg, 1, &value, 1);
}

uint8_t BMP580_InterfaceBase::ReadReg(uint8_t reg)
{
    uint8_t value;
    Read(&reg, 1, &value, 1);
    return value;
}

void BMP580_I2C_Interface::Write(const uint8_t *reg_addr,
                                 size_t reg_addr_size,
                                 const uint8_t *data,
                                 size_t length)
{
    device_.Write(reg_addr, reg_addr_size, data, length);
}

void BMP580_I2C_Interface::Read(const uint8_t *reg_addr,
                                size_t reg_addr_size,
                                uint8_t *data,
                                size_t length)
{
    device_.Read(reg_addr, reg_addr_size, data, length);
}

void BMP580_SPI_Interface::Write(const uint8_t *reg_addr,
                                 size_t reg_addr_size,
                                 const uint8_t *data,
                                 size_t length)
{
    if (length != 1)
        throw std::invalid_argument("BMP580_SPI_Interface::Write: length must be 1 for single register write");
    uint8_t buffer[2] = { static_cast<uint8_t>(reg_addr[0] & 0x7F), data[0] }; // Clear MSB for write operation
    device_.Transfer(buffer, nullptr, sizeof(buffer)); // Send register address and data
}

void BMP580_SPI_Interface::Read(const uint8_t *reg_addr,
                                size_t reg_addr_size,
                                uint8_t *data,
                                size_t length)
{
    if (length > 100)
        throw std::invalid_argument("BMP580_SPI_Interface::Read: length must not exceed 100 bytes for single register read");
    uint8_t buffer[102] = { static_cast<uint8_t>(reg_addr[0] | 0x80), 0x00 };
    device_.Transfer(buffer, data, sizeof(buffer)); // Send register address and read data
    memcpy(data, buffer + 2, length); // Copy the read buffer to the temporary buffer
}

void BMP580_Base::Reset()
{
    ifs_.WriteReg(REG_CMD, CMD_SOFT_RESET);
}

int BMP580_Base::Init()
{
    /* Check the chip ID */
    uint8_t chip_id = ifs_.ReadReg(REG_CHIP_ID);
    if (chip_id != EXPECTED_CHIP_ID) {
        return -ENODEV; // Device not found or incorrect chip ID
    }

    ifs_.WriteReg(REG_INT_SOURCE, BIT(0));

    return 0;
}

void BMP580_Base::SetOversampling(BMP580_Base::Oversampling osr_p, BMP580_Base::Oversampling osr_t)
{
    const uint8_t PRESS_OS_POS = 3;
    const uint8_t PRESS_EN_MSK = 0x40;
    uint8_t current_osr = static_cast<uint8_t>(osr_t) & 0x07;
    current_osr |= (static_cast<uint8_t>(osr_p) << PRESS_OS_POS) & 0x38;
    current_osr |= PRESS_EN_MSK;

    ifs_.WriteReg(BMP580_OSR, current_osr);
}

void BMP580_Base::SetDataRate(uint8_t odr)
{
    const uint8_t ODR_MSK = 0x7C;
    const uint8_t ODR_POS = 2;
    const uint8_t DEEP_DISABLE_MSK = 0x80;
    const uint8_t POWERMODE_NORMAL = 0x01;

    uint8_t current_odr_config = static_cast<uint8_t>((odr << ODR_POS) & ODR_MSK);
    current_odr_config |= DEEP_DISABLE_MSK | POWERMODE_NORMAL;

    ifs_.WriteReg(BMP580_ODR, current_odr_config);
}

BMP580_Base::Result BMP580_Base::ReadData()
{
    const uint8_t REG_TEMP_XLSB = 0x1D;
    uint8_t raw_buffer[6];
    ifs_.Read(&REG_TEMP_XLSB, 1, raw_buffer, sizeof(raw_buffer));
    int32_t raw_temp = static_cast<int32_t>((static_cast<uint32_t>(raw_buffer[2]) << 16) |
                                 (static_cast<uint32_t>(raw_buffer[1]) << 8) |
                                 (static_cast<uint32_t>(raw_buffer[0])));
    if (raw_temp & 0x800000)
    {
        raw_temp |= 0xFF000000;
    }

    uint32_t raw_press_u32 = (static_cast<uint32_t>(raw_buffer[5]) << 16) |
                             (static_cast<uint32_t>(raw_buffer[4]) << 8) |
                             (static_cast<uint32_t>(raw_buffer[3]));
    int32_t raw_press = static_cast<int32_t>(raw_press_u32);

    return Result(raw_temp, raw_press);
}

BMP580_Base::Result BMP580_Base::WaitForData()
{
    while (!IsDataReady()) { }
    return ReadData();
}

bool BMP580_Base::IsDataReady()
{
    return (ifs_.ReadReg(REG_INT_STATUS) & BIT(0));
}

BMP580_Base::Result::Result(int32_t raw_temp, int32_t raw_press)
{
    // BMP58x provides linearized values directly in fixed-point format.
    temperature = static_cast<float>(raw_temp) / 65536.0f;
    pressure = static_cast<float>(raw_press) / 64.0f;
}

#include <cmath>
float BMP580_Base::Result::GetAltitude(float sea_level_pressure) const
{
    // Calculate altitude in meters using the barometric formula
    return (1.0f - powf(pressure / sea_level_pressure, 0.190284f)) * 145366.45f * 0.3048f; // Convert feet to meters
}
