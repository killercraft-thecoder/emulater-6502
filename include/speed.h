#pragma once

// ------------------------------------------------------------
// CPU clock frequencies for various 6502-based systems
// ------------------------------------------------------------
// Values are in Hz (cycles per second)
constexpr double CPU_FREQ_C64        = 1022727.0; // PAL C64 ~1.0227 MHz
constexpr double CPU_FREQ_C64_NTSC   = 1022727.0; // NTSC C64 ~1.0227 MHz
constexpr double CPU_FREQ_BBC_MICRO  = 2000000.0; // BBC Micro Model B 2 MHz
constexpr double CPU_FREQ_APPLE_II   = 1023000.0; // Apple II ~1.023 MHz
constexpr double CPU_FREQ_ATARI_2600 = 1193191.0; // Atari 2600 ~1.19 MHz
constexpr double CPU_FREQ_NES        = 1789773.0; // NTSC NES ~1.789 MHz
constexpr double CPU_FREQ_ATARI_8BIT = 1773447.0; // NTSC Atari 8-bit ~1.773 MHz
constexpr double CPU_FREQ_VIC20      = 1108404.0; // PAL VIC-20 ~1.108 MHz

#pragma once

// Default CPU frequency (Hz)
inline double CPU_FREQ = CPU_FREQ_BBC_MICRO; // BBC Micro Model B 2 MHz by defualt

// Helpers for timing conversions
inline double CYCLES_PER_MS() { return CPU_FREQ / 1000.0; }
inline double CYCLES_PER_US() { return CPU_FREQ / 1000000.0; }