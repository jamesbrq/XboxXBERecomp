// Compile existing captured HLSL into the runtime cache without a device or game boot.
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "gpu/shader_cache.h"
#include <cstdio>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: warm_shaders <cache-directory> <capture-directory> [...]\n");
        return 2;
    }
    size_t compiled = 0, reused = 0, failed = 0;
    for (int arg = 2; arg < argc; arg++) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(argv[arg])) {
            auto name = file.path().filename().string();
            const char* profile = name.rfind("vs_", 0) == 0 ? "vs_5_0" : name.rfind("ps_", 0) == 0 ? "ps_5_0" : nullptr;
            if (!profile || file.path().extension() != ".hlsl") continue;
            // Captures use Windows text mode; restore the generator's LF source
            // so the disk key matches the runtime's original source exactly.
            std::ifstream input(file.path());
            std::string source{std::istreambuf_iterator<char>(input), {}};
            constexpr uint32_t flags = D3DCOMPILE_OPTIMIZATION_LEVEL1;
            uint64_t key = gpu::ShaderCacheKey(source, "main", profile, flags);
            char filename[32];
            std::snprintf(filename, sizeof(filename), "%016llX.bin", static_cast<unsigned long long>(key));
            auto cache = std::filesystem::path(argv[1]) / filename;
            std::vector<uint8_t> bytes;
            if (gpu::ReadShaderCache(cache, key, bytes) && bytes.size() >= 32 &&
                !std::memcmp(bytes.data(), "DXBC", 4)) { reused++; continue; }
            Microsoft::WRL::ComPtr<ID3DBlob> code, error;
            HRESULT hr = D3DCompile(source.data(), source.size(), nullptr, nullptr, nullptr,
                                    "main", profile, flags, 0, &code, &error);
            if (FAILED(hr)) {
                std::fprintf(stderr, "%s: %s\n", file.path().string().c_str(),
                             error ? static_cast<const char*>(error->GetBufferPointer()) : "compile failed");
                failed++;
                continue;
            }
            gpu::WriteShaderCache(cache, key, code->GetBufferPointer(), code->GetBufferSize());
            if (!gpu::ReadShaderCache(cache, key, bytes) || bytes.size() != code->GetBufferSize() ||
                std::memcmp(bytes.data(), code->GetBufferPointer(), bytes.size())) { failed++; continue; }
            compiled++;
        }
    }
    std::printf("Shaders compiled: %zu; cache hits: %zu; failures: %zu\n", compiled, reused, failed);
    return failed ? 1 : 0;
}
