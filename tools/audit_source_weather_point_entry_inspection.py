#!/usr/bin/env python3
"""Inspect SDK point inputs at an immutable actual391F0 entry; execute nothing.

Captured entry banks and later source-inspected caller stores are kept separate.
No original service, readiness, shader result or draw outcome is supplied.
"""
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words
from xbe import Xbe
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

RAM_PIN = 'f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    directory = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry'
    snapshot = Snapshot(directory)
    assert sha(snapshot.ram) == RAM_PIN == snapshot.report['physical_ram_sha256']
    assert snapshot.report['registers_before'] == snapshot.report['registers_after']
    registers = snapshot.report['registers_before']
    assert int(re.search(r'EIP=([0-9a-fA-F]+)', registers)[1], 16) == 0x391f0
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    with (ROOT / 'tools/reference/halo2-decompiled/config/functions.csv').open() as stream:
        functions = {int(row['va'], 16): row for row in csv.DictReader(stream)}
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    regions = {}
    for address in (0x391f0, 0x16b10, 0x400fa0, 0x3ff920, 0x403400):
        length = int(functions[address]['size'])
        raw = image.read(address, length)
        assert snapshot.read(address, length) == raw
        code_length = 0x403961 - address if address == 0x403400 else length
        instructions = list(disassembler.disasm(raw[:code_length], address))
        assert sum(instruction.size for instruction in instructions) == code_length
        regions[f'{address:08X}'] = dict(bytes=length, code_bytes=code_length,
            sha256=sha(raw), statically_decoded_instructions=len(instructions),
            captured_code_equals_XBE=True,
            embedded_tail_data_and_padding_hex=raw[code_length:].hex())
    device = uint(snapshot.read(0x407488, 4))
    specifications = [('point_globals', 0x407668, 32), ('dirty_state', 0x407490, 4),
        ('device_header', device, 16), ('device_texcoord_guard', device + 0x784, 4),
        ('device_resolution_bank', device + 0x93c, 64), ('device_raster_viewport', device + 0xee0, 48),
        ('texture_stage_state', 0x404dd0, 512), ('retained_line_width', 0x40770c, 4),
        ('resolution_modes', 0x407700, 8), ('texcoord_extra_guard', 0x407634, 4),
        ('render_reset_header', 0x485b48, 40), ('render_reset_dirty_flag', 0x4670bc, 1)]
    banks = {}
    for label, address, length in specifications:
        raw = snapshot.read(address, length)
        banks[label] = dict(address=f'{address:08X}', bytes=length, raw_hex=raw.hex(), sha256=sha(raw))
        if length % 4 == 0:
            banks[label]['words'] = [f'{value:08X}' for value in words(raw)]
    names = ['size', 'minimum', 'sprite_enable', 'scale_enable', 'A', 'B', 'C', 'maximum']
    points = {}
    for index, name in enumerate(names):
        address = 0x407668 + index * 4
        raw = snapshot.read(address, 4)
        points[name] = dict(address=f'{address:08X}', word=f'{uint(raw):08X}',
            value=uint(raw) if name in ('sprite_enable', 'scale_enable') else struct.unpack('<f', raw)[0])
    dirty = uint(snapshot.read(0x407490, 4))
    source_stores = [
        ('0003928C', '00407678', '00000000', 'A'),
        ('00039294', '0040767C', '00000000', 'B'),
        ('000392AF', '00407490', 'current value OR00000900', 'dirty state'),
        ('000392B8', '00407670', '00000001', 'sprite enable'),
        ('000392BE', '00407674', '00000001', 'scale enable'),
        ('000392C4', '00407680', '3F800000', 'C')]
    prior_paths = ['analysis/multiplayer-shading/weather-12fa0-producer-inspection.json',
        'analysis/multiplayer-shading/weather-12fa0-producer-inspection.md',
        'analysis/multiplayer-shading/weather-12fa0-producer-inspection-disasm.txt',
        'tools/audit_source_weather_projection_inspection.py']
    report = dict(authority=__doc__, source_executable_sha256=PIN,
        snapshot=str(directory.relative_to(ROOT)), snapshot_ram_sha256=RAM_PIN,
        snapshot_manifest_sha256=sha((directory / 'export.json').read_bytes()),
        captured_PC='000391F0', original_execution_count=0, original_services_replaced=[],
        live_guest_or_GPU_access=False, shader_or_hardware_outcome_supplied=False,
        code_inspection=regions, device=f'{device:08X}', entry_banks=banks, entry_point_fields=points,
        entry_dirty=dict(word=f'{dirty:08X}', point_bit100=bool(dirty & 0x100),
            texture_coord_bit800=bool(dirty & 0x800),
            not_promoted_to_later_flush='Shader, pixel and bitmap/bind callees occur before400FA0 and may mutate dirty/device banks.'),
        entry_device=dict(resolution_x_scale=struct.unpack('<f', snapshot.read(device + 0x95c, 4))[0],
            resolution_y_scale=struct.unpack('<f', snapshot.read(device + 0x960, 4))[0],
            minimum_resolution_scale=struct.unpack('<f', snapshot.read(device + 0x964, 4))[0],
            unsigned_raster_height=uint(snapshot.read(device + 0xeec, 4)),
            texcoord_guard_pointer=f'{uint(snapshot.read(device + 0x784, 4)):08X}',
            flags=f'{uint(snapshot.read(device + 8, 4)):08X}',
            first_render_reset_nested_pointer=f'{uint(snapshot.read(0x485b48 + 0x24, 4)):08X}'),
        later_source_inspected_stores=dict(phase='Only applies after source resource admission and setup, for type!=2; these are not captured entry values.',
            ordered=[dict(pc=pc, address=address, value=value, field=field) for pc, address, value, field in source_stores],
            retained_fields=['size407668', 'minimum40766C', 'maximum407684'],
            source_flush_order=['400FB3 calls3FF920 when dirty bit100', '400FBF calls403400 when bit800'],
            final_dirty_mask='400FA0:401013..40101A stores updated local mask ANDC0000070',
            point_params='3FF920 enabled branch consumes size/min/max/A/B/C and unsigned device+EEC, emits eight200A30 parameter words; fixedPOINTSIZE4043C converts retained source size407668 via size*8+0.5, float32 store,3FCF00 CVTTSS2SI and unsigned cap511.',
            fixed_point_size_attribution='3FF9D0 retains source size below ratio;3FFA2B pops squared ratio times coefficientC and3FFA37 pops minimum-derived offset, so3FFA3A consumes original size. Captured size1 would encode8 after scale is enabled; this inspection executes no flush.',
            stage_selection='403400 selects startstage3 when407670!=0, startstage0 otherwise; device+784 non-NULL bypasses this routine.'),
        remaining_dependencies=['Actual source callee mutations between391F0 entry, point stores and400FA0 are owned by the parent complete caller attempt.',
            'Actual3FF920/403400 command allocation, capacity and downstream device services are not executed in this inspection.',
            'Point parameter commands and fixedPOINTSIZE do not prove NV2A programmableoPts consumption, coverage, clipping, precision or appearance.'],
        frozen_projection_dependencies={name: sha((ROOT / name).read_bytes()) for name in prior_paths},
        tool_sha256=sha(Path(__file__).read_bytes()))
    output = ROOT / 'analysis/multiplayer-shading/weather-point-particle-entry-inspection.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(report=str(output.relative_to(ROOT)), device=f'{device:08X}',
        point_fields=points, dirty=f'{dirty:08X}', raster_height=report['entry_device']['unsigned_raster_height'],
        original_execution_count=0), indent=2))


if __name__ == '__main__':
    main()
