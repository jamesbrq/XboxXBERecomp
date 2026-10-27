"""Generate and execute a synthetic XBE; no game files are needed (Windows/Clang)."""
import argparse
import pathlib
import struct
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--compiler', required=True, type=pathlib.Path)
p.add_argument('--clang', default='clang++')
p.add_argument('--out', required=True, type=pathlib.Path)
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]
out = a.out.resolve()
out.mkdir(parents=True, exist_ok=True)
data = bytearray(0x1200)
def u32(at, value): struct.pack_into('<I', data, at, value)
data[:4] = b'XBEH'
for at, value in {0x104:0x10000, 0x108:0x1000, 0x10C:0x2000, 0x118:0x10200,
                  0x11C:1, 0x120:0x10300, 0x128:0x11000 ^ 0xA8FC57AB,
                  0x158:0x111F0 ^ 0x5B6D40B6}.items(): u32(at, value)
struct.pack_into('<6I', data, 0x300, 6, 0x11000, 0x200, 0x1000, 0x200, 0x10380)
data[0x380:0x386] = b'.text\0'
data[0x1000:] = b'\xCC' * 0x200
# CALL / indirect CALL / tail CALL return ZF, while a helper that does not
# alter flags must receive and preserve the caller's carry flag.
for at, code in {0x11000:'e8fb0000000f94c00fb6c0c3',
                 0x11040:'f9e8ca0000000f92c00fb6c0c3',
                 0x11080:'ba00110100ffd20f94c00fb6c0c3',
                 0x110C0:'e85b0000000f94c00fb6c0c3',
                 0x11100:'85c0c3', 0x11110:'b878563412c3',
                 0x11120:'e9dbffffff'}.items():
    raw = bytes.fromhex(code)
    data[0x1000 + at - 0x11000:0x1000 + at - 0x11000 + len(raw)] = raw
(out/'fixture.xbe').write_bytes(data)
(out/'fixture.toml').write_text('[xbe]\npath = "fixture.xbe"\nout = "generated"\nlibrary = "Fixture.dll"\npartitions = 1\ncode_sections = [".text"]\ngap_fill = false\npointer_seeds = false\n[functions]\n' + ''.join(f'0x{x:X} = ""\n' for x in [0x11040,0x11080,0x110C0,0x11100,0x11110,0x11120]))
subprocess.run([str(a.compiler.resolve()), 'fixture.toml'], cwd=out, check=True)
(out/'main.cpp').write_text(r'''
#include "cpu/x86_context.h"
#include <array>
#include <cassert>
extern "C" void sub_00011000(X86Context&, uint8_t*);
extern "C" void sub_00011040(X86Context&, uint8_t*);
extern "C" void sub_00011080(X86Context&, uint8_t*);
extern "C" void sub_000110C0(X86Context&, uint8_t*);
extern "C" void sub_00011100(X86Context&, uint8_t*);
void x86_icall(X86Context& c, uint8_t* b, uint32_t t) {
    assert(t == 0x11100); sub_00011100(c, b);
}

int main() {
    std::array<uint8_t, 0x30000> ram{};
    for (auto fn : {sub_00011000, sub_00011040, sub_00011080, sub_000110C0}) {
        X86Context c{}; c.esp = 0x2FF00;
        fn(c, ram.data());
        assert(c.eax == 1 && c.esp == 0x2FF04);
    }
}
''')
# Link the fixture statically; only remove the runtime import decoration.
headers = out/'include/cpu'
headers.mkdir(parents=True, exist_ok=True)
for name in ['x86_context.h', 'x86_ops.h']:
    source = (root/'runtime/cpu'/name).read_text().replace('#define X86_HOST_API __declspec(dllimport)', '#define X86_HOST_API')
    (headers/name).write_text(source)
subprocess.run([a.clang, '-std=c++20', '-DX86_GAME_CODE', '-I'+str(out/'include'),
                'main.cpp', 'generated/x86_recomp.0.cpp', '-o', 'call_flags.exe'], cwd=out, check=True)
subprocess.run([str(out/'call_flags.exe')], cwd=out, check=True)
print('PASS: direct, indirect, tail-call return flags and callee flag inputs.')
