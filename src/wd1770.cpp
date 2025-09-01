#include "wd1770.h"
#include "../include/speed.h"

WD1770::WD1770() {
    reset();
}

void WD1770::reset() {
    status = 0;
    track = 0;
    sector = 0;
    data = 0;
    irq = false;
    drq = false;
    busy = false;
    command = 0;
    dataPtr = 0;
    diskInserted = false;
    diskImage.clear();
}

uint8_t WD1770::read(uint16_t reg) {
    switch (reg & 0x03) {
        case REG_CMD_STATUS:
            return status;
        case REG_TRACK:
            return track;
        case REG_SECTOR:
            return sector;
        case REG_DATA:
            drq = false;
            status &= ~STATUS_DRQ;
            return data;
    }
    return 0xFF;
}

void WD1770::write(uint16_t reg, uint8_t value) {
    switch (reg & 0x03) {
        case REG_CMD_STATUS:
            executeCommand(value);
            break;
        case REG_TRACK:
            track = value;
            break;
        case REG_SECTOR:
            sector = value;
            break;
        case REG_DATA:
            data = value;
            drq = false;
            status &= ~STATUS_DRQ;
            break;
    }
}

void WD1770::insertDisk(const std::vector<uint8_t>& image) {
    diskImage = image;
    diskInserted = true;
}

void WD1770::ejectDisk() {
    diskImage.clear();
    diskInserted = false;
}

// WD1770 timing constants (seconds)
constexpr double STEP_TIME_S       = 0.006;  // 6 ms per track step
constexpr double HEAD_SETTLE_S     = 0.015;  // 15 ms head settle
constexpr double REVOLUTION_S      = 0.200;  // 200 ms per full revolution
constexpr double QUICK_FAIL_S      = 0.001;  // 1 ms for immediate fail
#define SEC_TO_CYCLES(sec) static_cast<uint32_t>((sec) * CPU_FREQ)

void WD1770::executeCommand(uint8_t cmd) {
    command = cmd;
    busy = true;
    status |= STATUS_BUSY;
    irq = false;
    drq = false;
    status &= ~(STATUS_DRQ | STATUS_INTRQ);

    bool error = false;

    if ((cmd & 0xF0) == 0x00) { 
        // Restore: assume max seek to track 0 + settle
        // For now, assume 40 tracks worst case
        commandCyclesRemaining = SEC_TO_CYCLES((STEP_TIME_S * 40) + HEAD_SETTLE_S);
    }
    else if ((cmd & 0xF0) == 0x10) { 
        // Seek: assume 1 track step + settle
        commandCyclesRemaining = SEC_TO_CYCLES(STEP_TIME_S + HEAD_SETTLE_S);
    }
    else if ((cmd & 0xF0) == 0x80) { 
        // Read sector
        if (!diskInserted) {
            status |= STATUS_RNF;
            error = true;
            commandCyclesRemaining = SEC_TO_CYCLES(QUICK_FAIL_S);
        } else {
            data = 0x00; // stubbed data
            drq = true;
            status |= STATUS_DRQ;
            // Worst case: wait for sector to come under head
            commandCyclesRemaining = SEC_TO_CYCLES(REVOLUTION_S);
        }
    }
    else if ((cmd & 0xF0) == 0xA0) { 
        // Write sector
        if (!diskInserted) {
            status |= STATUS_WP;
            error = true;
            commandCyclesRemaining = SEC_TO_CYCLES(QUICK_FAIL_S);
        } else {
            commandCyclesRemaining = SEC_TO_CYCLES(REVOLUTION_S);
        }
    }
    else {
        // Unknown/unsupported command
        error = true;
        commandCyclesRemaining = SEC_TO_CYCLES(QUICK_FAIL_S);
    }

    if (error) {
        // finishCommand() will set CRCERR or RNF
    }
}

void WD1770::tick() {
    if (busy && commandCyclesRemaining > 0) {
        --commandCyclesRemaining;
        if (commandCyclesRemaining == 0) {
            finishCommand((status & (STATUS_RNF | STATUS_WP)) != 0);
        }
    }
}

void WD1770::finishCommand(bool error) {
    busy = false;
    status &= ~STATUS_BUSY;
    if (error) status |= STATUS_CRCERR; // Example error bit
    irq = true;
    status |= STATUS_INTRQ;
}