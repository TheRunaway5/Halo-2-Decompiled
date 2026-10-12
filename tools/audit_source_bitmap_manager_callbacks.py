#!/usr/bin/env python3
"""Run unchanged image callbacks and complete manager deletion over retained banks.

Caller handles and a disposable CPU stack are supplied. Actual cases use
immutable image/physical pools, owners, headers and GPU fence RAM. Separate
query/ownerless controls explicitly replace raw row fields or4E6479; these
conditional metadata inputs do not assert a constructor/completion timeline.
Busy deletions stop before the real kernel wait; no acquisition status,
completion, fence result, hardware wait or readiness value is substituted.
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
SYNTH, STACK, STOP = 0x20000000, 0x200fe000, 0x200ff000

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path)
    args = parser.parse_args()
    snapshot = Snapshot(ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(SYNTH, 0x100000)
    pages, dirty, instructions, writes, calls, boundaries = {}, set(), {}, [], [], []
    call_counts = collections.Counter()

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

    def get(address):
        return uint(read(address, 4))

    def put(address, raw):
        read(address, len(raw))
        u.mem_write(address, raw)
        for at in range(address & ~4095, ((address + len(raw) - 1) & ~4095) + 4096, 4096):
            if not SYNTH <= at < SYNTH + 0x100000:
                dirty.add(at)

    def event(pc):
        return dict(entry=f'{pc:08X}', eax=u.reg_read(UC_X86_REG_EAX), edx=u.reg_read(UC_X86_REG_EDX),
            ecx=u.reg_read(UC_X86_REG_ECX), edi=u.reg_read(UC_X86_REG_EDI), ebx=u.reg_read(UC_X86_REG_EBX),
            stack=words(read(u.reg_read(UC_X86_REG_ESP) + 4, 16)))

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

    def code(g, pc, size, data):
        if pc == STOP:
            g.emu_stop()
            return
        try:
            raw = image.read(pc, size)
        except (ValueError, KeyError):
            boundaries.append(dict(kind='actual kernel entry; not executed', **event(pc)))
            g.emu_stop()
            return
        assert read(pc, size) == raw, (hex(pc), 'changed original instruction')
        instructions[pc] = size
        if pc in [0x12d160, 0x12d1a0, 0x12d200, 0x13d830, 0x16ba40, 0x3fb710, 0x3fad50, 0x3fac30]:
            calls.append(event(pc))
            call_counts[f'{pc:08X}'] += 1
        if pc in [0x3fad50, 0x3fac30]:
            boundaries.append(dict(kind='hardware wait/kick entry; not executed', **event(pc)))
            g.emu_stop()

    def writing(g, access, address, size, value, data):
        if not SYNTH <= address < SYNTH + 0x100000:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                dirty.add(at)
            writes.append(dict(address=f'{address:08X}', bytes=size,
                value=f'{value & ((1 << (8 * size)) - 1):0{2 * size}X}'))

    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', snapshot.report['registers_before']).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for reg, selector in [(UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16), (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)]:
        u.reg_write(reg, selector)
    manager, image_pool = get(0x4e6464), get(0x4e6454)
    physical_pool = get(manager + 0x64)
    image_rows, physical_rows = get(image_pool + 0x44), get(physical_pool + 0x44)
    image_bits, physical_bits = get(image_pool + 0x48), get(physical_pool + 0x48)
    specs = {'manager': (manager, 0x70), 'image_pool': (image_pool, 0x4c),
        'physical_pool': (physical_pool, 0x4c), 'image_rows': (image_rows, 512 * 40),
        'physical_rows': (physical_rows, 512 * 24), 'image_bits': (image_bits, 64), 'physical_bits': (physical_bits, 64)}
    bank_raw = {name: read(at, size) for name, (at, size) in specs.items()}
    assert bank_raw['image_bits'] == bank_raw['physical_bits']
    active = [i for i in range(512) if bank_raw['image_bits'][i >> 3] & (1 << (i & 7))]
    assert len(active) == 286
    assert words(read(manager + 0x20, 12)) == [0x12d200, 0x12d1a0, 0x12d160]
    handles, owners = {}, {}
    for index in active:
        row = read(image_rows + index * 40, 40)
        physical = read(physical_rows + index * 24, 24)
        assert row[:2] == physical[:2]
        handles[index] = int.from_bytes(row[:2], 'little') << 16 | index
        owner, partition = uint(row, 16), uint(row, 4)
        assert owner and partition < 3
        owners[owner] = read(owner, 116)
        assert uint(owners[owner], 0x28 + partition * 4) == handles[index]
    device = get(0x407488)
    completed_pointer = get(device + 0x30)
    fence_inputs = dict(device=f'{device:08X}', queued=get(device + 0x2c),
        completed_pointer=f'{completed_pointer:08X}', completed=get(completed_pointer))

    def reset():
        for at in dirty:
            u.mem_write(at, pages[at])
        dirty.clear()
        writes.clear()
        calls.clear()
        boundaries.clear()
        for reg in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]:
            u.reg_write(reg, 0)
        u.reg_write(UC_X86_REG_ESP, STACK)
        u.reg_write(UC_X86_REG_EFLAGS, 2)
        u.reg_write(UC_X86_REG_FPCW, 0x023f)
        u.reg_write(UC_X86_REG_FPTAG, 0xffff)
        u.reg_write(UC_X86_REG_FPSW, 0)
        u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        u.mem_write(STACK - 256, bytes(512))

    def differences(before, after):
        spans, at = [], 0
        while at < len(before):
            if before[at] == after[at]:
                at += 1
                continue
            start = at
            while at < len(before) and before[at] != after[at]:
                at += 1
            spans.append(dict(offset=start, before=before[start:at].hex(), after=after[start:at].hex()))
        return spans

    def run(kind, handle, label, row_input=None, override=None):
        reset()
        index = handle & 65535
        assert index in active
        row_at = image_rows + index * 40
        if row_input is not None:
            put(row_at, row_input)
        if override is not None:
            put(0x4e6479, bytes([override]))
        before_banks = {name: read(at, size) for name, (at, size) in specs.items()}
        initial_row = read(row_at, 40)
        owner = uint(initial_row, 16)
        entry = {'state': 0x12d160, 'busy': 0x12d1a0, 'delete': 0x12d200, 'manager_delete': 0x13d830}[kind]
        u.mem_write(STACK, packed([STOP, handle]))
        if kind == 'manager_delete':
            u.reg_write(UC_X86_REG_EDI, manager)
            u.reg_write(UC_X86_REG_EBX, handle)
        try:
            u.emu_start(entry, STOP, count=2000000)
        except UcError:
            if not boundaries:
                raise
        completed = not boundaries
        pc = u.reg_read(UC_X86_REG_EIP)
        assert pc == STOP or boundaries, hex(pc)
        if completed:
            assert u.reg_read(UC_X86_REG_ESP) == STACK + (4 if kind == 'manager_delete' else 8)
        return_eax = u.reg_read(UC_X86_REG_EAX) if completed else None
        if completed and kind == 'state':
            expected_al = 0x10 if initial_row[2] & 1 else (0x20 if uint(initial_row, 4) else 0)
            assert return_eax == ((index * 5) & 0xffffff00) | expected_al
        if boundaries:
            assert kind in ['delete', 'manager_delete']
            assert boundaries[0]['entry'] == '80020F5A' and boundaries[0]['stack'] == [8, 1, 0, 0]
        after_row = read(row_at, 40)
        assert after_row[3] == initial_row[3], 'callback changed readiness'
        changed, hashes = {}, {}
        for name, (at, size) in specs.items():
            after = read(at, size)
            hashes[name] = sha(after)
            diff = differences(before_banks[name], after)
            if diff:
                changed[name] = diff
        owner_after = read(owner, 116) if owner else b''
        owner_changes = differences(owners[owner], owner_after) if owner else []
        allowed = [(at, at + size) for at, size in specs.values()] + ([(owner, owner + 116)] if owner else [])
        for item in writes:
            at, size = int(item['address'], 16), item['bytes']
            assert any(start <= at and at + size <= end for start, end in allowed), item
        return dict(kind=kind, label=label, handle=f'{handle:08X}', index=index, row=f'{row_at:08X}',
            owner=f'{owner:08X}', completed=completed, return_al=return_eax & 255 if completed and kind in ['state', 'busy'] else None,
            return_eax=return_eax if completed and kind == 'state' else None,
            initial_row_hex=initial_row.hex(), final_row_hex=after_row.hex(), owner_after_hex=owner_after.hex(),
            row_input_hex=row_input.hex() if row_input is not None else None, supplied_override=override,
            bank_after_sha256=hashes, bank_changed_spans=changed, owner_changed_spans=owner_changes,
            calls=calls.copy(), writes=writes.copy(), boundaries=boundaries.copy())

    records = []
    for index in active:
        for kind in ['state', 'busy', 'delete', 'manager_delete']:
            records.append(run(kind, handles[index], 'literal active retained handle and banks'))
    for index in active[::16]:
        for upper in [0, 0x1234, 0xffff]:
            alias = upper << 16 | index
            for kind in ['state', 'busy']:
                records.append(run(kind, alias, 'explicit same-low16 salt alias; callback ignores upper word'))
    busy_index = next(r['index'] for r in records if r['kind'] == 'busy' and r['return_al'] == 1)
    clear_index = next(r['index'] for r in records if r['kind'] == 'busy' and r['return_al'] == 0)
    for index in [busy_index, clear_index]:
        original = bank_raw['image_rows'][index * 40:(index + 1) * 40]
        for flags in [0, 1, 2, 3, 0x80, 0xfd, 0xfe, 0xff]:
            for ready in [0, 1, 2, 255]:
                for job in [0xffffffff, uint(original, 12)]:
                    for override in [0, 1, 255]:
                        row = bytearray(original)
                        row[2:4] = bytes([flags, ready])
                        row[4:8] = packed([[0, 1, 2, 0x80000000][(flags + ready + override) % 4]])
                        row[12:16] = packed([job])
                        for kind in ['state', 'busy']:
                            records.append(run(kind, handles[index], 'explicit conditional row/global query inputs; no readiness producer claim', bytes(row), override))
        row = bytearray(original)
        row[16:20] = packed([0])
        for kind in ['delete', 'manager_delete']:
            records.append(run(kind, handles[index], 'explicit ownerNULL callback branch; no borrowed-buffer producer claim', bytes(row)))
    reset()
    for at, raw in pages.items():
        assert bytes(u.mem_read(at, 4096)) == raw, f'unrestored page{at:08X}'
    groups = {}
    for kind in ['state', 'busy', 'delete', 'manager_delete']:
        subset = [r for r in records if r['kind'] == kind]
        groups[kind] = dict(cases=len(subset), complete=sum(r['completed'] for r in subset),
            returns=dict(collections.Counter(str(r['return_al']) for r in subset if r['return_al'] is not None)),
            boundaries=dict(collections.Counter(b['entry'] for r in subset for b in r['boundaries'])))
    lines = []
    for name in sorted(specs):
        lines.append(f'B {name} {specs[name][0]:08X} {bank_raw[name].hex()}')
    for address, raw in sorted(owners.items()):
        lines.append(f'B owner_{address:08X} {address:08X} {raw.hex()}')
    lines.append(f'G {read(0x4e6479, 1)[0]:02X} {fence_inputs["queued"]:08X} {fence_inputs["completed"]:08X}')
    probe_lines, source_lines = lines.copy(), []
    for record in records:
        prefix = 'K '+record['kind']+' '+record['handle']+' '+(record['row_input_hex'] or '-')+' '+(f'{record["supplied_override"]:02X}' if record['supplied_override'] is not None else '-')
        patches = dict(record['bank_changed_spans'])
        if record['owner_changed_spans']:
            patches['owner_'+record['owner']] = record['owner_changed_spans']
        result = [str(int(not record['completed'])), f'{(record["return_al"] or 0):02X}', str(sum(len(p) for p in patches.values()))]
        for name, ranges in sorted(patches.items()):
            for diff in ranges:
                result.extend([name, f'{diff["offset"]:X}', diff['after']])
        result = ' '.join(result)
        source_lines.append(result)
        lines.append(prefix+' | '+result)
        probe_lines.append(prefix)
    fixture = ROOT / 'tests/fixtures/retail_source_bitmap_manager_transition.tsv'
    fixture.write_text('# Full raw B banks, G fence/context, K conditional input|boundary/query-value/exact output patches. Untouched bytes remain mandatory.\n'+'\n'.join(lines)+'\n')
    native_fixture_validation = None
    if args.binary:
        native = subprocess.run([str(args.binary.resolve()), '--probe'], input='\n'.join(probe_lines)+'\n', text=True, capture_output=True, check=True)
        actual = native.stdout.splitlines()
        assert len(actual) == len(source_lines)
        for index, (source, output) in enumerate(zip(source_lines, actual)):
            assert source.split() == output.split(), (index, records[index]['kind'], source, output)
        normal = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True, check=True)
        assert normal.stdout.strip() == '2024 source bitmap manager cases; 131681440 bank bytes exact; 9 admission/boundary controls'
        rejected = []
        with tempfile.TemporaryDirectory(prefix='halo2-bitmap-manager-corpus-') as temporary:
            temporary = Path(temporary)
            for label, content in [('missing', None), ('empty', ''), ('truncated', '\n'.join(lines[:-1])+'\n')]:
                path = temporary / (label+'.tsv')
                if content is not None:
                    path.write_text(content)
                outcome = subprocess.run([str(args.binary.resolve()), str(path)], text=True, capture_output=True)
                assert outcome.returncode != 0, (label, 'incomplete frozen evidence admitted')
                rejected.append(dict(kind=label, diagnostic=outcome.stderr.strip()))
        native_fixture_validation = dict(normal=normal.stdout.strip(), incomplete_fixture_rejections=rejected)
    report = dict(boundary=__doc__, source_xbe_sha256=PIN, snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha((snapshot.directory / 'export.json').read_bytes()), tool_sha256=sha(Path(__file__).read_bytes()),
        fpcw='023F', mxcsr='1F80', supplied_control_configuration=True, active_slots=len(active),
        source_registered_callbacks=words(read(manager + 0x20, 12)), fence_inputs=fence_inputs,
        cache_busy_override_byte=read(0x4e6479, 1).hex(),
        initial_banks={name: dict(address=f'{specs[name][0]:08X}', bytes=len(raw), sha256=sha(raw), raw_hex=raw.hex()) for name, raw in bank_raw.items()},
        initial_owners={f'{at:08X}': raw.hex() for at, raw in sorted(owners.items())},
        groups=groups, cases=len(records), complete_cases=sum(r['completed'] for r in records),
        unique_instructions=len(instructions), instruction_sha256=sha(b''.join(packed([pc]) + image.read(pc, n) for pc, n in sorted(instructions.items()))),
        source_calls=dict(call_counts), restored_pages_checked=len(pages), services_replaced=[], source_instruction_patches=[],
        fixture_sha256=sha(fixture.read_bytes()),
        native_binary_sha256=sha(args.binary.read_bytes()) if args.binary else None,
        native_fixture_validation=native_fixture_validation,
        helper_sha256=sha((ROOT / 'src/source_bitmap_manager_transition.cpp').read_bytes()),
        header_sha256=sha((ROOT / 'include/halo2/source_bitmap_manager_transition.hpp').read_bytes()),
        test_sha256=sha((ROOT / 'tests/source_bitmap_manager_transition_tests.cpp').read_bytes()),
        state_registration_scope='12D160 is registered atmanager+28; no call to this state pointer is present in audited13D170..13D9EE. Allocator invokes busy+24 and delete+20, notstate.',
        readiness_scope='Callbacks never set image+3.12D5A0 clearsit thenpassesactualaddressasdone to213760. Worker120C8C writes1 onlyfor actualcallbackresult1+nonnull done; I/O/worker timeline is separate.',
        abi={'callbacks': 'One saltedhandle atESP+4; ALbyte forstate/busy, voiddelete; RET4. Globalimagepool+44 +40*(handle&FFFF), no saltcomparison.',
            '12D160_EAX': 'Unchanged machine EAX is ((index*5)&FFFFFF00)|AL. Every state execution verifies this full register formula; native typed state API and fixture compare the registered AL byte contract.',
            '13D830': 'EDI actualmanager; EBX saltedhandle. Callsdelete+20 before physical unlink/poolrelease; returns normally with caller stack unchanged except returnaddress.'},
        records=records)
    path = ROOT / 'analysis/multiplayer-shading/source-bitmap-manager-callbacks-original.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps({k: v for k, v in report.items() if k not in ['records', 'initial_banks', 'initial_owners']}, indent=2)+'\n')
    print(json.dumps({k: report[k] for k in ['active_slots', 'cases', 'complete_cases', 'groups', 'unique_instructions', 'source_calls', 'restored_pages_checked']}, indent=2))

if __name__ == '__main__':
    main()
