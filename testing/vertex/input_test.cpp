#include <array>
#include <cassert>
#include <cstring>
#include "gpu/vertex.h"
#include "kernel/kernel.h"

static std::array<uint8_t, 65536> ram{};
namespace kernel { uint8_t* g_base = ram.data(); }
namespace mem { uint32_t VirtualFromPhysical(uint32_t address) { return address; } }
namespace logx { void Write(Level, const char*, ...) {} }

static float Position(const gpu::Assembled& a, uint32_t vertex) {
    float result;
    const auto& element = a.elements[0];
    const auto& buffer = a.buffers[element.buffer];
    std::memcpy(&result, buffer.src + vertex * buffer.stride + element.offset, sizeof(result));
    return result;
}
int main() {
    using namespace kernel;
    gpu::State s;
    s.vs_handle = 1;
    s.vs = std::make_shared<gpu::VertexShader>();
    s.vs->inputs[0] = {0, 4, 0x32};
    s.vs->stream_stride[0] = 16;
    s.streams[0] = {0x1000, 16, 8};
    W32(0x1004, 0x2000);
    W32(0x1104, 0x4000);
    for (uint32_t v = 0; v < 3; v++) {
        float ordinary[3] = {float(10 + v), 2, 3};
        float custom[3] = {float(100 + v), 4, 5};
        std::memcpy(HostPtr(0x2000 + 8 + 4 + v * 16), ordinary, sizeof(ordinary));
        std::memcpy(HostPtr(0x4000 + 32 + v * 16), custom, sizeof(custom));
    }
    gpu::Assembled output;
    assert(gpu::Assemble(s, 5, 0, 3, nullptr, output));
    assert(Position(output, 0) == 10 && Position(output, 2) == 12);
    // A separate declaration and byte offset must not read the ordinary stream.
    s.custom_input = true;
    s.custom_inputs[0] = {1, 0, 0x32};
    s.custom_strides[1] = 16;
    s.input_streams[1] = {0x1100, 16, 32};
    assert(gpu::Assemble(s, 5, 0, 3, nullptr, output));
    assert(Position(output, 0) == 100 && Position(output, 2) == 102);
    // Disabling the override restores the ordinary stream and declaration.
    s.custom_input = false;
    assert(gpu::Assemble(s, 5, 0, 3, nullptr, output));
    assert(Position(output, 1) == 11);
}
