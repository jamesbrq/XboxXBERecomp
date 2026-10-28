"""Create a full walktest ISO copy with the observed missing door transitions.

Uses the companion ghoulies-decomp BNL reader/writer. Never edits the source ISO.
All other file bytes, including default.xbe, are verified unchanged afterwards.
"""
import argparse
import copy
import hashlib
from pathlib import Path
import shutil
import struct
import sys
import tempfile


def entries(f):
    f.seek(32 * 2048)
    sector, size = struct.unpack_from('<II', f.read(2048), 20)
    out = {}

    def walk(sector, size, prefix=''):
        f.seek(sector * 2048)
        data = f.read(size)
        pending, seen = [0], set()
        while pending:
            at = pending.pop()
            if at in seen or at + 14 > len(data):
                continue
            seen.add(at)
            left, right, start, length, flags, n = struct.unpack_from('<HHIIBB', data, at)
            if left == right == 0xffff:
                continue
            name = data[at+14:at+14+n].decode('latin1')
            pending.extend(x * 4 for x in (left, right) if x)
            path = prefix + name
            if flags & 16:
                walk(start, length, path + '/')
            else:
                out[path.lower()] = (start * 2048, length, sector * 2048 + at + 4)
    walk(sector, size)
    return out


def read(f, entry):
    f.seek(entry[0])
    b = f.read(entry[1])
    assert len(b) == entry[1]
    return b


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('destination', type=Path)
    p.add_argument('--mod-tools', required=True, type=Path)
    a = p.parse_args()
    if a.destination.exists():
        p.error('destination must be a new file')
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(a.mod_tools))
    from bnl import BNL
    target = 'bundles/aid_script/ghoulies_ghmod_centralhallway.bnl'
    a.destination.parent.mkdir(parents=True, exist_ok=True)
    with a.source.open('rb') as src, tempfile.TemporaryDirectory(dir=a.destination.parent) as temp:
        original = entries(src)
        xbe = read(src, original['default.xbe'])
        if hashlib.sha1(xbe).hexdigest() != '7193b60bc75240bdc0e75e184bbbb22b56e8e397':
            p.error('source is not the selected full walktest build')
        bundle_path = Path(temp) / 'room.bnl'
        bundle_path.write_bytes(read(src, original[target]))
        bundle = BNL.read(bundle_path)
        added = []

        def alias(source, dest):
            if bundle.get(dest):
                return
            asset = bundle.get(source)
            if asset is None:
                raise ValueError('Missing fallback: ' + source)
            asset = copy.copy(asset)
            asset.name = dest
            bundle.assets.append(asset)
            added.append(dest)

        for dest, source in [('openevent', 'opennoentry'), ('eventopen', 'noentryopen'),
                             ('noentryevent', 'opennoentry')]:
            for kind, suffix in [('cutscene', ''), ('cutsceneevents', '_camera')]:
                prefix = f'aid_{kind}_ghoulies_doors_square_2_highleft_'
                alias(prefix + source + suffix, prefix + dest + suffix)
        # The fatal path also constructs a room-specific openevent camera name.
        camera = 'aid_cutscene_ghoulies_doors_cameras_centralhallway_1'
        alias(camera + 'opennoentry', camera + 'openevent')
        repaired_path = Path(temp) / 'repaired.bnl'
        bundle.write(repaired_path)
        repaired = repaired_path.read_bytes()
        assert all(BNL.read(repaired_path).get(n) is not None for n in added)
        a.destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(a.source, a.destination)
        with a.destination.open('r+b') as dst:
            dst.seek(0, 2)
            offset = (dst.tell() + 2047) // 2048 * 2048
            dst.seek(offset)
            dst.write(repaired)
            dst.write(b'\0' * (-len(repaired) % 2048))
            dst.seek(original[target][2])
            dst.write(struct.pack('<II', offset // 2048, len(repaired)))
            updated = entries(dst)
            assert updated.keys() == original.keys()
            for name, entry in original.items():
                expected = repaired if name == target else read(src, entry)
                assert read(dst, updated[name]) == expected, name
        print(f'Verified {len(original)} files; only {target} changed.')
        print('\n'.join(added))
        print(a.destination)


if __name__ == '__main__':
    main()
