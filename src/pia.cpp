#include "pia.h"

PIA::PIA() {
    reset();
}

void PIA::reset() {
    ora_ = orb_ = 0;
    ddra_ = ddrb_ = 0;
    cra_ = crb_ = 0;
    ira_ = irb_ = 0;
}

uint8_t PIA::read(uint16_t addr) {
    switch (addr & 0x03) {
        case REG_PORTA:
            if (cra_ & CR_DDR_ACCESS) {
                // Access DDR for port A
                return ddra_;
            } else {
                // Read port A: combine output bits and input bits
                uint8_t val = (ora_ & ddra_) | (ira_ & ~ddra_);
                // Clear CA1 interrupt flag on read
                cra_ &= ~CR_IRQ1_FLAG;
                return val;
            }

        case REG_CTRLA:
            return cra_;

        case REG_PORTB:
            if (crb_ & CR_DDR_ACCESS) {
                return ddrb_;
            } else {
                uint8_t val = (orb_ & ddrb_) | (irb_ & ~ddrb_);
                crb_ &= ~CR_IRQ1_FLAG;
                return val;
            }

        case REG_CTRLB:
            return crb_;
    }
    return 0xFF;
}

void PIA::write(uint16_t addr, uint8_t data) {
    switch (addr & 0x03) {
        case REG_PORTA:
            if (cra_ & CR_DDR_ACCESS) {
                // Write DDR for port A
                ddra_ = data;
            } else {
                // Write output register A
                ora_ = data;
                // In real hardware, this would drive pins for bits set as outputs
            }
            break;

        case REG_CTRLA:
            cra_ = data;
            break;

        case REG_PORTB:
            if (crb_ & CR_DDR_ACCESS) {
                ddrb_ = data;
            } else {
                orb_ = data;
            }
            break;

        case REG_CTRLB:
            crb_ = data;
            break;
    }
}

// --- Optional: external input handling ---
void PIA::setPortAInput(uint8_t val) {
    ira_ = val;
    // If CA1 interrupt enabled, set flag
    if (cra_ & CR_IRQ1_ENABLE) {
        cra_ |= CR_IRQ1_FLAG;
    }
}

void PIA::setPortBInput(uint8_t val) {
    irb_ = val;
    if (crb_ & CR_IRQ1_ENABLE) {
        crb_ |= CR_IRQ1_FLAG;
    }
}