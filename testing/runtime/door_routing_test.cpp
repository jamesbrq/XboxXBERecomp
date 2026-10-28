#include <cassert>
#include <cstring>
#include <array>
#include "cpu/x86_ops.h"

static unsigned original_calls;
X86_FUNC(__imp__sub_005AB190) {
    ++original_calls;
    ctx.eax = (ctx.eax & 0xffff0000u) | 1;
    ctx.esp += 4;
}
#define GHOULIES_WALKTEST 1
#include "../ghoulies/src/game/hooks.cpp"

int main() {
    std::array<uint8_t, 4096> memory{};
    // Native event start/end and mod challenge/repose requests must retain ids.
    for (uint32_t caller : {0xD0419u, 0xD0984u, 0xD0A6Fu, 0xD0AD1u,
                            0x5A7E10u, 0x5A8170u, 0x572000u, 0x5AD793u}) {
        X86Context ctx{};
        ctx.esp = 512;
        ctx.eax = 0x12340002;
        ctx.ecx = 0xabcdef;
        ctx.cf = ctx.pf = ctx.zf = 1;
        ctx.xmm[0].u[0] = 0xdeadbeef;
        auto expected = ctx;
        expected.esp += 4;
        const uint32_t immediate_return = 0x5AD235;
        std::memcpy(memory.data() + 512, &immediate_return, 4);
        std::memcpy(memory.data() + 516, &caller, 4);
        const auto before = memory;
        sub_005AB190(ctx, memory.data());
        assert(std::memcmp(&ctx, &expected, sizeof(ctx)) == 0);
        assert(memory == before);
        assert(original_calls == 0);
    }
    // Ordinary script/replay calls still run the original entrance remapper.
    for (uint32_t caller : {0x125BBCu, 0x126200u, 0x5C148u, 0x571FFFu, 0x5AD794u}) {
        X86Context ctx{};
        ctx.esp = 512;
        ctx.eax = 0x12340002;
        std::memcpy(memory.data() + 516, &caller, 4);
        auto calls = original_calls;
        sub_005AB190(ctx, memory.data());
        assert(original_calls == calls + 1);
        assert(ctx.esp == 516 && ctx.eax == 0x12340001);
    }
}
