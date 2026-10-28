#include "gpu/shader_cache.h"
#include <cassert>
#include <cstdio>

int main() {
    auto key = gpu::ShaderCacheKey("shader", "main", "ps_5_0", 0);
    assert(key != gpu::ShaderCacheKey("shader2", "main", "ps_5_0", 0));
    assert(key != gpu::ShaderCacheKey("shader", "other", "ps_5_0", 0));
    assert(key != gpu::ShaderCacheKey("shader", "main", "vs_5_0", 0));
    assert(key != gpu::ShaderCacheKey("shader", "main", "ps_5_0", 1));
    namespace fs = std::filesystem;
    auto dir = fs::current_path() / "out" / ("xbe-shader-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto file = dir / "shader.bin";
    std::vector<uint8_t> bytes(128, 42), result;
    assert(!gpu::ReadShaderCache(file, 123, result));
    gpu::WriteShaderCache(file, 123, bytes.data(), bytes.size());
    std::error_code ec;
    auto size = fs::file_size(file, ec);
    if (ec) std::fprintf(stderr, "cache %s: %s\n", file.string().c_str(), ec.message().c_str());
    assert(!ec && size == bytes.size() + 32);
    assert(gpu::ReadShaderCache(file, 123, result) && result == bytes);
    assert(!gpu::ReadShaderCache(file, 124, result) && result.empty());
    { // a corrupted payload must be recompiled, not fed to D3D
        std::fstream f(file, std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(35); f.put('!');
    }
    assert(!gpu::ReadShaderCache(file, 123, result));
    gpu::WriteShaderCache(file, 123, bytes.data(), bytes.size());
    assert(gpu::ReadShaderCache(file, 123, result) && result == bytes);
    fs::resize_file(file, 40);
    assert(!gpu::ReadShaderCache(file, 123, result));
    { std::ofstream f(file, std::ios::binary | std::ios::trunc); f << "interrupted"; }
    assert(!gpu::ReadShaderCache(file, 123, result));
    fs::remove(file);
    fs::remove(dir);
}
