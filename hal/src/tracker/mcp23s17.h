/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MCP23S17_H
#define MCP23S17_H

#include "static_recursive_mutex.h"
#include "pinmap_defines.h"
#include "pinmap_hal.h"
#include "spi_hal.h"
#include "interrupts_hal.h"
#include "concurrent_hal.h"
#include "spark_wiring_vector.h"

#define MCP23S17_PORT_COUNT             (2)
#define MCP23S17_PIN_COUNT_PER_PORT     (8)

#define MCP23S17_SPI_INTERFACE          (HAL_SPI_INTERFACE1)
#define MCP23S17_SPI_CS_PIN             (SS)
#define MCP23S17_RESET_PIN              (A0)
#define MCP23S17_INT_PIN                (A1)


namespace particle {

typedef void (*Mcp23s17InterruptCallback)(void* context);

class Mcp23s17 {
public:
    int begin();
    int end();
    bool initialized() const;
    int reset(bool verify = true);
    int sync();

    int setPinMode(uint8_t port, uint8_t pin, PinMode mode, bool verify = true);
    int setPinInputInverted(uint8_t port, uint8_t pin, bool enable, bool verify = true);
    int writePinValue(uint8_t port, uint8_t pin, uint8_t value, bool verify = true);
    int readPinValue(uint8_t port, uint8_t pin, uint8_t* value);
    int attachPinInterrupt(uint8_t port, uint8_t pin, InterruptMode trig, Mcp23s17InterruptCallback callback, void* context, bool verify = true);

    static Mcp23s17& getInstance();
    static int lock();
    static int unlock();

private:
    struct IoPinInterruptConfig {
        IoPinInterruptConfig()
                : port(PIN_INVALID),
                  pin(PIN_INVALID),
                  cb(nullptr),
                  context(nullptr) {
        }
        ~IoPinInterruptConfig() {}

        uint8_t port;
        uint8_t pin;
        InterruptMode trig;
        Mcp23s17InterruptCallback cb;
        void* context;
    };

    Mcp23s17();
    ~Mcp23s17();

    void resetRegValue();
    int writeRegister(const uint8_t addr, const uint8_t val) const;
    int readRegister(const uint8_t addr, uint8_t* const val) const;
    int readContinuousRegisters(const uint8_t start_addr, uint8_t* const val, uint8_t len) const;
    static os_thread_return_t ioInterruptHandleThread(void* param);

    // Resister address
    static constexpr uint8_t IODIR_ADDR[2]     = {0x00, 0x01};
    static constexpr uint8_t IPOL_ADDR[2]      = {0x02, 0x03};
    static constexpr uint8_t GPINTEN_ADDR[2]   = {0x04, 0x05};
    static constexpr uint8_t DEFVAL_ADDR[2]    = {0x06, 0x07};
    static constexpr uint8_t INTCON_ADDR[2]    = {0x08, 0x09};
    static constexpr uint8_t IOCON_ADDR[2]     = {0x0A, 0x0B};
    static constexpr uint8_t GPPU_ADDR[2]      = {0x0C, 0x0D};
    static constexpr uint8_t INTF_ADDR[2]      = {0x0E, 0x0F};
    static constexpr uint8_t INTCAP_ADDR[2]    = {0x10, 0x11};
    static constexpr uint8_t GPIO_ADDR[2]      = {0x12, 0x13};
    static constexpr uint8_t OLAT_ADDR[2]      = {0x14, 0x15};

    // Read/write coomand
    static constexpr uint8_t MCP23S17_CMD_READ = 0x41;
    static constexpr uint8_t MCP23S17_CMD_WRITE = 0x40;

    uint8_t iodir_[2];
    uint8_t ipol_[2];
    uint8_t gpinten_[2];
    uint8_t defval_[2];
    uint8_t intcon_[2];
    uint8_t iocon_[2];
    uint8_t gppu_[2];
    uint8_t intf_[2];
    uint8_t intcap_[2];
    uint8_t gpio_[2];
    uint8_t olat_[2];

    bool initialized_;
    HAL_SPI_Interface spi_;
    os_thread_t ioExpanderWorkerThread_;
    os_queue_t ioExpanderWorkerQueue_;
    bool ioExpanderWorkerThreadExit_;
    Vector<IoPinInterruptConfig> intConfigs_;

    static StaticRecursiveMutex mutex_;
}; // class Mcp23s17

class Mcp23s17Lock {
public:
    Mcp23s17Lock()
            : locked_(false) {
        lock();
    }

    ~Mcp23s17Lock() {
        if (locked_) {
            unlock();
        }
    }

    Mcp23s17Lock(Mcp23s17Lock&& lock)
            : locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        Mcp23s17::lock();
        locked_ = true;
    }

    void unlock() {
        Mcp23s17::unlock();
        locked_ = false;
    }

    Mcp23s17Lock(const Mcp23s17Lock&) = delete;
    Mcp23s17Lock& operator=(const Mcp23s17Lock&) = delete;

private:
    bool locked_;
};

} // namespace particle

#define MCP23S17 particle::Mcp23s17::getInstance()

#endif // MCP23S17_H
