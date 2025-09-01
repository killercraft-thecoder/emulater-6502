#include "via.h"

VIA6522::VIA6522() {
    ORA = ORB = 0;
    DDRA = DDRB = 0;
    T1C = T1L = 0;
    T2C = 0;
    SR = 0;
    ACR = PCR = 0;
    IFR = IER = 0;
}

uint8_t VIA6522::Read(uint8_t reg) {
    reg &= 0x0F;
    switch (reg) {
        case 0x0: return (ORB & DDRB) | (portB_in & ~DDRB);
        case 0x1: return (ORA & DDRA) | (portA_in & ~DDRA);
        case 0x2: return DDRB;
        case 0x3: return DDRA;
        case 0x4: return T1C & 0xFF;
        case 0x5: return T1C >> 8;
        case 0x6: return T1L & 0xFF;
        case 0x7: return T1L >> 8;
        case 0x8: return T2C & 0xFF;
        case 0x9: return T2C >> 8;
        case 0xA: return SR;
        case 0xB: return ACR;
        case 0xC: return PCR;
        case 0xD: return IFR;
        case 0xE: return IER | 0x80; // bit 7 always 1 when reading IER
        default:  return 0xFF;
    }
}

void VIA6522::Write(uint8_t reg, uint8_t val) {
    reg &= 0x0F;
    switch (reg) {
        case 0x0:
            ORB = val;
            portB_out = val;
            break;
        case 0x1:
            ORA = val;
            portA_out = val;
            break;
        case 0x2: DDRB = val; break;
        case 0x3: DDRA = val; break;
        case 0x4: // T1 low counter
            T1C = (T1C & 0xFF00) | val;
            break;
        case 0x5: // T1 high counter
            T1C = (val << 8) | (T1C & 0x00FF);
            T1L = T1C;
            SetIFR(0x40); // Timer 1 interrupt flag
            break;
        case 0x6: // T1 low latch
            T1L = (T1L & 0xFF00) | val;
            break;
        case 0x7: // T1 high latch
            T1L = (val << 8) | (T1L & 0x00FF);
            break;
        case 0x8: // T2 low counter
            T2C = (T2C & 0xFF00) | val;
            break;
        case 0x9: // T2 high counter
            T2C = (val << 8) | (T2C & 0x00FF);
            SetIFR(0x20); // Timer 2 interrupt flag
            break;
        case 0xA: SR = val; break;
        case 0xB: ACR = val; break;
        case 0xC: PCR = val; break;
        case 0xD: IFR &= ~val; break; // clear flags
        case 0xE:
            if (val & 0x80) IER |= (val & 0x7F);
            else IER &= ~(val & 0x7F);
            break;
    }
    UpdateIRQ();
}

void VIA6522::Tick() {
    // Timer 1
    if (T1C > 0) {
        T1C--;
        if (T1C == 0xFFFF) { // underflow
            SetIFR(0x40);
            if (ACR & 0x40) {
                T1C = T1L; // free-run
            }
        }
    }
    // Timer 2
    if (T2C > 0) {
        T2C--;
        if (T2C == 0xFFFF) {
            SetIFR(0x20);
        }
    }
    UpdateIRQ();
}

void VIA6522::UpdateIRQ() {
    irq_line = (IFR & IER & 0x7F) != 0;
}

void VIA6522::SetIFR(uint8_t mask) {
    IFR |= mask;
}