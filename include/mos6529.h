#ifndef MOS6529_H
#define MOS6529_H

#include <cstdint>

// MOS 6529 Single Port Interface (SPI)
// Simple 8-bit parallel I/O port with global direction control.
// No timers, no interrupts, no handshaking.

class MOS6529
{
public:
    MOS6529();

    // Reset to power-on state
    void reset();

    // Read from the port's memory-mapped register
    uint8_t read() const;

    // Write to the port's memory-mapped register
    void write(uint8_t value);

    // Set the port direction: true = output, false = input
    void setDirection(bool output);

    // Set the external input pins (only used if in input mode)
    void setInputPins(uint8_t value);

    // Get the current output latch value (only valid if in output mode)
    uint8_t getOutputLatch() const;

private:
    uint8_t portLatch;   // Latched output value
    uint8_t inputPins;   // External input value
    bool outputMode;     // true = output mode, false = input mode
};

#endif // MOS6529_H