#!/usr/bin/env python3
"""Execute unchanged complete13D370 over explicit literal texture-manager banks.

The first170 cases use immutable retained metadata and source allocator caller
arguments (168 requests plus two oversize controls). Separate conditional raw
metadata controls admit coherent ownerNULL/no-job chains; their ready bytes
remain unchanged and unused. These controls assert no historical producer,
kernel completion, resident pixels, or original scheduling timeline.
No wait, callback result, fence result, allocation result or instruction is
replaced. Actual kernel/hardware entries stop before their first instruction.
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
    spans, at = [], 0
    while at < len(before):
        if before[at] == after[at]:
            at += 1
            continue
        start = at
        while at < len(before) and before[at] != after[at]:
            at += 1
        spans.append((start, after[start:at]))
    return spans


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--dry-run', action='store_true', help='Run source without writing the fixture/report.')
    args = parser.parse_args()
    snapshot = Snapshot(ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    manager_fixture = ROOT / 'tests/fixtures/retail_source_bitmap_manager_transition.tsv'
    base_lines, specs, initial, context = [], {}, {}, None
    for line in manager_fixture.read_text().splitlines():
        if line.startswith('B '):
            _, name, address, raw = line.split()
            raw = bytes.fromhex(raw)
            assert name not in specs
            specs[name] = (int(address, 16), len(raw))
            initial[name] = raw
            assert snapshot.read(specs[name][0], len(raw)) == raw, name
            base_lines.append(line)
        elif line.startswith('G '):
            _, override, queued, completed = line.split()
            context = tuple(int(value, 16) for value in [override, queued, completed])
            base_lines.append(line)
    assert context is not None and len(specs) > 7
    manager = specs['manager'][0]
    assert uint(initial['manager'], 0x2c) == 2 and uint(initial['manager'], 0x34) == 12
    assert words(initial['manager'][0x20:0x2c]) == [0x12d200, 0x12d1a0, 0x12d160]
    assert initial['image_bits'] == initial['physical_bits']
    assert sum(byte.bit_count() for byte in initial['physical_bits']) == 286
    bank_bytes = sum(len(raw) for raw in initial.values())
    allowed = [(address, address + size) for address, size in specs.values()]
    mutation_ranges = allowed.copy()

    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(SYNTH, 0x100000)
    pages, dirty, seen = {}, set(), {}
    writes, boundaries, calls, deletions, metrics = [], [], collections.Counter(), [], collections.Counter()
    branch_hits = collections.Counter()
    pending = [NONE]

    def page(address):
        at = address & ~4095
        if at not in pages:
            raw = snapshot.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw

    def read(address, size):
        if not size:
            return b''
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            if not SYNTH <= at < SYNTH + 0x100000:
                page(at)
        return bytes(u.mem_read(address, size))

    def put(address, raw):
        if not raw:
            return
        assert any(start <= address and address + len(raw) <= end for start, end in mutation_ranges)
        read(address, len(raw))
        u.mem_write(address, raw)
        for at in range(address & ~4095, ((address + len(raw) - 1) & ~4095) + 4096, 4096):
            if not SYNTH <= at < SYNTH + 0x100000:
                dirty.add(at)

    def event(pc):
        return dict(entry=f'{pc:08X}', stack=words(read(u.reg_read(UC_X86_REG_ESP) + 4, 16)),
            ebx=u.reg_read(UC_X86_REG_EBX), edi=u.reg_read(UC_X86_REG_EDI))

    def missing(g, access, address, size, value, data):
        try:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                page(at)
            return True
        except ValueError as error:
            boundaries.append(dict(kind='uncaptured memory/MMIO', pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',
                address=f'{address:08X}', reason=str(error)))
            g.emu_stop()
            return False

    watch = {0x13d370, 0x13d320, 0x13d830, 0x16b840, 0x16ba40, 0x320560,
        0x12d160, 0x12d1a0, 0x12d200, 0x3fb710, 0x3fad50, 0x3fac30}
    branch_addresses = {0x13d458, 0x13d6d5, 0x13d703, 0x13d705, 0x13d6ec,
        0x13d6ef, 0x13d6f1, 0x13d6f6, 0x13d6f9, 0x13d6fd, 0x13d701, 0x13d76e, 0x16b8e7, 0x16b8ee}
    observed_addresses = watch | branch_addresses | {0x12d1d8, 0x12d1f0, 0x13d458,
        0x13d6ec, 0x13d703, 0x13d751, 0x13d426, 0x13d5f3}

    def code(g, pc, size, data):
        if pc == STOP:
            g.emu_stop()
            return
        if pc in seen:
            assert seen[pc] == size
            if pc not in observed_addresses:
                return
        else:
            try:
                original = image.read(pc, size)
            except (ValueError, KeyError):
                boundaries.append(dict(kind='actual kernel entry; not executed', **event(pc)))
                g.emu_stop()
                return
            assert read(pc, size) == original, (hex(pc), 'changed original instruction')
            assert not any(start < pc + size and pc < end for start, end in mutation_ranges)
            seen[pc] = size
        if pc in watch:
            calls[f'{pc:08X}'] += 1
        if pc in branch_addresses:
            branch_hits[f'{pc:08X}'] += 1
        if pc == 0x13d830:
            pending[0] = g.reg_read(UC_X86_REG_EBX)
            deletions.append(pending[0])
        if pc in [0x12d1d8, 0x12d1f0]:
            metrics['busy_true' if g.reg_read(UC_X86_REG_EAX) & 255 else 'busy_false'] += 1
        if pc == 0x13d458 and g.reg_read(UC_X86_REG_ECX) == g.reg_read(UC_X86_REG_EDX):
            metrics['candidate_ring_full'] += 1
        if pc == 0x13d6ec:
            metrics['oldest_predecessor_repair'] += 1
        if pc == 0x13d703:
            metrics['extra_oldest_delete'] += 1
        if pc == 0x13d751:
            metrics['overlap_delete'] += 1
        if pc == 0x13d426:
            metrics['failure_limit_store'] += 1
        if pc == 0x13d5f3:
            metrics['wrapping_weighted_rank'] += 1
        if pc == 0x13d76e and g.reg_read(UC_X86_REG_EAX) == NONE:
            metrics['failed_slot_allocation'] += 1
        if pc in [0x3fad50, 0x3fac30]:
            boundaries.append(dict(kind='hardware wait/kick entry; not executed', **event(pc)))
            g.emu_stop()

    def writing(g, access, address, size, value, data):
        if not SYNTH <= address < SYNTH + 0x100000:
            assert any(start <= address and address + size <= end for start, end in allowed), (hex(address), size)
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                dirty.add(at)
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
    device = uint(read(0x407488, 4))
    completed_pointer = uint(read(device + 0x30, 4))
    mutation_ranges.extend([(0x4e6479, 0x4e647a), (device + 0x2c, device + 0x30),
        (completed_pointer, completed_pointer + 4)])
    assert context == (read(0x4e6479, 1)[0], uint(read(device + 0x2c, 4)), uint(read(completed_pointer, 4)))
    for address, size in specs.values():
        read(address, size)

    def reset():
        for at in dirty:
            u.mem_write(at, pages[at])
        dirty.clear()
        writes.clear()
        boundaries.clear()
        calls.clear()
        deletions.clear()
        metrics.clear()
        branch_hits.clear()
        pending[0] = NONE
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

    def field(raw, at, value):
        raw[at:at + 4] = packed([value & NONE])

    def conditional_chain(count, *, clock=1024, salt_counter=None, reversed_slots=False, timestamps=None):
        bank = {name: bytearray(raw) for name, raw in initial.items()}
        physical, images = bank['physical_rows'], bank['image_rows']
        for index in range(512):
            physical[index * 24:index * 24 + 2] = b'\0\0'
            images[index * 40:index * 40 + 2] = b'\0\0'
        order = list(range(count))
        if reversed_slots:
            order.reverse()
        handles = {index: (0x8000 + index) << 16 | index for index in order}
        for ordinal, index in enumerate(order):
            p, im = index * 24, index * 40
            value = handles[index]
            physical[p:p + 2] = images[im:im + 2] = (value >> 16).to_bytes(2, 'little')
            field(physical, p + 4, 1)
            field(physical, p + 8, ordinal * 2)
            field(physical, p + 12, handles[order[ordinal + 1]] if ordinal + 1 < count else NONE)
            field(physical, p + 16, handles[order[ordinal - 1]] if ordinal else NONE)
            field(physical, p + 20, timestamps[ordinal] if timestamps else 1)
            images[im + 2] = 0
            # image+3 remains the immutable row byte; no readiness input/result
            # is changed, and no-job query/ownerNULL delete never consumes it.
            field(images, im + 12, NONE)
            field(images, im + 16, 0)
        bits = bytearray(64)
        for index in order:
            bits[index >> 3] |= 1 << (index & 7)
        bank['physical_bits'] = bits.copy()
        bank['image_bits'] = bits.copy()
        high = max(order) + 1 if order else 0
        first_free = next((index for index in range(512) if index not in handles), 512)
        for name in ['physical_pool', 'image_pool']:
            field(bank[name], 0x34, first_free)
            field(bank[name], 0x38, high)
            field(bank[name], 0x3c, count)
        if salt_counter is not None:
            bank['physical_pool'][0x40:0x42] = salt_counter.to_bytes(2, 'little')
        field(bank['manager'], 0x30, 1024 if count != 4 else 8)
        field(bank['manager'], 0x38, clock)
        field(bank['manager'], 0x3c, handles[order[0]] if order else NONE)
        field(bank['manager'], 0x40, handles[order[-1]] if order else NONE)
        for at in range(0x44, 0x64, 4):
            field(bank['manager'], at, 0x7fffffff)
        return {name: bytes(raw) for name, raw in bank.items()}

    cases = []

    def add(size, age, group, label, bank=None, **details):
        assert 0 < size <= 0x7fffffff and age in [2, 5]
        bank = initial if bank is None else bank
        assert bank['image_rows'][3::40] == initial['image_rows'][3::40]
        patches = [(name, 0, raw) for name, raw in sorted(bank.items()) if raw != initial[name]]
        cases.append(dict(size=size, age=age, group=group, label=label, patches=patches, details=details))

    allocator_path = ROOT / 'analysis/multiplayer-shading/source-bitmap-allocator-snapshot-original.json'
    allocator = json.loads(allocator_path.read_text())
    assert len(allocator['records']) == 168
    for index, record in enumerate(allocator['records']):
        call = next(row for row in record['source_calls'] if row['entry'] == '0013D370')
        assert call['stack'][0] == manager and call['stack'][1] == record['authored_bytes']
        add(call['stack'][1], call['stack'][2], 'retained_allocator_requests',
            'Literal allocator caller arguments over immutable retained banks; diagnostic scheduling only',
            allocator_record=index, bitmap=record['bitmap_record'], partition=record['partition'])
    for record in allocator['failure_controls']:
        add(record['size'], record['age'], 'retained_oversize',
            'One byte beyond the literal retained page capacity')
    capacity_bytes = uint(initial['manager'], 0x30) << 12
    for size in [1, 4095, 4096, 4097, 0x40000, capacity_bytes, capacity_bytes + 1, 0x7fffffff]:
        for age in [2, 5]:
            add(size, age, 'retained_request_edges', 'Explicit positive caller byte-count/rounding control over retained banks')
    for count in [0, 1, 2, 254, 255, 256, 257, 511, 512]:
        bank = conditional_chain(count)
        for requested_pages in [1, 2, 255, 256, 257, 600, 1024, 1025]:
            for age in [2, 5]:
                add(requested_pages << 12, age, 'conditional_ring_and_capacity',
                    'Explicit coherent ownerNULL/no-job metadata chain; no historical producer or readiness claim',
                    bank, occupied=count, requested_pages=requested_pages)
    for count in [0, 1, 512]:
        for next_salt in [0x8000, 0xfffd, 0xfffe]:
            bank = conditional_chain(count, salt_counter=next_salt)
            for age in [2, 5]:
                add(4096, age, 'conditional_next_salt', 'Explicit physical next-salt raw metadata input', bank,
                    occupied=count, next_salt=f'{next_salt:04X}')
    for limit in [0, 1, 2, 0x7fffffff, 0x80000000, NONE]:
        for requested_pages in [1, 2]:
            for age in [2, 5]:
                bank = conditional_chain(0)
                changed = bytearray(bank['manager'])
                field(changed, 0x44 + age * 4, limit)
                bank['manager'] = bytes(changed)
                add(requested_pages << 12, age, 'conditional_failure_limits',
                    'Explicit signed source age-limit raw metadata input', bank,
                    limit=f'{limit:08X}', requested_pages=requested_pages)
    for clock in [1, 0x7fffffff, 0x80000000, NONE]:
        for pattern in range(2):
            for age in [2, 5]:
                times = [((clock - age - index - 1) & NONE) for index in range(4)] if not pattern else [0xfffffff8, 0, 0x80000000, 1]
                add(3 << 12, age, 'conditional_wrapping_time',
                    'Explicit raw wrapping clock/timestamps; no captured chronological age claim',
                    conditional_chain(4, clock=clock, timestamps=times), clock=f'{clock:08X}', timestamps=times)
    for age in [2, 5]:
        add(4096, age, 'conditional_reversed_slots',
            'Explicit full chain with reversed absolute slots and address-ordered links',
            conditional_chain(512, reversed_slots=True), occupied=512)
    for age in [2, 5]:
        add(4096, age, 'conditional_full_protected_allocation_failure',
            'Explicit full512 ownerNULL/no-job chain with age-protected blocks and free page gaps; no oldest victim exists',
            conditional_chain(512, clock=1, timestamps=[1] * 512), occupied=512,
            clock=1, timestamp=1, expected_failure='Complete16B840 NONE followed by source age-limit lowering')
    assert len(cases) == 392
    print(json.dumps(dict(proposed_cases=len(cases), groups=dict(collections.Counter(row['group'] for row in cases)),
        bank_bytes_per_case=bank_bytes, checked_bank_bytes=bank_bytes * len(cases))), flush=True)

    records, fixture_lines, probe_lines, expected_lines = [], base_lines.copy(), base_lines.copy(), []
    totals, call_totals, all_branch_hits = collections.Counter(), collections.Counter(), collections.Counter()
    for index, case in enumerate(cases):
        reset()
        for name, offset, raw in case['patches']:
            put(specs[name][0] + offset, raw)
        put(0x4e6479, bytes([context[0]]))
        put(device + 0x2c, packed([context[1]]))
        put(completed_pointer, packed([context[2]]))
        before = {name: read(address, size) for name, (address, size) in specs.items()}
        u.mem_write(STACK, packed([STOP, manager, case['size'], case['age']]))
        try:
            u.emu_start(0x13d370, STOP, count=30000000)
        except UcError:
            if not boundaries:
                raise
        complete = not boundaries
        assert u.reg_read(UC_X86_REG_EIP) == STOP or boundaries, (index, hex(u.reg_read(UC_X86_REG_EIP)))
        if complete:
            assert u.reg_read(UC_X86_REG_ESP) == STACK + 16
        else:
            assert boundaries[0]['entry'] == '80020F5A' and boundaries[0]['stack'] == [8, 1, 0, 0]
            assert pending[0] != NONE
        # All proposed conditional controls must remain complete, never a
        # manufactured acquisition/completion or a supplied readiness outcome.
        assert complete, (index, case['label'], boundaries)
        handle = u.reg_read(UC_X86_REG_EAX) if complete else NONE
        output_pending = NONE if complete else pending[0]
        after = {name: read(address, size) for name, (address, size) in specs.items()}
        assert after['image_rows'][3::40] == before['image_rows'][3::40], 'source changed readiness'
        for address, size in writes:
            assert any(start <= address and address + size <= end for start, end in allowed), (index, hex(address), size)
        outputs = [(name, offset, raw) for name in sorted(specs) for offset, raw in differences(before[name], after[name])]
        prefix = f'R {case["size"]:08X} {case["age"]:08X} {context[0]:02X} {context[1]:08X} {context[2]:08X} {len(case["patches"])}'
        for name, offset, raw in case['patches']:
            prefix += f' {name} {offset:X} {raw.hex()}'
        output = f'{int(not complete)} {handle:08X} {output_pending:08X} {len(outputs)}'
        for name, offset, raw in outputs:
            output += f' {name} {offset:X} {raw.hex()}'
        fixture_lines.append(prefix + ' | ' + output)
        probe_lines.append(prefix)
        expected_lines.append(output)
        totals.update(metrics)
        call_totals.update(calls)
        all_branch_hits.update(branch_hits)
        records.append(dict(index=index, group=case['group'], label=case['label'], input_bytes=case['size'],
            age=case['age'], details=case['details'], input_patch_count=len(case['patches']),
            input_patch_sha256=sha((' '.join(f'{name}:{offset:X}:{raw.hex()}' for name, offset, raw in case['patches'])).encode()),
            completed=complete, returned_handle=f'{handle:08X}', pending_delete=f'{output_pending:08X}',
            output_patch_count=len(outputs), output_sha256=sha(output.encode()),
            bank_before_sha256={name: sha(raw) for name, raw in before.items()},
            bank_after_sha256={name: sha(raw) for name, raw in after.items()},
            source_calls=dict(calls), source_metrics=dict(metrics), specific_instruction_hits=dict(branch_hits),
            deleted_handles=[f'{value:08X}' for value in deletions],
            boundary=boundaries.copy(), source_write_events=len(writes)))
        if (index + 1) % 32 == 0 or index + 1 == len(cases):
            print(f'original reservation cases complete: {index + 1}/{len(cases)}', flush=True)
    assert totals['candidate_ring_full'] and totals['oldest_predecessor_repair'] and totals['extra_oldest_delete']
    assert totals['failure_limit_store'] and totals['overlap_delete'] and totals['wrapping_weighted_rank']
    assert totals['failed_slot_allocation'] == 2
    assert '0012D160' not in call_totals and '0013D320' not in call_totals
    reset()
    for at, raw in pages.items():
        assert bytes(u.mem_read(at, 4096)) == raw, f'unrestored page{at:08X}'
    summary = dict(cases=len(records), retained_argument_cases=170, conditional_control_cases=len(records) - 170,
        complete_cases=sum(row['completed'] for row in records), bank_bytes_per_case=bank_bytes,
        checked_bank_bytes=bank_bytes * len(records), source_metrics=dict(totals), source_calls=dict(call_totals),
        specific_instruction_hits=dict(all_branch_hits), unique_instructions=len(seen), restored_pages_checked=len(pages))
    print(json.dumps(summary, indent=2), flush=True)
    if args.dry_run:
        return
    validation = None
    if args.binary:
        native = subprocess.run([str(args.binary.resolve()), '--probe'], input='\n'.join(probe_lines) + '\n',
            text=True, capture_output=True, check=True)
        actual = native.stdout.splitlines()
        assert len(actual) == len(expected_lines), (len(actual), len(expected_lines))
        for index, (original, output) in enumerate(zip(expected_lines, actual)):
            assert original.split() == output.split(), (index, records[index]['group'], original[:1000], output[:1000])
        validation = dict(probe_cases=len(actual), all_output_patches_exact=True)
    fixture = ROOT / 'tests/fixtures/retail_source_bitmap_reservation.tsv'
    fixture.write_text('# B/G literal immutable banks/context; R controlled inputs | unchanged13D370 boundary/handle/pending-delete/full-bank output patches.\n' +
        '\n'.join(fixture_lines) + '\n')
    if args.binary:
        normal = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True, check=True)
        assert normal.stdout.strip() == '392 source reservation cases; 25503520 bank bytes exact; 14 admission/phase controls'
        validation['normal'] = normal.stdout.strip()
        rejected = []
        with tempfile.TemporaryDirectory(prefix='halo2-bitmap-reservation-corpus-') as temporary:
            temporary = Path(temporary)
            for label, content in [('missing', None), ('empty', ''), ('truncated', '\n'.join(fixture_lines[:-1]) + '\n')]:
                path = temporary / (label + '.tsv')
                if content is not None:
                    path.write_text(content)
                outcome = subprocess.run([str(args.binary.resolve()), str(path)], text=True, capture_output=True)
                assert outcome.returncode != 0, (label, 'incomplete frozen evidence admitted')
                rejected.append(dict(kind=label, diagnostic=outcome.stderr.strip()))
        validation['incomplete_fixture_rejections'] = rejected
    report = dict(boundary=__doc__, **summary,
        source_xbe_sha256=PIN, snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha((snapshot.directory / 'export.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()), manager_fixture_sha256=sha(manager_fixture.read_bytes()),
        allocator_report_sha256=sha(allocator_path.read_bytes()), fixture_sha256=sha(fixture.read_bytes()),
        native_binary_sha256=sha(args.binary.read_bytes()) if args.binary else None,
        helper_sha256=sha((ROOT / 'src/source_bitmap_reservation.cpp').read_bytes()),
        header_sha256=sha((ROOT / 'include/halo2/source_bitmap_reservation.hpp').read_bytes()),
        test_sha256=sha((ROOT / 'tests/source_bitmap_reservation_tests.cpp').read_bytes()),
        manager_helper_sha256=sha((ROOT / 'src/source_bitmap_manager_transition.cpp').read_bytes()),
        manager_header_sha256=sha((ROOT / 'include/halo2/source_bitmap_manager_transition.hpp').read_bytes()),
        pool_allocation_report_sha256=sha((ROOT / 'analysis/multiplayer-shading/source-pool-allocation-original.json').read_bytes()),
        native_fixture_validation=validation, initial_banks={name: dict(address=f'{address:08X}',
            bytes=size, sha256=sha(initial[name])) for name, (address, size) in sorted(specs.items())},
        context=dict(cache_busy_override=context[0], queued=context[1], completed=context[2],
            source_device=f'{device:08X}', source_completed_pointer=f'{completed_pointer:08X}'),
        fpcw='023F', mxcsr='1F80', supplied_control_configuration=True,
        instruction_sha256=sha(b''.join(packed([pc]) + image.read(pc, size) for pc, size in sorted(seen.items()))),
        instruction_verification='Each distinct reached title PC/size is checked against literal XBE once. Every original nonstack write must fall inside the exact raw data banks, which are checked disjoint from all reached code. Context patches use only pinned literal RAM fields. Reset restores and checks every loaded original page; no instructions can be modified between executions.',
        groups=dict(collections.Counter(row['group'] for row in records)), services_replaced=[],
        source_instruction_patches=[], native_residency_activation=False, original_completion_timeline_claimed=False,
        admission='Literal texture callbacks; ranking2,shift12,positive signed byte counts,ages2/5. Conditional coherent raw ownerNULL/no-job chains use original addressed pool layout, preserve ready bytes and claim no producer timeline.',
        abi='Complete13D370 stack[manager,bytes,age],RET0Chex,EAX salted physical handle orFFFFFFFF. Service stop uses latest13D830 EBX as pending_delete; no status/completion is supplied.',
        records=records)
    path = ROOT / 'analysis/multiplayer-shading/source-bitmap-reservation-original.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps({key: value for key, value in report.items()
        if key != 'records'}, indent=2) + '\n')


if __name__ == '__main__':
    main()
