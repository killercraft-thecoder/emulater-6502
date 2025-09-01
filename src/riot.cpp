#include "riot.h"

RIOT6532::RIOT6532() {
    reset();
}

void RIOT6532::reset() {
    ram_.fill(0);
    ora_ = orb_ = 0;
    ddra_ = ddrb_ = 0;
    timer_ = 0;
    timerShift_ = 0;
    timerRunning_ = false;
    timerIRQ_ = false;
}

void RIOT6532::setPortA(ReadPort in, WritePort out) {
    readA_ = std::move(in);
    writeA_ = std::move(out);
}

void RIOT6532::setPortB(ReadPort in, WritePort out) {
    readB_ = std::move(in);
    writeB_ = std::move(out);
}

uint8_t RIOT6532::portRead(uint8_t out, uint8_t ddr, ReadPort in) {
    uint8_t inVal = in ? in() : 0xFF;
    return (out & ddr) | (inVal & ~ddr);
}

void RIOT6532::portWrite(uint8_t& out, uint8_t ddr, WritePort outFn, uint8_t data) {
    out = data;
    if (outFn) outFn(out & ddr);
}

uint8_t RIOT6532::read(uint16_t addr) {
    addr &= 0x7F; // RIOT has 128-byte register space

    if (addr < 0x80) {
        // RAM: $00–$7F
        return ram_[addr];
    }

    uint8_t reg = addr & 0x1F;
    switch (reg) {
        case 0x00: // Port A data
            return portRead(ora_, ddra_, readA_);
        case 0x01: // Port A DDR
            return ddra_;
        case 0x02: // Port B data
            return portRead(orb_, ddrb_, readB_);
        case 0x03: // Port B DDR
            return ddrb_;
        case 0x04: // Timer read
            return static_cast<uint8_t>(timer_ & 0xFF);
        case 0x05: // Timer status (read clears IRQ flag)
        {
            uint8_t val = static_cast<uint8_t>(timer_ & 0xFF);
            timerIRQ_ = false;
            return val;
        }
        default:
            return 0xFF;
    }
}

void RIOT6532::write(uint16_t addr, uint8_t data) {
    addr &= 0x7F;

    if (addr < 0x80) {
        ram_[addr] = data;
        return;
    }

    uint8_t reg = addr & 0x1F;
    switch (reg) {
        case 0x00: // Port A data
            portWrite(ora_, ddra_, writeA_, data);
            break;
        case 0x01: // Port A DDR
            ddra_ = data;
            break;
        case 0x02: // Port B data
            portWrite(orb_, ddrb_, writeB_, data);
            break;
        case 0x03: // Port B DDR
            ddrb_ = data;
            break;
        case 0x14: // Timer write, prescale 1
            timerShift_ = 0;
            timer_ = data;
            timerRunning_ = true;
            timerIRQ_ = false;
            break;
        case 0x15: // Timer write, prescale 8
            timerShift_ = 3;
            timer_ = data;
            timerRunning_ = true;
            timerIRQ_ = false;
            break;
        case 0x16: // Timer write, prescale 64
            timerShift_ = 6;
            timer_ = data;
            timerRunning_ = true;
            timerIRQ_ = false;
            break;
        case 0x17: // Timer write, prescale 1024
            timerShift_ = 10;
            timer_ = data;
            timerRunning_ = true;
            timerIRQ_ = false;
            break;
        default:
            break;
    }
}

void RIOT6532::tick() {
    if (!timerRunning_) return;

    static uint16_t prescaleCounter = 0;
    prescaleCounter++;
    if (prescaleCounter >= (1u << timerShift_)) {
        prescaleCounter = 0;
        if (timer_ == 0) {
            timerIRQ_ = true;
            // Underflow: timer continues counting down from 0xFF
            timer_ = 0xFF;
        } else {
            timer_--;
        }
    }
}