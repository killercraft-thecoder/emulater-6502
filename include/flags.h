#ifndef FLAGS_H
#define FLAGS_H

#include <cstdint>

struct Flags {
    enum Status : uint8_t {
        C = 1 << 0, // Carry
        Z = 1 << 1, // Zero
        I = 1 << 2, // Interrupt Disable
        D = 1 << 3, // Decimal Mode
        B = 1 << 4, // Break
        U = 1 << 5, // Unused
        V = 1 << 6, // Overflow
        N = 1 << 7  // Negative
    };

    uint8_t reg = 0;

    inline void Set(Status flag, bool value) {
        if (value) reg |= flag;
        else       reg &= ~flag;
    }

    inline bool Get(Status flag) const {
        return (reg & flag) != 0;
    }

    inline void SetZN(uint8_t value) {
        Set(Z, value == 0);
        Set(N, value & 0x80);
    }
};

#endif // FLAGS_H