#!/usr/bin/env python3
"""Preserve actual camouflage pass/target owners and execute bounded consumers.

Whole 1AE70 -> source scalar/color dispatch, 19BE0 and 19CA0 execute with
unchanged Lockout compiled pass metadata and declared finite object/view/fog
inputs. Whole texture-extern17 -> 14F60 -> 1DD60/1DCC0 executes against a
declared cold target19 row; no ready target or graphics success is supplied.
Complete indexed static owner ranges preserve scheduling/copy/allocation and
implementation-selection contracts. These are not executed GPU submissions.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path

from oracle_camouflage_overlay import CamouflageOverlay, HANDLE, sha
from oracle_mp_spawn import Cache
from oracle_multiplayer_hud_positions import PIN, STOP, ROOT, Xbe
from oracle_damage_feedback import bits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_XMM0
from disasm import listing
from inventory import read_rows
from xbe import FUNCTIONS_CSV

OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-pass-20261011'
OWNERS = dict(group_builder=0x420a0, group_callback=0x41cc0,
              transparent_queue=0x1e370, layer_select=0x44370,
              layer_draw=0x44550, layer_finish=0x445d0,
              scene_copy=0x352e0, copy_quad=0x34a90, quad_builder=0x34420,
              target_texture=0x14f60, target_bind=0x14bc0,
              target_dimensions=0x1dd60, target_descriptor=0x1dcc0,
              target_surface=0x1dcf0, surface_aliases=0x14600,
              surface_bind=0x1cd30,
              target_setup=0x1d770, target_record=0x1df20,
              target_backing=0x1db10, target_format=0x1d6f0,
              frame_target_setup=0x12560, target_clear=0x14b60,
              stencil_control=0x15370, texture_extern=0x19530,
              texture_bank=0x18a90, bitmap_bind=0x143c0,
              bitmap_cache=0x137550, implementation=0x4cbf0,
              initial_implementation=0x15d70, draw_callback=0x4d0b0,
              shader_dispatch=0x1bbf0, pass_binder=0x16b90,
              extern_bank=0x1ae70, vertex_extern=0x19be0,
              pixel_extern=0x19ca0, extern_value=0x1b2c0,
              scalar_dispatch=0x336f0, color_dispatch=0x33980,
              color_pack=0x131e50, local_scalar_set=0x139296,
              state_override=0x18900, state_extern=0x1a170,
              patchy_target_use=0x26880)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    a = ap.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    static = Xbe(ROOT / 'assets/disc/default.xbe')
    rows = read_rows(FUNCTIONS_CSV)
    listings = []
    for name, entry in OWNERS.items():
        row = rows[entry]
        data = static.read(entry, int(row['size']))
        p = a.output / f'{name}-{entry:08x}.asm'
        p.write_text(f'{entry:08x} indexed_bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}\n'
                     + '\n'.join(listing(data, entry, rows)) + '\n')
        listings.append(dict(owner=name, entry=entry, indexed_bytes=len(data),
                             bytes_sha256=hashlib.sha256(data).hexdigest(),
                             listing=p.name, listing_sha256=sha(p)))

    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    o = CamouflageOverlay(cache)
    o.watched = tuple(OWNERS.values()) + (0x33850, 0x33670, 0xbadc0, 0x3385d, 0x33edd, 0x33ef9)
    o.recording = True
    o.w(o.unit + 0x14, 0xffffffff)
    o.g.mem_write(o.unit + 0xaa, b'\0')
    pool = o.r(0x4e0300)
    o.w(pool + 0x24, 12)
    o.w(pool + 0x38, 1)
    o.w(0x485a58, o.model_owner)
    bank = o.alloc(bytes(0x1800))
    scratch = o.alloc(bytes(16))
    programs = []
    for layer, datum in ((18, 0xe7ec0667), (19, 0xe7ef066a)):
        post = o.r(o.table + (datum & 65535) * 16 + 8)
        assert o.r(post + 28) == 1
        post = o.r(post + 32)
        count, base = o.r(post), o.r(post + 4)
        assert count == (4 if layer == 18 else 1)
        for index in range(count):
            impl = base + index * 306
            mappings = {}
            for label, offset in (('pixel', 0x122), ('vertex', 0x124)):
                packed = struct.unpack('<H', o.g.mem_read(impl + offset, 2))[0]
                mappings[label] = [bytes(o.g.mem_read(o.r(post + 0x3c) + ((packed & 511) + n) * 4, 4)).hex()
                                   for n in range(packed >> 9)]
            programs.append(dict(layer=layer, datum=datum, index=index, post=post, impl=impl, mappings=mappings))

    cases = []
    primary = [bits(v) for v in (-.1, 0., .25, .75, 1., 1.1)] + [0x31c00000]
    fogs = [dict(datum=0xffffffff, near=10., far=30.),
            dict(datum=7, near=10., far=30.), dict(datum=7, near=30., far=10.)]
    for program, camo, fog, user in itertools.product(programs, primary, fogs, (-1, 0, 3, 4)):
        o.g.mem_write(bank, bytes(0x1800))
        o.w(bank + 0x20, program['post'])
        o.w(bank + 0x24, program['impl'])
        masks = [o.r(program['impl'] + off) for off in (0x10c, 0x110, 0x104, 0x108, 0x114)]
        o.g.mem_write(bank + 0x44, struct.pack('<5I', *masks))
        o.g.mem_write(bank + 0x30, struct.pack('<5I', *masks))
        initial_pixel = bytes(o.g.mem_read(program['impl'] + 6, 240))
        o.g.mem_write(bank + 0x1424, initial_pixel)
        initial_vertex = struct.pack('<128f', *([.125, .25, .5, 1.] * 32))
        o.g.mem_write(bank + 0x1530, initial_vertex)
        o.w(o.unit + 0x2b0, camo)
        o.w(0x4b9ed8, user)
        o.w(0x4b9f5c, fog['datum'])
        o.w(0x4b9f80, bits(fog['near']))
        o.w(0x4b9f84, bits(fog['far']))
        o.g.mem_write(0x4b9dac, struct.pack('<3f', 1., -.5, .25))
        for slot, value in enumerate((.125, .25, .5, .75)):
            o.run(0x139296, registers=[(UC_X86_REG_EAX, slot), (UC_X86_REG_XMM0, bits(value))])
        before = dict(o.entries)
        unit_before = bytes(o.g.mem_read(o.unit, 0x400))
        o.run(0x1ae70, registers=[(UC_X86_REG_EDI, bank)])
        scalar = {str(i): o.r(bank + 0x5c + i * 4) for i in (37, 39, 40)}
        color40 = list(struct.unpack('<3I', o.g.mem_read(bank + 0x108 + 40 * 12, 12)))
        color41 = list(struct.unpack('<3I', o.g.mem_read(bank + 0x108 + 41 * 12, 12)))
        if masks[1] & (1 << 5):
            amount = struct.unpack('<f', struct.pack('<I', camo))[0]
            assert scalar['37'] == bits(max(0., min(1., amount)))
        o.run(0x19be0, argv=[bank, program['impl'] + 0x124])
        o.run(0x19ca0, argv=[program['impl'] + 0x122], registers=[(UC_X86_REG_ESI, bank)])
        assert bytes(o.g.mem_read(o.unit, 0x400)) == unit_before
        pixel = bytes(o.g.mem_read(bank + 0x1424, 240))
        vertex = bytes(o.g.mem_read(bank + 0x1530, 512))
        cases.append(dict(layer=program['layer'], implementation=program['index'], primary_bits=camo, fog=fog, local_user=user,
                          usage_masks=masks, mappings=program['mappings'], scalar_bits=scalar, color40_bits=color40, color41_bits=color41,
                          initial_pixel_hex=initial_pixel.hex(), pixel_hex=pixel.hex(), initial_vertex_hex=initial_vertex.hex(), vertex_hex=vertex.hex(),
                          pixel_dirty_masks=[o.r(bank + 0x1518), o.r(bank + 0x151c)],
                          vertex_dirty_masks=[o.r(bank + 0x1730), o.r(bank + 0x1734)],
                          calls={f'{e:08x}': o.entries[e] - before.get(e, 0) for e in o.watched if o.entries[e] != before.get(e, 0)}))

    # The source checks the literal target row. Do not supply nonzero readiness
    # or a resident graphics object merely to make this branch succeed.
    target = 19
    target_row = 0x4b4b58 + target * 0x98
    o.g.mem_write(target_row, bytes(0x98))
    cold = []
    for stage in range(4):
        o.g.mem_write(scratch, b'\xa5' * 16)
        o.w(0x51f3d8 + stage * 4, 0xa5a5a5a5)
        before = dict(o.entries)
        o.run(0x19530, argv=[0, stage, scratch, scratch + 4], registers=[(UC_X86_REG_EAX, 17)])
        dims = [o.r(scratch), o.r(scratch + 4)]
        descriptor = o.r(0x51f3d8 + stage * 4)
        assert dims == [0, 0] and descriptor == 0
        cold.append(dict(stage=stage, dimensions=dims, descriptor=descriptor,
                         calls={f'{e:08x}': o.entries[e] - before.get(e, 0) for e in o.watched if o.entries[e] != before.get(e, 0)}))
    assert bytes(o.g.mem_read(target_row, 0x98)) == bytes(0x98)
    # Execute the original resolution prefix to the real cache binder, without
    # executing its first instruction or replacing its return value.
    prefixes = []
    context = o.alloc(bytes(16))
    for external, shader in itertools.product((1, 26), (0xe7e90664, 0xe9080783, 0xe914078f)):
        for stage in range(4):
            o.w(context + 8, shader)
            before = dict(o.entries)
            o.run(0x19530, stop=0x143c0, argv=[context, stage, scratch, scratch + 4], registers=[(UC_X86_REG_EAX, external)])
            bitmap_input = dict(datum=o.g.reg_read(UC_X86_REG_EAX), index=o.g.reg_read(UC_X86_REG_ECX) & 65535,
                                stage=o.r(o.g.reg_read(UC_X86_REG_ESP) + 4))
            bitmap = cache.tags[bitmap_input['datum']]
            shader_at = cache.tags[shader]['offset']
            post = cache.u(shader_at + 0x24) - cache.magic
            special = cache.u(post + 0x60) - cache.magic
            own_index = struct.unpack_from('<h', cache.data, special + 0x10)[0]
            prefixes.append(dict(external=external, shader=shader, shader_name=cache.tags[shader]['name'],
                                 shader_bump_index=own_index, input=bitmap_input, bitmap_name=bitmap['name'],
                                 stop_before=0x143c0, calls={f'{e:08x}': o.entries[e] - before.get(e, 0) for e in o.watched if o.entries[e] != before.get(e, 0)}))
    executed = [{'address': at, 'bytes_hex': o.image.read(at, n).hex()} for at, n in sorted(o.executed.items())]
    tables = dict(layer_rows=[dict(layer=n, address=0x43fa1c + n * 0x24,
                                   words=list(struct.unpack('<9I', o.image.read(0x43fa1c + n * 0x24, 36)))) for n in (18, 19)],
                  scalar_entries={str(n): struct.unpack('<I', o.image.read(0x338d4 + n * 4, 4))[0] for n in (37, 39, 40)},
                  color_entries={str(n): struct.unpack('<I', o.image.read(0x33f8c + n * 4, 4))[0] for n in (40, 41)},
                  state_extern_table_hex=o.image.read(0x467020, 64).hex(),
                  color_masks=list(struct.unpack('<8I', o.image.read(0x467068, 32))))
    dependencies = [Path(__file__), ROOT / 'tools/oracle_camouflage_overlay.py', ROOT / 'tools/oracle_shield_overlay.py',
                    ROOT / 'tools/oracle_damage_feedback.py', ROOT / 'tools/oracle_multiplayer_hud_positions.py',
                    ROOT / 'tools/oracle_camouflage_render_context.py', ROOT / 'assets/disc/maps/lockout.map']
    report = dict(schema='original-camouflage-pass-contract-v1', boundary=__doc__, xbe_sha256=PIN,
                  cases=cases, cold_texture17=cold, texture_resolution_prefixes=prefixes, tables=tables, owners=listings, executed_original=executed,
                  executed_owner_counts={f'{e:08x}': n for e, n in sorted(o.entries.items())},
                  dependencies={str(p.resolve().relative_to(ROOT)): sha(p) for p in dependencies},
                  limits=['Composition of whole extern consumers is not whole16B90 or submitted rendering.',
                          'Vertex constant baselines and local view/fog inputs are declared; no authored animation is executed.',
                          'Cold target19 failure is executed. Positive target/cache readiness and graphics APIs are not supplied or executed.',
                          'Texture extern1/26 resolution stops at bitmap binder entry143C0; its resource/cache services are not executed.',
                          'Copy/schedule/allocation/implementation paths are complete indexed static ranges, not successful runtime execution.',
                          'Finite inputs only; no nonfinite policy or framebuffer/refraction/stencil pixel parity is claimed.'])
    p = a.output / 'report.json'
    p.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=len(cases), cold_texture17=len(cold), texture_prefixes=len(prefixes), owners=len(listings), executed_instructions=len(executed), report_sha256=sha(p)), indent=2))


if __name__ == '__main__':
    main()
