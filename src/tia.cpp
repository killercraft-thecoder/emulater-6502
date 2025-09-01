#include "tia.h"
#include <algorithm>

TIA::TIA(TIAColorSpace)
{
    // Allocate full-frame buffer (color clocks × scanlines)
    framebuffer_.assign(ScanlinesPerFrame, std::vector<uint8_t>(ColorClocksPerScanline, 0));
    reset(true);
}

void TIA::reset(bool ntsc)
{
    ntsc_ = ntsc;
    line_ = 0;
    dot_ = 0;
    frame_ = 0;

    for (auto &row : framebuffer_)
        std::fill(row.begin(), row.end(), 0);

    vsync_ = false;
    vblank_ = false;

    colubk_ = 0x00;
    colupf_ = 0x0E;

    pf0_ = pf1_ = pf2_ = 0;
    ctrlpf_ = 0;
}

void TIA::write(uint16_t addr, uint8_t v)
{
    tiaWriteReg(tiaAddr(addr), v);
}

uint8_t TIA::read(uint16_t addr)
{
    return tiaReadReg(tiaAddr(addr));
}

int TIA::tick(int colorClocks)
{
    int produced = 0;
    for (int i = 0; i < colorClocks; ++i)
    {
        renderDot();
        incrementBeam();
        produced++;
    }
    return produced;
}

uint8_t TIA::currentPixel() const
{
    // Guard against eol/eof boundary
    int l = std::min(std::max(line_, 0), ScanlinesPerFrame - 1);
    int d = std::min(std::max(dot_, 0), ColorClocksPerScanline - 1);
    return framebuffer_[l][d];
}

void TIA::incrementBeam()
{
    dot_++;
    if (dot_ >= ColorClocksPerScanline)
    {
        dot_ = 0;
        line_++;
        // Apply any latched HMOVE at line start (not implemented yet)
        hmoveLatchAndApply();
        if (line_ >= ScanlinesPerFrame)
        {
            line_ = 0;
            frame_++;
        }
    }
}

// Horizontal clocks per scanline for NTSC
static constexpr int HORIZ_CLOCKS_PER_LINE = 228;

void TIA::hmoveLatchAndApply()
{
    // Apply motion to each object
    applyHMOVE(player0_);
    applyHMOVE(player1_);
    applyHMOVE(missile0_);
    applyHMOVE(missile1_);
    applyHMOVE(ball_);

    // HMOVE blanking quirk:
    // On real hardware, writing HMOVE during horizontal blank
    // blanks the first 8 color clocks of the next scanline.
    return;
}

void TIA::applyHMOVE(Object &obj)
{
    // motion is already signed (-8..+7)
    obj.x += obj.motion;

    // Wrap around the scanline
    if (obj.x < 0)
        obj.x += HORIZ_CLOCKS_PER_LINE;
    else if (obj.x >= HORIZ_CLOCKS_PER_LINE)
        obj.x -= HORIZ_CLOCKS_PER_LINE;
}

static inline uint8_t reverse4(uint8_t v)
{
    // Reverse low nibble bits: abcd -> dcba
    return static_cast<uint8_t>(((v & 0x1) << 3) | ((v & 0x2) << 1) | ((v & 0x4) >> 1) | ((v & 0x8) >> 3));
}

void TIA::renderDot()
{
    uint8_t color = colubk_; // start with background

    // Respect VBLANK: simplest approach blanks to background color
    // (Real TIA can blank video output; we model as no playfield visibility)
    bool showVideo = !vblank_;

    // Build 20-bit playfield pattern for "left half"
    // PF0 contributes only its high nibble (bits 4..7), reversed on screen
    // Left half bits order: PF0[4..7 reversed], PF1[7..0], PF2[7..0] => total 4 + 8 + 8 = 20 bits
    uint8_t pf0_hi = static_cast<uint8_t>((pf0_ >> 4) & 0x0F);
    uint8_t pf0_disp = reverse4(pf0_hi);
    uint32_t pfLeft = (static_cast<uint32_t>(pf0_disp) << 16) |
                      (static_cast<uint32_t>(pf1_) << 8) |
                      (static_cast<uint32_t>(pf2_));

    // Determine whether current dot is in left or right half
    bool leftHalf = (dot_ < (ColorClocksPerScanline / 2));
    bool reflect = (ctrlpf_ & 0x01) != 0;

    // Each playfield bit spans 4 color clocks
    int bitIdx = (dot_ % (ColorClocksPerScanline / 2)) / 4; // 0..19

    bool pfBitOn = false;
    if (showVideo)
    {
        if (leftHalf)
        {
            // Left: render bits from MSB to LSB across 20 groups
            int fromLeft = 19 - bitIdx; // bit 19 at left edge to bit 0 at center
            pfBitOn = ((pfLeft >> fromLeft) & 0x1) != 0;
        }
        else
        {
            // Right: either reflect or repeat (mirror) the left half
            if (reflect)
            {
                // Reflect: right half is left half reversed (bit 0 at center to bit 19 at right edge)
                pfBitOn = ((pfLeft >> bitIdx) & 0x1) != 0;
            }
            else
            {
                // Repeat (no reflect): same order as left (i.e., mirrored copy)
                int fromLeft = 19 - bitIdx;
                pfBitOn = ((pfLeft >> fromLeft) & 0x1) != 0;
            }
        }
    }

    // --- Player 0 ---
    if (player0_.enabled)
    {
        int relX = (dot_ - player0_.x - player0_.motion + ColorClocksPerScanline) % ColorClocksPerScanline;
        if (relX < 8)
        {
            uint8_t mask = player0_.reflect ? (1 << relX) : (0x80 >> relX);
            if (player0_.gfx & mask)
                color = colupf_; // TODO: use COLUP0
        }
    }

    // --- Player 1 ---
    if (player1_.enabled)
    {
        int relX = (dot_ - player1_.x - player1_.motion + ColorClocksPerScanline) % ColorClocksPerScanline;
        if (relX < 8)
        {
            uint8_t mask = player1_.reflect ? (1 << relX) : (0x80 >> relX);
            if (player1_.gfx & mask)
                color = colupf_; // TODO: use COLUP1
        }
    }

    // --- Missile 0 ---
    if (enam0_)
    {
        if (dot_ == (missile0_.x + missile0_.motion) % ColorClocksPerScanline)
        {
            color = colupf_; // TODO: use COLUP0
        }
    }

    // --- Missile 1 ---
    if (enam1_)
    {
        if (dot_ == (missile1_.x + missile1_.motion) % ColorClocksPerScanline)
        {
            color = colupf_; // TODO: use COLUP1
        }
    }

    // --- Ball ---
    if (enabl_)
    {
        int bx = (ball_.x + ball_.motion) % ColorClocksPerScanline;
        if (dot_ >= bx && dot_ < bx + ballSize_)
        {
            color = colupf_; // TODO: use COLUPF
        }
    }

    if (pfBitOn)
    {
        color = colupf_;
    }

    // Store pixel
    framebuffer_[line_][dot_] = color;
}

uint8_t TIA::tiaReadReg(uint8_t r)
{
    switch (r)
    {
    // Collision registers would return latched bits; not implemented yet
    case INPT0:
    case INPT1:
    case INPT2:
    case INPT3:
    case INPT4:
    case INPT5:
        if (inputReader_)
            return inputReader_(r - INPT0) ? 0x80 : 0x00;
        return 0x00;
        // Collision registers (stubbed here; real impl would return latched bits)
    case CXM0P:
        return 0x00; // Missile 0–Player collisions
    case CXM1P:
        return 0x00;
    case CXP0FB:
        return 0x00; // Player 0–Playfield/Ball
    case CXP1FB:
        return 0x00;
    case CXM0FB:
        return 0x00; // Missile 0–Playfield/Ball
    case CXM1FB:
        return 0x00;
    case CXBLPF:
        return 0x00; // Ball–Playfield
    case CXPPMM:
        return 0x00; // Player–Player / Missile–Missile
    default:
        return 0x00;

    }
}

void TIA::tiaWriteReg(uint8_t r, uint8_t v)
{
    switch (r)
    {
    case VSYNC:
        // Bit 1 set -> VSYNC active for 3 scanlines typically
        vsync_ = (v & 0x02) != 0;
        break;
    case VBLANK:
        // Bit 1 set -> VBLANK (video off); bit 7 affects input latching
        vblank_ = (v & 0x02) != 0;
        break;
    case WSYNC:
        // Stall CPU until end-of-scanline; then advance to next line start
        if (wsyncStall_)
        {
            int remainingColor = ColorClocksPerScanline - dot_;
            int remainingCpu = remainingColor / 3;
            if (remainingCpu > 0)
                wsyncStall_(remainingCpu);
        }
        // Force beam to end of line; incrementBeam() will roll it on next tick
        dot_ = ColorClocksPerScanline - 1;
        break;
    case RSYNC:
        // Simplified: real RSYNC aligns horizontal sync; we can noop for now
        break;

    // Colors
    case COLUBK:
        colubk_ = v & 0x7F;
        break;
    case COLUPF:
        colupf_ = v & 0x7F;
        break;

    // Playfield
    case PF0:
        pf0_ = v;
        break;
    case PF1:
        pf1_ = v;
        break;
    case PF2:
        pf2_ = v;
        break;

        // === Player graphics ===
    case GRP0:
        player0_.gfx = v;
        break;
    case GRP1:
        player1_.gfx = v;
        break;

    // === Enable missiles/ball ===
    case ENAM0:
        enam0_ = (v & 0x02) != 0;
        break;
    case ENAM1:
        enam1_ = (v & 0x02) != 0;
        break;
    case ENABL:
        enabl_ = (v & 0x02) != 0;
        break;

    // === Reflection ===
    case REFP0:
        player0_.reflect = (v & 0x08) != 0;
        break;
    case REFP1:
        player1_.reflect = (v & 0x08) != 0;
        break;

    // === Reset positions to current beam ===
    case RESP0:
        player0_.x = dot_;
        break;
    case RESP1:
        player1_.x = dot_;
        break;
    case RESM0:
        missile0_.x = dot_;
        break;
    case RESM1:
        missile1_.x = dot_;
        break;
    case RESBL:
        ball_.x = dot_;
        break;

    // === Horizontal motion registers ===
    case HMP0:
        player0_.motion = static_cast<int8_t>(v) >> 4;
        break;
    case HMP1:
        player1_.motion = static_cast<int8_t>(v) >> 4;
        break;
    case HMM0:
        missile0_.motion = static_cast<int8_t>(v) >> 4;
        break;
    case HMM1:
        missile1_.motion = static_cast<int8_t>(v) >> 4;
        break;
    case HMBL:
        ball_.motion = static_cast<int8_t>(v) >> 4;
        break;

    // === Ball size (CTRLPF bits 4–5) ===
    case CTRLPF:
        ctrlpf_ = v;
        ballSize_ = 1 << ((v >> 4) & 0x03); // 1, 2, 4, or 8 pixels wide
        break;
    case NUSIZ0:
        nusiz0_ = v & 0x07;
        player0_.size = (nusiz0_ == 5) ? 2 : (nusiz0_ == 7 ? 4 : 1);
        break;
    case NUSIZ1:
        nusiz1_ = v & 0x07;
        player1_.size = (nusiz1_ == 5) ? 2 : (nusiz1_ == 7 ? 4 : 1);
        break;

    default:
        // Mirrors and unused register slots
        (void)v;
        break;
    }
}