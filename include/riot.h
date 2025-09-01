#pragma once
#include <cstdint>
#include <array>
#include <functional>

class RIOT6532 {
public:
    using ReadPort = std::function<uint8_t()>;
    using WritePort = std::function<void(uint8_t)>;

    RIOT6532();

    void reset();

    // Memory-mapped access
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

    // Advance timer by 1 CPU cycle
    void tick();

    // Hook up external I/O
    void setPortA(ReadPort in, WritePort out);
    void setPortB(ReadPort in, WritePort out);

private:
    // Internal RAM (128 bytes)
    std::array<uint8_t, 128> ram_{};

    // I/O ports
    uint8_t ora_ = 0; // Output register A
    uint8_t orb_ = 0; // Output register B
    uint8_t ddra_ = 0; // Data direction A
    uint8_t ddrb_ = 0; // Data direction B

    ReadPort  readA_{};
    WritePort writeA_{};
    ReadPort  readB_{};
    WritePort writeB_{};

    // Timer
    uint16_t timer_ = 0;
    uint8_t  timerShift_ = 0; // prescaler shift: 0=1, 3=8, 6=64, 10=1024
    bool     timerRunning_ = false;
    bool     timerIRQ_ = false;

    // Helper: read/write ports with DDR masking
    uint8_t portRead(uint8_t out, uint8_t ddr, ReadPort in);
    void    portWrite(uint8_t& out, uint8_t ddr, WritePort outFn, uint8_t data);
};