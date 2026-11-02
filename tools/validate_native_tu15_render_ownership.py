#!/usr/bin/env python3
"""Compare complete native renderer ownership leaves to unchanged TU1.5 code.

CTest runs this inside run_test.py. Cases are declared data inputs; original
instructions, callees and outcomes are never patched or supplied. Raw banks
stay in memory/pipes within the owning HALO2_TEST_SCRATCH lifetime.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
STACK, STOP = 0x62000000, 0x63000000
RANGES = ((0xbcfb0, 0xbcfd8), (0xbac70, 0xbacc1), (0x158290, 0x1582f0), (0x3e9b0, 0x3ea15))


def case(name, entry, *, depth=1, index=0, count=8, view=0, callback=0x245400,
         blend=0, gate=0, kind=0, matching=True, mxcsr=0x1f80, none=False):
    banks = {}
    def bank(at, size):
        banks[at] = bytearray(size)
        return banks[at]
    def word(at, value):
        for start, data in banks.items():
            if start <= at and at + 4 <= start + len(data):
                struct.pack_into('<I', data, at - start, value & 0xffffffff)
                return
        raise ValueError('Unowned declared input ' + hex(at))
    bank(0x4ecbd0, 4); word(0x4ecbd0, 0x71000000)
    bank(0x71000000, 0x48); word(0x71000044, 0x71001000)
    bank(0x71001000, 128 * 12)
    identities = [0xa1700000 + n for n in range(max(1, depth))]
    for n, identity in enumerate(identities):
        body = 0x72000000 + n * 0x200
        bank(body, 0x140)
        word(0x71001000 + n * 12 + 8, body)
        banks[0x71001000][n * 12 + 3] = kind if n == len(identities) - 1 else 2
        word(body, 0x56780001)
        word(body + 0x14, identities[n + 1] if n + 1 < len(identities) else 0xffffffff)
        word(body + 0x13c, 0x91000017 if matching else 0x91000018)
    bank(0x4f0414, 4); word(0x4f0414, 0x73000000)
    bank(0x73000000, 32); word(0x73000018, 0x73001000)
    bank(0x73001000, 0x9c); word(0x73001094, count); word(0x73001098, 0x74000000)
    bank(0x74000000, 24 * 8)
    for n in range(8): word(0x74000000 + n * 24 + 8, 0x13570000 + n)
    bank(0x4c66f8, 4); word(0x4c66f8, view)
    bank(0x4f54f0, 4); word(0x4f54f0, 0x75000000)
    bank(0x75000000, 28)
    for n in range(4): word(0x7500000c + n * 4, 0x91000017)
    bank(0x4f5514, 4 * 320)
    for n in range(4):
        word(0x4f5514 + n * 320 + 4, blend)
        word(0x4f5514 + n * 320 + 12, callback)
    if view not in (0, 1, 2, 3, 0xffffffff):
        row = (0x4f5514 + view * 320) & 0xffffffff
        unit = (0x7500000c + view * 4) & 0xffffffff
        if not any(at <= row and row + 16 <= at + len(data) for at, data in banks.items()):
            bank(row, 16)
        if not any(at <= unit and unit + 4 <= at + len(data) for at, data in banks.items()):
            bank(unit, 4)
        word(row + 4, blend); word(row + 12, callback); word(unit, 0x91000017)
    bank(0x46a0e8, 4)
    bank(0x52d6a0, 1)[0] = gate
    bank(STACK, 4096); word(STACK + 0x800, STOP)
    owner = 0xffffffff if none else identities[0]
    gp = [0x11111111, owner, 0x33333333, 0x44444444, STACK + 0x800,
          0x66666666, owner, 0x88888888]
    if entry == 0xbac70: gp[0] = 0x12340000 | (index & 65535)
    if entry == 0x158290: gp[0] = view
    return dict(name=name, entry=entry, gp=gp, eflags=0x602, mxcsr=mxcsr,
                banks=[dict(address=at, bytes=data.hex()) for at, data in sorted(banks.items())])


def cases():
    for view in (0x00cccccd, 0x80000000):
        for entry in (0x158290, 0x3e9b0):
            for blend in (0, 0x80000000, 1, 0x7f812345):
                yield case(f'wrapping-view-{entry:x}-{view:x}-{blend:x}', entry, view=view, blend=blend)
    yield case('root-NONE', 0xbcfb0, none=True)
    for depth in (1, 2, 3, 8, 64): yield case('root-depth-' + str(depth), 0xbcfb0, depth=depth)
    for count in (-1, 0, 1, 8):
        for index in (-32768, -2, -1, 0, 1, 4, 7, 8, 32767):
            yield case(f'attachment-{count}-{index}', 0xbac70, count=count, index=index)
    blends = (0, 0x80000000, 0x3f800000, 0xbf800000, 1, 0x80000001,
              0x00800000, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345, 0xff812345)
    for view in (0, 1, 3):
        for mxcsr in (0x1f80, 0x1fc0, 0x9fc0):
            for blend in blends:
                yield case(f'camera-{view}-{mxcsr:x}-{blend:x}', 0x158290,
                           view=view, mxcsr=mxcsr, blend=blend)
        for callback in (0x245ea0, 0x16f6a0, 0x246380, 0x24637f, 0xdeadbeef):
            for gate in (0, 1, 0x80, 0xff):
                yield case(f'camera-kind-{view}-{callback:x}-{gate}', 0x158290,
                           view=view, callback=callback, gate=gate)
    yield case('priority-NONE', 0x3e9b0, none=True)
    yield case('priority-no-view', 0x3e9b0, view=0xffffffff)
    for depth in (1, 3):
        for view in (0, 2):
            for kind in (0, 2, 255):
                for matching in (False, True):
                    for blend in (0, 0x80000000, 0x3f800000, 0x7fc12345, 0x7f812345):
                        yield case(f'priority-{depth}-{view}-{kind}-{matching}-{blend:x}', 0x3e9b0,
                                   depth=depth, view=view, kind=kind, matching=matching, blend=blend)
            for callback in (0x245ea0, 0x16f6a0, 0x246380, 0xdeadbeef):
                for gate in (0, 1):
                    yield case(f'priority-category-{depth}-{view}-{callback:x}-{gate}', 0x3e9b0,
                               depth=depth, view=view, callback=callback, gate=gate)
    # Every source data-read prefix on representative complete paths.
    for template in (case('fault-root', 0xbcfb0, depth=3),
                     case('fault-attachment', 0xbac70, index=3),
                     case('fault-camera', 0x158290),
                     case('fault-priority', 0x3e9b0, depth=3)):
        yield template


def physical_reference(image, rows, scratch):
    """Execute the original bodies on physical i386, using the existing observer.

    In particular, Unicorn does not reproduce every UCOMISS MXCSR exception.
    Physical original execution owns those expectations; native code is never
    changed to imitate an emulator's missing denormal/invalid status bits.
    """
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'tools'))
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES = tuple(first for first, _ in RANGES)
    common.geometry.STOP = STOP
    metadata = common.metadata(image)
    by_pc = {}
    for site in metadata['sites']:
        if site['instruction'] in by_pc and by_pc[site['instruction']] != site:
            raise ValueError('Conflicting original instruction metadata')
        by_pc[site['instruction']] = site
    metadata['sites'] = list(by_pc.values())
    directory = scratch / 'physical-original'; directory.mkdir()
    section = image.section('.text')
    (directory / 'text.bin').write_bytes(image.section_bytes(section))
    (directory / 'metadata.json').write_text(json.dumps(metadata))
    regions = {'constants': (0x46a000, 4096), 'view': (0x4c6000, 4096),
        'object_binding': (0x4ec000, 4096), 'tag_binding': (0x4f0000, 4096),
        'camera': (0x4f5000, 4096), 'first_person': (0x52d000, 4096),
        'stack': (STACK, 4096), 'objects': (0x71000000, 8192),
        'bodies': (0x72000000, 32768), 'definitions': (0x73000000, 8192),
        'attachments': (0x74000000, 4096), 'view_units': (0x75000000, 4096)}
    for row in rows:
        for bank in row['banks']:
            first, size = bank['address'], len(bank['bytes']) // 2
            if not any(at <= first and first + size <= at + extent for at, extent in regions.values()):
                regions[f'extra_{first:x}'] = (first & ~4095, ((first % 4096 + size + 4095) // 4096) * 4096)
    assembly = f'.code32\n.section .original,"ax"\n.incbin "{directory / "text.bin"}"\n'
    for name, (_, size) in regions.items():
        assembly += f'.section .{name},"aw"\n.space {size}\n'
    assembly += '.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n'
    (directory / 'reference.s').write_text(assembly)
    linker = f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    linker += ''.join(f'.{name} 0x{at:x}:{{*(.{name})}} ' for name, (at, _) in regions.items())
    linker += f'.stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n'
    (directory / 'link.ld').write_text(linker)
    commands = [['as', '--32', str(directory / 'reference.s'), '-o', str(directory / 'reference.o')],
        ['ld', '-melf_i386', '-T', str(directory / 'link.ld'), str(directory / 'reference.o'), '-o', str(directory / 'reference')],
        ['c++', '-std=c++20', '-O2', str(root / 'tools/observe_native_campaign_view_rectangle_tu15.cpp'),
         '-o', str(directory / 'observer')]]
    for command in commands:
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode: raise ValueError('Physical ownership reference build failed: ' + result.stderr[-4000:])
    inputs = []
    for index, row in enumerate(rows):
        inputs.append(dict(id=index, name=row['name'], entry=row['entry'], gp=row['gp'], matrix=0, source=0,
            banks=[dict(address=b['address'], before=b['bytes']) for b in row['banks']],
            before_fp=[0x27f, 0, row['mxcsr']],
            live_x87=[struct.pack('<QH', 0x8000000000000000, 0x3ffd).hex(),
                      struct.pack('<QH', 0xe000000000000000, 0xbfff).hex()]))
    input_path, output_path = directory / 'inputs.json', directory / 'observed.json'
    input_path.write_text(json.dumps(inputs, separators=(',', ':')))
    command = [str(directory / 'observer'), str(directory / 'reference'), str(directory / 'metadata.json'),
               str(input_path), str(output_path)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=90)
    if result.returncode: raise ValueError('Physical ownership original failed: ' + (result.stdout + result.stderr)[-4000:])
    proof = json.loads(output_path.read_text())
    if proof['instruction_patches'] or proof['supplied_original_outcomes']:
        raise ValueError('Physical ownership reference supplied original instructions or results')
    return {row['name']: original for row, original in zip(rows, proof['cases'])}, dict(
        backend='physical-unchanged-i386-offline', cases=len(rows), owners=metadata['owners'],
        original_instructions=proof['original_instructions'], commands=commands + [command],
        observer_sha256=hashlib.sha256((root / 'tools/observe_native_campaign_view_rectangle_tu15.cpp').read_bytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not args.xbe.is_file():
        print('SKIP missing owned English TU1.5 executable'); return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest() != PIN:
        raise ValueError('Present executable is not OG Xbox English TU1.5')
    sys.path.insert(0, str(Path(__file__).resolve().parent / 'reference/halo2-decompiled/tools'))
    from xbe import Xbe
    import unicorn
    from unicorn import x86_const as R
    image = Xbe(args.xbe)
    if image.read(0x46a0e8, 4) != bytes(4):
        raise ValueError('Original camera comparison constant differs from declared zero input')
    complete_rows = list(cases())
    physical, physical_receipt = physical_reference(image, complete_rows, scratch)
    cpu = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections: cpu.mem_write(section.va, image.section_bytes(section))
    cpu.mem_map(STOP, 4096)
    pages, authenticated = set(), {}
    current = {}
    def instruction(g, pc, size, _):
        raw = bytes(g.mem_read(pc, size))
        if not any(first <= pc < limit for first, limit in RANGES) or raw != image.read(pc, size):
            raise ValueError('Ownership execution escaped unchanged original bodies at ' + hex(pc))
        authenticated[pc] = raw.hex()
    def read(g, _access, address, size, _value, _):
        current['reads'].append([address, size])
        if not STACK <= address < STACK + 4096:
            ordinal = current['data_reads']; current['data_reads'] += 1
            if ordinal == current['row'].get('reject_read'):
                current['failed'] = [address, size]
                raise RuntimeError('Declared original read boundary')
        if not any(at <= address and address + size <= at + len(data) for at, data in current['banks']):
            current['failed'] = [address, size]
            raise RuntimeError('Undeclared original data owner')
    def write(g, _access, address, size, value, _):
        current['stores'].append(dict(pc=g.reg_read(R.UC_X86_REG_EIP), address=address,
            before=bytes(g.mem_read(address, size)).hex(), after=value.to_bytes(size, 'little').hex()))
    cpu.hook_add(unicorn.UC_HOOK_CODE, instruction)
    cpu.hook_add(unicorn.UC_HOOK_MEM_READ, read)
    cpu.hook_add(unicorn.UC_HOOK_MEM_WRITE, write)
    registers = ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')
    def original(row):
        banks = [(b['address'], bytes.fromhex(b['bytes'])) for b in row['banks']]
        for at, data in banks:
            for page in range(at & ~4095, (at + len(data) + 4095) & ~4095, 4096):
                if page not in pages and not 0x10000 <= page < 0x810000:
                    cpu.mem_map(page, 4096); pages.add(page)
            cpu.mem_write(at, data)
        current.clear(); current.update(row=row, banks=banks, reads=[], stores=[], data_reads=0, failed=None)
        for name, value in zip(registers, row['gp']): cpu.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
        for n in range(8):
            raw = struct.pack('<4I', *(0x7fc12000 + n * 4 + lane for lane in range(4)))
            cpu.reg_write(R.UC_X86_REG_XMM0 + n, int.from_bytes(raw, 'little'))
        cpu.reg_write(R.UC_X86_REG_EFLAGS, row['eflags'])
        cpu.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        cpu.reg_write(R.UC_X86_REG_MXCSR, row['mxcsr'])
        stopped = False
        try: cpu.emu_start(row['entry'], STOP, count=10000)
        except RuntimeError:
            if current['failed'] is None: raise
            stopped = True
        if not stopped and cpu.reg_read(R.UC_X86_REG_EIP) != STOP:
            raise ValueError('Original ownership function did not return')
        return dict(stopped=stopped, failed=current['failed'], reads=current['reads'][:], stores=current['stores'][:],
            gp=[cpu.reg_read(getattr(R, 'UC_X86_REG_' + name)) for name in registers],
            eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS) & 0x8d5,
            xmm=[list(struct.unpack('<4I', cpu.reg_read(R.UC_X86_REG_XMM0 + n).to_bytes(16, 'little'))) for n in range(8)],
            mxcsr=cpu.reg_read(R.UC_X86_REG_MXCSR),
            after=[dict(address=at, bytes=bytes(cpu.mem_read(at, len(data))).hex()) for at, data in banks])
    diagnostics = scratch / 'probe.stderr'
    count = failures = data_reads = stores = 0
    emulator_fp_differences = []
    groups = {}
    comparison = hashlib.sha256()
    with diagnostics.open('w+') as errors:
        process = subprocess.Popen([str(args.native)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=errors, text=True)
        try:
            def compare(row):
                nonlocal count, failures, data_reads, stores
                expected = original(row)
                if row['name'] in physical:
                    hardware = physical[row['name']]
                    if not hardware['completed'] or not hardware['live_x87_unchanged'] or hardware['boundary_gp'] != expected['gp']:
                        raise ValueError(row['name'] + ': physical original completion/GP differs from emulated original')
                    hardware_images = [dict(address=b['address'], bytes=b['after']) for b in hardware['banks']]
                    if hardware_images != expected['after']:
                        raise ValueError(row['name'] + ': physical original memory differs from emulated original')
                    hardware_reads = [[r['address'], r['bytes']] for r in hardware['reads']]
                    expected_data = [r for r in expected['reads'] if not STACK <= r[0] < STACK + 4096]
                    hardware_data = [r for r in hardware_reads if not STACK <= r[0] < STACK + 4096]
                    if hardware_data != expected_data:
                        raise ValueError(row['name'] + ': physical original data reads differ from emulator')
                    if hardware['after_fp'][2] != expected['mxcsr']:
                        emulator_fp_differences.append(dict(name=row['name'], emulator=expected['mxcsr'], physical=hardware['after_fp'][2]))
                    expected['mxcsr'] = hardware['after_fp'][2]
                process.stdin.write(json.dumps(row, separators=(',', ':')) + '\n'); process.stdin.flush()
                line = process.stdout.readline()
                if not line:
                    errors.seek(0)
                    raise ValueError(row['name'] + ': native probe stopped: ' + errors.read()[-4000:])
                actual = json.loads(line)
                if actual.get('generated_bridge_verified') is not True:
                    raise ValueError(row['name'] + ': generated caller boundary was not verified')
                fields = ('stopped', 'failed', 'reads', 'stores', 'after', 'mxcsr')
                if not expected['stopped']: fields += ('gp', 'eflags', 'xmm')
                for field in fields:
                    if actual[field] != expected[field]:
                        left, right = str(actual[field]), str(expected[field])
                        raise ValueError(row['name'] + ': ' + field + ' differs: native=' + left[:800] + ' original=' + right[:800])
                count += 1; failures += expected['stopped']
                data_reads += len(expected['reads']); stores += len(expected['stores'])
                key = f'{row["entry"]:08x}'; groups[key] = groups.get(key, 0) + 1
                comparison.update(json.dumps(expected, sort_keys=True, separators=(',', ':')).encode())
                return expected
            for row in complete_rows:
                expected = compare(row)
                if row['name'].startswith('fault-'):
                    total = sum(not STACK <= access[0] < STACK + 4096 for access in expected['reads'])
                    for ordinal in range(total):
                        failure = copy.deepcopy(row); failure['reject_read'] = ordinal
                        failure['name'] += '-' + str(ordinal); compare(failure)
            process.stdin.close()
            if process.wait(timeout=10) != 0:
                errors.seek(0); raise ValueError('Native ownership probe failed: ' + errors.read()[-4000:])
        finally:
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.kill(); process.wait()
    report = dict(passed=True, cases=count, cases_by_entry=groups, verified_read_fault_prefixes=failures,
        complete_original_comparisons=count-failures, generated_bridge_comparisons=count,
        unsupported_fp_admission_checks=count*5, mmx_alias_preservation_cases=count-failures,
        ordered_reads=data_reads, ordered_stores=stores, comparison_sha256=comparison.hexdigest(),
        original_xbe_sha256=PIN, instruction_patches=[], supplied_outcomes=[],
        original_instruction_bytes={f'{pc:08x}': raw for pc, raw in sorted(authenticated.items())},
        physical_original=physical_receipt, emulator_fp_differences=emulator_fp_differences,
        whole_renderer_or_visual_parity_verified=False)
    args.report.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(f'PASS {count} unchanged-TU1.5 comparisons ({count-failures} complete, {failures} read-fault prefixes), full ABI/reads/stores')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
