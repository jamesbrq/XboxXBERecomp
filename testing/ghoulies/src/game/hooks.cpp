#include "cpu/x86_ops.h"
#include <cstring>
#include <cmath>

#ifdef GHOULIES_WALKTEST
X86_EXTERN_FUNC(__imp__sub_005AB190);
X86_EXTERN_FUNC(__imp__sub_00033B50);
X86_EXTERN_FUNC(__imp__sub_0005AA90);

namespace {
uint32_t ReadWord(const uint8_t* base, uint32_t at) {
    uint32_t value;
    std::memcpy(&value, base + at, 4);
    return value;
}
float ReadFloat(const uint8_t* base, uint32_t at) {
    float value;
    std::memcpy(&value, base + at, 4);
    return value;
}
uint32_t WaitingDoorAnimation(const uint8_t* base) {
    const uint32_t flags = ReadWord(base, 0x50EA9C);
    if (ReadWord(base, 0x510CE8) || (flags & (0x20 | 0x100000 | 0x400000)) ||
        !(flags & 0x2000000) || ReadWord(base, 0x5A7CC0) != 2 ||
        !ReadWord(base, 0x5A7D60) || ReadWord(base, 0x5064D8) != 1)
        return 0;
    const uint32_t table = ReadWord(base, 0x4EE2B8);
    if (!table) return 0;
    const uint32_t begin = ReadWord(base, table + 4), end = ReadWord(base, table + 8);
    if (end < begin || end - begin > 0x154 * 256) return 0;
    const uint16_t id = uint16_t(ReadWord(base, 0x5064DC));
    for (uint32_t door = begin; door + 0x154 <= end; door += 0x154) {
        const uint32_t state = ReadWord(base, door + 4);
        if (uint16_t(state) != id || uint16_t(state >> 16) == 5 ||
            uint16_t(ReadWord(base, door + 8)) != 5 || !ReadWord(base, door + 0xB8))
            continue;
        return ReadWord(base, door + 0x140);
    }
    return 0;
}
}

// A challenge can own the door-camera freeze while the normal play update is
// suspended. Recover only a waiting animation that did not tick during this frame.
X86_FUNC(sub_00033B50) {
    const uint32_t animation = WaitingDoorAnimation(base);
    const float before = animation ? ReadFloat(base, animation + 0x2C) : 0;
    __imp__sub_00033B50(ctx, base);
    if (!animation || WaitingDoorAnimation(base) != animation ||
        ReadFloat(base, animation + 0x2C) != before)
        return;
    const float dt = ReadFloat(base, 0x5096C0);
    const float speed = ReadFloat(base, 0x4312C4);
    const float duration = ReadFloat(base, animation + 0x34);
    if (!std::isfinite(dt) || dt <= 0 || dt > 0.25f ||
        !std::isfinite(speed) || speed <= 0 || !std::isfinite(duration) || duration <= 0)
        return;
    // Run the original routine on a private register context and a normal guest
    // return slot. Its own completion path releases its camera/freeze counters.
    X86Context tick = ctx;
    tick.esp -= 4;
    const uint32_t saved = ReadWord(base, tick.esp), return_address = 0;
    std::memcpy(base + tick.esp, &return_address, 4);
    __imp__sub_0005AA90(tick, base);
    std::memcpy(base + ctx.esp - 4, &saved, 4);
}

// route_e1 precedes UpdateDoor's prologue: esp+4 is UpdateDoor's caller.
// Entrance script calls need remapping; event/mod calls use physical door ids.
X86_FUNC(sub_005AB190) {
    uint32_t caller;
    std::memcpy(&caller, base + ctx.esp + 4, sizeof(caller));
    const bool event = caller == 0xD0419 || caller == 0xD0984 ||
                       caller == 0xD0A6F || caller == 0xD0AD1;
    const bool mod = caller >= 0x572000 && caller < 0x5AD794;
    if (event || mod) {
        ctx.esp += 4;
        return;
    }
    __imp__sub_005AB190(ctx, base);
}
#endif
