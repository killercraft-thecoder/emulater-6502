#include "../include/memory.h"
#include <cstring>

constexpr bool DEFAULT_NTSC = true;

Memory::Memory(RomSpace romSpaceType)
#if defined(USE_TIA) || defined(USE_VIC) || defined(USE_VIA)
    :
#endif
#ifdef USE_TIA
      tia(TIAColorSpace::Index)
#endif
#if defined(USE_TIA) && (defined(USE_VIC) || defined(USE_VIA))
      ,
#endif
#ifdef USE_VIC
      vic(VICColorSpace::Index)
#endif
#if defined(USE_VIC) && defined(USE_VIA)
      ,
#endif
#ifdef USE_VIA
      via(),
#ifdef USE_MICRO

      via2(),
      disk()
#endif
#endif
#ifdef USE_PIA
      ,
      pia()
#endif
#ifdef USE_ACIA
      ,
      acia()
#endif

{
    this->romSpace = romSpaceType;
    Reset();
}

void Memory::Reset()
{
    std::memset(data, 0, sizeof(data));
#ifdef USE_TIA
    tia.reset(DEFAULT_NTSC);
#endif
#ifdef USE_RIOT
    riot.reset();
#endif
#ifdef USE_VIC
    vic.reset(DEFAULT_NTSC)
#endif
#ifdef USE_PIA
        pia.reset();
#endif
#ifdef USE_ACIA
    acia.reset();
#endif
#ifdef USE_VIA
    via.reset();
#ifdef USE_MICRO
    via2.reset();
    disk.reset();
#endif
#endif
}

uint8_t Memory::Read(uint16_t addr) const
{
    if (this->use6507addresspace == true)
    {
        addr &= 0x1FFF; // 8KB wrap , hard eltircal limit
    }
#ifdef USE_TIA
    if ((addr & 0x1080) == 0x0000)
        return tia.read(addr & 0x3F);
#endif
#ifdef USE_RIOT
    // RAM and I/O in one go
    if (addr >= 0x0080 && addr <= 0x00FF)
        return riot.read(addr & 0x7F);
    if (addr >= 0x0280 && addr <= 0x0297)
        return riot.read(addr & 0x1F);

#endif
#ifdef USE_VIA
#ifdef USE_MICRO
    // BBC Micro System VIA
    if (addr >= 0xFE40 && addr <= 0xFE4F)
        return via.read(addr & 0x0F);
    if (addr >= 0xFE50 && addr <= 0xFE5F)
        return via.read(addr & 0x0F); // mirror
    // BBC Micro User VIA
    if (addr >= 0xFE60 && addr <= 0xFE6F)
        return via2.read(addr & 0x0F);
    // Disk
    if (addr >= 0x1C00 && addr <= 0xFE83)
    {
        return disk.read(addr - 0x1C00);
    }
#else
    if (addr >= 0xFE40 && addr <= 0xFE5F)
        return via.read(addr & 0x0F);
#endif
#endif

#ifdef USE_VIC
    if ((addr & 0xFFF0) == 0x9000)
        return vic.read(addr & 0x0F);
#endif
#ifdef USE_PIA
    if (addr >= 0xE840 && addr <= 0xE843)
        return pia.read(addr & 0x03);
#endif
#ifdef USE_ACIA
    if (addr >= 0xD000 && addr <= 0xD001)
        return acia.read(addr & 0x01);
#endif
#ifdef USE_6529
    if (addr == 0x1C00)
        return io.read();
#endif
    return data[addr];
}

void Memory::Write(uint16_t addr, uint8_t value)
{
    if (this->use6507addresspace == true)
    {
        addr &= 0x1FFF; // 8KB wrap , hard eltircal limit
    }
#ifdef USE_ROM_PROTECT
    switch (romSpace)
    {
    case RomSpace::C64:
        // BASIC ROM $A000–$BFFF, KERNAL ROM $E000–$FFFF
        if ((addr >= 0xA000 && addr <= 0xBFFF) ||
            (addr >= 0xE000 && addr <= 0xFFFF))
            return;
        break;

    case RomSpace::C128:
        // BASIC ROM $4000–$7FFF, KERNAL ROM $E000–$FFFF (banked)
        if ((addr >= 0x4000 && addr <= 0x7FFF) ||
            (addr >= 0xE000 && addr <= 0xFFFF))
            return;
        break;

    case RomSpace::VIC20:
        // BASIC ROM $1000–$1FFF, Char ROM $8000–$8FFF, Kernal ROM $E000–$FFFF
        if ((addr >= 0x1000 && addr <= 0x1FFF) ||
            (addr >= 0x8000 && addr <= 0x8FFF) ||
            (addr >= 0xE000 && addr <= 0xFFFF))
            return;
        break;

    case RomSpace::PET:
        // BASIC ROM $C000–$FFFF (varies by model)
        if (addr >= 0xC000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::PLUS4:
        // BASIC ROM $8000–$BFFF, Kernal ROM $FC00–$FFFF
        if ((addr >= 0x8000 && addr <= 0xBFFF) ||
            (addr >= 0xFC00 && addr <= 0xFFFF))
            return;
        break;

    case RomSpace::BBC_MICRO:
        // Sideways ROM $8000–$BFFF, OS ROM $C000–$FFFF
        if ((addr >= 0x8000 && addr <= 0xBFFF) ||
            (addr >= 0xC000 && addr <= 0xFFFF))
            return;
        // Example: disk controller registers
        if (addr >= 0x1C00 && addr <= 0xFE83)
        {
            disk.write(addr - 0x1C00, value);
            return;
        }
        break;

    case RomSpace::BBC_MASTER:
        // Similar to BBC Micro but with more sideways banks
        if ((addr >= 0x8000 && addr <= 0xBFFF) ||
            (addr >= 0xC000 && addr <= 0xFFFF))
            return;
        break;

    case RomSpace::APPLE_II:
        // Monitor/BASIC ROM $D000–$FFFF
        if (addr >= 0xD000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::APPLE_II_C:
        // Similar to Apple IIe/c
        if (addr >= 0xC000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::APPLE_II_GS:
        // 65C816, ROM $E00000–$E1FFFF (banked) — simplified here
        if (addr >= 0xE000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::ATARI_2600:
        // Cartridge ROM $F000–$FFFF (varies with cart size)
        if (addr >= 0xF000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::ATARI_5200:
        // OS ROM $D800–$FFFF
        if (addr >= 0xD800 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::ATARI_7800:
        // BIOS ROM $F000–$FFFF
        if (addr >= 0xF000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::ATARI_8BIT:
        // OS ROM $C000–$FFFF
        if (addr >= 0xC000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::ATARI_LYNX:
        // Boot ROM $FE00–$FFFF
        if (addr >= 0xFE00 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::NES:
        // PRG ROM $8000–$FFFF
        if (addr >= 0x8000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::FAMICOM_DISK:
        // BIOS ROM $E000–$FFFF
        if (addr >= 0xE000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::ORIC:
        // ROM $C000–$FFFF
        if (addr >= 0xC000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::KIM1:
        // Monitor ROM $0000–$03FF
        if (addr <= 0x03FF)
            return;
        break;

    case RomSpace::SYM1:
        // Monitor ROM $0000–$0FFF
        if (addr <= 0x0FFF)
            return;
        break;

    case RomSpace::AIM65:
        // Monitor ROM $E000–$FFFF
        if (addr >= 0xE000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::COMMODORE_DISK_DRIVE_1541:
        // Drive ROM $C000–$FFFF
        if (addr >= 0xC000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::COMMODORE_DISK_DRIVE_1571:
        // Drive ROM $8000–$FFFF
        if (addr >= 0x8000 && addr <= 0xFFFF)
            return;
        break;

    case RomSpace::ATARI_1050_DRIVE:
        // Drive ROM $C000–$FFFF
        if (addr >= 0xC000 && addr <= 0xFFFF)
            return;
        break;

    default:
        break; // No ROM protection
    }
#endif

#ifdef USE_TIA
    // TIA registers are mirrored every 64 bytes in their range
    if ((addr & 0x1080) == 0x0000)
    {
        tia.write(addr & 0x3F, value);
        return;
    }
#endif

#ifdef USE_6529
    if (addr == 0x1C00)
        io.write(data);
        return;
#endif

#ifdef USE_RIOT
    // RIOT RAM
    if (addr >= 0x0080 && addr <= 0x00FF)
    {
        riot.write(addr & 0x7F, value);
        return;
    }
    // RIOT I/O + timer
    if (addr >= 0x0280 && addr <= 0x0297)
    {
        riot.write(addr & 0x1F, value);
        return;
    }
#endif

#ifdef USE_VIA
#ifdef USE_MICRO
    if (addr >= 0xFE40 && addr <= 0xFE4F)
    {
        via.write(addr & 0x0F, value);
        return;
    }
    if (addr >= 0xFE50 && addr <= 0xFE5F)
    {
        via.write(addr & 0x0F, value);
        return;
    }
    if (addr >= 0xFE60 && addr <= 0xFE6F)
    {
        via2.write(addr & 0x0F, value);
        return;
    }
    // WD1770 disk controller range: 0x1C00–0xFE83
    if (addr >= 0x1C00 && addr <= 0xFE83)
    {
        disk.write(addr - 0x1C00, value);
        return; // handled, don't fall through to RAM/ROM
    }
#else
    if (addr >= 0xFE40 && addr <= 0xFE5F)
    {
        via.write(addr & 0x0F, value);
        return;
    }
#endif
#endif

#ifdef USE_VIC
    if ((addr & 0xFFF0) == 0x9000)
    {
        vic.write(addr & 0x0F, value);
        return;
    }
#endif
#ifdef USE_PIA
    if (addr >= 0xE840 && addr <= 0xE843)
    {
        pia.write(addr & 0x03, value);
        return;
    }
#endif
#ifdef USE_ACIA
    if (addr >= 0xD000 && addr <= 0xD001)
    {
        acia.write(addr & 0x01, value);
        return;
    }
#endif

    // Default: write to RAM array
    data[addr] = value;
}

void Memory::Clock()
{
#ifdef USE_TIA
    tia.tick(); // Advance TIA video/audio by one cycle
#endif
#ifdef USE_RIOT
    riot.tick(); // Advance RIOT timer
#endif
#ifdef USE_VIC
    vic.tick(); // Advance VIC raster beam
#endif
#ifdef USE_VIA
    via.tick(); // Advance VIA timers/shift register
#ifdef USE_MICRO
    via2.tick();
    disk.tick();
#endif

#endif
#ifdef USE_PIA
    pia.tick(); // Advance PIA handshake/IRQ logic
#endif
#ifdef USE_ACIA
    acia.tick(); // Advance ACIA TX/RX timing
#endif
    return; // Nothing More to Do.
}

bool Memory::CheckIRQLines()
{
    bool irq_line = false;

    // --- VIA ---
#ifdef USE_VIA
    static constexpr uint8_t VIA_REG_IFR = 0x0D;
#ifdef USE_MICRO
    if (via.read(VIA_REG_IFR) & 0x80)
        irq_line = true; // System VIA
    if (via2.read(VIA_REG_IFR) & 0x80)
        irq_line = true; // User VIA
#else
    if (via.read(VIA_REG_IFR) & 0x80)
        irq_line = true;
#endif
#endif

    // --- PIA ---

#ifdef USE_PIA
    if (pia.read(0x01) & 0x80)
        irq_line = true; // PIA CRA
    if (pia.read(0x03) & 0x80)
        irq_line = true; // PIA CRB
#endif

    // --- ACIA ---
#ifdef USE_ACIA
    if (acia.read(0x01) & 0x80)
        irq_line = true; // ACIA status register, bit 7 = IRQ
#endif

    // --- TIA ---
#ifdef USE_TIA
    // TIA: IRQ handling is usually via RIOT, but if TIA has a status bit, check it here
    // (Most 2600 IRQs come from RIOT timer)
#endif

    // --- RIOT ---
#ifdef USE_RIOT
    if (riot.read(0x7F) & 0x80)
        irq_line = true; // RIOT IRQ register
#endif

    return irq_line;
}
