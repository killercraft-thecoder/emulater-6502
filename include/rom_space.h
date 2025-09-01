#pragma once

// Enum for different machine ROM layouts
enum class RomSpace {
    NONE,            // No ROM protection / generic 6502

    // Commodore family
    C64,             // Commodore 64 (6510 + VIC-II + SID)
    C128,            // Commodore 128 (8502 + Z80 + VIC-IIe + VDC)
    VIC20,           // Commodore VIC-20 (VIC + VIA)
    PET,             // Commodore PET/CBM series
    PLUS4,           // Commodore Plus/4, C16, C116 (TED chip)

    // Apple family
    APPLE_II,        // Apple II / II+ / IIe
    APPLE_II_C,      // Apple IIc
    APPLE_II_GS,     // Apple IIGS (65C816)

    // Atari family
    ATARI_2600,      // Atari 2600 (6507 + TIA + RIOT)
    ATARI_5200,      // Atari 5200 (ANTIC + GTIA + POKEY)
    ATARI_7800,      // Atari 7800 (MARIA + TIA)
    ATARI_8BIT,      // Atari 400/800/XL/XE (ANTIC + GTIA + POKEY)
    ATARI_LYNX,      // Atari Lynx (65SC02 + Suzy/Mikey)

    // Nintendo
    NES,             // Nintendo Entertainment System (Ricoh 2A03/2A07)
    FAMICOM_DISK,    // Famicom Disk System

    // Acorn / BBC
    BBC_MICRO,       // BBC Micro Model B (6845 CRTC + 6522 VIA + 1770 FDC)
    ACORN_ELECTRON,  // Acorn Electron
    BBC_MASTER,      // BBC Master series

    // Arcade / coin-op
    ARCADE_CENTIPEDE, // Atari Centipede hardware
    ARCADE_DONKEYKONG,// Nintendo Donkey Kong hardware
    ARCADE_ASTEROIDS, // Atari Asteroids hardware

    // Other home computers
    ORIC,            // Oric-1 / Atmos
    DRAGON,          // Dragon 32/64 (6809 normally, but some variants had 6502 boards)
    TRS80_COCO,      // Tandy Color Computer (again, mostly 6809, but placeholder if needed)
    KIM1,            // MOS KIM-1 trainer board
    SYM1,            // Synertek SYM-1
    AIM65,           // Rockwell AIM 65

    // Game consoles (other)
    COLECOVISION,    // ColecoVision (Z80 main, but 6502 in some expansions)
    VECTREX,         // Vectrex (6809 main, but placeholder if needed for 6502 co-proc boards)

    // Embedded / misc
    COMMODORE_DISK_DRIVE_1541, // 6502 inside the C1541 floppy drive
    COMMODORE_DISK_DRIVE_1571, // 6502 inside the C1571 floppy drive
    ATARI_1050_DRIVE,          // 6502 inside Atari 1050 floppy drive

    // Add more as needed...
};