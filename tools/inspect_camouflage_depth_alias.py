#!/usr/bin/env python3
"""Read-only source evidence for camouflage target3 depth/color aliasing.

Reads an immutable original RAM export, executes unchanged whole XDK surface
format owner3FD860 using actual copied headers, verifies saved sampled-alias
pixels, and records pinned xemu source limitations. No graphics, guest I/O,
instruction patches, supplied game returns, or native activation.
"""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows
from disasm import listing
from differential_source_object_admission_snapshot import Snapshot, uint, words, PIN

OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-depth-alias-20261011'
SNAPSHOT = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry'
XEMU = ROOT / 'tools/reference/xemu-0.8.136'
COMMIT = 'fc24584ce88f0915ad7f04775bb7712c2e3f49ee'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def record(path):
    return dict(path=str(path.relative_to(ROOT)), bytes=path.stat().st_size, sha256=digest(path.read_bytes()))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    a = ap.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert digest(image.data) == PIN
    s = Snapshot(SNAPSHOT)
    descriptors = []
    headers = {}
    for name, global_at in (('main_color', 0x50935c), ('other_color', 0x509360), ('depth', 0x509364), ('target3_color_alias', 0x509370)):
        pointer = uint(s.read(global_at, 4))
        raw = s.read(pointer, 24)
        headers[name] = raw
        descriptors.append(dict(name=name, global_address=global_at, pointer=pointer, words=words(raw), header_hex=raw.hex()))
    assert uint(headers['target3_color_alias'], 4) == uint(headers['depth'], 4) == 0x03b58000
    assert headers['target3_color_alias'][12:24] == headers['main_color'][12:24]
    assert uint(headers['main_color'], 12) == uint(headers['target3_color_alias'], 12) == 0x11229
    assert uint(headers['depth'], 12) == 0x12e29

    rows = read_rows(FUNCTIONS_CSV)
    owners = []
    for name, entry in dict(surface_aliases=0x14600, target_bind=0x14bc0, surface_bind=0x1cd30,
                            xdk_full_bind=0x3f7870, xdk_equal_format_bind=0x3f7b30,
                            xdk_surface_format=0x3fd860, xdk_format_packet=0x3f6520, xdk_state_packet=0x3f5cf0,
                            layer_select=0x44370, stencil_control=0x15370).items():
        raw = image.read(entry, int(rows[entry]['size']))
        p = a.output / f'{name}-{entry:08x}.asm'
        p.write_text(f'{entry:08X} indexed_bytes={len(raw)} sha256={digest(raw)}\n' + '\n'.join(listing(raw, entry, rows)) + '\n')
        owners.append(dict(entry=entry, bytes_sha256=digest(raw), listing=p.name, listing_sha256=digest(p.read_bytes())))

    u = Uc(UC_ARCH_X86, UC_MODE_32)
    for at in (0x3fd000, 0x3f5000, 0x403000, 0x404000):
        u.mem_map(at, 4096)
        # Some XBE sections share a virtual page. Xbe.read intentionally
        # returns zero for ranges crossing sections, so copy each intersection.
        for section in image.sections:
            lo, hi = max(at, section.va), min(at + 4096, section.va + section.rsize)
            if lo < hi:
                u.mem_write(lo, image.section_bytes(section)[lo-section.va:hi-section.va])
    base, stack, stop = 0x20000000, 0x20010000, 0x2001f000
    u.mem_map(base, 0x20000)
    executed = {}
    def code(uc, pc, size, _):
        actual = bytes(uc.mem_read(pc, size))
        assert actual == image.read(pc, size), (hex(pc), size, actual.hex(), image.read(pc, size).hex())
        executed[pc] = actual.hex()
    u.hook_add(UC_HOOK_CODE, code)
    format_cases = []
    for color, depth in (('main_color', 'depth'), ('other_color', 'depth'), ('target3_color_alias', 'depth'),
                         ('main_color', None), ('target3_color_alias', None)):
        u.mem_write(base, headers[color])
        if depth:
            u.mem_write(base + 0x100, headers[depth])
        u.mem_write(stack, struct.pack('<I', stop))
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_ECX, base)
        u.reg_write(UC_X86_REG_ESI, base + 0x100 if depth else 0)
        u.emu_start(0x3fd860, stop, count=1000)
        result = u.reg_read(UC_X86_REG_EAX)
        assert result == 0x128
        assert bytes(u.mem_read(base, 24)) == headers[color]
        if depth:
            assert bytes(u.mem_read(base + 0x100, 24)) == headers[depth]
        format_cases.append(dict(color=color, depth=depth, returned_NV097_surface_format=result,
                                 color_format=result & 15, zeta_format=(result >> 4) & 15,
                                 layout_type=(result >> 8) & 15, headers_unchanged=True))

    authored_path = ROOT / 'analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json'
    authored = next(m for m in json.loads(authored_path.read_text())['maps'] if m['map'] == 'lockout')
    raw_states = authored['passes']['e7ef066a']['postprocess'][0]['implementations'][0]['render_states']
    packets = []
    for raw_hex in raw_states:
        raw_state = bytes.fromhex(raw_hex)
        state, value = raw_state[0], uint(raw_state, 1)
        packet = uint(image.read(0x43e6f0 + state * 4, 4))
        u.mem_write(0x404fe0, struct.pack('<II', base + 0x1000, base + 0x2000))
        u.mem_write(stack, struct.pack('<I', stop))
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_ECX, packet)
        u.reg_write(UC_X86_REG_EDX, value)
        u.emu_start(0x3f5cf0, stop, count=1000)
        emitted = words(bytes(u.mem_read(base + 0x1000, 8)))
        assert emitted == [packet, value] and uint(bytes(u.mem_read(0x404fe0, 4))) == base + 0x1008
        packets.append(dict(authored_state_hex=raw_hex, state=state, value=value,
                            source_packet_table_address=0x43e6f0 + state * 4, emitted_words=emitted,
                            boundary='Declared sufficient push buffer; unchanged whole3F5CF0; no queue/GPU submission.'))
    assert next(p for p in packets if p['state'] == 67)['emitted_words'] == [0x40358, 1]

    tree = json.loads((XEMU / 'tree.json').read_text())
    assert tree['sha'] == COMMIT and tree['truncated'] is False
    entries = {e['path']: e for e in tree['tree']}
    source_paths = ['hw/xbox/nv2a/nv2a_regs.h', 'hw/xbox/nv2a/pgraph/pgraph.c',
                    'hw/xbox/nv2a/pgraph/gl/constants.h', 'hw/xbox/nv2a/pgraph/gl/surface.c',
                    'hw/xbox/nv2a/pgraph/vk/constants.h', 'hw/xbox/nv2a/pgraph/vk/surface.c',
                    'hw/xbox/nv2a/pgraph/vk/surface-compute.c', 'hw/xbox/nv2a/pgraph/vk/texture.c']
    sources = []
    for name in source_paths:
        p = XEMU / name
        data = p.read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        assert blob == entries[name]['sha']
        sources.append(dict(**record(p), git_blob_sha1=blob, url=f'https://github.com/xemu-project/xemu/blob/{COMMIT}/{name}'))
    for backend in ('gl', 'vk'):
        text = (XEMU / f'hw/xbox/nv2a/pgraph/{backend}/surface.c').read_text()
        assert 'NV2A_UNIMPLEMENTED("Same color & zeta surface offset")' in text
    assert 'depth_stencil_out[idx_out] = depth_value << 8 | stencil_value;' in (XEMU / 'hw/xbox/nv2a/pgraph/vk/surface-compute.c').read_text()

    capture = ROOT / 'analysis/multiplayer-shading/weather-plate-original-reference/event-215778'
    raw = (capture / 'before-scene-depth.raw').read_bytes()
    after = (capture / 'after-scene-depth.raw').read_bytes()
    alias = (capture / 'stage-0-mip-0.rgba').read_bytes()
    assert len(raw) == len(alias) == 640 * 480 * 4 and raw == after
    expected = bytearray(len(raw))
    for i in range(0, len(raw), 4):
        expected[i:i+4] = bytes((raw[i+2], raw[i+1], raw[i], raw[i+3]))
    assert bytes(expected) == alias
    sampled = dict(event=215778, pixels=len(raw)//4, alias_rgba_from_raw_bytes=[2, 1, 0, 3],
                   raw_depth_shift=8, raw_stencil_mask=255, unique_stencil_bytes=sorted(set(raw[::4])),
                   depth_stencil_unchanged=True, files=[record(capture / n) for n in
                   ('before-scene-depth.raw', 'after-scene-depth.raw', 'stage-0-mip-0.rgba', 'surfaces.json', 'draw.json')])
    # Logical storage projection only: B is already a quantized output byte.
    # This is not an original GPU execution or a model of blend quantization.
    controls = 0
    for depth in (0, 1, 0x123456, 0xffffff):
        for stencil in range(256):
            word = (depth << 8) | stencil
            for quantized_blue in range(256):
                updated = (word & 0xffffff00) | quantized_blue
                assert updated >> 8 == depth and updated & 255 == quantized_blue
                controls += 1

    census = []
    # Saved metadata inventory only; raw RDCs are deliberately not replayed.
    meta_root = ROOT / 'analysis/multiplayer-shading/original-xemu'
    for p in sorted(meta_root.glob('*capture*.json')):
        j = json.loads(p.read_text())
        if isinstance(j, dict) and 'capture' in j:
            census.append(dict(**record(p), capture=j['capture'], actions=len(j.get('actions', [])),
                               resources=len(j.get('textures', [])), contains_named_camo_pass='active_camo' in p.read_text().lower()))
    report = dict(schema='original-camouflage-depth-color-alias-v1', boundary=__doc__,
                  tool=record(Path(__file__)), xbe_sha256=PIN, snapshot_manifest=record(SNAPSHOT / 'export.json'),
                  dependencies=[record(ROOT / p) for p in ('tools/differential_source_object_admission_snapshot.py',
                    'tools/reference/halo2-decompiled/tools/xbe.py', 'tools/reference/halo2-decompiled/tools/inventory.py',
                    'tools/reference/halo2-decompiled/tools/disasm.py')],
                  unicorn_version=importlib.metadata.version('unicorn'),
                  snapshot_ram_sha256=digest(s.ram), descriptors=descriptors, owners=owners,
                  whole_surface_format_cases=format_cases, executed_instructions=[dict(pc=k, hex=v) for k,v in sorted(executed.items())],
                  authored_camo_asset_manifest=record(authored_path), lockout_cache_sha256=authored['cache_sha256'], whole_state_packet_cases=packets,
                  xemu_commit=COMMIT, xemu_tree=record(XEMU / 'tree.json'), xemu_sources=sources,
                  original_sampled_alias=sampled, logical_storage_controls=controls,
                  saved_capture_metadata_census=census, instruction_patches=[], supplied_returns=[],
                  limits=['Target3 color header and depth header share actual captured Data03B58000; this snapshot is weather particle entry, not a camouflage draw.',
                          'Only complete original3FD860 and3F5CF0 execute here. Other original owner listings are static source evidence.',
                          'Blue-only mask1 selects little-endian color byte0, which is Z24S8 stencil byte0; logical controls assume an already quantized output byte.',
                          'Exact NV2A blend quantization, concurrent depth-EQUAL behavior and actual camouflage fragment/output pixels remain unproved.',
                          'Pinned xemu both backends explicitly unbind the opposite target for same color/zeta offset; this is a direct fidelity limitation.',
                          'Saved capture metadata census is not a shader/state census or exhaustive raw-RDC search; no identified camouflage draw capture is established.',
                          'Existing weather plate sampled-alias and original20 sky stencil captures are not camouflage layer18/19 execution proof.',
                          'No native renderer, CMake, progress, variant admission, GPU, guest or source capture is changed.'])
    p = a.output / 'report.json'
    p.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(report=str(p), sha256=digest(p.read_bytes()), original_format_cases=len(format_cases),
                          original_instructions=len(executed), state_packet_cases=len(packets), sampled_alias_pixels=sampled['pixels'],
                          logical_storage_controls=controls, capture_metadata=len(census)), indent=2))


if __name__ == '__main__':
    main()
