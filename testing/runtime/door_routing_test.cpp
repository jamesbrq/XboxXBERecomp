#include <cassert>
#include <cstring>
#include <array>
#include <vector>
#include "cpu/x86_ops.h"

static unsigned original_calls;
static unsigned animation_ticks;
static bool normal_tick;
static void word(uint8_t* b, uint32_t a, uint32_t v) { std::memcpy(b + a, &v, 4); }
static void real(uint8_t* b, uint32_t a, float v) { std::memcpy(b + a, &v, 4); }
X86_FUNC(__imp__sub_00033B50) {
    if (normal_tick) real(base, 0x802c, 0.5f);
    ctx.eax = 0x12345678;
    ctx.esp += 4;
}
X86_FUNC(__imp__sub_0005AA90) {
    ++animation_ticks;
    real(base, 0x802c, 0.5f);
    ctx.eax = 0xdeadbeef;
    ctx.esp += 4;
}
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
    std::vector<uint8_t> guest(0x600000);
    auto setup = [&] {
        std::fill(guest.begin(), guest.end(), 0);
        auto b = guest.data();
        word(b, 0x50ea9c, 0x2000000);
        word(b, 0x5a7cc0, 2); word(b, 0x5a7d60, 0x7000);
        word(b, 0x5064d8, 1); word(b, 0x5064dc, 2);
        word(b, 0x4ee2b8, 0x6000);
        word(b, 0x6004, 0x7000); word(b, 0x6008, 0x7154);
        word(b, 0x7004, 2); word(b, 0x7008, 5);
        word(b, 0x70b8, 1); word(b, 0x7140, 0x8000);
        real(b, 0x8034, 3.833f); real(b, 0x5096c0, 1.f/60); real(b, 0x4312c4, 1);
        word(b, 512, 0xabcdef);
        normal_tick = false;
        animation_ticks = 0;
    };
    auto frame = [&] {
        X86Context ctx{}; ctx.esp = 512; ctx.cf = ctx.pf = 1;
        sub_00033B50(ctx, guest.data());
        assert(ctx.eax == 0x12345678 && ctx.esp == 516 && ctx.cf == 1 && ctx.pf == 1);
        uint32_t saved; std::memcpy(&saved, guest.data() + 512, 4);
        assert(saved == 0xabcdef);
    };
    setup(); frame(); assert(animation_ticks == 1);
    setup(); normal_tick = true; frame(); assert(animation_ticks == 0);
    for (uint32_t flags : {0x2000020u, 0x2100000u, 0x2400000u, 0u}) {
        setup(); word(guest.data(), 0x50ea9c, flags); frame(); assert(animation_ticks == 0);
    }
    setup(); word(guest.data(), 0x510ce8, 1); frame(); assert(animation_ticks == 0);
    setup(); word(guest.data(), 0x5064d8, 2); frame(); assert(animation_ticks == 0);
    setup(); word(guest.data(), 0x5a7d60, 0); frame(); assert(animation_ticks == 0);
    setup(); real(guest.data(), 0x5096c0, 0); frame(); assert(animation_ticks == 0);
    setup(); real(guest.data(), 0x5096c0, 1); frame(); assert(animation_ticks == 0);
}
