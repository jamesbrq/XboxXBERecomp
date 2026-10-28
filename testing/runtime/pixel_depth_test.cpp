#include <cassert>
#include <array>
#include <string>
#include "gpu/shaders.h"
#include "gpu/vertex.h"
#include "kernel/kernel.h"
#include <d3dcompiler.h>

namespace kernel { uint8_t* g_base = nullptr; }
namespace mem { uint32_t VirtualFromPhysical(uint32_t a) { return a; } }
namespace logx { void Write(Level, const char*, ...) {} }

static void Compile(const std::string& hlsl, bool depth) {
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT result = D3DCompile(hlsl.data(), hlsl.size(), nullptr, nullptr, nullptr,
                                "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
    if (errors) { if (FAILED(result)) std::fprintf(stderr, "%s", (char*)errors->GetBufferPointer()); errors->Release(); }
    assert(SUCCEEDED(result));
    ID3D11ShaderReflection* reflection = nullptr;
    assert(SUCCEEDED(D3DReflect(code->GetBufferPointer(), code->GetBufferSize(),
                               IID_ID3D11ShaderReflection, (void**)&reflection)));
    D3D11_SHADER_DESC desc{};
    reflection->GetDesc(&desc);
    bool found = false;
    for (unsigned i = 0; i < desc.OutputParameters; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC output{};
        reflection->GetOutputParameterDesc(i, &output);
        found |= output.SystemValueType == D3D_NAME_DEPTH;
    }
    assert(found == depth);
    reflection->Release(); code->Release();
}

int main() {
    {
        gpu::State state;
        gpu::Assembled lines;
        lines.topology = gpu::Topology::Lines;
        lines.regs = (1 << 0) | (1 << 3); // captured position/diffuse FVF, no normals
        state.rs[gpu::kRsLighting] = 1;
        assert(gpu::ExpandConsoleLine(state, lines, 3));
        assert(!gpu::ExpandConsoleLine(state, lines, 1));
        lines.regs |= 1 << 2;
        assert(!gpu::ExpandConsoleLine(state, lines, 3));
        state.vs = std::make_shared<gpu::VertexShader>();
        lines.pretransformed = true;
        assert(!gpu::ExpandConsoleLine(state, lines, 3));
    }
    // The UI's line geometry shader must compile as line input -> four-vertex
    // triangle strip, preserving the vertex/pixel interface.
    {
        const auto source = gpu::LineHlsl();
        ID3DBlob* code = nullptr; ID3DBlob* errors = nullptr;
        HRESULT result = D3DCompile(source.data(), source.size(), nullptr, nullptr, nullptr,
            "main", "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
        if (errors) { if (FAILED(result)) std::fprintf(stderr, "%s", (char*)errors->GetBufferPointer()); errors->Release(); }
        assert(SUCCEEDED(result));
        ID3D11ShaderReflection* reflection = nullptr;
        assert(SUCCEEDED(D3DReflect(code->GetBufferPointer(), code->GetBufferSize(),
            IID_ID3D11ShaderReflection, (void**)&reflection)));
        D3D11_SHADER_DESC desc{}; reflection->GetDesc(&desc);
        assert(desc.InputPrimitive == D3D_PRIMITIVE_LINE);
        assert(desc.GSOutputTopology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        assert(desc.GSMaxOutputVertexCount == 4);
        reflection->Release(); code->Release();
    }
    gpu::PixelKey k{};
    k.programmable = 1;
    k.tex[1].kind = gpu::kTex2D;
    k.tex[1].rect = 1;
    // Captured depth-reconstruction pass: project2d, dotproduct, DOT_ZW.
    k.tex_modes = (1 << 5) | (0x11 << 10) | (0xA << 15);
    for (bool reversed : {false, true}) {
        gpu::g_reversed_depth = reversed;
        auto hlsl = gpu::PixelHlsl(k);
        assert(hlsl.find("replacementDepth = dp2 / dp3") != std::string::npos);
        assert(hlsl.find(reversed ? "pixelDepth = 1 - replacementDepth" : "pixelDepth = replacementDepth") != std::string::npos);
        Compile(hlsl, true);
        k.quad = 1; Compile(gpu::PixelHlsl(k), true); k.quad = 0;
        // Captured pass reads the same live depth alias for both dot operands.
        k.input_texture = (1 << 16) | (1 << 20);
        k.tex[1].flags = gpu::kTexDepthValue | gpu::kTexTarget;
        hlsl = gpu::PixelHlsl(k);
        assert(hlsl.find(reversed ? "replacementDepth = 1 - t1.r" : "replacementDepth = t1.r") != std::string::npos);
        assert(hlsl.find("replacementDepth = dp2 / dp3") == std::string::npos);
        Compile(hlsl, true);
        k.tex[1].flags = gpu::kTexTarget | gpu::kTexRegion;
        hlsl = gpu::PixelHlsl(k);
        assert(hlsl.find("sample_region[1].xy, sample_region[1].zw") != std::string::npos);
        Compile(hlsl, true);
        k.tex[1].flags = 0;
        k.input_texture = 0;
    }
    k.tex_modes = (1 << 5) | (0x11 << 10) | (0x11 << 15);
    Compile(gpu::PixelHlsl(k), false); // Ordinary dots must retain interpolated depth.
    k.tex_modes = (1 << 5) | (0xA << 10); // Invalid predecessor cannot reference an undeclared dp.
    Compile(gpu::PixelHlsl(k), false);
}
