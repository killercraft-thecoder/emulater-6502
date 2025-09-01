#pragma once
#include <cstdint>

class PIA {
public:
    PIA();

    void reset();

    // Memory-mapped access
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);
    void setPortAInput(uint8_t val);
    void setPortBInput(uint8_t val);

    // --- Register offsets (relative to base address) ---
    static constexpr uint8_t REG_PORTA   = 0x00; // Data register A
    static constexpr uint8_t REG_CTRLA   = 0x01; // Control register A
    static constexpr uint8_t REG_PORTB   = 0x02; // Data register B
    static constexpr uint8_t REG_CTRLB   = 0x03; // Control register B

    // --- Control register bit flags ---
    // CA1/CB1 control bits
    static constexpr uint8_t CR_IRQ1_ENABLE   = 0x01; // Interrupt enable for CA1/CB1
    static constexpr uint8_t CR_IRQ1_FLAG     = 0x80; // Interrupt flag for CA1/CB1

    // CA2/CB2 control bits (modes vary: input, output, handshake, pulse)
    static constexpr uint8_t CR_CA2_OUTPUT    = 0x0E; // Bits 1-3 control CA2/CB2 mode
    static constexpr uint8_t CR_CA2_INPUT     = 0x00;

    // Data direction control
    static constexpr uint8_t CR_DDR_ACCESS    = 0x04; // If set, access DDR instead of data reg

private:
    uint8_t ora_ = 0;   // Output register A
    uint8_t orb_ = 0;   // Output register B
    uint8_t ddra_ = 0;  // Data direction register A
    uint8_t ddrb_ = 0;  // Data direction register B
    uint8_t cra_ = 0;   // Control register A
    uint8_t crb_ = 0;   // Control register B

    // Input latches (external signals)
    uint8_t ira_ = 0;
    uint8_t irb_ = 0;
};