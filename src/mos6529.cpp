#include "mos6529.h"

MOS6529::MOS6529()
{
    reset();
}

void MOS6529::reset()
{
    portLatch  = 0xFF;   // Power-on default: all high
    inputPins  = 0xFF;   // Assume pull-ups on inputs
    outputMode = false;  // Default to input mode
}

uint8_t MOS6529::read() const
{
    if (outputMode)
    {
        // In output mode, reading returns the latched output value
        return portLatch;
    }
    else
    {
        // In input mode, reading returns the external pin state
        return inputPins;
    }
}

void MOS6529::write(uint8_t value)
{
    if (outputMode)
    {
        // In output mode, writing changes the latch
        portLatch = value;
    }
    else
    {
        // In input mode, writes are ignored (real chip ignores them)
    }
}

void MOS6529::setDirection(bool output)
{
    outputMode = output;
}

void MOS6529::setInputPins(uint8_t value)
{
    inputPins = value;
}

uint8_t MOS6529::getOutputLatch() const
{
    return portLatch;
}