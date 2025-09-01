#pragma once
#include <cstdint>
#include <functional>
#include <vector>

enum class TIAColorSpace : uint8_t
{
    Index
};

class TIA
{
public:
    // Public timing constants (NTSC defaults)
    static constexpr int ColorClocksPerScanline = 228;
    static constexpr int ScanlinesPerFrame = 262;
    static constexpr int CpuCyclesPerScanline = 76; // 3 color clocks per CPU cycle

    // Callbacks
    using InputReader = std::function<bool(int line)>;     // INPT0..INPT5 -> returns "pressed" (true -> bit7=1)
    using AudioSink = std::function<void(int16_t sample)>; // optional
    using WsyncStall = std::function<void(int cpuCycles)>; // burn CPU cycles to end of scanline

    explicit TIA(TIAColorSpace cs = TIAColorSpace::Index);

    // Lifecycle
    void reset(bool ntsc = true);

    // Memory-mapped IO (TIA mirrored every 64 bytes; pass system address, we’ll mask)
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr);

    // Tick TIA by N color clocks (3 per CPU cycle)
    int tick(int colorClocks);

    // Pixel access
    uint8_t currentPixel() const; // color index at current beam pos
    const std::vector<std::vector<uint8_t>> &frame() const { return framebuffer_; }

    // Introspection
    int scanline() const { return line_; }
    int dot() const { return dot_; }
    bool inVBlank() const { return vblank_; }

    // Hooks
    void setInputReader(InputReader f) { inputReader_ = std::move(f); }
    void setAudioSink(AudioSink f) { audioSink_ = std::move(f); }
    void setWsyncStall(WsyncStall f) { wsyncStall_ = std::move(f); }

private:
    // TIA register indices (masked to 0x00..0x3F)
    enum : uint8_t
    {
        // Writes 0x00..0x2D
        VSYNC = 0x00,
        VBLANK = 0x01,
        WSYNC = 0x02,
        RSYNC = 0x03,
        NUSIZ0 = 0x04,
        NUSIZ1 = 0x05,
        COLUP0 = 0x06,
        COLUP1 = 0x07,
        COLUPF = 0x08,
        COLUBK = 0x09,
        CTRLPF = 0x0A,
        REFP0 = 0x0B,
        REFP1 = 0x0C,
        PF0 = 0x0D,
        PF1 = 0x0E,
        PF2 = 0x0F,
        RESP0 = 0x10,
        RESP1 = 0x11,
        RESM0 = 0x12,
        RESM1 = 0x13,
        RESBL = 0x14,
        AUDC0 = 0x15,
        AUDC1 = 0x16,
        AUDF0 = 0x17,
        AUDF1 = 0x18,
        AUDV0 = 0x19,
        AUDV1 = 0x1A,
        GRP0 = 0x1B,
        GRP1 = 0x1C,
        ENAM0 = 0x1D,
        ENAM1 = 0x1E,
        ENABL = 0x1F,
        HMP0 = 0x20,
        HMP1 = 0x21,
        HMM0 = 0x22,
        HMM1 = 0x23,
        HMBL = 0x24,
        VDELP0 = 0x25,
        VDELP1 = 0x26,
        VDELBL = 0x27,
        RESMP0 = 0x28,
        RESMP1 = 0x29,
        HMOVE = 0x2A,
        HMCLR = 0x2B,
        CXCLR = 0x2C,

        // Reads 0x30..0x3D
        CXM0P = 0x30,
        CXM1P = 0x31,
        CXP0FB = 0x32,
        CXP1FB = 0x33,
        CXM0FB = 0x34,
        CXM1FB = 0x35,
        CXBLPF = 0x36,
        CXPPMM = 0x37,
        INPT0 = 0x38,
        INPT1 = 0x39,
        INPT2 = 0x3A,
        INPT3 = 0x3B,
        INPT4 = 0x3C,
        INPT5 = 0x3D
    };

    // Address mirror helper
    static inline uint8_t tiaAddr(uint16_t a) { return static_cast<uint8_t>(a & 0x3F); }

    // Core pipeline
    void renderDot();
    void incrementBeam();
    void hmoveLatchAndApply(); // placeholder for future HMOVE details
    uint8_t tiaReadReg(uint8_t reg);
    void tiaWriteReg(uint8_t reg, uint8_t v);

    // Timing
    bool ntsc_ = true;
    int line_ = 0; // 0..261
    int dot_ = 0;  // 0..227
    int frame_ = 0;

    // Visible buffer of color indices (one per color clock)
    std::vector<std::vector<uint8_t>> framebuffer_;

    // Callbacks
    InputReader inputReader_{};
    AudioSink audioSink_{};
    WsyncStall wsyncStall_{};

    // Minimal video state implemented so far
    bool vsync_ = false;
    bool vblank_ = false;

    // Colors (7-bit)
    uint8_t colubk_ = 0x00; // background
    uint8_t colupf_ = 0x0E; // playfield

    // Playfield registers
    uint8_t pf0_ = 0;
    uint8_t pf1_ = 0;
    uint8_t pf2_ = 0;
    uint8_t ctrlpf_ = 0; // bit0: reflect playfield; bit1: score mode (unused yet)

    // === Player / Missile / Ball state ===
    struct Object
    {
        int x = 0;               // horizontal position in color clocks (0..227)
        uint8_t gfx = 0;         // 8-bit graphics pattern (players), 1-bit for missiles/ball
        bool enabled = false;    // visible?
        bool reflect = false;    // horizontal mirror
        uint8_t size = 1;        // 1, 2, or 4 copies (NUSIZ)
        uint8_t copySpacing = 0; // spacing between copies in pixels
        int motion = 0;          // HMOVE offset (-8..+7)
    };

    Object player0_, player1_;
    Object missile0_, missile1_;
    Object ball_;
    
    void TIA::applyHMOVE(Object &obj);

    // NUSIZ registers (affect size/duplication for players/missiles)
    uint8_t nusiz0_ = 0;
    uint8_t nusiz1_ = 0;

    // Enable bits
    bool enam0_ = false;
    bool enam1_ = false;
    bool enabl_ = false;

    // Ball size (CTRLPF bits 4–5)
    uint8_t ballSize_ = 1;

    // Lookup tables for NUSIZ patterns (index = NUSIZ value 0..7)
    static constexpr uint8_t NUSIZ_COPIES[8] = {1, 2, 2, 3, 2, 1, 3, 1};
    static constexpr uint8_t NUSIZ_SPACING[8][3] = {
        {0, 0, 0},   // 1 copy
        {0, 16, 0},  // 2 copies, close
        {0, 32, 0},  // 2 copies, medium
        {0, 16, 32}, // 3 copies, close
        {0, 64, 0},  // 2 copies, wide
        {0, 0, 0},   // 1 copy (double width)
        {0, 32, 64}, // 3 copies, medium
        {0, 0, 0}    // 1 copy (quad width)
    };
};