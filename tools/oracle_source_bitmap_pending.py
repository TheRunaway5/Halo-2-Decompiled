#!/usr/bin/env python3
"""Complete unchanged 12D5A0 prefixes over explicit retained cache/job banks.

168 cases use the literal retained owner/partition/wait/force arguments.
Other cases are labeled conditional raw controls, not authored assets or live
pressure/scheduling history. No allocation, busy result, wait status, owned
node, I/O outcome, job ID or ready=true is supplied. Complete failures return;
pending callers stop before the first actual kernel instruction. The one
future callback metadata word is source-inspected and linked to the separate
complete owned-node binding proof, never labeled executed in this prefix.
"""
from pathlib import Path
import argparse, collections, hashlib, json, re, subprocess, sys, tempfile
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH, STACK, STOP, NONE = 0x20000000, 0x200fe000, 0x200ff000, 0xffffffff


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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    snapshot = Snapshot(ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    allocator_path = ROOT / 'analysis/multiplayer-shading/source-bitmap-allocator-snapshot-original.json'
    allocator = json.loads(allocator_path.read_text())
    assert allocator['cases'] == 168 and allocator['candidates'] == 42
    header_report_path = ROOT / 'analysis/multiplayer-shading/source-bitmap-pending-header-original.json'
    header_report = json.loads(header_report_path.read_text())
    assert header_report['cases'] == 19472
    manager_fixture = ROOT / 'tests/fixtures/retail_source_bitmap_manager_transition.tsv'
    initial, specs = {}, {}
    for line in manager_fixture.read_text().splitlines():
        if not line.startswith('B '):
            continue
        _, name, address, raw = line.split()
        raw, address = bytes.fromhex(raw), int(address, 16)
        assert name not in specs and snapshot.read(address, len(raw)) == raw
        specs[name], initial[name] = (address, len(raw)), raw
    manager = specs['manager'][0]
    incoming = sorted({int(row['bitmap_record'], 16) for row in allocator['records']})
    appended = []
    for address in incoming:
        if address not in [at for at, _ in specs.values()]:
            name = f'owner_{address:08X}'
            specs[name], initial[name] = (address, 116), snapshot.read(address, 116)
            appended.append(name)
    assert len(appended) == 4
    for name, address, size in [('async', 0x4e0364, 9616), ('loading', 0x4e6484, 4)]:
        specs[name], initial[name] = (address, size), snapshot.read(address, size)
    bank_bytes = sum(len(raw) for raw in initial.values())
    assert bank_bytes == 75144
    allowed = [(address, address + size) for address, size in specs.values()]
    assert all(end <= start2 or end2 <= start for i, (start, end) in enumerate(allowed)
        for start2, end2 in allowed[i + 1:])
    override = snapshot.read(0x4e6479, 1)[0]
    device = uint(snapshot.read(0x407488, 4))
    fence_pointer = uint(snapshot.read(device + 0x30, 4))
    queued, completed = uint(snapshot.read(device + 0x2c, 4)), uint(snapshot.read(fence_pointer, 4))
    physical_base = uint(snapshot.read(0x4e6460, 4))
    physical_bytes = uint(initial['manager'], 0x30) << uint(initial['manager'], 0x34)
    assert physical_base == 0x8066c000 and physical_bytes == 19402752
    # This is literal already-existing snapshot storage, not an allocation
    # supplied by this constructor. No prefix reads or writes these pixels.
    physical_sha = sha(snapshot.read(physical_base, physical_bytes))
    active = uint(snapshot.read(0x55aca8, 4))
    file_addresses = [0x557c90 + active * 0x804, 0x558c98, 0x557c90, 0x558494]
    files = [uint(snapshot.read(address, 4)) for address in file_addresses]
    assert files == [76, 72, 64, 68]
    free_mutex, work_mutex = uint(snapshot.read(0x4e0358, 4)), uint(snapshot.read(0x4e035c, 4))
    assert (free_mutex, work_mutex) == (4, 8)
    callback_store = image.read(0x1a0f90, 7)
    assert callback_store == bytes.fromhex('C74034700E1A00')
    inspected_callback = uint(callback_store[3:7])

    def owner_name(address):
        matches = [name for name, (at, size) in specs.items() if name.startswith('owner_') and at == address]
        assert len(matches) == 1
        return matches[0]

    cases = []

    def add(owner, partition, wait, force, priority, group, label, patches=None, **details):
        values = [owner, partition, wait, force, priority & NONE, override, queued, completed,
            physical_base, physical_base, physical_bytes] + files + [0x4e0364, 1, free_mutex, 1, work_mutex]
        assert len(values) == 20 and wait <= 255 and force <= 255
        cases.append(dict(values=values, group=group, label=label, patches=patches or [], details=details))

    for index, record in enumerate(allocator['records']):
        address = int(record['bitmap_record'], 16)
        assert initial[owner_name(address)].hex() == record['before_record_hex']
        add(address, record['partition'], record['wait'], record['force'], record['priority'],
            'retained_allocator_arguments', 'Literal retained owner bytes and original allocator caller arguments',
            allocator_record=index)
    first = allocator['records'][0]
    first_owner = int(first['bitmap_record'], 16)
    first_name = owner_name(first_owner)

    def owner_word(offset, value, size=4):
        return (first_name, offset, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

    for wait in [0, 1]:
        add(first_owner, 0, wait, 0, 2, 'completed_oversize_failure',
            'Explicit one byte beyond actual retained capacity; completed source failure and limit writes',
            [owner_word(0x34, physical_bytes + 1)])
    for format in [9, 10, 11]:
        for special in [0, 0x20]:
            for partition in range(3):
                add(first_owner, partition, 0, 0, 2, 'conditional_linear_headers',
                    'Explicit source metadata control; no authored image/D3D support claim',
                    [owner_word(0xc, format, 2), owner_word(0xe, 0x213 | special, 2)])
    for format in [10, 11, 16]:
        for kind in [0, 1, 2, 0xffff]:
            for special in [0, 0x20]:
                add(first_owner, 0, 0, 0, 2, 'conditional_type_and_format',
                    'Explicit source metadata control; exact header packing only',
                    [owner_word(0xc, format, 2), owner_word(0xa, kind, 2), owner_word(0xe, 0x203 | special, 2)])
    for selector in range(4):
        for offset in [0, 1, 0x3ffffffe, 0x3fffffff]:
            add(first_owner, 0, 0, 0, 2, 'conditional_file_location',
                'Explicit selector/offset token control; no I/O executes',
                [owner_word(0x1c, selector << 30 | offset)])
    for added in [0, 1, 0x7fffffff, NONE]:
        for counter in [0, NONE]:
            add(first_owner, 0, 0, 0, 2, 'conditional_request_counters',
                'Explicit actual counter words; no job counter advancement before acquisition',
                [('async', 0, packed([counter])), ('async', 9612, packed([added]))])
    for wait, force in [(2, 0), (0, 2), (2, 255), (255, 0)]:
        for priority in [0x80000000, 0x7fffffff]:
            add(first_owner, 0, wait, force, priority, 'conditional_noncanonical_booleans',
                'Explicit source byte controls; nonzero semantics and priority override')
    for age, wait in [(2, 1), (5, 0)]:
        for limit in [0, NONE]:
            add(first_owner, 0, wait, 0, 2, 'completed_immediate_limit_failure',
                'Explicit signed age limit; complete failure before source reservation scan',
                [('manager', 0x44 + age * 4, packed([limit]))])
    assert len(cases) == 248
    print(json.dumps(dict(cases=len(cases), bank_bytes_per_case=bank_bytes,
        checked_bank_bytes=bank_bytes * len(cases))), flush=True)

    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(SYNTH, 0x100000)
    pages, dirty, seen = {}, set(), {}
    boundaries, writes, events = [], [], []
    calls = collections.Counter()
    task_base, boundary_values, latest_delete = [None], [None], [NONE]
    mutation_ranges = allowed + [(0x4e6479, 0x4e647a), (device + 0x2c, device + 0x30),
        (fence_pointer, fence_pointer + 4), (0x4e6460, 0x4e6464)] + [(at, at + 4) for at in file_addresses]
    watch = {0x12d5a0, 0x13d370, 0x13d830, 0x16b840, 0x16b910, 0x16ba00, 0x16ba40,
        0x12d200, 0x12d1a0, 0x12d6d0, 0x359e0, 0x1365a0, 0x213760, 0x1a0f10, 0x1208b0}

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
        assert raw and any(start <= address and address + len(raw) <= end for start, end in mutation_ranges)
        read(address, len(raw))
        u.mem_write(address, raw)
        dirty.update(range(address & ~4095, ((address + len(raw) - 1) & ~4095) + 4096, 4096))

    def event(pc):
        return dict(entry=f'{pc:08X}', eax=u.reg_read(UC_X86_REG_EAX), ecx=u.reg_read(UC_X86_REG_ECX),
            ebx=u.reg_read(UC_X86_REG_EBX), esi=u.reg_read(UC_X86_REG_ESI),
            stack=words(read(u.reg_read(UC_X86_REG_ESP) + 4, 24)))

    def missing(g, access, address, size, value, data):
        try:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                page(at)
            return True
        except ValueError as error:
            boundaries.append(dict(kind='uncaptured memory', pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',
                address=f'{address:08X}', reason=str(error)))
            g.emu_stop()
            return False

    def code(g, pc, size, data):
        if pc == STOP:
            g.emu_stop()
            return
        if pc not in seen:
            try:
                original = image.read(pc, size)
            except (ValueError, KeyError):
                boundaries.append(dict(kind='actual kernel instruction not executed', **event(pc)))
                g.emu_stop()
                return
            assert read(pc, size) == original, (hex(pc), 'changed original instruction')
            assert not any(start < pc + size and pc < end for start, end in mutation_ranges)
            seen[pc] = size
        else:
            assert seen[pc] == size
        if pc in watch:
            calls[f'{pc:08X}'] += 1
            if pc not in [0x12d1a0, 0x16ba40]:
                events.append(event(pc))
        if pc == 0x13d830:
            latest_delete[0] = g.reg_read(UC_X86_REG_EBX)
        if pc == 0x1a0f10:
            task_base[0] = g.reg_read(UC_X86_REG_ESP) - 0x28
        if pc == 0x1208b0:
            esp = g.reg_read(UC_X86_REG_ESP)
            boundary_values[0] = [uint(read(esp + 0x48, 4)), uint(read(esp + 0x44, 4)), g.reg_read(UC_X86_REG_EBX)]
        if pc in [0x3fac30, 0x3fad50]:
            boundaries.append(dict(kind='hardware wait/kick not executed', **event(pc)))
            g.emu_stop()

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
        boundaries.clear()
        writes.clear()
        events.clear()
        task_base[0], boundary_values[0], latest_delete[0] = None, None, NONE
        for reg in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
            UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]:
            u.reg_write(reg, 0)
        u.reg_write(UC_X86_REG_ESP, STACK)
        u.reg_write(UC_X86_REG_EFLAGS, 2)
        u.reg_write(UC_X86_REG_FPCW, 0x023f)
        u.reg_write(UC_X86_REG_FPTAG, 0xffff)
        u.reg_write(UC_X86_REG_FPSW, 0)
        u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        u.mem_write(STACK - 0x4000, bytes(0x5000))

    base_lines = [f'B {name} {address:08X} {initial[name].hex()}' for name, (address, size) in sorted(specs.items())]
    fixture_lines, probe_lines, expected_lines, records = base_lines.copy(), base_lines.copy(), [], []
    totals = collections.Counter()
    for index, case in enumerate(cases):
        reset()
        row = case['values']
        for name, offset, raw in case['patches']:
            put(specs[name][0] + offset, raw)
        put(0x4e6479, bytes([row[5]]))
        put(device + 0x2c, packed([row[6]]))
        put(fence_pointer, packed([row[7]]))
        put(0x4e6460, packed([row[8]]))
        for address, value in zip(file_addresses, row[11:15]):
            put(address, packed([value]))
        before = {name: read(address, size) for name, (address, size) in specs.items()}
        u.reg_write(UC_X86_REG_EAX, row[2])
        u.reg_write(UC_X86_REG_ECX, row[0])
        u.mem_write(STACK, packed([STOP, row[1], row[3], row[4]]))
        try:
            u.emu_start(0x12d5a0, STOP, count=30000000)
        except UcError:
            if not boundaries:
                raise
        after = {name: read(address, size) for name, (address, size) in specs.items()}
        assert before['loading'] == after['loading'], 'unreturned source incremented loading'
        assert before['async'][:9612] == after['async'][:9612], 'unacquired source mutated node/head/job fields'
        observed_read, observed_task = None, None
        if not boundaries:
            assert u.reg_read(UC_X86_REG_EIP) == STOP and u.reg_read(UC_X86_REG_ESP) == STACK + 16
            assert u.reg_read(UC_X86_REG_EAX) & 255 == 0 and not calls['00213760']
            output = [0, NONE, NONE, 0, 0, 0] + [0] * 7 + [0] + [0] * 10 + [0] * 4
            assert before['async'] == after['async']
            totals['completed_source_failures'] += 1
        else:
            assert boundaries[0]['entry'] == '80020F5A', (index, boundaries)
            kernel_stack = boundaries[0]['stack'][:4]
            assert kernel_stack[1:] == [1, 0, 0]
            if kernel_stack[0] == row[17]:
                assert calls['00213760'] == 1 and task_base[0] and boundary_values[0]
                event_read = next(value for value in events if value['entry'] == '00213760')
                observed_read = [event_read['ecx']] + event_read['stack']
                observed_task = words(read(task_base[0], 40))
                priority, category, done = boundary_values[0]
                assert [observed_read[6], observed_read[5], observed_read[4]] == [priority, category, done]
                handle = uint(after[owner_name(row[0])], 0x28 + row[1] * 4)
                image_row = after['image_rows'][(handle & 65535) * 40:((handle & 65535) + 1) * 40]
                assert image_row[3] == 0 and uint(image_row, 8) == 0 and uint(image_row, 12) == 0
                assert uint(after['async'], 9612) == (uint(before['async'], 9612) + 1) & NONE
                output = [2, handle, NONE, 1, kernel_stack[0], 1] + observed_read + [1] + observed_task + [
                    priority, category, inspected_callback, done]
                totals['real_free_mutex_boundaries'] += 1
            else:
                assert kernel_stack[0] == row[19] and not calls['00213760'] and latest_delete[0] != NONE
                output = [1, NONE, latest_delete[0], 1, kernel_stack[0], 0] + [0] * 7 + [0] + [0] * 10 + [0] * 4
                assert before['async'] == after['async']
                totals['real_work_mutex_boundaries'] += 1
        assert len(output) == 28
        outputs = [(name, offset, raw) for name in sorted(specs) for offset, raw in differences(before[name], after[name])]
        prefix = 'P ' + ' '.join(f'{value:08X}' for value in row) + f' {len(case["patches"])}'
        for name, offset, raw in case['patches']:
            prefix += f' {name} {offset:X} {raw.hex()}'
        suffix = ' '.join(f'{value:08X}' for value in output) + f' {len(outputs)}'
        for name, offset, raw in outputs:
            suffix += f' {name} {offset:X} {raw.hex()}'
        fixture_lines.append(prefix + ' | ' + suffix)
        probe_lines.append(prefix)
        expected_lines.append(suffix)
        records.append(dict(index=index, group=case['group'], label=case['label'], details=case['details'],
            input_words=row, output_words=output, source_calls=dict(calls), observed_read=observed_read,
            observed_task=observed_task, observed_request_bank=boundary_values[0], boundaries=boundaries.copy(),
            source_write_events=len(writes), output_patch_count=len(outputs), output_sha256=sha(suffix.encode()),
            before_banks_sha256=sha(b''.join(before[name] for name in sorted(specs))),
            after_banks_sha256=sha(b''.join(after[name] for name in sorted(specs)))))
        if (index + 1) % 16 == 0 or index + 1 == len(cases):
            print(f'original pending prefixes complete: {index + 1}/{len(cases)}', flush=True)
    assert totals['completed_source_failures'] == 6 and totals['real_free_mutex_boundaries'] == 242
    reset()
    for at, raw in pages.items():
        assert bytes(u.mem_read(at, 4096)) == raw, f'unrestored original page {at:08X}'
    summary = dict(cases=len(records), checked_bank_bytes=bank_bytes * len(records), bank_bytes_per_case=bank_bytes,
        checked_result_words=28 * len(records), retained_cases=168, conditional_controls=80, outcomes=dict(totals),
        unique_instructions=len(seen), restored_pages_checked=len(pages),
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
            assert original.split() == output.split(), (index, records[index]['group'], original[:2000], output[:2000])
        validation = dict(probe_cases=len(actual), all_result_words_and_output_patches_exact=True)
    fixture = ROOT / 'tests/fixtures/retail_source_bitmap_pending.tsv'
    fixture.write_text('# B literal raw banks; P explicit inputs/patches | unchanged 12D5A0 prefix result words/full-bank patches. Callback word26 is separately source-inspected, not executed here.\n' +
        '\n'.join(fixture_lines) + '\n')
    if args.binary:
        normal = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True, check=True)
        assert normal.stdout.startswith('248 source pending cases; 18635712 bank bytes exact;')
        validation['normal'] = normal.stdout.strip()
        rejected = []
        with tempfile.TemporaryDirectory(prefix='halo2-bitmap-pending-corpus-') as temporary:
            temporary = Path(temporary)
            for label, content in [('missing', None), ('empty', ''), ('comment-only', '# no evidence\n'),
                ('truncated', '\n'.join(fixture_lines[:-1]) + '\n')]:
                path = temporary / (label + '.tsv')
                if content is not None:
                    path.write_text(content)
                outcome = subprocess.run([str(args.binary.resolve()), str(path)], text=True, capture_output=True)
                assert outcome.returncode != 0, (label, 'incomplete frozen evidence admitted')
                rejected.append(dict(kind=label, diagnostic=outcome.stderr.strip()))
        validation['incomplete_fixture_rejections'] = rejected
    report = dict(authority=__doc__, **summary, source_xbe_sha256=PIN, snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha((snapshot.directory / 'export.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()), allocator_report_sha256=sha(allocator_path.read_bytes()),
        header_report_sha256=sha(header_report_path.read_bytes()), manager_fixture_sha256=sha(manager_fixture.read_bytes()),
        fixture_sha256=sha(fixture.read_bytes()), native_binary_sha256=sha(args.binary.read_bytes()) if args.binary else None,
        helper_sha256=sha((ROOT / 'src/source_bitmap_pending.cpp').read_bytes()),
        header_sha256=sha((ROOT / 'include/halo2/source_bitmap_pending.hpp').read_bytes()),
        test_sha256=sha((ROOT / 'tests/source_bitmap_pending_tests.cpp').read_bytes()),
        dependencies={str(path): sha((ROOT / path).read_bytes()) for path in [
            'src/source_bitmap_reservation.cpp', 'src/source_bitmap_manager_transition.cpp',
            'src/source_bitmap_image_binding.cpp', 'src/source_bitmap_pending_header.cpp',
            'src/source_bitmap_async_read.cpp', 'src/source_async_jobs.cpp']},
        native_fixture_validation=validation, incoming_owner_records=incoming, appended_owner_banks=appended,
        initial_banks={name: dict(address=f'{address:08X}', bytes=size, sha256=sha(initial[name]))
            for name, (address, size) in sorted(specs.items())},
        physical_storage=dict(address=f'{physical_base:08X}', bytes=physical_bytes, sha256=physical_sha,
            pixel_access_or_initialization=False, source_allocation_result_supplied=False),
        file_bank=dict(active_index=active, addresses=[f'{value:08X}' for value in file_addresses], tokens=files),
        mutexes=dict(free=free_mutex, work=work_mutex, wait_executed=False,
            service='NtWaitForSingleObjectEx80020F5A', wait_mode='1 UserMode', alertable=0, timeout=0),
        async_layout=dict(counter='004E0364', nodes='004E0368', free='004E28E8', work='004E28EC', added='004E28F0',
            native_queue_bitcast_used=False, fields_mapped_independently=True),
        result_word_authority=dict(callback_index=26, source_instruction='001A0F90', bytes=callback_store.hex().upper(),
            executed_in_this_prefix=False, independent_owned_binding_proof='source-async-read-original.json:680 owned1A0F75 cases',
            other_words='Original observed boundaries/arguments/task outputs or API absent-value sentinels; no completed caller success/job value supplied.'),
        fpcw='023F', mxcsr='1F80', supplied_control_configuration=True,
        instruction_sha256=sha(b''.join(packed([pc]) + image.read(pc, size) for pc, size in sorted(seen.items()))),
        instruction_verification='Each distinct title PC/size is checked against literal XBE once. All original nonstack writes are restricted to the exact disjoint raw data banks, checked disjoint from reached code. Reset restores and reads every loaded original page.',
        services_replaced=[], source_instruction_patches=[], wait_or_kernel_result_supplied=False,
        native_residency_activation=False, original_history_or_pressure_claimed=False, records=records)
    path = ROOT / 'analysis/multiplayer-shading/source-bitmap-pending-original.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps({key: value for key, value in report.items()
        if key != 'records'}, indent=2) + '\n')


if __name__ == '__main__':
    main()
