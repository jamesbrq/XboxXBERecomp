#include "cpu/x86_ops.h"
#include <cstring>

#ifdef GHOULIES_WALKTEST
X86_EXTERN_FUNC(__imp__sub_005AB190);

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
