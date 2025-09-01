#pragma once
#include <cstdint>
#include <vector>
#include <functional>

enum class VICColorSpace : uint8_t { Index };

class VIC {
public:
    // VIC-20 visible area (approximate)
    static constexpr int ScreenWidth  = 176;
    static constexpr int ScreenHeight = 184;

    using ReadMem = std::function<uint8_t(uint16_t)>;

    explicit VIC(VICColorSpace cs = VICColorSpace::Index);

    void reset(bool pal = true);

    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr);

    void tick(); // advance one pixel

    const std::vector<std::vector<uint8_t>>& frame() const { return framebuffer_; }

    void setMemoryReader(ReadMem f) { memRead_ = std::move(f); }

private:
    void renderPixel();
    void nextRaster();

    bool pal_ = true;
    int rasterX_ = 0;
    int rasterY_ = 0;
    int frameCount_ = 0;

    // Registers
    uint8_t ctrlReg1_ = 0;
    uint8_t ctrlReg2_ = 0;
    uint8_t rasterReg_ = 0;
    uint8_t bgColor_   = 0;
    uint8_t borderColor_ = 0;
    uint16_t screenMemBase_ = 0x1E00;
    uint16_t charMemBase_   = 0x1000;

    // Framebuffer
    std::vector<std::vector<uint8_t>> framebuffer_;

    // Memory access
    ReadMem memRead_{};

    // Border size (simplified)
    static constexpr int BorderLeft   = 16;
    static constexpr int BorderRight  = 16;
    static constexpr int BorderTop    = 16;
    static constexpr int BorderBottom = 16;
};