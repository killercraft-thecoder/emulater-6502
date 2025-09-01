#pragma once
#include <cstdint>
#include <vector>

class WD1770 {
public:
    WD1770();

    void reset();

    // Memory-mapped register access
    uint8_t read(uint16_t reg);
    void    write(uint16_t reg, uint8_t value);

    // Insert/eject disk image
    void insertDisk(const std::vector<uint8_t>& image);
    void ejectDisk();

    // Advance internal state by one emulated cycle
    void tick();

    // IRQ/DRQ lines for CPU polling
    bool irqLine() const { return irq; }
    bool drqLine() const { return drq; }

private:
    enum RegIndex { REG_CMD_STATUS = 0, REG_TRACK = 1, REG_SECTOR = 2, REG_DATA = 3 };

    // Registers
    uint8_t status;
    uint8_t track;
    uint8_t sector;
    uint8_t data;

    // State
    bool irq;
    bool drq;
    bool busy;
    uint8_t command;
    size_t dataPtr;

    // Disk image
    std::vector<uint8_t> diskImage;
    bool diskInserted;
    uint32_t commandCyclesRemaining; // countdown until command completes


    // Internal helpers
    void executeCommand(uint8_t cmd);
    void finishCommand(bool error = false);

    // Status bit masks (WD1770)
    static constexpr uint8_t STATUS_BUSY   = 0x01;
    static constexpr uint8_t STATUS_DRQ    = 0x02;
    static constexpr uint8_t STATUS_CRCERR = 0x08;
    static constexpr uint8_t STATUS_RNF    = 0x10; // Record Not Found
    static constexpr uint8_t STATUS_WP     = 0x40; // Write Protect
    static constexpr uint8_t STATUS_INTRQ  = 0x80; // IRQ pending
};