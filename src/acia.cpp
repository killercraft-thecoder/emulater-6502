#include "acia.h"
#include <algorithm>

ACIA::ACIA() {
    reset();
}

void ACIA::reset() {
    dataReg_    = 0;
    statusReg_  = SR_TDRE; // TX empty at reset
    controlReg_ = 0;
    rxBufferFull_ = false;
    txBufferEmpty_ = true;
    irqAsserted_ = false;
    rxShiftCounter_ = 0;
    txShiftCounter_ = 0;
}

uint8_t ACIA::read(uint16_t addr) {
    uint8_t reg = addr & 0x01;

    if (reg == REG_DATA) {
        // Reading DATA register returns received byte
        uint8_t val = dataReg_;
        rxBufferFull_ = false;
        statusReg_ &= ~SR_RDRF; // clear receive full flag
        updateIRQ();
        return val;
    } else {
        // STATUS register
        return statusReg_;
    }
}

void ACIA::write(uint16_t addr, uint8_t data) {
    uint8_t reg = addr & 0x01;

    if (reg == REG_DATA) {
        // Writing DATA register = transmit
        if (!txBufferEmpty_) {
            // Overwrite before previous byte "sent" — could set overrun or ignore
        }
        txBuffer_ = data;
        txBufferEmpty_ = false;
        statusReg_ &= ~SR_TDRE; // TX not empty
        // Start transmit timer based on controlReg_ baud bits
        txShiftCounter_ = txCyclesForCurrentBaud();
    } else {
        // CONTROL register
        controlReg_ = data;
        // Could parse baud rate, word length, parity here
        updateIRQ();
    }
}

void ACIA::receiveByte(uint8_t data, bool framingError, bool parityError) {
    if (rxBufferFull_) {
        // Overrun: data lost
        statusReg_ |= SR_OVRN;
    } else {
        dataReg_ = data;
        rxBufferFull_ = true;
        statusReg_ |= SR_RDRF;
        if (framingError) statusReg_ |= SR_FE;
        if (parityError)  statusReg_ |= SR_PE;
    }
    updateIRQ();
}

void ACIA::Clock() {
    // Simulate TX timing
    if (!txBufferEmpty_) {
        if (txShiftCounter_ > 0) {
            --txShiftCounter_;
            if (txShiftCounter_ == 0) {
                // Transmission complete
                txBufferEmpty_ = true;
                statusReg_ |= SR_TDRE;
                // In a real ACIA, the byte would go out on the serial line here
                updateIRQ();
            }
        }
    }

    // RX timing would be handled by external serial source calling receiveByte()
}

void ACIA::updateIRQ() {
    bool irq = false;

    // RX interrupt enable?
    if ((controlReg_ & CR_RIE) && (statusReg_ & SR_RDRF)) {
        irq = true;
    }
    // TX interrupt enable? (Not in our minimal control bits, but could be added)
    // if ((controlReg_ & TX_IRQ_ENABLE) && (statusReg_ & SR_TDRE)) irq = true;

    if (irq) statusReg_ |= SR_IRQ;
    else     statusReg_ &= ~SR_IRQ;

    irqAsserted_ = irq;
}

unsigned ACIA::txCyclesForCurrentBaud() const {
    // Simplified: just pick a fixed delay for now
    // In a real ACIA, this depends on CR_CLK_DIV_* bits and CPU clock
    switch (controlReg_ & 0x03) {
        case CR_CLK_DIV_1:  return 10;  // fastest
        case CR_CLK_DIV_16: return 160;
        case CR_CLK_DIV_64: return 640;
        default:            return 10;
    }
}