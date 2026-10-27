#include "gpu/block_decode.h"

#include <cassert>

int main() {
    // Red first row, green second row: preserve both rows of a 4x2 ramp.
    std::vector<uint8_t> bc1 = {0x00, 0xF8, 0xE0, 0x07, 0x00, 0x55, 0xAA, 0xFF};
    uint32_t pitch = 8;
    gpu::ExpandDxt(0x0C, 4, 2, 1, bc1, pitch);
    assert(pitch == 16 && bc1.size() == 32);
    assert(bc1[2] == 255 && bc1[1] == 0 && bc1[3] == 255);
    assert(bc1[16 + 1] == 255 && bc1[16 + 2] == 0 && bc1[16 + 3] == 255);

    // BC1's three-color mode retains transparent pixels at odd extents.
    std::vector<uint8_t> transparent = {0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    pitch = 8;
    gpu::ExpandDxt(0x0C, 1, 1, 1, transparent, pitch);
    assert(pitch == 4 && transparent.size() == 4 && transparent[3] == 0);

    // DXT3 alpha nibbles and DXT5 endpoint/index alpha are independent of color.
    for (uint32_t format : {0x0Eu, 0x0Fu}) {
        std::vector<uint8_t> pixels(32, 0);
        pixels[0] = format == 0x0E ? 0xA3 : 73;
        pixels[8] = 0x00; pixels[9] = 0xF8;
        pixels[16] = format == 0x0E ? 0x5F : 219;
        pixels[24] = 0xE0; pixels[25] = 0x07;
        pitch = 16;
        gpu::ExpandDxt(format, 2, 1, 2, pixels, pitch);
        assert(pitch == 8 && pixels.size() == 16);
        assert(pixels[2] == 255 && pixels[9] == 255);
        assert(pixels[3] == (format == 0x0E ? 51 : 73));
        assert(pixels[7] == (format == 0x0E ? 170 : 73));
        assert(pixels[11] == (format == 0x0E ? 255 : 219));
    }
}
