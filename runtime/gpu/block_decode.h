#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace gpu {

// DXT blocks to BGRA for textures whose top level cannot be represented by a
// native BC resource without padding. Keep the guest extent for filtering/wrap.
inline void ExpandDxt(uint32_t format, uint32_t w, uint32_t h, uint32_t depth,
                      std::vector<uint8_t>& pixels, uint32_t& pitch) {
    const uint32_t block_bytes = format == 0x0C ? 8 : 16;
    const uint32_t rows = (h + 3) / 4;
    std::vector<uint8_t> out(size_t(w) * h * depth * 4);
    for (uint32_t z = 0; z < depth; z++) {
        for (uint32_t by = 0; by < rows; by++) {
            for (uint32_t bx = 0; bx < (w + 3) / 4; bx++) {
                const uint8_t* block = pixels.data() + (size_t(z) * rows + by) * pitch + bx * block_bytes;
                const uint8_t* color = block + (block_bytes == 16 ? 8 : 0);
                uint16_t endpoints[2];
                uint32_t indices;
                std::memcpy(endpoints, color, 4);
                std::memcpy(&indices, color + 4, 4);
                uint8_t colors[4][4] = {};
                for (int i = 0; i < 2; i++) {
                    uint32_t b = endpoints[i] & 31, g = (endpoints[i] >> 5) & 63, r = endpoints[i] >> 11;
                    colors[i][0] = uint8_t((b << 3) | (b >> 2));
                    colors[i][1] = uint8_t((g << 2) | (g >> 4));
                    colors[i][2] = uint8_t((r << 3) | (r >> 2));
                    colors[i][3] = 255;
                }
                bool four = format != 0x0C || endpoints[0] > endpoints[1];
                for (int c = 0; c < 3; c++) {
                    colors[2][c] = uint8_t(four ? (2 * colors[0][c] + colors[1][c]) / 3
                                               : (colors[0][c] + colors[1][c]) / 2);
                    colors[3][c] = uint8_t(four ? (colors[0][c] + 2 * colors[1][c]) / 3 : 0);
                }
                colors[2][3] = 255;
                colors[3][3] = four ? 255 : 0;
                uint8_t alpha[8] = {block[0], block[1]};
                uint64_t alpha_bits = 0;
                if (format == 0x0F) {
                    for (int i = 0; i < 6; i++) alpha_bits |= uint64_t(block[i + 2]) << (8 * i);
                    int n = alpha[0] > alpha[1] ? 7 : 5;
                    for (int i = 1; i < n; i++) alpha[i + 1] = uint8_t(((n - i) * alpha[0] + i * alpha[1]) / n);
                    if (n == 5) { alpha[6] = 0; alpha[7] = 255; }
                }
                for (uint32_t y = 0; y < 4 && by * 4 + y < h; y++) {
                    for (uint32_t x = 0; x < 4 && bx * 4 + x < w; x++) {
                        uint32_t i = y * 4 + x;
                        uint8_t* dst = out.data() + ((size_t(z) * h + by * 4 + y) * w + bx * 4 + x) * 4;
                        std::memcpy(dst, colors[(indices >> (2 * i)) & 3], 4);
                        if (format == 0x0E) dst[3] = uint8_t(((block[i / 2] >> ((i & 1) * 4)) & 15) * 17);
                        if (format == 0x0F) dst[3] = alpha[(alpha_bits >> (3 * i)) & 7];
                    }
                }
            }
        }
    }
    pixels.swap(out);
    pitch = w * 4;
}

}  // namespace gpu
