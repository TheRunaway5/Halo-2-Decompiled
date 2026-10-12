#!/usr/bin/env python3
"""Replay a saved actual whole38450 call without instruction/service replacement.

Execution-relevant integer, SSE/x87 and segment state, the original stack and
memory come from the immutable entry RAM. Privileged CPU state is not emulated.
The saved return and literal38B02/38B08 pair are independent xemu observations.
This comparison does not assign RSQRTSS precision to physical Xbox hardware or
publish resource readiness, infer a simulation clock, or operate a live guest.
"""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import re
import struct
import sys

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

SESSION = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1'
GPRS = ['EAX', 'EBX', 'ECX', 'EDX', 'ESI', 'EDI', 'EBP', 'ESP']


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def reg(registers, name):
    return int(re.search(r'\b' + name + r'=([0-9a-fA-F]+)', registers)[1], 16)


def xmm(registers, index):
    match = re.search(r'XMM%02d=([0-9a-fA-F]{16})\s+([0-9a-fA-F]{16})' % index, registers)
    return int(match[1] + match[2], 16)


def load(directory):
    s = Snapshot(directory)
    assert sha(s.ram) == s.report['physical_ram_sha256']
    assert s.report['registers_before'] == s.report['registers_after']
    return s


def snapshot_record(s):
    return dict(directory=str(s.directory.relative_to(ROOT)),
        RAM_sha256=sha(s.ram), export_sha256=sha((s.directory / 'export.json').read_bytes()),
        EIP=f'{reg(s.report["registers_before"], "EIP"):08X}')


def float_words(raw):
    return [f'{v:08X}' for v in words(raw)]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--entry', type=Path, default=SESSION / 'lockout-weather-particle-simulation-entry')
    p.add_argument('--return-state', type=Path, default=SESSION / 'lockout-weather-particle-simulation-return')
    p.add_argument('--rsqrt-before', type=Path, default=SESSION / 'lockout-weather-rsqrt-before')
    p.add_argument('--rsqrt-after', type=Path, default=SESSION / 'lockout-weather-rsqrt-after')
    p.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer-shading/weather-particle-simulation-original/report.json')
    args = p.parse_args()
    entry, actual_return = load(args.entry), load(args.return_state)
    actual_before, actual_after = load(args.rsqrt_before), load(args.rsqrt_after)
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    regs = entry.report['registers_before']
    assert reg(regs, 'EIP') == 0x38450
    stack = reg(regs, 'ESP')
    return_pc, owner = words(entry.read(stack, 8))
    assert return_pc == 0x36e99 and reg(actual_return.report['registers_before'], 'EIP') == return_pc
    assert reg(actual_return.report['registers_before'], 'ESP') == stack + 8
    assert reg(actual_before.report['registers_before'], 'EIP') == 0x38b02
    assert reg(actual_after.report['registers_before'], 'EIP') == 0x38b08
    system = reg(regs, 'EBX')
    group = uint(entry.read(system + 0x68, 4))
    group_header = entry.read(group, 16)
    count, render = words(group_header)[:2]
    state = uint(group_header, 12)
    assert 0 < count <= 1_000_000
    auxiliary = uint(entry.read(0x4e7408, 4))
    view_pool = uint(entry.read(0x4e8c20, 4))
    bsp = uint(entry.read(0x4e0348, 4))
    exclusions, exclusion_ptr = words(entry.read(bsp + 0x8c, 8))
    # No geometry service output is supplied. Original12DE70 executes on these
    # same captured descriptor/cache/manager banks, including its ready gate.
    manager = uint(entry.read(0x4e649c, 4))
    physical_pool = uint(entry.read(manager + 0x64, 4))
    physical_rows = uint(entry.read(physical_pool + 0x44, 4))
    resource_handle = uint(entry.read(system + 0x60, 4))
    assert entry.read(system + 0x5e, 1) == b'\1' and resource_handle != 0xffffffff
    resource_row = physical_rows + (resource_handle & 65535) * 24
    regions = [('system', system, 140), ('owner_weat', owner, 176), ('group', group, 52),
        ('render', render, count * 20), ('state', state, count * 32), ('auxiliary_RNG', auxiliary + 4, 4),
        ('elapsed', 0x509418, 4), ('intensity', 0x4670fc, 4), ('view_slot', 0x485600, 2),
        ('view_count', view_pool + 8, 2), ('camera_position', 0x485618, 12),
        ('forward_transform', 0x48568c, 52), ('inverse_transform', 0x4856c0, 52),
        ('geometry_manager', manager, 112), ('physical_resource_row', resource_row, 24)]
    if exclusions:
        regions.append(('exclusion_volumes', exclusion_ptr, exclusions * 24))
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    pages, written, instructions = {}, set(), {}
    calls = collections.Counter()
    boundaries, rsqrt, counts = [], [], collections.Counter()
    pending_rsqrt = [None]

    def page(address):
        at = address & ~4095
        if at not in pages:
            raw = entry.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw

    def read(address, size):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            page(at)
        return bytes(u.mem_read(address, size))

    def missing(g, access, address, size, value, _):
        try:
            read(address, size)
            return True
        except ValueError as error:
            boundaries.append(dict(kind='Uncaptured original memory/MMIO, not supplied',
                PC=f'{g.reg_read(UC_X86_REG_EIP):08X}', address=f'{address:08X}', reason=str(error)))
            g.emu_stop()
            return False

    def cpu():
        return {name: f'{u.reg_read(globals()["UC_X86_REG_" + name]):08X}' for name in GPRS}

    def code(g, pc, size, _):
        if pc == return_pc:
            g.emu_stop()
            return
        try:
            raw = image.read(pc, size)
        except (ValueError, KeyError):
            boundaries.append(dict(kind='Actual external/kernel boundary, not replaced', PC=f'{pc:08X}', CPU=cpu()))
            g.emu_stop()
            return
        assert read(pc, size) == raw, f'changed original instruction {pc:08X}'
        instructions[pc] = size
        counts['instructions'] += 1
        if pc in {0x38450, 0x12de70, 0x38390, 0x322b0c, 0x12e3a0, 0x223c70}:
            calls[f'{pc:08X}'] += 1
        if pc == 0x38b02:
            assert raw.hex() == 'f30f52442440'
            sp = g.reg_read(UC_X86_REG_ESP)
            pending_rsqrt[0] = dict(CPU=cpu(), operand=f'{uint(read(sp + 0x40, 4)):08X}',
                XMM0_before=f'{g.reg_read(UC_X86_REG_XMM0):032X}',
                state=f'{g.reg_read(UC_X86_REG_EBP):08X}', MXCSR=f'{g.reg_read(UC_X86_REG_MXCSR):08X}')
        if pc == 0x38b08 and pending_rsqrt[0] is not None:
            row = pending_rsqrt[0]
            row['XMM0_after'] = f'{g.reg_read(UC_X86_REG_XMM0):032X}'
            rsqrt.append(row)
            pending_rsqrt[0] = None

    def writing(g, access, address, size, value, _):
        assert not any(pc < address + size and address < pc + n for pc, n in instructions.items()), 'Source instruction overwrite'
        read(address, size)
        written.update(range(address, address + size))
        counts['writes'] += 1
        if address == auxiliary + 4:
            assert size == 4
            counts['auxiliary_rng_draws'] += 1

    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', regs).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for name in ['CS', 'SS', 'DS', 'ES', 'FS', 'GS']:
        selector = int(re.search(r'\b' + name + r'\s*=([0-9a-fA-F]+)', regs)[1], 16)
        u.reg_write(globals()['UC_X86_REG_' + name], selector)
    for name in GPRS:
        u.reg_write(globals()['UC_X86_REG_' + name], reg(regs, name))
    u.reg_write(UC_X86_REG_EFLAGS, reg(regs, 'EFL'))
    for i in range(8):
        u.reg_write(globals()['UC_X86_REG_XMM' + str(i)], xmm(regs, i))
    # QEMU FTW is the abridged physical x87 tag mask. This actual entry has no
    # live stack operands; preserve all raw80-bit registers and convert tag00
    # to the full empty tagFFFF rather than inventing initial arithmetic state.
    assert reg(regs, 'FTW') == 0
    for i in range(8):
        match = re.search(r'FPR%d=([0-9a-fA-F]{16})\s+([0-9a-fA-F]{4})' % i, regs)
        u.reg_write(globals()['UC_X86_REG_FP' + str(i)], (int(match[1], 16), int(match[2], 16)))
    u.reg_write(UC_X86_REG_FPCW, reg(regs, 'FCW'))
    u.reg_write(UC_X86_REG_FPSW, reg(regs, 'FSW'))
    u.reg_write(UC_X86_REG_FPTAG, 0xffff)
    u.reg_write(UC_X86_REG_MXCSR, reg(regs, 'MXCSR'))
    input_banks = {label: entry.read(at, n) for label, at, n in regions}
    try:
        u.emu_start(0x38450, return_pc, count=4_000_000)
    except UcError:
        if not boundaries:
            raise
    complete = not boundaries and u.reg_read(UC_X86_REG_EIP) == return_pc
    assert complete or boundaries, 'Instruction budget is not an external-service result'
    output_dir = args.output.parent
    output_dir.mkdir(parents=True, exist_ok=True)
    banks = {}
    for label, at, n in regions:
        raw, expected = read(at, n), actual_return.read(at, n)
        before = input_banks[label]
        mismatch = [i for i, (a, b) in enumerate(zip(raw, expected)) if a != b]
        (output_dir / (label + '-entry.bin')).write_bytes(before)
        (output_dir / (label + '-replay.bin')).write_bytes(raw)
        (output_dir / (label + '-actual-return.bin')).write_bytes(expected)
        banks[label] = dict(address=f'{at:08X}', bytes=n, entry_sha256=sha(before),
            replay_sha256=sha(raw), actual_return_sha256=sha(expected), differing_bytes=len(mismatch),
            changed_bytes=sum(a != b for a, b in zip(before, expected)),
            first_differences=[dict(offset=i, replay=raw[i], actual=expected[i]) for i in mismatch[:32]])
    # The first actual pair is observed during this same complete call. Other
    # per-particle RSQRT observations below are replay outputs, not extra guest captures.
    actual_rs_regs = actual_before.report['registers_before']
    actual_sp = reg(actual_rs_regs, 'ESP')
    actual_rsqrt = dict(operand=f'{uint(actual_before.read(actual_sp + 0x40, 4)):08X}',
        XMM0_before=f'{xmm(actual_rs_regs, 0):032X}',
        XMM0_after=f'{xmm(actual_after.report["registers_before"], 0):032X}',
        MXCSR=f'{reg(actual_rs_regs, "MXCSR"):08X}')
    first_rsqrt_equal = bool(rsqrt) and all(rsqrt[0][name] == value for name, value in actual_rsqrt.items())
    actual_return_regs = actual_return.report['registers_before']
    output_cpu = cpu()
    cpu_compare = {name: dict(replay=output_cpu[name], actual=f'{reg(actual_return_regs, name):08X}',
        equal=output_cpu[name] == f'{reg(actual_return_regs, name):08X}') for name in GPRS}
    for i in range(8):
        replay_word = u.reg_read(globals()['UC_X86_REG_XMM' + str(i)])
        actual_word = xmm(actual_return_regs, i)
        cpu_compare['XMM' + str(i)] = dict(replay=f'{replay_word:032X}', actual=f'{actual_word:032X}', equal=replay_word == actual_word)
    for name, register in [('EFLAGS', UC_X86_REG_EFLAGS), ('FCW', UC_X86_REG_FPCW),
                           ('FSW', UC_X86_REG_FPSW), ('MXCSR', UC_X86_REG_MXCSR)]:
        actual_word = reg(actual_return_regs, 'EFL' if name == 'EFLAGS' else name)
        replay_word = u.reg_read(register)
        cpu_compare[name] = dict(replay=f'{replay_word:08X}', actual=f'{actual_word:08X}', equal=replay_word == actual_word)
    nonscratch_written = sorted(a for a in written if not stack - 0x1000 <= a < stack + 8)
    written_diff = [a for a in nonscratch_written if read(a, 1) != actual_return.read(a, 1)]
    def ages(raw):
        values = [struct.unpack_from('<f', raw, i * 32 + 28)[0] for i in range(count)]
        return dict(positive=sum(a > 0 for a in values), zero=sum(a == 0 for a in values), negative=sum(a < 0 for a in values))
    input_ages = ages(input_banks['state'])
    expected_rng_draws = (input_ages['positive'] + input_ages['zero']) * 3 + input_ages['negative'] * 11
    assert complete and first_rsqrt_equal and not written_diff
    assert all(cpu_compare[name]['equal'] for name in GPRS)
    assert counts['auxiliary_rng_draws'] == expected_rng_draws
    report = dict(authority=__doc__, source_executable_sha256=PIN, tool_sha256=sha(Path(__file__).read_bytes()),
        inputs={name: snapshot_record(s) for name, s in [('entry', entry), ('return', actual_return),
            ('rsqrt_before', actual_before), ('rsqrt_after', actual_after)]},
        entry=dict(PC='00038450', EBX_system=f'{system:08X}', stack=f'{stack:08X}',
            actual_return_PC=f'{return_pc:08X}', owner_weat=f'{owner:08X}',
            portal_distance=f'{xmm(regs, 0) & 0xffffffff:08X}', FCW=f'{reg(regs, "FCW"):04X}',
            FSW=f'{reg(regs, "FSW"):04X}', abridged_FTW='00', MXCSR=f'{reg(regs, "MXCSR"):08X}',
            initialized_group=f'{group:08X}', count=count, render=f'{render:08X}', state=f'{state:08X}',
            exclusions=exclusions, exclusions_pointer=f'{exclusion_ptr:08X}',
            actual_ready_descriptor_byte=1, physical_resource_handle=f'{resource_handle:08X}'),
        replay=dict(completed_whole38450=complete, PC=f'{u.reg_read(UC_X86_REG_EIP):08X}',
            unique_original_instructions=len(instructions), executed_instructions=counts['instructions'],
            write_operations=counts['writes'], written_bytes=len(written),
            nonstack_written_bytes=len(nonscratch_written), nonstack_differing_written_bytes=len(written_diff),
            first_nonstack_differences=[f'{a:08X}' for a in written_diff[:32]], mapped_pages=len(pages),
            calls=dict(calls), boundaries=boundaries, CPU=cpu_compare, banks=banks,
            auxiliary_rng_draws=counts['auxiliary_rng_draws'], independently_expected_rng_draws=expected_rng_draws,
            entry_ages=input_ages, return_ages=ages(actual_return.read(state, count * 32)),
            excluded_stack_scratch='Original per-call scratch in [entryESP-1000,entryESP+8), not compared as lasting state.',
            excluded_CPU_state='Privileged CR/IDT/TR/LDT and x87 instruction/data pointers are not restored; empty raw x87 register payloads and entire64MiB returnRAM are not final-state assertions.'),
        actual_literal_RSQRT=dict(before_PC='00038B02', after_PC='00038B08', source_bytes='F30F52442440',
            observed=actual_rsqrt, first_replay_matches_actual=first_rsqrt_equal,
            replay_count=len(rsqrt), replay_observations=rsqrt,
            physical_Xbox_answer_claimed=False, source_instruction_or_service_replaced=False),
        original_instruction_patches=[], services_replaced=[], original_ready_result_supplied=False,
        actual_xemu_entry_to_return=True, full_native_implementation_claimed=False,
        GPU_jobs=0, guest_IO=False, live_runtime_activation=False)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(report=str(args.output.relative_to(ROOT)), complete=complete,
        count=count, rsqrt_count=len(rsqrt), first_rsqrt_matches_actual=first_rsqrt_equal,
        nonstack_written_bytes=len(nonscratch_written), differing_written_bytes=len(written_diff),
        differing_bank_bytes={label: row['differing_bytes'] for label, row in banks.items() if row['differing_bytes']},
        report_sha256=sha(args.output.read_bytes())), indent=2))


if __name__ == '__main__':
    main()
