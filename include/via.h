#pragma once
#include <cstdint>

class VIA6522 {
public:
    VIA6522();

    // Read/write a register (reg = 0x0–0xF)
    uint8_t Read(uint8_t reg);
    void    Write(uint8_t reg, uint8_t val);

    // Advance timers/shift register by one CPU cycle
    void Tick();

    // IRQ output line (true = active)
    bool irq_line = false;

    // External access to ports (simulate GPIO)
    uint8_t portA_in = 0; // external inputs to Port A
    uint8_t portB_in = 0; // external inputs to Port B
    uint8_t portA_out = 0; // last written outputs
    uint8_t portB_out = 0;

private:
    // Registers
    uint8_t ORB = 0, ORA = 0;
    uint8_t DDRB = 0, DDRA = 0;
    uint16_t T1C = 0, T1L = 0;
    uint16_t T2C = 0;
    uint8_t SR = 0;
    uint8_t ACR = 0, PCR = 0;
    uint8_t IFR = 0, IER = 0;

    // Internal helpers
    void UpdateIRQ();
    void SetIFR(uint8_t mask);
};