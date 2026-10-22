#!/usr/bin/env python3
"""Execute complete unchanged 16B910/16BA00 over explicit addressed image banks.

168 inputs are exact frozen unchanged 13D370 post-reservation outputs. That
producer is not reexecuted here. Other inputs are the immutable retained pool
or explicitly labeled raw controls, not historical allocation events. No wait,
I/O, job ID, ready=true, kernel status, callback result, or instruction is
replaced. The output is only the image-pool binding phase before 12D5A0 fills
the pending image header and submits a read.
"""
from pathlib import Path
import argparse, collections, hashlib, json, re, subprocess, sys, tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, packed
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH, STACK, STOP, NONE = 0x20000000, 0x200fe000, 0x200ff000, 0xffffffff
NAMES = ['image_bits', 'image_pool', 'image_rows']


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def differences(before, after):
    out, at = [], 0
    assert len(before) == len(after)
    while at < len(before):
        if before[at] == after[at]:
            at += 1
            continue
        start = at
        while at < len(before) and before[at] != after[at]:
            at += 1
        out.append((start, after[start:at]))
    return out


def parse_patches(fields, at, bank):
    count = int(fields[at])
    at += 1
    for _ in range(count):
        name, offset, raw = fields[at:at + 3]
        at += 3
        offset, raw = int(offset, 16), bytes.fromhex(raw)
        assert offset + len(raw) <= len(bank[name])
        bank[name][offset:offset + len(raw)] = raw
    assert at == len(fields)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    snapshot = Snapshot(ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    manager_fixture = ROOT / 'tests/fixtures/retail_source_bitmap_manager_transition.tsv'
    reservation_fixture = ROOT / 'tests/fixtures/retail_source_bitmap_reservation.tsv'
    reservation_report = ROOT / 'analysis/multiplayer-shading/source-bitmap-reservation-original.json'
    reservation_provenance = reservation_fixture.with_suffix('.provenance.json')
    producer = json.loads(reservation_report.read_text())
    assert producer['cases'] == 392 and producer['complete_cases'] == 392
    assert producer['fixture_sha256'] == sha(reservation_fixture.read_bytes())
    assert json.loads(reservation_provenance.read_text())['fixture_sha256'] == producer['fixture_sha256']
    specs, initial, base_lines = {}, {}, []
    for line in manager_fixture.read_text().splitlines():
        if not line.startswith('B '):
            continue
        _, name, address, raw = line.split()
        if name not in NAMES:
            continue
        address, raw = int(address, 16), bytes.fromhex(raw)
        assert name not in specs and snapshot.read(address, len(raw)) == raw
        specs[name], initial[name] = (address, len(raw)), raw
        base_lines.append(line)
    assert sorted(specs) == NAMES
    assert specs['image_pool'][1] == 76 and specs['image_rows'][1] == 512 * 40 and specs['image_bits'][1] == 64
    pool = specs['image_pool'][0]
    assert uint(initial['image_pool'], 0x20) == 512 and uint(initial['image_pool'], 0x24) == 40
    assert uint(initial['image_pool'], 0x44) == specs['image_rows'][0]
    assert uint(initial['image_pool'], 0x48) == specs['image_bits'][0]
    assert sum(byte.bit_count() for byte in initial['image_bits']) == 286
    bank_bytes = sum(len(raw) for raw in initial.values())
    allowed = [(address, address + size) for address, size in specs.values()]
    assert all(end <= start2 or end2 <= start for i, (start, end) in enumerate(allowed)
        for start2, end2 in allowed[i + 1:])

    cases = []

    def add(handle, group, label, bank=None, repeats=1, **details):
        bank = initial if bank is None else bank
        patches = [(name, offset, raw) for name in NAMES
            for offset, raw in differences(initial[name], bytes(bank[name]))]
        cases.append(dict(handle=handle, repeats=repeats, group=group, label=label, patches=patches, details=details))

    reservation_initial = {}
    reservation_rows = []
    for line in reservation_fixture.read_text().splitlines():
        if line.startswith('B '):
            _, name, address, raw = line.split()
            reservation_initial[name] = bytes.fromhex(raw)
        elif line.startswith('R '):
            reservation_rows.append(line)
    assert len(reservation_rows) == 392
    for index, line in enumerate(reservation_rows[:168]):
        source_record = producer['records'][index]
        assert source_record['group'] == 'retained_allocator_requests' and source_record['completed']
        input_fields, output_fields = [part.split() for part in line.split('|')]
        bank = {name: bytearray(raw) for name, raw in reservation_initial.items()}
        parse_patches(input_fields, 6, bank)
        assert all(sha(raw) == source_record['bank_before_sha256'][name] for name, raw in bank.items())
        assert output_fields[0] == '0' and output_fields[2] == 'FFFFFFFF'
        handle = int(output_fields[1], 16)
        assert handle != NONE and output_fields[1] == source_record['returned_handle']
        parse_patches(output_fields, 3, bank)
        assert all(sha(raw) == source_record['bank_after_sha256'][name] for name, raw in bank.items())
        part = handle & 65535
        assert bank['image_rows'][part * 40:part * 40 + 2] == b'\0\0'
        assert not bank['image_bits'][part >> 3] & (1 << (part & 7))
        assert int.from_bytes(bank['physical_rows'][part * 24:part * 24 + 2], 'little') == handle >> 16
        add(handle, 'frozen_post_reservation', 'Exact frozen unchanged 13D370 output, supplied as complete 16B910 input', bank,
            reservation_record=index, producer_output_sha256=source_record['output_sha256'], **source_record['details'])
        cases[-1]['paired_physical_bits'] = bytes(bank['physical_bits'])
        cases[-1]['paired_physical_count'] = uint(bank['physical_pool'], 0x3c)
        cases[-1]['paired_physical_rows'] = bytes(bank['physical_rows'])

    for index in range(512):
        salt = int.from_bytes(initial['image_rows'][index * 40:index * 40 + 2], 'little')
        handle = ((salt if salt else 0xe80c) << 16) | index
        add(handle, 'retained_slots', 'Immutable retained image pool with explicit supplied handle; no historical event',
            retained_index=index, retained_row_salt=f'{salt:04X}')
    for index in [512, 513, 0x7fff, 0x8000, 0xffff]:
        for salt in [0, 1, 0x7fff, 0x8000, 0xfffe, 0xffff]:
            add(salt << 16 | index, 'invalid_indices', 'Explicit supplied low16 index outside actual 512-slot layout')

    free_index = 286

    def controlled():
        bank = {name: bytearray(raw) for name, raw in initial.items()}
        offset = free_index * 40
        bank['image_rows'][offset:offset + 40] = bytes([0xa5]) * 40
        bank['image_rows'][offset:offset + 2] = b'\0\0'
        bank['image_rows'][offset + 3] = initial['image_rows'][offset + 3]
        return bank

    def field(bank, at, value):
        bank['image_pool'][at:at + 4] = packed([value])

    for supplied_salt in [0, 1, 0x7fff, 0x8000, 0xfffe, 0xffff]:
        for next_salt in [0, 1, 0x7fff, 0x8000, 0xfffd, 0xfffe, 0xffff]:
            bank = controlled()
            bank['image_pool'][0x40:0x44] = next_salt.to_bytes(2, 'little') + bytes.fromhex('efbe')
            add(supplied_salt << 16 | free_index, 'supplied_and_generated_salts',
                'Explicit independent supplied salt and generated pool salt; unused upper word sentinel', bank,
                supplied_salt=f'{supplied_salt:04X}', generated_next_salt=f'{next_salt:04X}')
    for high in [0, free_index, free_index + 1, 512, 0x80000000, NONE]:
        for count in [0, NONE]:
            for occupied in [False, True]:
                bank = controlled()
                field(bank, 0x34, NONE)
                field(bank, 0x38, high)
                field(bank, 0x3c, count)
                bank['image_bits'][free_index >> 3] &= ~(1 << (free_index & 7))
                if occupied:
                    bank['image_bits'][free_index >> 3] |= 1 << (free_index & 7)
                add(0xe80c0000 | free_index, 'raw_counter_highwater_bit',
                    'Explicit conditional raw counters/high-water/occupied bit; source tests only zero row salt', bank,
                    high_water=f'{high:08X}', count=f'{count:08X}', occupied_bit=occupied)
    for salt in [0, 1, 0x7fff, 0x8000, 0xfffe, 0xffff]:
        add(salt << 16, 'occupied_salt_aliases', 'Actual occupied row zero rejects all caller upper16 aliases')
    for debug in [False, True]:
        bank = controlled()
        bank['image_pool'][0x2a] &= ~8
        if debug:
            bank['image_pool'][0x2a] |= 8
        add(0xe80c0000 | free_index, 'debug_flag_ignored',
            'Explicit pool debug bit control; complete 16B910/16BA00 never reads it', bank, flag_bit3=debug)
    for next_salt in [0xfffe, 0xffff]:
        bank = controlled()
        bank['image_pool'][0x40:0x42] = next_salt.to_bytes(2, 'little')
        add(free_index, 'repeated_supplied_zero_salt',
            'Two complete calls; supplied zero leaves row salt zero, so occupied bit does not block the second call',
            bank, repeats=2, generated_next_salt=f'{next_salt:04X}')
    bank = {name: bytearray(raw) for name, raw in initial.items()}
    bank['image_rows'][0:2] = b'\0\0'
    add(0, 'zero_return_handle', 'Explicit zero-salt row zero: EAX zero is a successful handle, not NONE', bank)
    assert len(cases) == 787 and bank_bytes * len(cases) == 16227940

    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(SYNTH, 0x100000)
    pages, dirty, seen = {}, set(), {}
    calls, writes = collections.Counter(), []

    def page(address):
        at = address & ~4095
        if at not in pages:
            raw = snapshot.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw

    def read(address, size):
        assert size > 0
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            if not SYNTH <= at < SYNTH + 0x100000:
                page(at)
        return bytes(u.mem_read(address, size))

    def put(address, raw):
        assert raw and any(start <= address and address + len(raw) <= end for start, end in allowed)
        read(address, len(raw))
        u.mem_write(address, raw)
        dirty.update(range(address & ~4095, ((address + len(raw) - 1) & ~4095) + 4096, 4096))

    def missing(g, access, address, size, value, data):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            page(at)
        return True

    def code(g, pc, size, data):
        if pc == STOP:
            g.emu_stop()
            return
        # Every reached instruction is literal XBE, including every repetition.
        assert read(pc, size) == image.read(pc, size), (hex(pc), 'changed original instruction')
        assert not any(start < pc + size and pc < end for start, end in allowed)
        seen[pc] = size
        if pc in [0x16b910, 0x16ba00]:
            calls[f'{pc:08X}'] += 1

    def writing(g, access, address, size, value, data):
        if not SYNTH <= address < SYNTH + 0x100000:
            assert any(start <= address and address + size <= end for start, end in allowed), (hex(address), size)
            dirty.update(range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096))
            writes.append((address, size))

    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga, gl = (int(value, 16) for value in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',
        snapshot.report['registers_before']).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for reg, value in [(UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16),
        (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)]:
        u.reg_write(reg, value)
    for address, size in specs.values():
        read(address, size)

    def reset():
        for at in dirty:
            u.mem_write(at, pages[at])
        dirty.clear()
        calls.clear()
        writes.clear()
        for reg in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
            UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]:
            u.reg_write(reg, 0)
        for reg, value in [(UC_X86_REG_EBX, 0x11112222), (UC_X86_REG_ESI, 0x33334444),
            (UC_X86_REG_EDI, 0x55556666), (UC_X86_REG_EBP, 0x77778888)]:
            u.reg_write(reg, value)
        u.reg_write(UC_X86_REG_ESP, STACK)
        u.reg_write(UC_X86_REG_EFLAGS, 2)
        u.reg_write(UC_X86_REG_FPCW, 0x023f)
        u.reg_write(UC_X86_REG_FPTAG, 0xffff)
        u.reg_write(UC_X86_REG_FPSW, 0)
        u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        u.mem_write(STACK - 0x1000, bytes(0x2000))

    records, fixture_lines, probe_lines, expected_lines = [], base_lines.copy(), base_lines.copy(), []
    total_calls = collections.Counter()
    for index, case in enumerate(cases):
        reset()
        for name, offset, raw in case['patches']:
            put(specs[name][0] + offset, raw)
        before = {name: read(address, size) for name, (address, size) in specs.items()}
        for repeat in range(case['repeats']):
            u.reg_write(UC_X86_REG_ESP, STACK)
            u.reg_write(UC_X86_REG_EAX, case['handle'])
            u.reg_write(UC_X86_REG_EDX, pool)
            u.mem_write(STACK, packed([STOP]))
            u.emu_start(0x16b910, STOP, count=10000)
            assert u.reg_read(UC_X86_REG_EIP) == STOP and u.reg_read(UC_X86_REG_ESP) == STACK + 4
            assert [u.reg_read(reg) for reg in [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]] == [
                0x11112222, 0x33334444, 0x55556666, 0x77778888]
        returned = u.reg_read(UC_X86_REG_EAX)
        after = {name: read(address, size) for name, (address, size) in specs.items()}
        assert after['image_pool'][0x34:0x38] == before['image_pool'][0x34:0x38], 'source changed first-free'
        if not calls['0016BA00']:
            assert returned == NONE and after == before
        else:
            assert returned == case['handle']
            row = after['image_rows'][(returned & 65535) * 40:((returned & 65535) + 1) * 40]
            assert row == (returned >> 16).to_bytes(2, 'little') + bytes(38)
        if case['group'] == 'frozen_post_reservation':
            assert after['image_bits'] == case['paired_physical_bits']
            assert uint(after['image_pool'], 0x3c) == case['paired_physical_count']
            for slot in range(512):
                if case['paired_physical_bits'][slot >> 3] & (1 << (slot & 7)):
                    assert after['image_rows'][slot * 40:slot * 40 + 2] == case['paired_physical_rows'][slot * 24:slot * 24 + 2]
        outputs = [(name, offset, raw) for name in NAMES for offset, raw in differences(before[name], after[name])]
        prefix = f'I {case["handle"]:08X} {case["repeats"]} {len(case["patches"])}'
        for name, offset, raw in case['patches']:
            prefix += f' {name} {offset:X} {raw.hex()}'
        output = f'{returned:08X} {len(outputs)}'
        for name, offset, raw in outputs:
            output += f' {name} {offset:X} {raw.hex()}'
        fixture_lines.append(prefix + ' | ' + output)
        probe_lines.append(prefix)
        expected_lines.append(output)
        total_calls.update(calls)
        records.append(dict(index=index, group=case['group'], label=case['label'], details=case['details'],
            input_handle=f'{case["handle"]:08X}', repetitions=case['repeats'], returned_handle=f'{returned:08X}',
            completed=True, source_calls=dict(calls), source_write_events=len(writes),
            input_patch_count=len(case['patches']), output_patch_count=len(outputs), output_sha256=sha(output.encode()),
            bank_before_sha256={name: sha(raw) for name, raw in before.items()},
            bank_after_sha256={name: sha(raw) for name, raw in after.items()}))
    reset()
    for at, raw in pages.items():
        assert bytes(u.mem_read(at, 4096)) == raw, f'unrestored original page {at:08X}'
    summary = dict(cases=len(records), original_calls=sum(row['repetitions'] for row in records), complete_cases=len(records),
        frozen_post_reservation_cases=168, retained_slot_cases=512, conditional_control_cases=len(records) - 680,
        bank_bytes_per_case=bank_bytes, checked_bank_bytes=bank_bytes * len(records),
        source_calls=dict(total_calls), unique_instructions=len(seen), restored_pages_checked=len(pages),
        groups=dict(collections.Counter(row['group'] for row in records)))
    print(json.dumps(summary, indent=2), flush=True)
    if args.dry_run:
        return
    validation = None
    if args.binary:
        native = subprocess.run([str(args.binary.resolve()), '--probe'], input='\n'.join(probe_lines) + '\n',
            text=True, capture_output=True, check=True)
        actual = native.stdout.splitlines()
        assert len(actual) == len(expected_lines)
        for index, (original, output) in enumerate(zip(expected_lines, actual)):
            assert original.split() == output.split(), (index, records[index]['group'], original, output)
        validation = dict(probe_cases=len(actual), all_output_patches_exact=True)
    fixture = ROOT / 'tests/fixtures/retail_source_bitmap_image_binding.tsv'
    fixture.write_text('# B literal immutable image banks; I supplied handle/repetitions/input patches | unchanged 16B910 EAX/full-bank patches.\n' +
        '\n'.join(fixture_lines) + '\n')
    if args.binary:
        normal = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True, check=True)
        assert normal.stdout.strip() == '787 source image binding cases; 16227940 bank bytes exact; 17 admission/early-return controls'
        validation['normal'] = normal.stdout.strip()
        rejected = []
        with tempfile.TemporaryDirectory(prefix='halo2-bitmap-image-binding-corpus-') as temporary:
            temporary = Path(temporary)
            for label, content in [('missing', None), ('empty', ''), ('truncated', '\n'.join(fixture_lines[:-1]) + '\n')]:
                path = temporary / (label + '.tsv')
                if content is not None:
                    path.write_text(content)
                outcome = subprocess.run([str(args.binary.resolve()), str(path)], text=True, capture_output=True)
                assert outcome.returncode != 0, (label, 'incomplete frozen evidence admitted')
                rejected.append(dict(kind=label, diagnostic=outcome.stderr.strip()))
        validation['incomplete_fixture_rejections'] = rejected
    report = dict(authority=__doc__, **summary, source_entry='0016B910', source_constructor='0016BA00',
        source_xbe_sha256=PIN, snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha((snapshot.directory / 'export.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()), manager_fixture_sha256=sha(manager_fixture.read_bytes()),
        reservation_fixture_sha256=sha(reservation_fixture.read_bytes()),
        reservation_report_sha256=sha(reservation_report.read_bytes()),
        reservation_provenance_sha256=sha(reservation_provenance.read_bytes()), fixture_sha256=sha(fixture.read_bytes()),
        native_binary_sha256=sha(args.binary.read_bytes()) if args.binary else None,
        helper_sha256=sha((ROOT / 'src/rendering/source_bitmap_image_binding.cpp').read_bytes()),
        header_sha256=sha((ROOT / 'include/halo2/source_bitmap_image_binding.hpp').read_bytes()),
        test_sha256=sha((ROOT / 'tests/source_bitmap_image_binding_tests.cpp').read_bytes()),
        native_fixture_validation=validation,
        initial_banks={name: dict(address=f'{address:08X}', bytes=size, sha256=sha(initial[name]))
            for name, (address, size) in sorted(specs.items())},
        fpcw='023F', mxcsr='1F80', supplied_control_configuration=True,
        instruction_sha256=sha(b''.join(packed([pc]) + image.read(pc, size) for pc, size in sorted(seen.items()))),
        instruction_verification='Every reached title instruction is checked against literal XBE on every execution. Original nonstack writes are confined to the three full data banks, disjoint from code. Reset restores and reads every loaded original page. No instruction or service result is patched.',
        abi='EDX actual pool, EAX supplied salted handle; complete RET returns EAX supplied handle or FFFFFFFF. EBX/ESI/EDI preserved by original title routine.',
        admission='Separate addressed 76-byte header, 512 by 40 rows, 64-byte bit bank. Only layout and consumed storage are checked; count/popcount, first-free, supplied salt, occupied bit and unused flags are not invented admission conditions.',
        producer_chain='The first 168 frozen 13D370 rows and all bank hashes are validated against the final reservation report. Those literal post-reservation bytes become explicit inputs; only complete 16B910/16BA00 is executed here.',
        services_replaced=[], source_instruction_patches=[], read_completion_or_ready_result_manufactured=False,
        completed_read_job_or_pending_header_outcome_supplied=False, original_history_or_pressure_claimed=False,
        native_residency_activation=False, records=records)
    path = ROOT / 'analysis/multiplayer-shading/source-bitmap-image-binding-original.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps({key: value for key, value in report.items()
        if key != 'records'}, indent=2) + '\n')


if __name__ == '__main__':
    main()
