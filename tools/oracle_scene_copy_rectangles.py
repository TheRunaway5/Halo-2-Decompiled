#!/usr/bin/env python3
"""Whole retail352E0(NONE,1), captured resources and explicit rectangle inputs.

One captured baseline and33 declared in-bounds camera/raster contexts execute
the unchanged original owners. Only declared eight-byte rectangle fields vary.
No target/header/ready/device/program/capacity state or return is supplied.
Exact original CPU geometry, tap and viewport output is not GPU raster-center,
interpolation, sampling, command acceptance or framebuffer evidence.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_camouflage_scene_copy import CopyTrace, SNAPSHOT, RAM_PIN, ROOT, PIN, Xbe, Snapshot, read_rows, FUNCTIONS_CSV, uint, words, listing
from oracle_camouflage_pass_services import UC_X86_REG_EDX, UC_X86_REG_ECX

OUT = ROOT / 'analysis/multiplayer/game-variants/scene-copy-rectangles-20261011'
FIXTURE = ROOT / 'tests/fixtures/scene_copy_geometry_original.json'
PRIOR = ROOT / 'analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json'
PRIOR_PIN = 'e96e09b2c9a0e41e179e98d587c310b353583e59a74919eacdf315ecff6309f8'
STATIC = dict(world_camera_copy=0x2ba10, alternate_camera_copy=0x2c790,
              raster_descriptor_copy=0x132d0, viewport=0x3f8100,
              viewport_transform=0x3f6060, viewport_publication=0x3f6190,
              viewport_clip=0x3f8760, viewport_defaults=0x3fba30,
              fixed_function_program=0x3f6f40)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def inventory():
    full = [0, 0, 480, 640]
    rectangles = [('top_half', [0, 0, 240, 640]), ('bottom_half', [240, 0, 480, 640]),
                  ('left_half', [0, 0, 480, 320]), ('right_half', [0, 320, 480, 640]),
                  ('quarter_top_left', [0, 0, 240, 320]), ('quarter_top_right', [0, 320, 240, 640]),
                  ('quarter_bottom_left', [240, 0, 480, 320]), ('quarter_bottom_right', [240, 320, 480, 640]),
                  ('offset_inset', [20, 40, 300, 440]), ('offset_odd321x241', [13, 17, 254, 338]),
                  ('one_pixel_bottom_right', [479, 639, 480, 640])]
    result = [dict(name='captured_full', provenance='captured actual camera/raster fields',
                   camera_rectangle_shorts=full, raster_rectangle_shorts=full)]
    for role in ('camera_only', 'raster_only', 'paired'):
        for name, rect in rectangles:
            top, left, bottom, right = rect
            assert 0 <= top < bottom <= 480 and 0 <= left < right <= 640
            result.append(dict(name=f'{role}:{name}', provenance='declared caller rectangle input',
                               camera_rectangle_shorts=full if role == 'raster_only' else rect,
                               raster_rectangle_shorts=full if role == 'camera_only' else rect))
    return result


class RectangleTrace(CopyTrace):
    def code(self, g, pc, size, unused):
        completing = self.pending.get(pc, [None])[-1]
        super().code(g, pc, size, unused)
        if self.boundary:
            return
        device = self.u(0x407488)
        if completing:
            if completing['entry'] == 0x3f8100:
                completing['effective_viewport_words'] = words(self.read(device + 0xee0, 24))
                completing['device_viewport_offset_bits'] = words(self.read(device + 0xef8, 8))
            elif completing['entry'] == 0x3f6060:
                completing['offset_after_bits'] = words(self.read(completing['offset_pointer'], 16))
                completing['scale_after_bits'] = words(self.read(completing['scale_pointer'], 16))
        if pc == 0x3f6060:
            self.event(pc, offset_pointer=g.reg_read(UC_X86_REG_EDX),
                       scale_pointer=g.reg_read(UC_X86_REG_ECX),
                       effective_viewport_words=words(self.read(device + 0xee0, 24)),
                       resolution_scale_bits=words(self.read(device + 0x95c, 8)),
                       viewport_offset_bits=words(self.read(device + 0xef8, 8)),
                       depth_scale_bits=self.u(device + 0x948),
                       device_flags=self.u(device + 8), flag4076f8=self.u(0x4076f8))


def compact_result(result):
    attributes = [event for event in result['events'] if event['entry'] == 0x34100]
    vertices = []
    for offset in range(0, len(attributes), 16):
        group = attributes[offset:offset + 16]
        assert len(group) == 16 and [q['attribute'] for q in group] == list(range(15, -1, -1))
        assert len({tuple(q['corner_bits']) for q in group}) == 1
        vertices.append(dict(corner_bits=group[0]['corner_bits'],
                             attributes=[dict(index=q['attribute'], returned=bool(q['returned_eax'] & 255),
                                              value_bits=q['output_after_bits']) for q in reversed(group)]))
    quad = next(event for event in result['events'] if event['entry'] == 0x34420)
    program = next(event for event in result['events'] if event['entry'] == 0x3fa3d0)
    bindings = [event for event in result['events'] if event['entry'] == 0x14f60]
    assert len(bindings) == 4 and all(event['dimensions'] == [640, 480] for event in bindings)
    return dict(complete_original_return=result['complete_original_return'], stop_before=result['stop_before'],
                destination_rectangle_bits=words(bytes.fromhex(quad['destination_rectangle']['raw_hex'])),
                context_bits=words(bytes.fromhex(quad['context']['raw_hex'])),
                vertices=vertices, pixel_program_words=words(bytes.fromhex(program['program_payload_hex'])),
                source_dimensions=bindings[0]['dimensions'], destination_dimensions=[320, 240],
                near_bits=result['before']['depth_range_bits'][0], far_bits=result['before']['depth_range_bits'][1],
                viewports=[dict(input_words=event['viewport_words'], effective_words=event['effective_viewport_words'],
                                device_offset_bits=event['device_viewport_offset_bits'])
                           for event in result['events'] if event['entry'] == 0x3f8100],
                viewport_transforms=[{k: v for k, v in event.items() if k not in ('return_address', 'command', 'command_after', 'entry', 'returned_eax')}
                                     for event in result['events'] if event['entry'] == 0x3f6060])


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    ap.add_argument('--fixture', type=Path, default=FIXTURE)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    snapshot = Snapshot(SNAPSHOT)
    assert sha(snapshot.ram) == RAM_PIN and sha(PRIOR.read_bytes()) == PRIOR_PIN
    prior = json.loads(PRIOR.read_text())
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    rows = read_rows(FUNCTIONS_CSV)
    inputs, results, detailed, all_executed = inventory(), [], [], {}
    for cell in inputs:
        trace = RectangleTrace(snapshot, image, rows)
        writes = []
        for name, address in (('camera_rectangle_shorts', 0x4b9dd0), ('raster_rectangle_shorts', 0x485648)):
            raw = struct.pack('<4h', *cell[name])
            before = trace.read(address, 8)
            if raw != before:
                trace.put(address, raw)
                writes.append(dict(field=name, address=address, before_hex=before.hex(), declared_hex=raw.hex()))
        assert not writes if cell['name'] == 'captured_full' else writes
        # Resource descriptors and all genuine readiness/capacity inputs are
        # still read from the same captured pages; only the two declared fields
        # above were written before entering the original owner.
        result = trace.run_copy()
        assert result['before']['target19'] == prior['result']['before']['target19']
        assert result['command_begin'] == prior['result']['command_begin'] and result['command_limit'] == prior['result']['command_limit']
        assert result['before']['current_target'] == prior['result']['before']['current_target']
        for field in ('camera_rectangle_shorts', 'raster_rectangle_shorts'):
            assert result['before'][field] == cell[field] == result['after'][field]
        compact = compact_result(result) if result['complete_original_return'] else dict(complete_original_return=False, stop_before=result['stop_before'])
        results.append(compact)
        detailed.append(dict(input_name=cell['name'], declared_writes=writes, original=result))
        all_executed.update(trace.executed)
    baseline = results[0]
    for cell, result in zip(inputs, results):
        if cell['name'].startswith('raster_only:'):
            assert result == baseline
        if cell['name'].startswith('paired:'):
            camera_index = next(i for i, other in enumerate(inputs) if other['name'] == cell['name'].replace('paired:', 'camera_only:'))
            assert result == results[camera_index]
    fixture = dict(schema='original-scene-copy-geometry-v1', boundary=__doc__, inputs=inputs,
                   versions=dict(retail=dict(image_sha256=PIN, results=results)), instruction_patches=[], supplied_returns=[])
    args.fixture.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':')) + '\n')
    owners = []
    for name, address in STATIC.items():
        raw = image.read(address, int(rows[address]['size']))
        path = args.output / f'{name}-{address:08x}.asm'
        path.write_text(f'{address:08x} indexed_bytes={len(raw)} sha256={sha(raw)}\n' + '\n'.join(listing(raw, address, rows)) + '\n')
        owners.append(dict(owner=name, address=address, listing=path.name, listing_sha256=sha(path.read_bytes())))
    program = bytes.fromhex(prior['screen_vertex_program']['program_hex'])
    tokens = []
    for index in range(7):
        token = struct.unpack_from('<4I', program, 4 + index * 16)
        tokens.append(dict(index=index, words=list(token), mac=(token[1] >> 21) & 15,
                           input_register=(token[1] >> 9) & 15, input_mux=(token[2] >> 26) & 3,
                           output_register=(token[3] >> 3) & 15, output_mask=(token[3] >> 12) & 15,
                           output_ilu=bool(token[3] & 4), final=bool(token[3] & 1)))
    report = dict(schema='original-scene-copy-rectangles-report-v1', boundary=__doc__,
                  prior_report_sha256=PRIOR_PIN, fixture_sha256=sha(args.fixture.read_bytes()),
                  snapshot_ram_sha256=RAM_PIN, image_sha256=PIN, inputs=inputs,
                  complete_original_calls=sum(result['complete_original_return'] for result in results),
                  stopped_original_calls=sum(not result['complete_original_return'] for result in results),
                  cases=detailed, owners=owners, screen_vertex_program_tokens=tokens,
                  executed_original=[dict(address=pc, bytes_hex=image.read(pc, size).hex()) for pc, size in sorted(all_executed.items())],
                  dependencies={str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in (Path(__file__).resolve(), ROOT / 'tools/oracle_camouflage_scene_copy.py', ROOT / 'tools/oracle_camouflage_pass_services.py', ROOT / 'tools/differential_source_object_admission_snapshot.py')},
                  instruction_patches=[], supplied_returns=[],
                  limits=['One actual captured rectangle context; all33 other camera/raster rectangles are explicit manufactured caller inputs, not recovered live split-screen producers.',
                          'Only full-destination/camera-source mode1 flags from352E0 are executed, with genuine source640x480 and destination320x240 readiness. Other target sizes and flag combinations are outside this corpus.',
                          'Near1/64, far1024 and FP controls are retained from the baseline call. Other depth ranges, rounding modes and nonfinite arithmetic are not covered.',
                          '3F8100 and3F6060 outputs are actual CPU cache/packet preparation; compiled vertex words are static decoding only. No NV2A execution, raster centers, interpolation, texture samples or framebuffer parity.'])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, sort_keys=True, separators=(',', ':')) + '\n')
    print(json.dumps(dict(cases=len(results), complete=report['complete_original_calls'], stopped=report['stopped_original_calls'],
                          distinct_instructions=len(all_executed), fixture_sha256=report['fixture_sha256'], report_sha256=sha(path.read_bytes())), indent=2))


if __name__ == '__main__':
    main()
