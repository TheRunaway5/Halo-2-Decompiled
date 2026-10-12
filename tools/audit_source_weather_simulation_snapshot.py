#!/usr/bin/env python3
"""Read-only source dependency audit for 38450/38390 and real12DE70.

The immutable retained BSP has an empty weather palette and exclusion-volume
bank. That palette admits no particle systems, so this audit proves no
historical particle simulation. Complete12DE70 is replayed over actual retained
BSP geometry descriptors, with explicit mode3 weather-call arguments and
separate mode0 query controls. No resource, kernel, callback, completion or
original instruction is replaced.
"""
from pathlib import Path
import collections, hashlib, json, re, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_OP_MEM
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH, STACK, STOP = 0x20000000, 0x200fe000, 0x200ff000


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    snapshot = Snapshot(ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    xbe = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(xbe.data) == PIN
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    disassembler.detail = True
    instructions = {}
    for pc, size in [(0x38390, 184), (0x38450, 3483), (0x12de70, 591), (0x12e3a0, 211), (0x322b0c, 117)]:
        rows = list(disassembler.disasm(xbe.read(pc, size), pc))
        instructions[f'{pc:08X}'] = dict(bytes=size, instruction_count=len(rows),
            sha256=sha(xbe.read(pc, size)),
            direct_calls=[dict(pc=f'{i.address:08X}', target=i.op_str) for i in rows if i.mnemonic == 'call'],
            absolute_memory=sorted({f'{o.mem.disp & 0xffffffff:08X}' for i in rows for o in i.operands
                if o.type == CS_OP_MEM and o.mem.base == 0 and o.mem.index == 0}))
    assert [row['target'] for row in instructions['00038450']['direct_calls']] == ['0x12de70', '0x322b0c', '0x38390']
    bsp = uint(snapshot.read(0x4e0348, 4))
    weather_count, weather_pointer = words(snapshot.read(bsp + 0x84, 8))
    exclusion_count, exclusion_pointer = words(snapshot.read(bsp + 0x8c, 8))
    assert (weather_count, weather_pointer, exclusion_count, exclusion_pointer) == (0, 0, 0, 0)
    descriptors = []
    for label, offset, stride in [('cluster', 0x9c, 176), ('definition', 0x138, 200)]:
        count, pointer = words(snapshot.read(bsp + offset, 8))
        for index in range(count):
            address = pointer + index * stride + 0x28
            descriptors.append((label, index, address, snapshot.read(address, 0x24)))
    assert len(descriptors) == 74
    manager = uint(snapshot.read(0x4e649c, 4))
    manager_raw = snapshot.read(manager, 0x70)
    physical_pool = uint(manager_raw, 0x64)
    physical_rows = uint(snapshot.read(physical_pool + 0x44, 4))
    cache_pool = uint(snapshot.read(0x4e648c, 4))
    cache_rows = uint(snapshot.read(cache_pool + 0x44, 4))
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(SYNTH, 0x100000)
    pages, dirty, seen = {}, set(), {}
    boundaries, writes, calls = [], [], collections.Counter()
    watch = {0x12de70, 0x12e3a0, 0x13d370, 0x16b840, 0x16b910,
        0x213760, 0x120ce0, 0x2d2084, 0x125d60, 0x223c70}

    def page(address):
        at = address & ~4095
        if at not in pages:
            raw = snapshot.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw

    def read(address, size):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            if not SYNTH <= at < SYNTH + 0x100000:
                page(at)
        return bytes(u.mem_read(address, size))

    def unmapped(g, access, address, size, value, data):
        try:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                page(at)
            return True
        except ValueError as error:
            boundaries.append(dict(kind='uncaptured memory/MMIO; not replaced',
                pc=f'{g.reg_read(UC_X86_REG_EIP):08X}', address=f'{address:08X}', detail=str(error)))
            g.emu_stop()
            return False

    def code(g, pc, size, data):
        if pc == STOP:
            g.emu_stop()
            return
        try:
            raw = xbe.read(pc, size)
        except (ValueError, KeyError):
            boundaries.append(dict(kind='actual kernel/external entry; not executed or replaced',
                pc=f'{pc:08X}', stack=words(read(g.reg_read(UC_X86_REG_ESP) + 4, 24)),
                eax=f'{g.reg_read(UC_X86_REG_EAX):08X}', ecx=f'{g.reg_read(UC_X86_REG_ECX):08X}'))
            g.emu_stop()
            return
        assert read(pc, size) == raw, f'changed original instruction {pc:08X}'
        seen[pc] = size
        if pc in watch:
            calls[f'{pc:08X}'] += 1

    def writing(g, access, address, size, value, data):
        if not SYNTH <= address < SYNTH + 0x100000:
            assert not any(pc < address + size and address < pc + count for pc, count in seen.items())
            before = read(address, size)
            writes.append(dict(address=f'{address:08X}', size=size, before=before.hex(),
                after=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                dirty.add(at)

    u.hook_add(UC_HOOK_MEM_UNMAPPED, unmapped)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga, gl = (int(value, 16) for value in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',
        snapshot.report['registers_before']).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for register, value in [(UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16),
        (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)]:
        u.reg_write(register, value)

    def restore():
        for at in dirty:
            u.mem_write(at, pages[at])
        dirty.clear()
        boundaries.clear()
        writes.clear()
        calls.clear()

    records, totals = [], collections.Counter()
    for label, index, address, raw in descriptors:
        handle = uint(raw, 0x20)
        cache_raw = snapshot.read(cache_rows + (handle & 65535) * 12, 12) if handle != 0xffffffff else None
        physical_raw = snapshot.read(physical_rows + (handle & 65535) * 24, 24) if handle != 0xffffffff else None
        for flags in [3, 0]:
            restore()
            for register in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]:
                u.reg_write(register, 0)
            u.reg_write(UC_X86_REG_EAX, flags)
            u.reg_write(UC_X86_REG_ECX, address)
            u.reg_write(UC_X86_REG_ESP, STACK)
            u.reg_write(UC_X86_REG_EFLAGS, 2)
            u.reg_write(UC_X86_REG_FPCW, 0x023f)
            u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            u.mem_write(STACK - 0x4000, bytes(0x5000))
            u.mem_write(STACK, packed([STOP]))
            try:
                u.emu_start(0x12de70, STOP, count=2000000)
            except UcError:
                if not boundaries:
                    raise
            complete = not boundaries
            assert u.reg_read(UC_X86_REG_EIP) == STOP or boundaries, 'instruction budget is not a service boundary'
            if complete:
                assert u.reg_read(UC_X86_REG_ESP) == STACK + 4
            output = u.reg_read(UC_X86_REG_EAX) & 255 if complete else None
            totals[f'mode{flags}_' + ('ready' if output else 'not_ready') if complete else f'mode{flags}_unknown'] += 1
            records.append(dict(kind=label, index=index, descriptor=f'{address:08X}', raw_descriptor=raw.hex(),
                flags=flags, phase_label='diagnostic fresh replay, not a historical request/completion timeline',
                retained_runtime_linked=bool(raw[0x1e]), retained_cache_handle=f'{handle:08X}',
                retained_cache_row=cache_raw.hex() if cache_raw else None,
                retained_physical_row=physical_raw.hex() if physical_raw else None,
                completed=complete, source_AL=output, source_calls=dict(calls),
                boundary=boundaries.copy(), original_writes=writes.copy(), after_descriptor=read(address, 36).hex()))
    restore()
    for at, raw in pages.items():
        assert bytes(u.mem_read(at, 4096)) == raw
    report = dict(boundary=__doc__, xbe_sha256=PIN, snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha((snapshot.directory / 'export.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()), instructions=instructions,
        retained_BSP=f'{bsp:08X}', weather_palette=[weather_count, f'{weather_pointer:08X}'],
        exclusion_volumes=[exclusion_count, f'{exclusion_pointer:08X}'],
        retained_manager=f'{manager:08X}', retained_manager_sha256=sha(manager_raw),
        cache_pool=f'{cache_pool:08X}', physical_pool=f'{physical_pool:08X}',
        resource_descriptor_cases=len(records), result_counts=dict(totals),
        unique_executed_title_instructions=len(seen), restored_original_pages=len(pages),
        replay_fpcw='023F', replay_mxcsr='1F80',
        source_instruction_patches=[], services_replaced=[], original_completion_timeline_claimed=False,
        historical_weather_particle_simulation_claimed=False, GPU_or_live_activation=False,
        ABI=dict(simulation='38450: EBX=140-byte particle system, XMM0=portal distance, [ESP+4]=weat definition pointer, RET4, AL1 including real not-ready path.',
            exclusion='38390: ESI=point3f pointer, ECX=24-byte sphere/plane-volume pointer, no stack arguments, AL bool, RET.',
            resource='12DE70: ECX=36-byte resource descriptor, EAX=flags, no stack arguments, AL bool, RET. Weather supplies flags3.'),
        caller=dict(pc='00036E8D..00036E94', particle_system='weat+4 pointer + ordinal*140',
            portal_distance='XMM0 from actual current-cluster/visible-portal admission36C00..36DD9',
            parameter_pointer='owning weat definition, pushed at36E93',
            ordering='36B50: view0 derives509418 from admitted double4858A0 and calls398D0; visible palette admits4B3D0 wind once on view0, then37260 plates,38450 particles,37A60 plates,391F0 particle submission.'),
        particle_inputs=dict(system_fields={
            '0008/000C/0010':'camera box extents', '0014':'near-camera exclusion radius',
            '0018':'velocity magnitude limit', '001C/0020':'respawn mass endpoints',
            '0024/0028':'respawn size endpoints', '0030/0034/0038':'respawn velocity scales',
            '0040':'full36-byte resource descriptor', '0068':'published resource group pointer',
            '006C':'WORD particle type;2 skips the stationary near-origin opacity branch',
            '0080':'elapsed accumulator, incremented before readiness query on every admitted call'},
            owning_weat_fields={'0030/0034/0038':'wind/base velocity', '0090/0094/0098':'axis perturbation scales',
                '009C':'vertical acceleration subtraction', '00A8':'auxiliary random perturbation step',
                '00AC':'portal attenuation distance'},
            global_fields={'00509418':'source admitted elapsed float', '004670FC':'source intensity',
                '00485600':'signed16 current view slot', '004E8C20+8':'signed16 view count clamped1..4',
                '00485618..00485620':'source camera position', '0048568C..004856BC':'forward camera transform',
                '004856C0..004856F0':'inverse camera transform', '004E7408+4':'auxiliary RNG state',
                '004E0348+8C/+90':'BSP exclusion volume count/pointer'},
            resource_group='DWORD count,+4 render pointer20-byte rows,+C state pointer32-byte rows',
            view_partition='38517..3855C: signed count / clamped view count; first=signed16 viewslot*quotient; desired=CVTTSS2SI(float(quotient)*intensity). No forced minimum or remainder redistribution.',
            cold_constructor='36AB0 resolves authored palette datums and calls39880, which calls36F50 then39A80. It does not call12DE70.38450 does not call39A80 or inspect system+86; already-published resource ownership and actual initialization chronology remain caller responsibilities.'),
        arithmetic_boundaries=dict(
            auxiliary_draw_order='Active age>=0 branch: perturbation draws state+14,+C,+10; optional extra mass draw when mass<=epsilon. Respawn age<0 branch consumes11 draws:mass,velocityXYZ,size,positionXYZ,perturbationXYZ.',
            velocity_limit='38AF3..38B3E uses actual RSQRTSS at38B02 after both squared-speed thresholds; host sqrt is not its source instruction.',
            opacity='38D56..38D7E uses x87 products and complete original322B0C (__ftol2), then EAX<<24 |00FFFFFF.',
            box_wrap='CVTTSS2SI fractional wrapping and exact forward/inverse SSE matrix order at38B83..38E48; translations and scale branches are read from original transforms.',
            exclusion='38390 sphere sum=(dx*dx+dz*dz)+dy*dy, strict radiusSquared>distanceSquared. Signed plane count<=0 accepts sphere hit. Plane sum=(ny*y+nz*z)+nx*x-d; rejects only ordered distance>epsilon. Boundary equality is accepted.',
            desired_count_edges='38EFF active branch rejects only when accumulated active count>desired;38F2F respawn branch requires active count<desired. They use different signed comparisons.'),
        minimum_safe_seam='CPU transition over exclusively owned already-published initialized system/render/state banks, explicit owning-weat fields, source elapsed/intensity/view partition, transforms/camera/exclusion banks and auxiliary RNG. Ready12DE70 may execute complete against owned raw geometry manager/cache/fixup banks. Pending resource admission reaches real kernel services and cannot be replaced with ready/false; the bitmap reservation helper proves texture callbacks and does not prove geometry callbacks12E210/12E290.',
        readiness='12E3A0 clears descriptor+1E and passes cache-row+2 as the actual done pointer through 213760/1A0F10 to 1208B0. The actual worker120C8C writes done=1 only for callback result1 and nonNULL done.12DE70 accepts that retained done byte and calls complete223C70 before storing descriptor+1E=1 at12E086. Pending external services are unknown, never false or ready.',
        records=records)
    output = ROOT / 'analysis/multiplayer-shading/weather-simulation-dependency-snapshot-original.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: value for key, value in report.items() if key not in ['records', 'instructions']}, indent=2))


if __name__ == '__main__':
    main()
