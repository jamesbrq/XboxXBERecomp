#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

namespace gpu {

inline uint64_t ShaderChecksum(const void* data, size_t bytes, uint64_t h = 1469598103934665603ull) {
    auto p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < bytes; i++) h = (h ^ p[i]) * 1099511628211ull;
    return h;
}

inline uint64_t ShaderCacheKey(std::string_view source, std::string_view entry, std::string_view profile, uint32_t flags) {
    uint64_t h = ShaderChecksum(profile.data(), profile.size());
    h = ShaderChecksum(entry.data(), entry.size(), h);
    h = ShaderChecksum(source.data(), source.size(), h);
    return ShaderChecksum(&flags, sizeof(flags), h);
}

// Cache misses, old entries and interrupted/corrupt writes all fall back to
// compilation. The key includes source, entry point, profile and compiler flags.
inline bool ReadShaderCache(const std::filesystem::path& path, uint64_t key, std::vector<uint8_t>& out) {
    out.clear();
    std::error_code ec;
    uint64_t size = std::filesystem::file_size(path, ec);
    if (ec || size < 32 || size > 64 * 1024 * 1024) return false;
    std::ifstream file(path, std::ios::binary);
    std::array<uint64_t, 4> header{};
    if (!file.read(reinterpret_cast<char*>(header.data()), 32) ||
        header[0] != 0x3145484341434258ull || header[1] != key || header[2] != size - 32) return false;
    out.resize(size_t(header[2]));
    if (!file.read(reinterpret_cast<char*>(out.data()), std::streamsize(out.size())) ||
        ShaderChecksum(out.data(), out.size()) != header[3]) {
        out.clear();
        return false;
    }
    return true;
}

inline void WriteShaderCache(const std::filesystem::path& path, uint64_t key, const void* data, size_t bytes) {
    if (!bytes || bytes > 64 * 1024 * 1024 - 32) return;
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return;
    auto temp = path;
    temp += "." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".tmp";
    std::array<uint64_t, 4> header{0x3145484341434258ull, key, bytes, ShaderChecksum(data, bytes)};
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(header.data()), 32);
        file.write(static_cast<const char*>(data), std::streamsize(bytes));
        file.close();
        if (!file) { std::filesystem::remove(temp, ec); return; }
    }
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        // Windows does not replace an existing destination through rename.
        std::filesystem::remove(path, ec);
        std::filesystem::rename(temp, path, ec);
    }
    if (ec) std::filesystem::remove(temp, ec);
}

}  // namespace gpu
