#!/usr/bin/env python3
"""Whole original retail352E0(NONE,1) over immutable captured Xbox state.

Only the external call stack and observed FP-control setup are declared. Actual
current target/camera, target19, texture/program headers, device/cache banks and
command capacity stay captured inputs. Every reached original instruction is
byte checked; no instruction, readiness or function return is substituted.
Complete CPU execution/packet emission does not execute NV2A commands, establish
their acceptance, capture the resulting current frame or prove framebuffer parity.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_camouflage_pass_services import (Trace, Snapshot, SNAPSHOT, RAM_PIN,
    ROOT, PIN, Xbe, read_rows, FUNCTIONS_CSV, uint, words, packed, STACK, STOP,
    UcError, UC_X86_REG_ESP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW,
    UC_X86_REG_MXCSR, UC_X86_REG_FPTAG, UC_X86_REG_FPSW, UC_X86_REG_EAX,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_EBX, UC_X86_REG_EIP, listing)
from oracle_mp_spawn import Cache, CACHE_PIN

OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-scene-copy-20261011'
OWNERS = dict(scene_copy=0x352e0, copy_quad=0x34a90, quad_builder=0x34420,
              quad_attribute=0x34100, position_attribute=0x350e0,
              target_texture=0x14f60, target_dimensions=0x1dd60,
              target_select=0x14bc0, target_surface=0x1dcf0,
              surface_publication=0x1cd30, viewport=0x3f8100,
              copy_vertex_context=0x1c590, vertex_prepare=0x1c710,
              vertex_program_lookup=0x1c2e0, vertex_program_select=0x3f6c60,
              vertex_layout=0x1c330, vertex_sdk=0x3f7440,
              pixel_sdk=0x3fa3d0, texture_publication=0x1cf50,
              texture_sdk=0x3f8320, dirty_flush=0x400fa0)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def decode_packets(raw):
    data, result, at = words(raw), [], 0
    while at < len(data):
        header = data[at]
        count = (header >> 18) & 0x7ff
        # This decoder only describes normal generated method packets. The
        # complete raw command bytes remain evidence if another class appears.
        if header & 3 or header & 0xa0000000 or not count or at + 1 + count > len(data):
            result.append(dict(word_offset=at, unsupported_header=header,
                               remaining_words=data[at:]))
            break
        result.append(dict(word_offset=at, header=header, method=header & 0x1ffc,
                           subchannel=(header >> 13) & 7,
                           nonincrement=bool(header & 0x40000000),
                           values=data[at + 1:at + 1 + count]))
        at += 1 + count
    return result


class CopyTrace(Trace):
    def __init__(self, *args):
        super().__init__(*args)
        self.events, self.pending = [], {}

    def block(self, address, size):
        return dict(address=address, bytes=size, raw_hex=self.read(address, size).hex())

    def event(self, pc, **values):
        row = dict(entry=pc, command=self.u(self.u(0x407488)),
                   fpcw=self.g.reg_read(UC_X86_REG_FPCW),
                   mxcsr=self.g.reg_read(UC_X86_REG_MXCSR), **values)
        self.events.append(row)
        sp = self.g.reg_read(UC_X86_REG_ESP)
        row['return_address'] = self.u(sp)
        self.pending.setdefault(row['return_address'], []).append(row)
        return row

    def code(self, g, pc, size, unused):
        super().code(g, pc, size, unused)
        if self.boundary:
            return
        if pc in self.pending:
            row = self.pending[pc].pop()
            if not self.pending[pc]:
                del self.pending[pc]
            row.update(completed=True, returned_eax=g.reg_read(UC_X86_REG_EAX),
                       command_after=self.u(self.u(0x407488)))
            if 'output_pointer' in row:
                row['output_after_bits'] = words(self.read(row['output_pointer'], 16))
            if row['entry'] == 0x14f60:
                p = self.u(0x51f3d8 + row['stage'] * 4)
                row.update(selected_header=p, selected_header_hex=self.read(p, 20).hex() if p else '',
                           dimensions=[self.u(0x485af4 + row['stage'] * 4), self.u(0x485b04 + row['stage'] * 4)])
            if row['entry'] == 0x1c710:
                row['context_after_hex'] = self.read(row['context'], 0x2d8).hex()
        sp = g.reg_read(UC_X86_REG_ESP)
        if pc == 0x14f60:
            self.event(pc, source_target=g.reg_read(UC_X86_REG_EDI) & 65535,
                       stage=g.reg_read(UC_X86_REG_EBX) & 65535)
        elif pc == 0x34a90:
            self.event(pc, arguments=words(self.read(sp + 4, 44)))
        elif pc == 0x14bc0:
            self.event(pc, target=self.u(sp + 4), argument2=self.u(sp + 8),
                       al=g.reg_read(UC_X86_REG_EAX) & 255)
        elif pc == 0x1cd30:
            color, depth = g.reg_read(UC_X86_REG_ESI), g.reg_read(UC_X86_REG_EAX)
            self.event(pc, color_surface=color, depth_surface=depth,
                       color_surface_hex=self.read(color, 24).hex() if color else '',
                       depth_surface_hex=self.read(depth, 24).hex() if depth else '',
                       argument=self.u(sp + 4))
        elif pc == 0x3f8100:
            at = self.u(sp + 4)
            self.event(pc, viewport_pointer=at, viewport_words=words(self.read(at, 24)))
        elif pc == 0x34420:
            rect, attribute, finish, context = words(self.read(sp + 4, 16))
            self.event(pc, before_callback=g.reg_read(UC_X86_REG_EAX),
                       destination_rectangle=self.block(rect, 16),
                       attribute_callback=attribute, finish_callback=finish,
                       context=self.block(context, 0x68))
        elif pc == 0x34100:
            attribute, rect, corner, output, context = words(self.read(sp + 4, 20))
            self.event(pc, attribute=attribute, rectangle_bits=words(self.read(rect, 16)),
                       corner_bits=words(self.read(corner, 8)), output_pointer=output,
                       output_before_bits=words(self.read(output, 16)),
                       context_hex=self.read(context, 0x68).hex())
        elif pc == 0x350e0:
            rect, corner, output = (g.reg_read(r) for r in (UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ECX))
            self.event(pc, rectangle_bits=words(self.read(rect, 16)),
                       corner_bits=words(self.read(corner, 8)), depth_bits=self.u(sp + 4),
                       output_pointer=output, output_before_bits=words(self.read(output, 16)),
                       depth_range_bits=words(self.read(0x485ad4, 8)))
        elif pc == 0x1c590:
            self.event(pc, shader=g.reg_read(UC_X86_REG_EAX), index=self.u(sp + 4),
                       context=g.reg_read(UC_X86_REG_ESI))
        elif pc == 0x1c710:
            context = self.u(sp + 4)
            self.event(pc, context=context, context_before_hex=self.read(context, 0x2d8).hex())
        elif pc == 0x3f6c60:
            program, unused_argument = words(self.read(sp + 4, 8))
            count = int.from_bytes(self.read(program + 2, 2), 'little') if program else 0
            self.event(pc, program=program, argument2=unused_argument,
                       instruction_count=count,
                       program_bytes_hex=self.read(program, 4 + count * 16).hex() if program else '')
        elif pc == 0x3f7440:
            header, count, patches = words(self.read(sp + 4, 12))
            self.event(pc, program_argument=header, program_header=header & ~1,
                       program_header_hex=self.read(header & ~1, 0x114).hex() if header else '',
                       patch_count=count, patches_pointer=patches,
                       patches_hex=self.read(patches, count * 12).hex() if count else '')
        elif pc == 0x3fa3d0:
            header = g.reg_read(UC_X86_REG_EAX)
            self.event(pc, program_header=header, program_header_hex=self.read(header, 12).hex(),
                       program_payload=self.u(header + 8),
                       program_payload_hex=self.read(self.u(header + 8), 240).hex())

    def context(self):
        return dict(current_target=self.u(0x4858b8), scale_bits=self.u(0x4b8494),
                    camera_rectangle_shorts=list(struct.unpack('<4h', self.read(0x4b9dd0, 8))),
                    raster_rectangle_shorts=list(struct.unpack('<4h', self.read(0x485648, 8))),
                    depth_range_bits=words(self.read(0x485ad4, 8)),
                    camera_context=self.u(0x485a80),
                    camera_context_hex=self.read(self.u(0x485a80), 0x100).hex(),
                    frame_number_words=words(self.read(0x485aa0, 8)),
                    device=self.block(self.u(0x407488), 0x40),
                    target19=self.block(0x4b56a0, 0x98),
                    texture_stage_headers=words(self.read(0x51f3d8, 16)),
                    texture_dimensions=words(self.read(0x485af4, 32)),
                    texture_states=self.block(0x404dd0, 0x200),
                    pixel_payload=self.block(0x484f68, 240),
                    vertex_context=self.block(0x51f0f0, 0x2d8),
                    device_vertex_program=self.u(self.u(0x407488) + 0x794))

    def run_copy(self):
        before = self.context()
        device = self.u(0x407488)
        command, limit = self.u(device), self.u(device + 4)
        self.put(STACK, packed([STOP, 0xffffffff, 1]))
        for reg, value in ((UC_X86_REG_ESP, STACK), (UC_X86_REG_EFLAGS, 2),
                           (UC_X86_REG_FPCW, 0x023f), (UC_X86_REG_MXCSR, 0x1fa0),
                           (UC_X86_REG_FPTAG, 0xffff), (UC_X86_REG_FPSW, 0)):
            self.g.reg_write(reg, value)
        try:
            self.g.emu_start(0x352e0, STOP, count=2000000)
        except UcError:
            if not self.boundary:
                raise
        self.finish_writes()
        complete = not self.boundary and self.g.reg_read(UC_X86_REG_EIP) == STOP
        assert complete or self.boundary
        if complete:
            assert self.g.reg_read(UC_X86_REG_ESP) == STACK + 12
            assert all(row.get('completed') for row in self.events)
        length = self.u(device) - command
        assert 0 <= length <= limit - command
        raw = self.read(command, length)
        return dict(entry=0x352e0, arguments=[0xffffffff, 1],
                    before=before, after=self.context(), complete_original_return=complete,
                    stop_before=self.boundary, events=self.events,
                    original_owner_sequence=[f'{pc:08x}' for pc in self.sequence],
                    command_begin=command, command_limit=limit, command_bytes=len(raw),
                    command_hex=raw.hex(), command_sha256=sha(raw),
                    decoded_method_packets=decode_packets(raw),
                    source_writes=self.writes, mapped_original_pages=len(self.pages),
                    distinct_original_instructions=len(self.executed),
                    returned_fpcw=self.g.reg_read(UC_X86_REG_FPCW),
                    returned_mxcsr=self.g.reg_read(UC_X86_REG_MXCSR))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    snapshot = Snapshot(SNAPSHOT)
    assert sha(snapshot.ram) == RAM_PIN
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    rows = read_rows(FUNCTIONS_CSV)
    trace = CopyTrace(snapshot, image, rows)
    result = trace.run_copy()
    cache_path = ROOT / 'assets/disc/maps/lockout.map'
    assert sha(cache_path.read_bytes()) == CACHE_PIN
    cache = Cache(cache_path)
    screen = next(event['shader'] for event in result['events'] if event['entry'] == 0x1c590)
    tag_table = uint(snapshot.read(0x4e3b44, 4))
    tag_header = snapshot.read(tag_table + (screen & 65535) * 16, 16)
    tag_root = uint(tag_header, 8)
    program_record = uint(snapshot.read(tag_root + 8, 4))
    program_size = uint(snapshot.read(program_record + 12, 4))
    program_pointer = uint(snapshot.read(program_record + 16, 4))
    program_bytes = snapshot.read(program_pointer, program_size)
    screen_program = dict(datum=screen, name=cache.tags[screen]['name'],
                          tag_header_hex=tag_header.hex(), tag_root=tag_root,
                          tag_root_hex=snapshot.read(tag_root, 12).hex(),
                          program_record=program_record,
                          program_record_hex=snapshot.read(program_record, 28).hex(),
                          program_pointer=program_pointer, program_size=program_size,
                          program_hex=program_bytes.hex(), program_sha256=sha(program_bytes))
    assert any(event['entry'] == 0x3f6c60 and event['program'] == program_pointer and
               event['program_bytes_hex'] == program_bytes.hex() for event in result['events'])
    resources = []
    for event in result['events']:
        if event['entry'] == 0x14f60:
            pointer, width, height = event['selected_header'], *event['dimensions']
            header = bytes.fromhex(event['selected_header_hex'])
            physical = uint(header, 4) & 0x0fffffff
            size = width * height * 4
            assert physical + size <= len(snapshot.ram)
            resources.append(dict(role=f"source-stage{event['stage']}", header=pointer,
                                  header_hex=header.hex(), width=width, height=height,
                                  physical_backing=physical, physical_bytes=size,
                                  physical_captured_sha256=sha(snapshot.ram[physical:physical + size])))
    target_raw = bytes.fromhex(result['before']['target19']['raw_hex'])
    physical, width, height = uint(target_raw, 8), uint(target_raw, 0x80), uint(target_raw, 0x84)
    size = width * height * 4
    resources.append(dict(role='destination-target19', record=0x4b56a0, header=0x4b56a4,
                          header_hex=target_raw[4:24].hex(), width=width, height=height,
                          physical_backing=physical, physical_bytes=size,
                          physical_captured_sha256=sha(snapshot.ram[physical:physical + size])))
    owners = []
    for name, address in OWNERS.items():
        raw = image.read(address, int(rows[address]['size']))
        path = args.output / f'{name}-{address:08x}.asm'
        path.write_text(f'{address:08x} indexed_bytes={len(raw)} sha256={sha(raw)}\n' + '\n'.join(listing(raw, address, rows)) + '\n')
        owners.append(dict(owner=name, address=address, listing=path.name, listing_sha256=sha(path.read_bytes())))
    report = dict(schema='original-camouflage-scene-copy-v1', boundary=__doc__,
                  snapshot=str(SNAPSHOT.relative_to(ROOT)), snapshot_ram_sha256=RAM_PIN,
                  snapshot_manifest_sha256=sha((SNAPSHOT / 'export.json').read_bytes()),
                  captured_entry=0x391f0, source_xbe_sha256=PIN,
                  declarations=dict(stack=STACK, arguments=[0xffffffff, 1], fpcw=0x023f,
                                    mxcsr=0x1fa0, fpsw=0, fptag=0xffff),
                  result=result, resources=resources, screen_vertex_program=screen_program, owners=owners,
                  executed_original=[dict(address=pc, bytes_hex=image.read(pc, size).hex()) for pc, size in sorted(trace.executed.items())],
                  dependencies={str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in (Path(__file__).resolve(), ROOT / 'tools/oracle_camouflage_pass_services.py', ROOT / 'tools/differential_source_object_admission_snapshot.py', cache_path)},
                  instruction_patches=[], supplied_returns=[],
                  limits=['Fresh whole original call over the authentic captured weather-entry environment, not a historical camouflage-copy invocation.',
                          'Only captured current target0, full640x480 camera/raster rectangle and ready320x240 target19 are executed. Other viewport/target contexts need their own declared proof.',
                          'No GPU consumes the emitted commands in this oracle. Captured backing hashes identify actual input surfaces, not the resulting copied frame or command acceptance.',
                          'Retail captured environment only. Title-update scene-copy services, layer18/19 scheduling, framebuffer/depth alias and final camouflage draw remain separate.'])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(complete=result['complete_original_return'], boundary=result['stop_before'],
                          events=len(result['events']), instructions=len(trace.executed),
                          command_bytes=result['command_bytes'], report_sha256=sha(path.read_bytes())), indent=2))


if __name__ == '__main__':
    main()
