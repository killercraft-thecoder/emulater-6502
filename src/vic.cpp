#include "vic.h"
#include <algorithm>

VIC::VIC(VICColorSpace) {
    framebuffer_.assign(ScreenHeight, std::vector<uint8_t>(ScreenWidth, 0));
    reset(true);
}

void VIC::reset(bool pal) {
    pal_ = pal;
    rasterX_ = 0;
    rasterY_ = 0;
    frameCount_ = 0;
    ctrlReg1_ = ctrlReg2_ = rasterReg_ = 0;
    bgColor_ = 0;
    borderColor_ = 0;
    screenMemBase_ = 0x1E00;
    charMemBase_   = 0x1000;
    for (auto& row : framebuffer_) std::fill(row.begin(), row.end(), 0);
}

void VIC::write(uint16_t addr, uint8_t data) {
    uint8_t reg = addr & 0x0F;
    switch (reg) {
        case 0x00: ctrlReg1_ = data; break;
        case 0x01: ctrlReg2_ = data; break;
        case 0x02: rasterReg_ = data; break;
        case 0x03: bgColor_ = data & 0x0F; break;
        case 0x04: borderColor_ = data & 0x0F; break;
        case 0x05: screenMemBase_ = (data & 0xF0) << 6; break;
        case 0x06: charMemBase_   = (data & 0xF0) << 6; break;
        default: break;
    }
}

uint8_t VIC::read(uint16_t addr) {
    uint8_t reg = addr & 0x0F;
    switch (reg) {
        case 0x00: return ctrlReg1_;
        case 0x01: return ctrlReg2_;
        case 0x02: return rasterReg_;
        case 0x03: return bgColor_;
        case 0x04: return borderColor_;
        default: return 0xFF;
    }
}

void VIC::tick() {
    renderPixel();
    rasterX_++;
    if (rasterX_ >= ScreenWidth) {
        rasterX_ = 0;
        nextRaster();
    }
}

void VIC::renderPixel() {
    // Border check
    if (rasterX_ < BorderLeft || rasterX_ >= ScreenWidth - BorderRight ||
        rasterY_ < BorderTop || rasterY_ >= ScreenHeight - BorderBottom) {
        framebuffer_[rasterY_][rasterX_] = borderColor_;
        return;
    }

    if (!memRead_) {
        framebuffer_[rasterY_][rasterX_] = bgColor_;
        return;
    }

    // Character cell coordinates
    int cellX = (rasterX_ - BorderLeft) / 8;
    int cellY = (rasterY_ - BorderTop) / 8;

    uint16_t screenAddr = screenMemBase_ + cellY * 22 + cellX;
    uint8_t charCode = memRead_(screenAddr);

    // Fetch bitmap row from character memory
    int rowInChar = (rasterY_ - BorderTop) % 8;
    uint16_t charAddr = charMemBase_ + charCode * 8 + rowInChar;
    uint8_t pattern = memRead_(charAddr);

    int bit = 7 - ((rasterX_ - BorderLeft) % 8);
    bool pixelOn = (pattern >> bit) & 1;

    framebuffer_[rasterY_][rasterX_] = pixelOn ? 1 : bgColor_;
}

void VIC::nextRaster() {
    rasterY_++;
    if (rasterY_ >= ScreenHeight) {
        rasterY_ = 0;
        frameCount_++;
    }
}