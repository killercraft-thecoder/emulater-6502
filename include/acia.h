#pragma once
#include <cstdint>

class ACIA
{
public:
    ACIA();

    void reset();

    // Memory-mapped access
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    void receiveByte(uint8_t data, bool framingError, bool parityError);
    void Clock();

    // --- Register offsets ---
    static constexpr uint8_t REG_DATA = 0x00;    // Transmit/Receive data
    static constexpr uint8_t REG_STATUS = 0x01;  // Status register (read)
    static constexpr uint8_t REG_CONTROL = 0x01; // Control register (write)

    // --- Status register flags ---
    static constexpr uint8_t SR_RDRF = 0x01; // Receive Data Register Full
    static constexpr uint8_t SR_TDRE = 0x02; // Transmit Data Register Empty
    static constexpr uint8_t SR_DCD = 0x04;  // Data Carrier Detect
    static constexpr uint8_t SR_CTS = 0x08;  // Clear To Send
    static constexpr uint8_t SR_FE = 0x10;   // Framing Error
    static constexpr uint8_t SR_OVRN = 0x20; // Overrun
    static constexpr uint8_t SR_PE = 0x40;   // Parity Error
    static constexpr uint8_t SR_IRQ = 0x80;  // Interrupt Request

    // --- Control register bits ---
    // Bits 0-1: Clock divide select
    static constexpr uint8_t CR_CLK_DIV_1 = 0x00;
    static constexpr uint8_t CR_CLK_DIV_16 = 0x01;
    static constexpr uint8_t CR_CLK_DIV_64 = 0x02;

    // Bits 2-4: Word select (data bits, parity, stop bits)
    static constexpr uint8_t CR_WS_8N1 = 0x10; // 8 data, no parity, 1 stop
    static constexpr uint8_t CR_WS_8E1 = 0x14; // 8 data, even parity, 1 stop
    static constexpr uint8_t CR_WS_8O1 = 0x16; // 8 data, odd parity, 1 stop
    // ... add more as needed

    // Bits 5-6: Transmit control
    static constexpr uint8_t CR_TX_ENABLE = 0x20;
    static constexpr uint8_t CR_TX_DISABLE = 0x00;

    // Bit 7: Receive interrupt enable
    static constexpr uint8_t CR_RIE = 0x80;

private:
    uint8_t dataReg_ = 0;
    uint8_t statusReg_ = SR_TDRE; // TX empty at reset
    uint8_t controlReg_ = 0;

    uint8_t txBuffer_ = 0;
    bool txBufferEmpty_ = true;
    bool rxBufferFull_ = false;

    bool irqAsserted_ = false;

    unsigned txShiftCounter_ = 0;
    unsigned rxShiftCounter_ = 0; // if you later simulate RX timing

    void updateIRQ();
    unsigned txCyclesForCurrentBaud() const;
};