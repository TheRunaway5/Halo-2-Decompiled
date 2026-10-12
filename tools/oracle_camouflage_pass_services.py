#!/usr/bin/env python3
"""Unchanged16B90 over captured retail device, bitmap and target banks.

The immutable original Lockout weather-entry RAM is read through its CPU page
tables. Fresh material/owner records, selected original shader, finite primary
amount and optional fog controls are declared caller inputs. Existing cache,
ready bytes, program objects, target19 header/backing and command capacity are
never manufactured. The whole binder returns where those actual inputs permit;
other cases stop before12CE00, push-buffer capacity or external/MMIO services.
This is CPU pass preparation/packet emission, not an observed camouflage draw,
GPU acceptance, scene-copy history, geometry, or framebuffer/alias parity.
"""
import argparse
import collections
import hashlib
import itertools
import json
import re
import struct
from pathlib import Path

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed, fw
from oracle_mp_spawn import Cache, CACHE_PIN
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows
from disasm import listing

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-pass-services-20261011'
SNAPSHOT = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry'
RAM_PIN = 'f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd'
SYNTH, BANK, OWNER, STACK, STOP = 0x20000000, 0x20000000, 0x20002000, 0x200fe000, 0x200ff000
OWNER_DATUM, BODY = 0xe2820013, 0x82d8be40
OVERRIDE, MC, DEFAULT, EMBLEM = 0xe7e90664, 0xe9080783, 0xe7e90664, 0xe914078f
BOUNDARIES = {0x12ce00: 'Actual bitmap request owner; allocation/I/O/fallback continuation not supplied',
              0x3faae0: 'Actual push-buffer capacity/wait owner; device progress not supplied'}
STATIC = dict(pass_binder=0x16b90, bitmap_bind=0x143c0, bitmap_record=0x137550,
              bitmap_fast_lookup=0x12310, bitmap_lookup=0x12ccf0, bitmap_request=0x12ce00,
              texture_sdk_bind=0x3f8320, texture_publication=0x1cf50,
              pixel_program_bind=0x1b050, pixel_sdk_bind=0x3fa3d0,
              vertex_sdk_upload=0x3f7230, target_descriptor=0x1dcc0,
              target_dimensions=0x1dd60, target_texture=0x14f60,
              shader_dispatch=0x1bbf0, bank_reset=0x16b10, fog_program=0x3f6f40,
              texture_control_packet=0x3f5ff0, scalar_dispatch=0x336f0, color_dispatch=0x33980)
FOG_FP = {0x3385d, 0x3387a, 0x3388b, 0x3388f, 0x33896, 0x3389a, 0x3389e,
          0x33ef9, 0x33f12, 0x33f1f, 0x33f2b, 0x33f32, 0x33f39, 0x33f49,
          0x33f50, 0x33f61, 0x33f65, 0x33f6f, 0x33f6e}


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


class Trace:
    def __init__(self, snapshot, image, rows):
        self.s, self.image, self.rows = snapshot, image, rows
        self.g = Uc(UC_ARCH_X86, UC_MODE_32)
        self.g.mem_map(SYNTH, 0x100000)
        self.pages, self.executed, self.sequence, self.calls = {}, {}, [], []
        self.writes, self.boundary, self.pending_bitmap = [], None, None
        self.pending_writes = []
        self.state75_at_binder_entry = None
        self.fog_observations = []
        self.g.hook_add(UC_HOOK_MEM_UNMAPPED, self.missing)
        self.g.hook_add(UC_HOOK_CODE, self.code)
        self.g.hook_add(UC_HOOK_MEM_WRITE, self.write)
        ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', self.s.report['registers_before']).groups())
        self.read(ga, gl + 1)
        self.g.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
        for reg, value in ((UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16), (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)):
            self.g.reg_write(reg, value)

    def page(self, address):
        at = address & ~4095
        if at not in self.pages:
            raw = self.s.read(at, 4096)
            self.g.mem_map(at, 4096)
            self.g.mem_write(at, raw)
            self.pages[at] = raw

    def read(self, address, size):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            if not SYNTH <= at < SYNTH + 0x100000:
                self.page(at)
        return bytes(self.g.mem_read(address, size))

    def u(self, address):
        return uint(self.read(address, 4))

    def put(self, address, raw):
        self.read(address, len(raw))
        self.g.mem_write(address, raw)

    def missing(self, g, access, address, size, value, unused):
        try:
            self.read(address, size)
            return True
        except ValueError as error:
            self.boundary = dict(kind='Uncaptured/MMIO memory, not replaced', pc=g.reg_read(UC_X86_REG_EIP), address=address, reason=str(error))
            g.emu_stop()
            return False

    def code(self, g, pc, size, unused):
        self.finish_writes()
        if pc in BOUNDARIES:
            sp = g.reg_read(UC_X86_REG_ESP)
            self.boundary = dict(kind=BOUNDARIES[pc], pc=pc, eax=g.reg_read(UC_X86_REG_EAX), arguments=words(self.read(sp + 4, 20)))
            if pc == 0x12ce00:
                record = self.u(sp + 4)
                self.boundary.update(record=record, record_hex=self.read(record, 116).hex(), request_bits=g.reg_read(UC_X86_REG_XMM1) & 0xffffffff)
            g.emu_stop()
            return
        try:
            original = self.image.read(pc, size)
        except (ValueError, KeyError):
            self.boundary = dict(kind='External instruction service, not executed', pc=pc)
            g.emu_stop()
            return
        assert self.read(pc, size) == original, f'Changed original instruction{pc:08X}'
        self.executed[pc] = size
        if pc == 0x16b90:
            self.state75_at_binder_entry = self.u(0x407498 + 75 * 4)
        if pc in FOG_FP:
            self.fog_observations.append(dict(pc=pc, fpcw=g.reg_read(UC_X86_REG_FPCW), mxcsr=g.reg_read(UC_X86_REG_MXCSR),
                                             xmm_low_bits=[g.reg_read(reg) & 0xffffffff for reg in (UC_X86_REG_XMM0, UC_X86_REG_XMM1, UC_X86_REG_XMM2, UC_X86_REG_XMM3)]))
        if pc in self.rows:
            self.sequence.append(pc)
        if pc in (0x143c0, 0x12310, 0x3fa3d0, 0x3f8320):
            sp = g.reg_read(UC_X86_REG_ESP)
            row = dict(entry=pc, eax=g.reg_read(UC_X86_REG_EAX), ecx=g.reg_read(UC_X86_REG_ECX), stack12_hex=self.read(sp + 4, 12).hex())
            if pc == 0x143c0:
                row.update(bitmap=g.reg_read(UC_X86_REG_EAX), index=g.reg_read(UC_X86_REG_ECX) & 65535, stage=self.u(sp + 4), request_bits=g.reg_read(UC_X86_REG_XMM2) & 0xffffffff)
                self.pending_bitmap = row
            elif pc == 0x12310:
                at = row['eax']
                row.update(record_hex=self.read(at, 116).hex(), cache_clock=self.u(0x4e6488))
            elif pc == 0x3fa3d0:
                at = row['eax']
                row.update(program_header_hex=self.read(at, 12).hex(), program_payload_hex=self.read(self.u(at + 8), 240).hex())
            else:
                at = self.u(sp + 4)
                row.update(stage=row['eax'], texture_header=at, texture_header_hex=self.read(at, 20).hex() if at else '')
            self.calls.append(row)
        if pc == 0x1446b and self.pending_bitmap:
            row = self.pending_bitmap
            stage = row['stage']
            header = self.u(0x51f3d8 + stage * 4)
            row.update(completed=True, selected_header=header, selected_header_hex=self.read(header, 20).hex() if header else '', dimensions=words(self.read(0x485af4 + stage * 4, 4) + self.read(0x485b04 + stage * 4, 4)))
            self.pending_bitmap = None

    def write(self, g, access, address, size, value, unused):
        if SYNTH <= address < SYNTH + 0x100000:
            return
        row = dict(pc=g.reg_read(UC_X86_REG_EIP), address=address, size=size,
                   before_hex=self.read(address, size).hex())
        self.writes.append(row)
        self.pending_writes.append(row)

    def finish_writes(self):
        for row in self.pending_writes:
            row['after_hex'] = self.read(row['address'], row['size']).hex()
        self.pending_writes.clear()

    def run(self, layer, implementation, shader, primary, fog=False, dispatch=False):
        bank = 0x485b48 if dispatch else BANK
        self.put(bank + 8, packed([shader]))
        self.put(bank + 0x30, packed([0xffffffff] * 5))
        self.put(bank + 0x1530, struct.pack('<128f', *([.125, .25, .5, 1.] * 32)))
        self.put(OWNER, packed([OWNER_DATUM]))
        self.put(0x485a58, packed([OWNER]))
        self.put(BODY + 0x2b0, packed([primary]))
        self.put(0x4b9ed8, packed([0xffffffff]))
        if fog:
            self.put(0x4b9f5c, packed([7]))
            self.put(0x4b9f80, packed([fw(10.)]))
            self.put(0x4b9f84, packed([fw(30.)]))
            self.put(0x4b9dac, struct.pack('<3f', 1., -.5, .25))
        initial = self.read(bank, 0x1800)
        fog_initial = self.fog_context()
        device = self.u(0x407488)
        command = self.u(device)
        limit = self.u(device + 4)
        initial_state75 = self.u(0x407498 + 75 * 4)
        arguments = [0, layer, 0, implementation, fw(1.)] if dispatch else [bank, OVERRIDE, 0, layer, 0, implementation, fw(1.)]
        self.put(STACK, packed([STOP, *arguments]))
        self.g.reg_write(UC_X86_REG_EDI, OVERRIDE if dispatch else 0)
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        self.g.reg_write(UC_X86_REG_EFLAGS, 2)
        self.g.reg_write(UC_X86_REG_FPCW, 0x023f)
        self.g.reg_write(UC_X86_REG_MXCSR, 0x1fa0)
        self.g.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.g.reg_write(UC_X86_REG_FPSW, 0)
        try:
            self.g.emu_start(0x1bbf0 if dispatch else 0x16b90, STOP, count=2000000)
        except UcError:
            if not self.boundary:
                raise
        self.finish_writes()
        complete = not self.boundary and self.g.reg_read(UC_X86_REG_EIP) == STOP
        assert complete or self.boundary
        if complete:
            assert self.g.reg_read(UC_X86_REG_ESP) == STACK + (24 if dispatch else 32)
            assert 0x3fa3d0 in self.sequence
        length = self.u(device) - command
        assert 0 <= length <= limit - command
        headers = words(self.read(0x51f3d8, 16))
        return dict(entry=0x1bbf0 if dispatch else 0x16b90, bank=bank, layer=layer, implementation=implementation, bank_shader=shader, primary_bits=primary,
                    fog_control='declared7/near10/far30/forward(1,-.5,.25)' if fog else 'retained NONE',
                    complete_original_return=complete, stop_before=self.boundary,
                    arguments=arguments,
                    initial_bank_hex=initial.hex(), final_bank_hex=self.read(bank, 0x1800).hex(),
                    owner_sequence=[f'{pc:08x}' for pc in self.sequence], resource_calls=self.calls,
                    command_begin=command, command_limit=limit, command_hex=self.read(command, length).hex(),
                    initial_state75=initial_state75, state75_at_binder_entry=self.state75_at_binder_entry,
                    state75=self.u(0x407498 + 75 * 4), scalar37_bits=self.u(bank + 0x5c + 37 * 4),
                    color40_bits=words(self.read(bank + 0x108 + 40 * 12, 12)),
                    scalar39_bits=self.u(bank + 0x5c + 39 * 4), color41_bits=words(self.read(bank + 0x108 + 41 * 12, 12)),
                    fog_context_before=fog_initial, fog_context_after=self.fog_context(), fog_fp_observations=self.fog_observations,
                    stage_headers=[dict(pointer=p, words=words(self.read(p, 20)) if p else []) for p in headers],
                    stage_dimensions=words(self.read(0x485af4, 32)),
                    verified_instructions=len(self.executed), mapped_original_pages=len(self.pages), source_writes=self.writes,
                    returned_fpcw=self.g.reg_read(UC_X86_REG_FPCW), returned_mxcsr=self.g.reg_read(UC_X86_REG_MXCSR))

    def fog_context(self):
        pointer = self.u(0x4686d4)
        return dict(datum=self.u(0x4b9f5c), near_bits=self.u(0x4b9f80), far_bits=self.u(0x4b9f84),
                    forward_bits=words(self.read(0x4b9dac, 12)), default_pointer=pointer,
                    default_rgb_hex=self.read(pointer + 4, 12).hex())


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    s = Snapshot(SNAPSHOT)
    assert sha(s.ram) == RAM_PIN
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    assert sha((ROOT / 'assets/disc/maps/lockout.map').read_bytes()) == CACHE_PIN
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    rows = read_rows(FUNCTIONS_CSV)
    table = uint(s.read(0x4e3b44, 4))
    pool = uint(s.read(0x4e0300, 4))
    object_header = uint(s.read(pool + 0x44, 4)) + (OWNER_DATUM & 65535) * 12
    raw_header = s.read(object_header, 12)
    assert uint(raw_header, 8) == BODY and int.from_bytes(raw_header[:2], 'little') == OWNER_DATUM >> 16
    assert s.read(BODY + 0xaa, 1) == b'\0' and uint(s.read(BODY + 0x14, 4)) == 0xffffffff
    device = uint(s.read(0x407488, 4))
    target = 0x4b4b58 + 19 * 0x98
    target_raw = s.read(target, 0x98)
    assert uint(target_raw, 0x88) and not target_raw[0x95]
    inputs = dict(snapshot=str(SNAPSHOT.relative_to(ROOT)), snapshot_ram_sha256=RAM_PIN,
                  snapshot_manifest_sha256=sha((SNAPSHOT / 'export.json').read_bytes()), captured_entry=0x391f0,
                  source_xbe_sha256=PIN, lockout_sha256=CACHE_PIN,
                  object=dict(datum=OWNER_DATUM, header=object_header, header_hex=raw_header.hex(), body=BODY,
                              body_hex=s.read(BODY, 0x300).hex()),
                  device=dict(pointer=device, header_hex=s.read(device, 0x40).hex(),
                              existing_vertex_program=uint(s.read(device + 0x794, 4)),
                              existing_vertex_program_hex=s.read(uint(s.read(device + 0x794, 4)), 32).hex()),
                  target19=dict(record=target, raw_hex=target_raw.hex(), texture_header=target + 4,
                                physical_backing=uint(target_raw, 8), physical_320x240x4_sha256=sha(s.ram[uint(target_raw, 8):uint(target_raw, 8) + 320 * 240 * 4])),
                  declarations=dict(fresh_bank=BANK, fresh_owner=OWNER, owner_datum=OWNER_DATUM, metric_bits=fw(1.),
                                    initial_extern_dirty=[0xffffffff] * 5, initial_vertex_vector=[.125, .25, .5, 1.],
                                    local_user=0xffffffff, fpcw=0x023f, mxcsr=0x1fa0, fpsw=0, fptag=0xffff))
    cases, all_executed = [], {}
    amounts = [fw(-.1), 0, fw(.25), 0x31c00000]
    for (layer, impl), shader, amount in itertools.product(((18, 0), (18, 1), (18, 2), (18, 3), (19, 0)), (MC, DEFAULT, EMBLEM), amounts):
        t = Trace(s, image, rows)
        result = t.run(layer, impl, shader, amount)
        for call in result['resource_calls']:
            if 'bitmap' in call:
                call['bitmap_name'] = cache.tags[call['bitmap']]['name']
        cases.append(result)
        all_executed.update(t.executed)
    for amount in amounts:
        t = Trace(s, image, rows)
        cases.append(t.run(19, 0, MC, amount, True))
        all_executed.update(t.executed)
    for (layer, impl), shader in itertools.product(((18, 0), (18, 1), (18, 2), (18, 3), (19, 0)), (MC, DEFAULT, EMBLEM)):
        t = Trace(s, image, rows)
        result = t.run(layer, impl, shader, fw(.25), dispatch=True)
        for call in result['resource_calls']:
            if 'bitmap' in call:
                call['bitmap_name'] = cache.tags[call['bitmap']]['name']
        assert result['initial_state75'] == result['state75_at_binder_entry']
        cases.append(result)
        all_executed.update(t.executed)
    bitmaps = {}
    for case in cases:
        for call in case['resource_calls']:
            if call['entry'] != 0x143c0:
                continue
            datum, index = call['bitmap'], call['index']
            key = f'{datum:08x}:{index}'
            if key in bitmaps:
                continue
            root = uint(s.read(table + (datum & 65535) * 16 + 8, 4))
            record = uint(s.read(root + 0x48, 4)) + index * 116
            raw = s.read(record, 116)
            image_pool = uint(s.read(0x4e6454, 4))
            image_rows = uint(s.read(image_pool + 0x44, 4))
            partitions = []
            for n in range(3):
                handle = uint(raw, 0x28 + n * 4)
                row = dict(partition=n, handle=handle, authored_resource_pointer=uint(raw, 0x1c + n * 4), authored_resource_bytes=uint(raw, 0x34 + n * 4))
                if handle != 0xffffffff:
                    p = image_rows + (handle & 65535) * 40
                    header = s.read(p + 20, 20)
                    data = uint(header, 4) & 0x0fffffff
                    count = row['authored_resource_bytes']
                    resource_file = ROOT / 'assets/disc/maps' / (('lockout.map', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[row['authored_resource_pointer'] >> 30])
                    with resource_file.open('rb') as stream:
                        stream.seek(row['authored_resource_pointer'] & 0x3fffffff)
                        authored = stream.read(count)
                    assert len(authored) == count and data + count <= len(s.ram)
                    physical = s.ram[data:data + count]
                    row.update(image_row=p, image_row_hex=s.read(p, 40).hex(), ready=s.read(p + 3, 1)[0], texture_header=p + 20,
                               texture_header_hex=header.hex(), physical_data=data, physical_bytes_sha256=sha(physical),
                               authored_file=str(resource_file.relative_to(ROOT)), authored_bytes_sha256=sha(authored), physical_authored_byte_equal=physical == authored)
                partitions.append(row)
            bitmaps[key] = dict(datum=datum, index=index, name=cache.tags[datum]['name'], root=root, record=record, runtime_record_hex=raw.hex(),
                                cached_header=uint(raw, 0x50), cached_header_hex=s.read(uint(raw, 0x50), 20).hex() if uint(raw, 0x50) else '',
                                cached_deadline=uint(raw, 0x70), cache_clock=uint(s.read(0x4e6488, 4)), partitions=partitions)
    owners = []
    for name, address in STATIC.items():
        raw = image.read(address, int(rows[address]['size']))
        path = args.output / f'{name}-{address:08x}.asm'
        path.write_text(f'{address:08x} indexed_bytes={len(raw)} sha256={sha(raw)}\n' + '\n'.join(listing(raw, address, rows)) + '\n')
        owners.append(dict(owner=name, address=address, listing=path.name, listing_sha256=sha(path.read_bytes())))
    report = dict(schema='original-camouflage-pass-services-v1', boundary=__doc__, inputs=inputs, cases=cases, bitmaps=bitmaps,
                  owners=owners, complete_calls=sum(c['complete_original_return'] for c in cases),
                  stopped_calls=sum(not c['complete_original_return'] for c in cases),
                  executed_original=[dict(address=pc, bytes_hex=image.read(pc, size).hex()) for pc, size in sorted(all_executed.items())],
                  dependencies={str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in (Path(__file__).resolve(), ROOT / 'tools/differential_source_object_admission_snapshot.py', ROOT / 'tools/oracle_mp_spawn.py', ROOT / 'tools/oracle_source_bitmap_async_read.py', ROOT / 'assets/disc/maps/lockout.map', ROOT / 'assets/disc/maps/mainmenu.map')},
                  instruction_patches=[], supplied_returns=[],
                  limits=['Fresh original calls over an authentic captured environment with declared bank, owner, shader, primary and optional fog inputs; not historical camouflage draws.',
                          'Actual cache/header/ready/target/program bytes and command capacity remain captured inputs.12CE00 and command-capacity owners are boundaries without supplied outcomes.',
                          'Whole16B90 return proves CPU preparation and command emission only, not NV2A acceptance, vertex resource selection, drawable geometry, target19 current-frame copy or framebuffer output.',
                          'Retail snapshot only; title-update execution/resources are not established by this report.',
                          'Original target3 simultaneous color/depth alias rendering remains unsupported in the pinned xemu backend and has no complete framebuffer proof.'])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(cases=len(cases), complete=report['complete_calls'], stopped=report['stopped_calls'], bitmaps=len(bitmaps), instructions=len(all_executed), report_sha256=sha(path.read_bytes())), indent=2))


if __name__ == '__main__':
    main()
