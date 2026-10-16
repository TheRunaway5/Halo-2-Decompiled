#!/usr/bin/env python3
"""Whole unchanged TU1.5 AC97 start/stop CPU research; offline inputs only.

Saved source channels/descriptors come from the original SDK, channel and PCM
constructor proofs. Halted/running status and completed/unfinished reset are
declared offline physical inputs. This tool executes no kernel body, native
audio consumer, DSP program, or successful whole startup constructor.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_sound_sdk_native_prefix_tu15 import GP, FS, GDT, POOL, STOP, segment

ROOT = Path(__file__).resolve().parents[1]
PRIOR = ROOT / 'analysis/asset-setup/mcc-native-20261015'
STORAGE = PRIOR / 'native-sound-sdk-prefix/original-driver-storage-prefix/report.json'
PCM = PRIOR / 'native-sound-pcm-buffers/reference/report.json'
ENTRIES = [0x38f0aa, 0x38eeb5, 0x38efea, 0x38edf4, 0x38edc5,
           0x38f05c, 0x38f1f4, 0x38eef1, 0x38f104, 0x38ef59,
           0x38ee84, 0x38e484, 0x38ec8b, 0x388a1d, 0x388a8b,
           0x38e606, 0x38e7d5, 0x38e2e0, 0x383d70, 0x383d92]
STACK = 0x2018000


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(image, storage, row, fill):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    for address, size in [(0x10000, 0x600000), (0x2000000, 0x20000),
                          (STOP, 4096), (0x80000000, 4096), (FS, 4096),
                          (GDT, 4096), (0xfec00000, 4096), (POOL, 0x10000)]:
        cpu.mem_map(address, size)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    for region in storage['regions']:
        cpu.mem_write(region['address'], bytes.fromhex(region['hex']))
    for saved in row['retained_raw_hex']:
        cpu.mem_write(saved['source'], bytes.fromhex(saved['hex']))
    for saved in row['retained_dma_hex'] + row['physical_allocations']:
        size = saved.get('capacity', saved.get('bytes'))
        cpu.mem_map(saved['source'], size)
        cpu.mem_write(saved['source'], bytes.fromhex(saved['hex']))
    cpu.mem_write(0x2010000, bytes([fill]) * 0x10000)
    cpu.mem_write(GDT + 8, segment(0, 0x9b) + segment(0, 0x93) + segment(FS, 0x93))
    cpu.reg_write(R.UC_X86_REG_GDTR, (0, GDT, 4095, 0))
    for reg, value in [(R.UC_X86_REG_CS, 8), (R.UC_X86_REG_DS, 16),
                       (R.UC_X86_REG_ES, 16), (R.UC_X86_REG_SS, 16), (R.UC_X86_REG_FS, 24)]:
        cpu.reg_write(reg, value)
    gp = [0x21000000 + i * 0x111111 for i in range(8)]
    gp[1], gp[4] = row['channel_reference']['channel'], STACK
    for reg, value in zip(GP, gp):
        cpu.reg_write(reg, value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    cpu.reg_write(R.UC_X86_REG_FPCW, 0x27f)
    cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
    return cpu


def run(image, authenticated, executed, storage, row, fill, mode):
    cpu = load(image, storage, row, fill)
    channel = row['channel_reference']['channel']
    index = row['index']
    base = 0xfec00110 + index * 0x60
    table = struct.unpack('<I', cpu.mem_read(channel + 0x18, 4))[0]
    assert bytes(cpu.mem_read(channel + 0x24, 2)) == b'\1\0'
    assert struct.unpack('<2I', cpu.mem_read(table, 8)) == (row['buffer_physical'], 0xc0001000)
    warm = mode.startswith('warm')
    offset = 128 if 'seek128' in mode else 0 if 'seek0' in mode else 0xffffffff
    cpu.mem_write(base + 6, struct.pack('<H', 0 if warm else 1))
    cpu.mem_write(base + 11, b'\x1d' if warm else b'\x00')
    cpu.mem_write(STACK, struct.pack('<2I', STOP, offset))
    instructions, stores, reads, platform, inputs, phases = [], [], [], [], [], []
    depth = 0
    phase = 'start'
    denied = False

    def word(address):
        return struct.unpack('<I', cpu.mem_read(address, 4))[0]

    def code(m, pc, size, _):
        nonlocal depth, denied
        if pc in [0x800000a0, 0x800000a1]:
            sp = m.reg_read(R.UC_X86_REG_ESP)
            returned = word(sp)
            if pc == 0x800000a0:
                assert returned == 0x383d8e and not depth and m.reg_read(R.UC_X86_REG_ECX) & 255 == 2
                depth = 2
                m.reg_write(R.UC_X86_REG_EAX, m.reg_read(R.UC_X86_REG_EAX) & 0xffffff00)
            else:
                assert returned == 0x383da3 and depth == 2 and not m.reg_read(R.UC_X86_REG_ECX) & 255
                depth = 0
            m.mem_write(FS + 0x24, bytes([depth]))
            platform.append(dict(phase=phase, target=pc, returned=returned, new_exclusion_level=depth))
            m.reg_write(R.UC_X86_REG_ESP, sp + 4)
            m.reg_write(R.UC_X86_REG_EIP, returned)
            return
        assert pc in authenticated, hex(pc)
        raw = bytes(m.mem_read(pc, size))
        assert raw == image.read(pc, size)
        assert executed.setdefault(f'{pc:08x}', raw.hex()) == raw.hex()
        instructions.append(dict(phase=phase, pc=pc))
        if pc == 0x38f00e and mode != 'reset_no_ack':
            assert bytes(m.mem_read(base + 11, 1)) == b'\x02'
            m.mem_write(base + 11, b'\x00')
            inputs.append(dict(phase=phase, before_instruction=pc, address=base + 11,
                               value=0, declared_completed_hardware_reset_input=True))
        if phase == 'start' and pc == 0x38f0f3 and mode == 'start_action_denied':
            denied = True
            m.emu_stop()

    def store(m, _, at, size, value, __):
        stores.append(dict(phase=phase, instruction=m.reg_read(R.UC_X86_REG_EIP), address=at,
                           before=bytes(m.mem_read(at, size)).hex(),
                           written=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))

    def read(m, _, at, size, value, __):
        if at >= 0xfec00000:
            reads.append(dict(phase=phase, instruction=m.reg_read(R.UC_X86_REG_EIP), address=at,
                              size=size, value=bytes(m.mem_read(at, size)).hex()))

    cpu.hook_add(UC_HOOK_CODE, code)
    cpu.hook_add(UC_HOOK_MEM_WRITE, store)
    cpu.hook_add(UC_HOOK_MEM_READ, read)
    before = bytes(cpu.mem_read(channel, 0x30)).hex()

    def invoke(entry, arg=None):
        cpu.reg_write(R.UC_X86_REG_ECX, channel)
        cpu.reg_write(R.UC_X86_REG_ESP, STACK)
        cpu.mem_write(STACK, struct.pack('<I', STOP) + (struct.pack('<I', arg) if arg is not None else b''))
        cpu.emu_start(entry, STOP, count=10000)
        returned = cpu.reg_read(R.UC_X86_REG_EIP) == STOP
        phases.append(dict(phase=phase, entry=entry, returned=returned,
                           endpoint_command=bytes(cpu.mem_read(base + 11, 1)).hex(),
                           source_channel_hex=bytes(cpu.mem_read(channel, 0x30)).hex(),
                           gp=[cpu.reg_read(reg) for reg in GP], eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS)))
        return returned

    returned = invoke(0x38f0aa, offset)
    start_stores = [r for r in stores if r['instruction'] == 0x38f0f3]
    if mode == 'reset_no_ack':
        assert not returned and cpu.reg_read(R.UC_X86_REG_EIP) in [0x38f020, 0x38f022]
        assert not start_stores and depth == 2
    elif mode == 'start_action_denied':
        assert denied and not returned and not start_stores and depth == 2
        assert cpu.reg_read(R.UC_X86_REG_EIP) == 0x38f0f3
    else:
        assert returned and not depth and len(start_stores) == 1 and start_stores[0]['written'] == '1d'
        assert cpu.reg_read(R.UC_X86_REG_ESP) == STACK + 8
        if offset != 0xffffffff:
            assert bytes(cpu.mem_read(base + 8, 2)) == struct.pack('<H', (0x1000 - offset // 2) & 0xffff)
            assert any(r['instruction'] == 0x38eedb and r['address'] == channel + 0x2c for r in stores)
        else:
            assert not any(r['instruction'] == 0x38eed4 for r in stores)
        if mode.startswith('completion_stop'):
            phase = 'declared_descriptor_completion'
            assert invoke(0x38f1f4) and word(channel + 4) == 0x1000 and not depth
            phase = 'stop'
            clear = int(mode.endswith('clear'))
            assert invoke(0x38f05c, clear) and not depth
            assert word(channel + 4) == (0 if clear else 0x1000)
            assert bytes(cpu.mem_read(channel + 0x24, 2)) == b'\0\0'
    result = dict(mode=mode, fill=fill, index=index, channel=channel, descriptor_source=table,
                  declared_initial_physical_status=0 if warm else 1, offset_argument=offset,
                  returned=returned, stopped_pc=cpu.reg_read(R.UC_X86_REG_EIP),
                  initial_channel_hex=before, phases=phases, instructions=instructions, stores=stores,
                  physical_reads=reads, explicit_offline_reset_inputs=inputs, kernel_interface_calls=platform,
                  reset_called=any(r['pc'] == 0x38efea for r in instructions),
                  start_command_executed=bool(start_stores),
                  whole_start_returned=returned, native_start_or_audio_consumption_proven=False,
                  source_or_dsp_producer_proven=False)
    return result


def failure_continuation(image, authenticated, site):
    # Negative child return is declared at the real return continuation.
    # Earlier child constructors are not run or given successful results.
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    for p, n in [(0x10000, 0x600000), (0x2010000, 0x10000), (STOP, 4096)]:
        cpu.mem_map(p, n)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    saved_edi, saved_esi, failure = 0x71717171, 0x62626262, 0x80004005
    cpu.mem_write(STACK, struct.pack('<3I', saved_edi, saved_esi, STOP))
    cpu.reg_write(R.UC_X86_REG_ESP, STACK)
    cpu.reg_write(R.UC_X86_REG_EAX, failure)
    instructions = []
    def hook(m, pc, size, _):
        assert pc in authenticated and bytes(m.mem_read(pc, size)) == image.read(pc, size)
        instructions.append(pc)
    cpu.hook_add(UC_HOOK_CODE, hook)
    cpu.emu_start(site, STOP, count=100)
    assert cpu.reg_read(R.UC_X86_REG_EIP) == STOP and cpu.reg_read(R.UC_X86_REG_EAX) == failure
    assert cpu.reg_read(R.UC_X86_REG_EDI) == saved_edi and cpu.reg_read(R.UC_X86_REG_ESI) == saved_esi
    assert not any(pc in [0x38ece3, 0x38eced] for pc in instructions)
    return dict(entry_at_real_child_return=site, declared_negative_hresult=failure,
                instructions=instructions, returned_hresult=cpu.reg_read(R.UC_X86_REG_EAX),
                either_start_called=False, earlier_constructors_executed_or_supplied=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    storage, pcm = json.loads(STORAGE.read_text()), json.loads(PCM.read_text())
    assert storage['passed'] and pcm['passed'] and not storage['instruction_patches'] and not pcm['instruction_patches']
    assert pcm['storage_reference_sha256'] == digest(STORAGE)
    assert storage['original_xbe_sha256'] == pcm['original_xbe_sha256'] == NEW_PIN
    code = _Code(image, 'DSOUND')
    bodies = {e: _trace(code, e, set(ENTRIES)) for e in ENTRIES}
    authenticated = set().union(*(r.code for r in bodies.values()))
    body_manifest = {}
    for entry, body in bodies.items():
        path = args.output / f'original-{entry:08x}.asm'
        lines = []
        for pc in sorted(body.code):
            op = code.at(pc)
            assert image.read(pc, op.size) == bytes(op.bytes)
            lines.append(f'{pc:08x} {bytes(op.bytes).hex()} {op.mnemonic} {op.op_str}')
        path.write_text('\n'.join(lines) + '\n')
        body_manifest[f'{entry:08x}'] = dict(instructions=len(body.code), sha256=digest(path), sites=sorted(body.code))
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.skipdata = True
    section = image.section('DSOUND')
    callers = [dict(site=op.address, returned=op.address + op.size, source_bytes=bytes(op.bytes).hex())
               for op in md.disasm(image.section_bytes(section), section.va)
               if op.mnemonic == 'call' and op.op_str == '0x38f0aa']
    assert [r['site'] for r in callers] == [0x388a6c, 0x388a7a, 0x388b5f, 0x388b69, 0x38ece3, 0x38eced]
    literal = struct.pack('<I', 0x38f0aa)
    literal_refs = []
    for section in image.sections:
        data = image.section_bytes(section)
        at = data.find(literal)
        while at >= 0:
            literal_refs.append(dict(section=section.name, address=section.va + at))
            at = data.find(literal, at + 1)
    assert image.read(0x390b94, 8) == struct.pack('<2I', 0x10, 0x70)
    executed = {}
    modes = ['cold_resume', 'warm_resume', 'cold_seek0', 'cold_seek128', 'warm_seek128',
             'reset_no_ack', 'start_action_denied', 'completion_stop_keep', 'completion_stop_clear']
    cases = [run(image, authenticated, executed, storage['cases'][0], row, fill, mode)
             for row in pcm['cases'] for fill in [0, 0x5a, 0xa5] for mode in modes]
    failures = [failure_continuation(image, authenticated, site)
                for site in [0x38ec9b, 0x38eca8, 0x38ecb5, 0x38ecc2, 0x38eccf]]
    report = dict(passed=True, scope=__doc__, original_xbe_sha256=NEW_PIN,
                  storage_reference=str(STORAGE.relative_to(ROOT)), storage_reference_sha256=digest(STORAGE),
                  original_pcm_reference=str(PCM.relative_to(ROOT)), original_pcm_reference_sha256=digest(PCM),
                  tool_sha256=digest(Path(__file__)), bodies=body_manifest, direct_start_callers=callers,
                  literal_start_references=literal_refs, channel_offset_table_hex=image.read(0x390b94, 8).hex(),
                  executed_instruction_bytes=executed, cases=cases, negative_caller_continuations=failures,
                  instruction_patches=[], original_kernel_bodies_executed=False,
                  native_consumer_or_physical_hardware_proven=False, dsp_pcm_producer_proven=False,
                  whole_constructor_or_initializer45_completed=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, whole_start_stop_cases=len(cases), failure_continuations=len(failures),
                          authenticated_bodies=len(bodies), executed_sites=len(executed), direct_callers=len(callers))))


if __name__ == '__main__':
    main()
