#!/usr/bin/env python3
"""Audit original async queue constructors, owning handles, and worker boundaries.

Title code is byte-checked against the disc XBE. Constructor, wait, release,
thread-priority and close calls stop at their first actual kernel entry; no
kernel status, handle, register result, job completion or readiness is supplied.
A separate unchanged pure kernel handle lookup runs over retained table RAM.
It neither acquires a wait object nor simulates a thread or interrupt schedule.
"""
from pathlib import Path
import bisect, collections, csv, hashlib, json, re, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_OP_MEM
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

SYNTH, STACK, STOP = 0x20000000, 0x200fe000, 0x200ff000
NODES, NODE_COUNT, NODE_SIZE = 0x4e0368, 150, 64
OUT = ROOT / 'analysis/multiplayer-shading/source-async-handle-lifecycle'
PRIMARY_COMMIT = '585c49a50af1255ab155099e06f24505f9c5a800'

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def main():
    snapshot = Snapshot(ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(SYNTH, 0x100000)
    pages, dirty, title_instructions, lookup_instructions = {}, set(), {}, {}
    boundary, writes = [], []
    pure_lookup = [False]

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

    def event(pc, size):
        sp = u.reg_read(UC_X86_REG_ESP)
        return dict(entry=f'{pc:08X}', stack=words(read(sp + 4, size * 4)),
                    eax=u.reg_read(UC_X86_REG_EAX), ecx=u.reg_read(UC_X86_REG_ECX),
                    edx=u.reg_read(UC_X86_REG_EDX))

    def unmapped(g, access, address, size, value, data):
        try:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                page(at)
            return True
        except ValueError as error:
            boundary.append(dict(kind='uncaptured memory', pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',
                                 address=f'{address:08X}', size=size, reason=str(error)))
            g.emu_stop()
            return False

    def code(g, pc, size, data):
        if pc == STOP:
            g.emu_stop()
            return
        if pc >= 0x80000000:
            if pure_lookup[0] and 0x800215c4 <= pc < 0x800215f6:
                assert read(pc, size) == snapshot.read(pc, size)
                lookup_instructions[pc] = size
                return
            boundary.append(dict(kind='actual kernel entry, not executed', **event(pc, 10)))
            g.emu_stop()
            return
        assert read(pc, size) == image.read(pc, size), (hex(pc), 'changed title instruction')
        title_instructions[pc] = size

    def write(g, access, address, size, value, data):
        if not SYNTH <= address < SYNTH + 0x100000:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                dirty.add(at)
            writes.append(dict(address=f'{address:08X}', size=size,
                               value=f'{value & ((1 << (8 * size)) - 1):X}'))

    u.hook_add(UC_HOOK_MEM_UNMAPPED, unmapped)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', snapshot.report['registers_before']).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for reg, selector in [(UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16), (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)]:
        u.reg_write(reg, selector)

    initial = snapshot.read(0x4e0354, 0x25a0)
    handles = {name: uint(snapshot.read(at, 4)) for name, at in
               [('thread', 0x4e0354), ('free_list_mutex', 0x4e0358),
                ('work_list_mutex', 0x4e035c), ('work_semaphore', 0x4e0360)]}
    assert handles == dict(thread=24, free_list_mutex=4, work_list_mutex=8, work_semaphore=16)

    def reset():
        for at in dirty:
            u.mem_write(at, pages[at])
        dirty.clear()
        writes.clear()
        boundary.clear()
        u.mem_write(SYNTH, bytes(0x1000))
        u.mem_write(STACK, bytes(0x100))
        for reg in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]:
            u.reg_write(reg, 0)
        u.reg_write(UC_X86_REG_ESP, STACK)
        u.reg_write(UC_X86_REG_EFLAGS, 2)
        u.reg_write(UC_X86_REG_FPCW, 0x023f)
        u.reg_write(UC_X86_REG_FPTAG, 0xffff)
        u.reg_write(UC_X86_REG_FPSW, 0)
        u.reg_write(UC_X86_REG_MXCSR, 0x1f80)

    def run(label, entry, arguments, argc, expected_kernel=None, lookup=False):
        reset()
        pure_lookup[0] = lookup
        u.mem_write(STACK, packed([STOP, *arguments]))
        try:
            u.emu_start(entry, STOP, count=200000)
        except UcError:
            if not boundary:
                raise
        pc = u.reg_read(UC_X86_REG_EIP)
        row = dict(label=label, title_entry=f'{entry:08X}', explicit_arguments=arguments,
                   completed=pc == STOP, return_eax=u.reg_read(UC_X86_REG_EAX) if pc == STOP else None,
                   boundary=boundary.copy(), source_writes=writes.copy())
        if expected_kernel is not None:
            assert pc == expected_kernel and len(boundary) == 1, row
            row['boundary'][0]['stack'] = row['boundary'][0]['stack'][:argc]
        if lookup:
            assert pc == STOP and not boundary and not writes
            assert u.reg_read(UC_X86_REG_ESP) == STACK + 8
        return row

    startup = run('complete title startup prefix to first mutex constructor', 0x120a90, [], 3, 0x80013355)
    startup_bank = read(NODES, NODE_COUNT * NODE_SIZE)
    for index in range(NODE_COUNT):
        raw = startup_bank[index * 64:(index + 1) * 64]
        assert uint(raw, 4) == 0xffffffff
        assert uint(raw, 60) == (NODES + (index + 1) * 64 if index + 1 < NODE_COUNT else 0)
        assert raw[:4] + raw[8:60] == bytes(56)
    assert uint(read(0x4e28e8, 4)) == NODES and uint(read(0x4e28ec, 4)) == 0
    assert read(0x4e0354, 16) == initial[:16]
    assert read(0x4e0364, 4) == initial[16:20]
    startup['constructed_nodes_sha256'] = sha(startup_bank)
    startup['constructed_node_count'] = 150
    startup['handle_stores_completed'] = 0
    startup['task_id_counter_preserved'] = uint(read(0x4e0364, 4))

    prefixes = [startup,
        run('mutex wrapper with literal startup arguments', 0x2d626c, [0, 0, 0], 3, 0x80013355),
        run('semaphore wrapper with literal startup arguments', 0x2d61eb, [0, 0, 150, 0], 4, 0x80013b7a),
        run('thread wrapper with literal startup arguments', 0x2d21c6, [0, 0x4000, 0x120c30, 0, 0, 0], 10, 0x8002190e),
        run('priority wrapper with actual retained worker handle', 0x2d1fb6, [handles['thread'], 1], 3, 0x80020524),
        run('actual free-node owner wait prefix', 0x1208b0, [], 4, 0x80020f5a),
        run('actual worker start, semaphore wait prefix', 0x120c30, [0], 4, 0x80020f5a),
        run('retained work-mutex wait, supplied infinite timeout', 0x2d63fa, [handles['work_list_mutex'], 0xffffffff], 4, 0x80020f5a),
        run('retained free-mutex release wrapper', 0x2d62c4, [handles['free_list_mutex']], 2, 0x8001338f),
        run('retained work-mutex release wrapper', 0x2d62c4, [handles['work_list_mutex']], 2, 0x8001338f),
        run('retained work semaphore release, source count1', 0x2d6246, [handles['work_semaphore'], 1, SYNTH], 3, uint(snapshot.read(0x411648, 4))),
    ]
    for name in ['thread', 'work_semaphore', 'work_list_mutex', 'free_list_mutex']:
        prefixes.append(run('close wrapper, retained ' + name, 0x2d1f3a, [handles[name]], 1, uint(snapshot.read(0x411544, 4))))
    assert prefixes[5]['boundary'][0]['stack'] == [4, 1, 0, 0]
    assert prefixes[6]['boundary'][0]['stack'] == [16, 1, 0, 0]

    lookup_rows = []
    objects = {}
    for name, handle in handles.items():
        for alias in range(4):
            row = run('retained pure nonreferencing handle lookup', 0x800215c4, [handle | alias], 0, lookup=True)
            lookup_rows.append(row)
            obj = row['return_eax']
            assert obj
            if alias:
                assert obj == int(objects[name]['object'], 16)
                continue
            raw, head = read(obj, 0x40), read(obj - 0x10, 0x10)
            typ = uint(head, 8)
            offset = uint(read(typ + 0x14, 4))
            dispatcher = (obj + offset) & 0xffffffff if offset < 0x80000000 else offset
            objects[name] = dict(handle=handle, object=f'{obj:08X}', object_header_hex=head.hex(),
                object_type=f'{typ:08X}', type_hex=read(typ, 0x1c).hex(),
                dispatcher=f'{dispatcher:08X}', raw_hex=raw.hex(),
                dispatcher_type=raw[0], signal_state=uint(raw, 4),
                semaphore_limit=uint(raw, 16) if name == 'work_semaphore' else None,
                mutant_owner=uint(raw, 24) if name.endswith('_mutex') else None)
    for handle in [0, 0x100, 0xfffffffc]:
        row = run('out-of-bank/unused pure handle control', 0x800215c4, [handle], 0, lookup=True)
        assert row['return_eax'] == 0
        lookup_rows.append(row)
    assert objects['free_list_mutex']['dispatcher_type'] == objects['work_list_mutex']['dispatcher_type'] == 2
    assert objects['free_list_mutex']['signal_state'] == objects['work_list_mutex']['signal_state'] == 1
    assert objects['free_list_mutex']['mutant_owner'] == objects['work_list_mutex']['mutant_owner'] == 0
    assert objects['work_semaphore']['dispatcher_type'] == 5 and objects['work_semaphore']['signal_state'] == 0
    assert objects['work_semaphore']['semaphore_limit'] == 150

    reset()
    assert read(0x4e0354, len(initial)) == initial
    free_chain, at = [], uint(read(0x4e28e8, 4))
    while at:
        assert at not in free_chain and NODES <= at < NODES + 150 * 64 and (at - NODES) % 64 == 0
        assert uint(read(at + 4, 4)) == 0xffffffff
        free_chain.append(at)
        at = uint(read(at + 60, 4))
    assert len(free_chain) == 150 and uint(read(0x4e28ec, 4)) == 0

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    functions = []
    for row in csv.reader((ROOT / 'tools/reference/halo2-decompiled/config/functions.csv').open()):
        try:
            functions.append((int(row[0], 16), int(row[1])))
        except (ValueError, IndexError):
            pass
    functions.sort()
    starts = [v[0] for v in functions]
    references = {}
    for address in range(0x4e0354, 0x4e0364, 4):
        hits = {}
        for section in image.sections:
            raw = image.section_bytes(section)
            needle = packed([address])
            pos = raw.find(needle)
            while pos >= 0:
                pc = section.va + pos
                index = bisect.bisect_right(starts, pc) - 1
                if index >= 0:
                    function, size = functions[index]
                    if pc < function + size:
                        for instruction in md.disasm(image.read(function, size), function):
                            if any(v.type == CS_OP_MEM and v.mem.disp == address for v in instruction.operands):
                                hits[instruction.address] = dict(pc=f'{instruction.address:08X}', function=f'{function:08X}',
                                    instruction=instruction.mnemonic + ' ' + instruction.op_str)
                pos = raw.find(needle, pos + 1)
        references[f'{address:08X}'] = [v for _, v in sorted(hits.items())]

    regions = {}
    for label, address, size in [
        ('title initialization caller', 0x12000, 125), ('title shutdown', 0x12080, 176),
        ('async queue and worker', 0x1208b0, 0x4d0), ('create/release/wait wrappers', 0x2d61eb, 0x21e),
        ('thread create wrapper', 0x2d21c6, 108), ('thread startup/TLS', 0x2d212e, 152),
        ('close wrapper', 0x2d1f3a, 28), ('set priority wrapper', 0x2d1fb6, 80)]:
        raw = image.read(address, size)
        regions[label] = dict(address=f'{address:08X}', bytes=size, sha256=sha(raw),
            instructions=[dict(pc=f'{i.address:08X}', op=i.mnemonic + ' ' + i.op_str) for i in md.disasm(raw, address)])
    kernel_regions = {}
    for label, address, size in [
        ('pure handle lookup, executed separately', 0x800215c4, 50),
        ('referencing handle lookup, not executed', 0x80021579, 75),
        ('NtWaitForSingleObjectEx, not executed', 0x80020f5a, 115),
        ('NtCreateMutant, not executed', 0x80013355, 58),
        ('NtCreateSemaphore, not executed', 0x80013b7a, 83),
        ('kernel wait state producer, not executed', 0x80019cdc, 0x240)]:
        raw = snapshot.read(address, size)
        kernel_regions[label] = dict(address=f'{address:08X}', bytes=size, sha256=sha(raw),
            instructions=[dict(pc=f'{i.address:08X}', op=i.mnemonic + ' ' + i.op_str) for i in md.disasm(raw, address)])

    primary = []
    for filename, path, urlpath in [
        ('EmuKrnlNt.cpp', OUT / 'EmuKrnlNt.cpp', 'src/core/kernel/exports/EmuKrnlNt.cpp'),
        ('EmuKrnlOb.cpp', OUT / 'EmuKrnlOb.cpp', 'src/core/kernel/exports/EmuKrnlOb.cpp'),
        ('EmuKrnlPs.cpp', ROOT / 'analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/EmuKrnlPs.cpp', 'src/core/kernel/exports/EmuKrnlPs.cpp')]:
        primary.append(dict(file=str(path.relative_to(ROOT)), sha256=sha(path.read_bytes()),
            url=f'https://raw.githubusercontent.com/Cxbx-Reloaded/Cxbx-Reloaded/{PRIMARY_COMMIT}/{urlpath}',
            scope='Pinned primary compatibility source inspected only; not executed or original Xbox kernel implementation.'))
    for path in [ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h',
                 ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-types.h',
                 ROOT / 'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/NativeHandle.h',
                 ROOT / 'tools/reference/halo2-decompiled/src/async.cpp',
                 ROOT / 'tools/reference/halo2-decompiled/include/async.h',
                 ROOT / 'tools/reference/halo2-decompiled/include/job_queue.h']:
        primary.append(dict(file=str(path.relative_to(ROOT)), sha256=sha(path.read_bytes()), scope='Local primary/reference declaration or decompile, inspected only.'))
    iat = {f'{p:08X}': dict(disc_word=f'{uint(image.read(p, 4)):08X}', retained_entry=f'{uint(snapshot.read(p, 4)):08X}')
           for p in [0x411544, 0x4115a4, 0x4115a8, 0x4115ac, 0x4115b0, 0x4115c4, 0x411644, 0x411648, 0x41164c, 0x411650, 0x411654]}
    handle_limit = uint(read(0x8003abc8, 4))
    handle_table = uint(read(0x8003abcc, 4))
    handle_page = uint(read(handle_table, 4))
    report = dict(source_xbe_sha256=PIN, snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha((snapshot.directory / 'export.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()), configuration=dict(fpcw='023F', mxcsr='1F80', supplied=True),
        prefix_cases=len(prefixes), kernel_boundaries=collections.Counter(v['boundary'][0]['entry'] for v in prefixes),
        completed_kernel_services=0, services_replaced=[], source_instruction_patches=[], prefixes=prefixes,
        pure_kernel_lookup_cases=len(lookup_rows), pure_kernel_lookup_rows=lookup_rows,
        unique_title_instructions=len(title_instructions), unique_lookup_instructions=len(lookup_instructions),
        title_instruction_sha256=sha(b''.join(packed([p]) + image.read(p, n) for p, n in sorted(title_instructions.items()))),
        lookup_instruction_sha256=sha(b''.join(packed([p]) + snapshot.read(p, n) for p, n in sorted(lookup_instructions.items()))),
        actual_handles=handles, actual_objects=objects, iat=iat,
        retained_handle_table=dict(limit_global='8003ABC8', handle_limit=handle_limit,
            table_global='8003ABCC', table_pointer=f'{handle_table:08X}',
            first_page=f'{handle_page:08X}', first_page_hex=read(handle_page, 256).hex(),
            first_page_sha256=sha(read(handle_page, 256)),
            scope='Pure lookup masks low2 bits, rejects handle>=limit, indexes page byhandle>>8 and byte offsetbyhandle&FF. Protected lookup/reference is not executed.'),
        retained_queue=dict(nodes=NODES, node_count=150, node_size=64, free_chain=free_chain,
            bank_sha256=sha(initial), task_id_counter=uint(initial, 16), work_head=uint(snapshot.read(0x4e28ec, 4)),
            tasks_added=uint(snapshot.read(0x4e28f0, 4)), last_worker_node=uint(snapshot.read(0x510810, 4))),
        literal_absolute_handle_references=references, title_regions=regions, retained_kernel_regions=kernel_regions,
        primary_sources=primary,
        abi_correction='NtWaitForSingleObjectEx argument2 is KPROCESSOR_MODE WaitMode=1 (UserMode), not WaitReason. Retained kernel separately supplies WaitReason6 to KeWaitForSingleObject.',
        rules=dict(startup='120A90 clears150nodes, sets each stateNONE and next links, builds free/work heads, creates two unnamed initially unowned mutants, creates semaphore initial0/max150, creates thread120C30 stack4000 not suspended and sets priority1. Task-ID counter is not reset by120A90.',
            ownership='4E0358 protects free-list pop/push;4E035C protects work-list insert/remove/category/priority queries;4E0360 counts pending work;4E0354 owns a persistent worker thread handle.',
            worker='120C40 waits on semaphore then work mutex, snapshots head, releases work mutex, calls callback outside either list mutex. Result0 re-signals semaphore1. Result1 with done pointer sets done1. Any nonzero result removes job, sets stateNONE and returns node under free mutex.',
            shutdown='12080 performs other title cleanup, then closes thread, semaphore, work mutex, free mutex through2D1F3A. That body does not clear those four globals, signal cancellation, join or terminate the forever worker; whole shutdown is inspected, not executed.',
            compatibility='Pinned Cxbx forwards mutant/semaphore creation and releases to real native NT objects. Thread creation owns an ETHREAD/object-manager handle plus a real native proxy thread; multiwait maps object-manager thread handles to native handles and preserves WaitMode/APC handling. This is implementation evidence for native-owned primitives, not executed Xbox scheduling parity.',
            seam='Own two source-compatible mutexes, a counting semaphore0/max150, a150node priority queue and a persistent callback worker, with real operations/statuses and explicit file I/O/completion services. Source completion still requires callback result1 and nonNULLdone. No fake status/handle, imported thread state, cancellation policy or residency-pressure budget follows from these prefixes.'),
        boundary='All title service calls stop before first actual kernel instruction. The only separately executed kernel body is pure800215C4 nonreferencing handle lookup. Protected lookup, mutant acquisition/release, semaphore scheduling, object creation, thread startup and shutdown remain unexecuted.')
    OUT.mkdir(exist_ok=True)
    path = OUT / 'report.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['prefix_cases', 'kernel_boundaries', 'completed_kernel_services',
          'pure_kernel_lookup_cases', 'unique_title_instructions', 'unique_lookup_instructions', 'actual_handles']}, indent=2))

if __name__ == '__main__':
    main()
