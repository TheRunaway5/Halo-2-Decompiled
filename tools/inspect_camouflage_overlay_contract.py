#!/usr/bin/env python3
"""Preserve original camouflage overlay/FP-context owners and probe identity.

Whole 165E9F, 3D4F0, retained 3DDD0, 1BD50 and scalar-37 path execute with
declared backed FP records, matrices, object/header and retained cache banks.
The producer/geometry/draw chain between them is complete static evidence,
not successful execution of 165F16/4B5D0/133F70/2CBF0/4D9C0 or a GPU draw.
"""
import argparse
import itertools
import json
import struct
from pathlib import Path

from oracle_camouflage_overlay import CamouflageOverlay, OUT, HANDLE
from oracle_camouflage_render_context import Context, HudCache, ROOT, literal, sha
from oracle_mp_spawn import Cache
from oracle_damage_feedback import bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_XMM0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT / 'contract')
    a = ap.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    t = Context('retail', HudCache(ROOT / 'assets/disc/maps/lockout.map'))
    owners = dict(rasterizer_pointer=0x22590, geometry_selector=0x4bcc0,
                  geometry_builder=0x4b5d0, collector=0x133ab0, subpart_collector=0x133f70,
                  draw_allocator=0x2cbf0, fp_bank_init=0x3d2c0, fp_producer=0x165f16,
                  fp_record_builder=0x165e9f, model_identity=0x3d4f0, object_cache=0x3ddd0,
                  draw_binder=0x4d9c0, context_installer=0x1bd50, shader_dispatch=0x1bbf0,
                  fp_dispatcher=0x402b0, fp_draw_callback=0x4d0b0,
                  scalar37_shim=0x33850, scalar37_resolver=0x33670)
    listings = []
    for name, entry in owners.items():
        _, instructions = literal(t.code, entry, t.starts)
        p = a.output / f'{name}-{entry:08x}.asm'
        p.write_text('\n'.join(f'{i.address:08x}  {i.bytes.hex():<22} {i.mnemonic} {i.op_str}' for i in instructions) + '\n')
        listings.append(dict(owner=name, entry=entry, instructions=len(instructions), listing=p.name,
                             listing_sha256=sha(p), source=[dict(address=i.address, bytes_hex=i.bytes.hex(), mnemonic=i.mnemonic, operands=i.op_str) for i in instructions]))

    c = Cache(ROOT / 'assets/disc/maps/lockout.map')
    o = CamouflageOverlay(c)
    o.watched = (*o.watched, 0x165e9f, 0x3d4f0, 0x3ddd0, 0x1bd50, 0x336f0, 0x33850, 0x33670, 0xbadc0)
    weapon = next(x for x in c.tags.values() if x['group'] == 'weap' and 'covenant_carbine' in x['name'])
    fp_rows = c.u(weapon['offset'] + 0x2ac) - c.magic
    assert c.u(weapon['offset'] + 0x2a8) == 2
    fp_mode = c.u(fp_rows + 4)
    node_count = c.u(c.tags[fp_mode]['offset'] + 0x48)
    assert 0 < node_count <= 64
    identity = struct.pack('<13f', 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0)
    matrices = o.alloc(identity * node_count)
    output = o.alloc(bytes(24))
    cache_pool = o.alloc(bytes(0x48))
    cache_bank = o.alloc(bytes(0x200))
    o.w(cache_pool + 0x44, cache_bank)
    o.w(0x509434, cache_pool)
    pool = o.r(0x4e0300)
    o.w(pool + 0x24, 12)
    o.w(pool + 0x38, 2)
    o.g.mem_write(o.headers, struct.pack('<HBBHHIHBBHHI', 0x1234, 0, 2, 0xffff, 0x400, o.unit,
                                       0x1234, 0, 0, 0xffff, 0x400, o.parent))
    rows = [dict(owner_slot=owner, attached=attached, primary_bits=primary)
            for owner, attached, primary in itertools.product((0, 1), (False, True), (0, bits(.25), 0x31c00000))]
    results = []
    for row in rows:
        o.g.mem_write(o.unit, bytes(0x400))
        o.g.mem_write(o.parent, bytes(0x400))
        o.g.mem_write(o.unit + 0xaa, b'\x02')
        o.w(o.unit, int(weapon['datum'], 16))
        o.w(o.unit + 0x14, HANDLE + 1 if row['attached'] else 0xffffffff)
        o.w(o.parent + 0x14, 0xffffffff)
        o.w(o.parent + 0x2b0, row['primary_bits'])
        owner = HANDLE + row['owner_slot']
        body = o.unit if row['owner_slot'] == 0 else o.parent
        cache_handle = 0x56780000 + row['owner_slot']
        cache_entry = cache_bank + row['owner_slot'] * 0x100
        o.w(body + 0xcc, cache_handle)
        o.w(cache_entry + 4, owner)
        o.w(cache_entry + 0x44, owner)
        before_counts = dict(o.entries)
        o.recording = True
        o.run(0x165e9f, argv=[owner, 1, 0, 0, 0x4c1bd8],
              registers=[(UC_X86_REG_EAX, fp_mode), (UC_X86_REG_ECX, node_count), (UC_X86_REG_EDX, matrices)])
        bank_words = struct.unpack('<3I', o.g.mem_read(0x4c1bd8, 12))
        assert bank_words == (fp_mode, owner, 1)
        o.run(0x3d4f0, argv=[0, output, output + 4, output + 8, output + 12, output + 16],
              registers=[(UC_X86_REG_EAX, 1), (UC_X86_REG_EBX, HANDLE + 1)])
        outputs = struct.unpack('<5I', o.g.mem_read(output, 20))
        assert outputs == (1, owner, fp_mode, 0x4c1be4, node_count)
        cache_result = o.run(0x3ddd0, argv=[outputs[1]])
        assert cache_result == cache_handle
        o.w(0x485a58, 0)
        o.run(0x1bd50, registers=[(UC_X86_REG_EAX, cache_entry + 0x44)])
        assert o.r(0x485a58) == cache_entry + 0x44
        o.run(0x336f0, registers=[(UC_X86_REG_EAX, 37)])
        scalar = o.g.reg_read(UC_X86_REG_XMM0) & 0xffffffff
        expected = row['primary_bits'] if row['owner_slot'] == 1 or row['attached'] else 0
        assert scalar == expected
        o.recording = False
        results.append(dict(fp_bank_words=bank_words, identity_outputs=outputs, cache_result=cache_result,
                            installed_context_owner=o.r(o.r(0x485a58)), scalar37_bits=scalar,
                            entered={f'{at:08x}': o.entries[at] - before_counts.get(at, 0) for at in o.watched}))
    report = dict(schema='original-camouflage-overlay-context-contract-v1', boundary=__doc__,
                  image_sha256=t.spec['pin'], cache_sha256=sha(ROOT / 'assets/disc/maps/lockout.map'),
                  oracle_sha256=sha(__file__), dependencies={str(p.relative_to(ROOT)): sha(p) for p in
                      (ROOT / 'tools/oracle_camouflage_overlay.py', ROOT / 'tools/oracle_camouflage_render_context.py')},
                  complete_static_owners=listings, declared_fp_mode=fp_mode, declared_identity_node_count=node_count,
                  inputs=rows, results=results, instruction_patches=[], supplied_returns=[],
                  executed_instructions=[dict(address=at, bytes_hex=o.image.read(at, size).hex()) for at, size in sorted(o.executed.items())],
                  limits=['Whole helper composition uses declared identity matrices and retained cache records, not source animation/controller or natural cache allocation.',
                          'Actual 165F16 hand-weapon producer and full draw-context transport are complete static source contracts only.',
                          'No source part/resource acceptance, render target, framebuffer, stencil, refraction or GPU success is supplied or claimed.'])
    p = a.output / 'report.json'
    p.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=len(rows), static_owners=len(listings), report_sha256=sha(p)), indent=2))


if __name__ == '__main__':
    main()
