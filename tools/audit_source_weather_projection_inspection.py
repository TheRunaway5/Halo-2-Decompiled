#!/usr/bin/env python3
"""Static XBE/cache and immutable-RAM inspection of the weather projection seam.

No original instructions are executed, and no original service is replaced.
The retained banks are paused-state inputs, not an observed 391F0 call trace.
"""
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from audit_rendering import Cache
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words
from xbe import Xbe
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def shader_source(token, which):
    if which == 0:
        mux, register, negative = token[2] >> 26 & 3, token[2] >> 28, token[1] >> 8 & 1
        swizzle = [token[1] >> bit & 3 for bit in (6, 4, 2, 0)]
    elif which == 1:
        mux, register, negative = token[2] >> 11 & 3, token[2] >> 13 & 15, token[2] >> 25 & 1
        swizzle = [token[2] >> bit & 3 for bit in (23, 21, 19, 17)]
    else:
        mux = token[3] >> 28 & 3
        register, negative = (token[2] & 3) * 4 + (token[3] >> 30), token[2] >> 10 & 1
        swizzle = [token[2] >> bit & 3 for bit in (8, 6, 4, 2)]
    source = ['NONE', f'r{register}', f'v{token[1] >> 9 & 15}', f'c{token[1] >> 13 & 255}'][mux]
    return ('-' if negative else '') + source + '.' + ''.join('xyzw'[v] for v in swizzle)


def mask(value):
    return ''.join('xyzw'[index] for index in range(4) if value & (8 >> index))


def main():
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    with (ROOT / 'tools/reference/halo2-decompiled/config/functions.csv').open() as source:
        functions = {int(row[0], 16): row for row in csv.reader(source) if row and row[0] != 'va'}
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    regions, disassembly = {}, []
    addresses = [0x12d50, 0x12fa0, 0x132d0, 0x3f5730, 0x3fcf00,
                 0x3f6190, 0x3f6060, 0x3f7280, 0x3f7330, 0x3fac20,
                 0x3f6520, 0x3ff920, 0x403400]
    for address in addresses:
        raw = image.read(address, int(functions[address][1]))
        # 403400's inventory extent also contains a three-byte pad and its
        # embedded jump tables after RET8 at40395E. Do not count table bytes
        # as statically decoded source instructions.
        code_bytes = 0x403961 - address if address == 0x403400 else len(raw)
        instructions = list(disassembler.disasm(raw[:code_bytes], address))
        assert sum(instruction.size for instruction in instructions) == code_bytes
        regions[f'{address:08X}'] = dict(bytes=len(raw), sha256=sha(raw),
            code_bytes=code_bytes, embedded_tail_data_and_padding_hex=raw[code_bytes:].hex(),
            statically_decoded_instructions=len(instructions),
            direct_calls=[dict(pc=f'{instruction.address:08X}', target=instruction.op_str)
                          for instruction in instructions if instruction.mnemonic == 'call'])
        disassembly.append(f'0x{address:08x} size {len(raw)} sha256 {sha(raw)}\n')
        disassembly.extend(f'  {instruction.address:08x} {instruction.bytes.hex():24s} '
                           f'{instruction.mnemonic} {instruction.op_str}\n' for instruction in instructions)
        if code_bytes != len(raw):
            disassembly.append(f'  {address + code_bytes:08x} embedded tail data/padding '
                               f'({len(raw) - code_bytes} bytes): {raw[code_bytes:].hex()}\n')
    snapshot_path = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-state1'
    snapshot = Snapshot(snapshot_path)
    assert sha(snapshot.ram) == snapshot.report['physical_ram_sha256']
    device = uint(snapshot.read(0x407488, 4))
    bank_specs = [('camera', 0x485618, 0x74), ('projection', 0x48568c, 0xb8),
                  ('camera_far_divisor', 0x48565c, 4), ('depth_clip_planes', 0x485adc, 8),
                  ('render_mode', 0x4858b8, 4), ('device_resolution', device + 0x93c, 0x40),
                  ('device_raster_viewport', device + 0xee0, 0x30),
                  ('device_depth_clip', device + 0x1c08, 0x14),
                  ('point_parameters', 0x407668, 0x20), ('retained_line_width', 0x40770c, 4),
                  ('constant_shadow_c0_c11', 0x407738, 48 * 4),
                  ('constant_submission_state', 0x404fe0, 12)]
    banks = {}
    for name, address, length in bank_specs:
        raw = snapshot.read(address, length)
        banks[name] = dict(address=f'{address:08X}', bytes=length, sha256=sha(raw),
                           raw_hex=raw.hex(), words=[f'{v:08X}' for v in words(raw)])
    shaders = []
    for label, relative in [('lockout', 'assets/disc/maps/lockout.map'),
                            ('warlock', 'assets/map-pack/maps/warlock.map'),
                            ('backwash', 'assets/map-pack/maps/backwash.map')]:
        cache = Cache(ROOT / relative)
        try:
            global_tag = next(tag for tag in cache.tags.values() if tag['group'] == 'matg')
            _, globals_ = cache.tag(global_tag['datum'], 'matg')
            rasterizer = cache.block(globals_ + 0x108, 0x108)[0]
            programs = cache.block(rasterizer + 0x58, 8)
            for ordinal in (21, 22):
                datum = cache.u32(programs[ordinal] + 4)
                tag, root = cache.tag(datum, 'vrtx')
                cls = cache.block(root + 4, 28)[0]
                length = cache.u32(cls + 12)
                address = cache.pointer(cache.u32(cls + 16), cache.magic, length)
                raw = cache.data[address:address + length]
                header, count = struct.unpack_from('<HH', raw)
                assert header == 0x2078 and len(raw) == 4 + count * 16
                saved = ROOT / f'analysis/multiplayer-shading/weather-gpu-assets/{label}-global{ordinal}-class0.vp'
                assert saved.read_bytes() == raw
                tokens = []
                for index in range(count):
                    token = struct.unpack_from('<4I', raw, 4 + index * 16)
                    tokens.append(dict(index=index, words=[f'{v:08X}' for v in token],
                        mac=token[1] >> 21 & 15, ilu=token[1] >> 25 & 7,
                        operands=[shader_source(token, n) for n in range(3)],
                        temporary=token[3] >> 20 & 15, mac_mask=mask(token[3] >> 24 & 15),
                        ilu_mask=mask(token[3] >> 16 & 15), output=token[3] >> 3 & 15,
                        output_mask=mask(token[3] >> 12 & 15), output_ilu=bool(token[3] & 4)))
                shaders.append(dict(map=label, cache=relative, datum=f'{datum:08X}', name=tag['name'],
                    ordinal=ordinal, cls=0, bytes=len(raw), sha256=sha(raw), instructions=tokens))
        finally:
            cache.close()
    assert len({entry['sha256'] for entry in shaders if entry['ordinal'] == 21}) == 1
    assert len({entry['sha256'] for entry in shaders if entry['ordinal'] == 22}) == 1
    manifest_path = ROOT / 'analysis/multiplayer-shading/weather-vertex-validation.json'
    manifest = json.loads(manifest_path.read_text())
    manifest_hashes = {}
    for relative, recorded in manifest['hashes'].items():
        current = sha((ROOT / relative).read_bytes())
        manifest_hashes[relative] = dict(recorded_sha256=recorded, current_sha256=current,
                                       unchanged=current == recorded)
    geometry_rows = []
    for line in (ROOT / 'tests/fixtures/retail_source_weather_geometry.tsv').read_text().splitlines():
        if line and not line.startswith('#'):
            before, after = line.split('|')
            geometry_rows.append(([int(v, 16) for v in before.split()], [int(v, 16) for v in after.split()]))
    modes = {str(mode): sum(row[0][0] == mode for row in geometry_rows) for mode in range(4)}
    assert modes == {'0': 384, '1': 2, '2': 256, '3': 112}
    assert all(len(output) == 8 for inputs, output in geometry_rows if inputs[0] == 3)
    literal_constants = {}
    for address in (0x4407b8, 0x45df78, 0x45dbc0, 0x45de50, 0x45dd60,
                    0x45dbbc, 0x45dbc4, 0x45dc0c, 0x45e460, 0x45dd40,
                    0x45dc18, 0x45dc90):
        raw = image.read(address, 4)
        literal_constants[f'{address:08X}'] = dict(raw_hex=raw.hex(),
            word=f'{uint(raw):08X}', float=struct.unpack('<f', raw)[0])
    report = dict(authority='Literal original XBE instructions/cache payloads plus immutable paused RAM; static inspection only.',
        source_executable_sha256=PIN, executed_original_calls=0, replaced_original_services=0,
        inspected_regions=regions, literal_constants=literal_constants,
        inspected_snapshot=dict(path=str(snapshot_path.relative_to(ROOT)),
            export_sha256=sha((snapshot_path / 'export.json').read_bytes()),
            physical_ram_sha256=sha(snapshot.ram), device=f'{device:08X}', banks=banks),
        shaders=shaders, existing_fixture_review=dict(geometry_cases=len(geometry_rows),
            geometry_mode_counts=modes, viewport_compared_words=112 * 8,
            compared_viewport_registers=['c10', 'c11'], analytic_vertex_cases=448,
            analytic_authority='Synthetic independent arithmetic using authored five/nine instruction programs; not original GPU output.',
            manifest_current_hashes=manifest_hashes),
        sdk_point_dependency=dict(source='Complete original3FF920 and403400 inspected, never executed here.',
            inputs={'point_size': '407668', 'minimum': '40766C', 'maximum': '407684',
                'sprite_enable': '407670', 'scale_enable': '407674',
                'coefficients_A_B_C': ['407678', '40767C', '407680'],
                'device_resolution_scale': 'device+964', 'unsigned_raster_height': 'device+EEC'},
            nonrain_391f0_stores={'407670': '00000001', '407674': '00000001',
                '407678': '00000000', '40767C': '00000000', '407680': '3F800000'},
            nonrain_391f0_retains=['407668', '40766C', '407684'],
            enabled_params='3FF920 emits method200A30 with eight source point parameter words; final method4043C converts retained source point size407668, not the minimum or shader oPts.',
            enabled_fixed_point_size=dict(source='407668 retained on the x87 stack by3FF9D0',
                stack_trace=['3FF9D0 loads size below interval/(unsigned raster height*size)',
                    '3FFA11/3FFA13 squares ratio;3FFA2B pops squared ratio times coefficientC',
                    '3FFA2E loads minimum;3FFA37 pops negative minimum/interval, leaving original size'],
                conversion='3FFA3A..3FFA73: source size*8+0.5, float32 store,3FCF00 CVTTSS2SI, unsigned cap511'),
            texcoord_selection='403400:403433..403465 selects initial stage3 when407670!=0, otherwise stage0, from stage-state bank404DD0+stage*80; loop ends atstage4.',
            texcoord_early_return='403400:403404..403421 returns unchanged incoming command flags if device+784 is non-NULL.',
            hardware_closure=False),
        seam=dict(caller='391F0 at39267', projection_register='EAX=48568C', camera_register='ECX=485618',
            stack=['+4 scaled byte=0', '+8 mode DWORD=4858B8'], return_cleanup=8,
            required_projection_indices=list(range(1, 13)) + list(range(14, 23)) + list(range(30, 46)),
            camera_inputs={'position': ['+0', '+4', '+8'], 'signed_rectangle': ['+30 top', '+32 left', '+34 bottom', '+36 right']},
            common_scale='float32(16777215 / camera_far at48565C)',
            depth_clip_inputs=['485ADC', '485AE0'],
            publication='12 vectors to hardware constant indices0..11; optional shadow407738 controlled by404FE8 bit10',
            projection_12d50_abi=['EAX projection', 'BL scaled=0', 'stack+4 output pointer', 'RET4'],
            no_dynamic_proof='Neither full projection producer nor actual391F0 draw was executed in this inspection.'),
        unresolved_boundaries=['391F0 residency/device services in the existing geometry oracle are supplied hooks.',
            '12FA0 c0..c9 and SDK upload/depth-plane branches have static authority, not whole-original execution fixtures.',
            'No original NV2A RCP/RCC output, point/line raster coverage, hardware cap behavior, or final weather appearance is proved.'])
    disassembly_path = ROOT / 'analysis/multiplayer-shading/weather-12fa0-producer-inspection-disasm.txt'
    disassembly_path.write_text(''.join(disassembly))
    report['disassembly_sha256'] = sha(disassembly_path.read_bytes())
    report['inspection_tool_sha256'] = sha(Path(__file__).read_bytes())
    output = ROOT / 'analysis/multiplayer-shading/weather-12fa0-producer-inspection.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(report=str(output.relative_to(ROOT)), original_execution=0,
        code_regions=len(regions), cache_programs=len(shaders), retained_device=f'{device:08X}',
        viewport_fixture_cases=112, manifest_unchanged=all(v['unchanged'] for v in manifest_hashes.values())), indent=2))


if __name__ == '__main__':
    main()
