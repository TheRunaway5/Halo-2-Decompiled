#!/usr/bin/env python3
"""Original grenade variant/default-pose/bounds with explicit raw ABI controls.

The body is the genuine released frag record. Plasma changes only the definition
datum as an explicitly declared direct-consumer control, not a plasma allocation
claim. Pool/tag locator backing is explicit; no game/service success is supplied.
"""
import argparse
import csv
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from inspect_multiplayer import Cache
from xbe import Xbe
from functions import _Code, _trace
from research_mp_original_data_u5 import norm
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import *

PRIOR = ROOT / 'analysis/multiplayer/integration/live-grenade-20261010/release-probe/report.json'
PRIOR_PIN = 'b459716943cc586b46be8b7fac796aea57349ff8efdb804a9c8f041890b2674d'
IMAGES = [
    (ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
     'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
     ROOT / 'analysis/map-pack-update/functions.tsv'),
    (Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'),
     '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
     ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')]
ENTRIES = (0xbb430, 0xbb480, 0xbb580, 0x16ec00, 0x16ed00, 0xbdf80,
           0x16f3c0, 0xbeb30, 0x143960, 0x1441d0, 0xbede0, 0xc0490)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    assert digest(PRIOR) == PRIOR_PIN
    prior = json.loads(PRIOR.read_text())
    original = next(o for o in prior['states'][183]['objects'] if o['datum'] == 'e2710002')
    body = bytes.fromhex(original['body_hex'])
    assert len(body) == 522 and struct.unpack_from('<I', body, 0x14)[0] == 0xffffffff
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    images = [Xbe(path) for path, _, _ in IMAGES]
    codes = [_Code(image, '.text') for image in images]
    starts = [{int(r['address'], 16) for r in csv.DictReader(path.open(), delimiter='\t')}
              for _, _, path in IMAGES]
    pairs = {}
    args.output.mkdir(parents=True)
    for entry in ENTRIES:
        old = [codes[0].at(a) for a in sorted(_trace(codes[0], entry, starts[0] | set(ENTRIES)).code)]
        pos = {i.address: n for n, i in enumerate(old)}
        tokens = [norm(i, images[0], pos) for i in old]
        candidates = raw_prefix_candidates(codes[0], images[0], codes[1], entry, 12)
        matches = []
        for candidate in candidates:
            try:
                new = [codes[1].at(a) for a in sorted(_trace(codes[1], candidate, starts[1]).code)]
                np = {i.address: n for n, i in enumerate(new)}
                if [norm(i, images[1], np) for i in new] == tokens:
                    matches.append((candidate, new))
            except Exception:
                pass
        assert len(matches) == 1, (hex(entry), [hex(a) for a, _ in matches])
        target, new = matches[0]
        pairs[entry] = dict(entry=target, instructions=len(old),
                            pair=[dict(old=a.address, new=b.address) for a, b in zip(old, new)])
        for label, ins in [('1.1', old), ('1.5', new)]:
            (args.output / f'{entry:08x}-{label}.asm').write_text(''.join(
                f'{i.address:08x} {bytes(i.bytes).hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
    rows = []
    # Authored grenades are selected by the current matg rows, not a path match.
    matg = next(t for t in cache.tags.values() if t['group'] == 'matg')
    definitions = [cache.u(at + 0x28) for at in cache.block(matg['offset'] + 0x100, 0x2c)]
    assert definitions[:2] == [0xe7a80623, 0xe7b2062d], definitions
    for image_index, (image, (_, pin, _)) in enumerate(zip(images, IMAGES)):
        assert digest(image.path) == pin
        guest = Uc(UC_ARCH_X86, UC_MODE_32)
        guest.mem_map(0x10000, 0x900000)
        for section in image.sections:
            guest.mem_write(section.va, image.section_bytes(section))
        page = cache.magic & ~4095
        guest.mem_map(page, ((len(cache.data) + cache.magic - page + 4095) // 4096) * 4096)
        guest.mem_write(cache.magic, bytes(cache.data))
        guest.mem_map(0x1000000, 0x200000)
        pool, headers, tags, storage, stack, stop = 0x1000000, 0x1001000, 0x1010000, 0x1110000, 0x11fe000, 0x11ff000
        def w(address, value):
            guest.mem_write(address, struct.pack('<I', value & 0xffffffff))
        w(0x4e78d0 if image_index == 0 else 0x4ecbd0, pool)
        w(pool + 0x44, headers)
        w(headers + 2 * 12 + 8, storage)
        guest.mem_write(headers + 2 * 12, bytes.fromhex(original['header_hex']))
        w(headers + 2 * 12 + 8, storage)
        w(0x4eb114 if image_index == 0 else 0x4f0414, tags)
        for datum, tag in cache.tags.items():
            if tag['offset'] is not None:
                guest.mem_write(tags + (datum & 65535) * 16, tag['group'].encode()[::-1])
                w(tags + (datum & 65535) * 16 + 8, tag['offset'] + cache.magic)
        stores, calls = [], []
        boundary = []
        bounds_unregistration = 0xb96a0 if image_index == 0 else 0 # set from the paired BEDE0 CALL operand below
        old_bounds = [codes[0].at(a) for a in sorted(_trace(codes[0],0xbede0,starts[0]).code)]
        new_bounds = old_bounds if image_index == 0 else [codes[1].at(pairs[0xbede0]['pair'][n]['new']) for n in range(len(old_bounds))]
        bounds_unregistration = next(int(b.op_str,16) for a,b in zip(old_bounds,new_bounds) if a.mnemonic=='call' and a.op_str=='0xb96a0')
        def write(uc, access, address, size, value, _):
            if storage <= address < storage + len(body):
                stores.append(dict(instruction=f'{uc.reg_read(UC_X86_REG_EIP):08x}',
                                   offset=address-storage, size=size,
                                   value=f'{value & ((1 << (size*8))-1):0{size*2}x}'))
        def code(uc, address, size, _):
            if address == bounds_unregistration:
                boundary.append(dict(kind='source-world-unregistration',entry=f'{address:08x}'))
                uc.emu_stop()
                return
            if address in (ENTRIES if image_index == 0 else [v['entry'] for v in pairs.values()]):
                calls.append(f'{address:08x}')
        guest.hook_add(UC_HOOK_MEM_WRITE, write)
        guest.hook_add(UC_HOOK_CODE, code)
        def invalid(uc, access, address, size, value, _):
            boundary.append(dict(kind='unmapped',access=access,address=f'{address:08x}',size=size,instruction=f'{uc.reg_read(UC_X86_REG_EIP):08x}'))
            return False
        guest.hook_add(UC_HOOK_MEM_INVALID, invalid)
        def run(entry, arguments=(), registers=()):
            actual = entry if image_index == 0 else pairs[entry]['entry']
            guest.mem_write(stack, struct.pack('<'+'I'*(1+len(arguments)), stop, *arguments))
            guest.reg_write(UC_X86_REG_ESP, stack)
            for reg, value in registers:
                guest.reg_write(reg, value)
            guest.reg_write(UC_X86_REG_FPCW, 0x27f)
            guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            try:
                guest.emu_start(actual, stop, count=1000000)
            except Exception:
                if not boundary: raise
            assert guest.reg_read(UC_X86_REG_EIP) == stop or boundary, (hex(guest.reg_read(UC_X86_REG_EIP)),boundary)
            if entry!=0xbede0: assert guest.reg_read(UC_X86_REG_EIP)==stop, boundary
            return guest.reg_read(UC_X86_REG_EIP)==stop
        for definition in definitions:
            before = bytearray(body)
            struct.pack_into('<I', before, 0, definition)
            for context in ('captured_release', 'translated_release'):
                if context == 'translated_release':
                    struct.pack_into('<3f', before, 0x64, 2., -3., 4.)
                guest.mem_write(storage, bytes(before))
                stores.clear(); calls.clear(); boundary.clear()
                run(0xbb430, registers=((UC_X86_REG_EAX, 0xe2710002), (UC_X86_REG_ECX, 0)))
                run(0xbb480, (0xe2710002, 0))
                run(0xbdf80, (0xe2710002,))
                pose_after = bytes(guest.mem_read(storage,len(body)))
                bounds_returned = run(0xbede0, (0xe2710002,))
                after = bytes(guest.mem_read(storage, len(body)))
                rows.append(dict(image=image_index, definition=f'{definition:08x}', context=context,
                                 before_hex=bytes(before).hex(), pose_after_hex=pose_after.hex(), after_hex=after.hex(), stores=stores.copy(), calls=calls.copy(), bounds_returned=bounds_returned,boundary=boundary.copy()))
        print('Original grenade render consumers', image_index, len(rows), flush=True)
    report = dict(schema='grenade-scene-state-v1', tool_sha256=digest(Path(__file__)), scope=__doc__,
                  prior=dict(path=str(PRIOR), sha256=PRIOR_PIN, state=183),
                  images=[dict(path=str(path), sha256=pin) for path, pin, _ in IMAGES], pairs=pairs,
                  map_sha256=digest(ROOT/'assets/disc/maps/lockout.map'), cases=rows)
    (args.output/'report.json').write_text(json.dumps(report, sort_keys=True, indent=2)+'\n')


if __name__ == '__main__':
    main()
