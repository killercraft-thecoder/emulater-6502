#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include "../include/rom_space.h"


#ifdef USE_TIA
#include "tia.h"
#endif
#ifdef USE_RIOT
#include "riot.h"
#endif
#ifdef USE_VIC
#include "vic.h"
#endif
#ifdef USE_PIA
#include "pia.h"
#endif
#ifdef USE_ACIA
#include "acia.h"
#endif
#ifdef USE_VIA
#include "via.h"
#ifdef USE_MICRO
#include "wd1770.h"
#endif
#endif
#ifdef USE_6529
#include "mos6529.h"
#endif
class Memory
{
public:
#ifdef USE_TIA
    TIA tia;
#endif
#ifdef USE_RIOT
    RIOT6532 riot;
#endif
#ifdef USE_VIA
    VIA6522 via;
#ifdef USE_MICRO
    VIA via2; // BBC Micro's second VIA
    WD1770 disk; // BBC Micro's Disk Controller
#endif

#endif
#ifdef USE_VIC
    VIC vic;
#endif
#ifdef USE_PIA
    PIA pia;
#endif
#ifdef USE_ACIA
    ACIA acia;
#endif
#ifdef USE_6529
  MOS6529 io;
#endif

    static constexpr uint32_t MAX_MEM = 64 * 1024;

    Memory(RomSpace romSpace = RomSpace::NONE);
    void Reset();
    uint8_t Read(uint16_t addr) const;
    void Write(uint16_t addr, uint8_t data);
    void Clock();
    bool CheckIRQLines();
    bool use6507addresspace = false;

private:
    RomSpace romSpace; // Which ROM layout to protect
    uint8_t data[MAX_MEM];
};

#endif // MEMORY_H