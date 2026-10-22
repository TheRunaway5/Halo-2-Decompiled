#!/usr/bin/env python3
"""Freeze untouched original async ordering and explicitly owned queue cores.

Complete120900/1A1720 execute without platform services. Enqueue-ID, priority,
count, category and unlink critical-section bodies admit exclusive node-bank
ownership at their literal post-wait instruction and stop before release.
Initialization stops before the first CreateMutex entry. Free take and return
admit explicit owned/post-unlink phases and stop before mutex release; an empty
take is an availability boundary, never a fabricated whole1208B0 return.
Their whole callers separately stop at the actual kernel wait; no wait result
or release status is supplied. This does not create, run, or complete a worker.
"""
from pathlib import Path
import argparse, collections, hashlib, json, random, re, subprocess, sys
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH, STATE, NEWTASK, NEXTTASK, STACK, STOP = 0x20000000, 0x20000100, 0x20000200, 0x20000300, 0x200fe000, 0x200ff000
NODES, HEAD, COUNTER = 0x4e0368, 0x4e28ec, 0x4e0364
FREE, ADDED = 0x4e28e8, 0x4e28f0
READ, OTHER, NONE = 0x1a0e70, 0x1a1020, 0xffffffff

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def signed(value):
    return value if value < 0x80000000 else value - 0x100000000

def compare_model(values):
    _, priority, other_priority, callback, other_callback, file, offset, other_file, other_offset, state_file, state_offset, state_word = values
    if signed(priority) != signed(other_priority):
        before = signed(priority) > signed(other_priority)
    elif callback == READ and other_callback != READ:
        before = True
    elif callback != READ and other_callback == READ:
        before = False
    elif callback == other_callback == READ and state_word & 255:
        distance, other_distance = (file - state_file) & NONE, (other_file - state_file) & NONE
        before = distance < other_distance if distance != other_distance else ((offset - state_offset) & NONE) < ((other_offset - state_offset) & NONE)
    else:
        before = False
    if other_callback == READ:
        state_file, state_offset, state_word = other_file, other_offset, (state_word & 0xffffff00) | 1
    return [int(before), state_file, state_offset, state_word]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, help='Optional native probe consuming the documented fixture word protocol.')
    args = parser.parse_args()
    snapshot = Snapshot(ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(SYNTH, 0x100000)
    pages, dirty, instructions, writes, callbacks = {}, set(), {}, [], []
    counts = collections.Counter()
    stop, boundary = [0], []

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

    def put(address, raw):
        read(address, len(raw))
        u.mem_write(address, raw)
        for at in range(address & ~4095, ((address + len(raw) - 1) & ~4095) + 4096, 4096):
            if not SYNTH <= at < SYNTH + 0x100000:
                dirty.add(at)

    def event(pc):
        return dict(entry=f'{pc:08X}', stack=words(read(u.reg_read(UC_X86_REG_ESP) + 4, 16)))

    def missing(g, access, address, size, value, data):
        try:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                page(at)
            return True
        except ValueError as error:
            boundary.append(dict(kind='uncaptured memory', pc=f'{g.reg_read(UC_X86_REG_EIP):08X}', address=f'{address:08X}', reason=str(error)))
            g.emu_stop()
            return False

    def code(g, pc, size, data):
        if pc == STOP:
            g.emu_stop()
            return
        try:
            raw = image.read(pc, size)
        except (ValueError, KeyError):
            boundary.append(dict(kind='actual kernel entry, not executed', **event(pc)))
            g.emu_stop()
            return
        assert read(pc, size) == raw, (hex(pc), 'changed source instruction')
        instructions[pc] = size
        if pc in [0x120900, 0x1a1720, 0x1209c0, 0x120ce0, 0x120bf0, 0x120b50, 0x120a30, 0x120a90, 0x1208c0, 0x120ca2, 0x1208b0]:
            counts[f'{pc:08X}'] += 1
        if pc == 0x1a1720:
            sp = g.reg_read(UC_X86_REG_ESP)
            state = uint(read(sp + 4, 4))
            new, other = g.reg_read(UC_X86_REG_EDI), g.reg_read(UC_X86_REG_EBX)
            callbacks.append(dict(priority=g.reg_read(UC_X86_REG_EDX), other_priority=g.reg_read(UC_X86_REG_ECX),
                new_callback=uint(read(sp + 8, 4)), other_callback=uint(read(sp + 12, 4)),
                new_file=uint(read(new, 4)), new_offset=uint(read(new + 12, 4)),
                other_file=uint(read(other, 4)), other_offset=uint(read(other + 12, 4)),
                state_before=words(read(state, 12))))
        if pc == stop[0]:
            g.emu_stop()

    def write(g, access, address, size, value, data):
        if not SYNTH <= address < SYNTH + 0x100000:
            for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
                dirty.add(at)
            writes.append((address, size))

    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', snapshot.report['registers_before']).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for reg, value in [(UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16), (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)]:
        u.reg_write(reg, value)

    initial_bank = snapshot.read(NODES, 150 * 64)
    actual_counter = uint(snapshot.read(COUNTER, 4))
    actual_added = uint(snapshot.read(ADDED, 4))
    actual_nodes = [words(initial_bank[i * 64:(i + 1) * 64]) for i in range(150)]
    for node in actual_nodes:
        node[15] = NONE

    def reset():
        for at in dirty:
            u.mem_write(at, pages[at])
        dirty.clear()
        writes.clear()
        callbacks.clear()
        boundary.clear()
        stop[0] = 0
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
        u.mem_write(STACK, packed([STOP, 0, 0, 0]))

    def run(entry):
        try:
            u.emu_start(entry, STOP, count=200000)
        except UcError:
            if not boundary:
                raise
        pc = u.reg_read(UC_X86_REG_EIP)
        assert pc in [STOP, stop[0]] or boundary, (hex(pc), hex(entry))
        return pc

    def pointer(index, count):
        assert index == NONE or index < count
        return 0 if index == NONE else NODES + index * 64

    def ordinal(ptr, count):
        if not ptr:
            return NONE
        assert NODES <= ptr < NODES + count * 64 and (ptr - NODES) % 64 == 0
        return (ptr - NODES) // 64

    def original(values):
        reset()
        kind = values[0]
        if kind == 0:
            assert len(values) == 12
            u.mem_write(STATE, packed(values[9:12]))
            u.mem_write(NEWTASK, packed([values[5], 0, 0, values[6]]))
            u.mem_write(NEXTTASK, packed([values[7], 0, 0, values[8]]))
            u.mem_write(STACK, packed([STOP, STATE, values[3], values[4]]))
            for reg, value in [(UC_X86_REG_EDX, values[1]), (UC_X86_REG_ECX, values[2]), (UC_X86_REG_EDI, NEWTASK), (UC_X86_REG_EBX, NEXTTASK)]:
                u.reg_write(reg, value)
            assert run(0x1a1720) == STOP and not boundary and not writes
            result = [u.reg_read(UC_X86_REG_EAX) & 255, *words(read(STATE, 12))]
            assert result == compare_model(values), (values, result, compare_model(values))
            return result
        owned_phase = kind in [7, 8, 9]
        width = 8 if owned_phase else 7
        if owned_phase:
            _, head, argument, free, counter, count, added, reserved = values[:8]
            assert count == 150
        else:
            _, head, argument, extra, counter, count, reserved = values[:7]
        assert 0 <= count <= 150 and reserved == 0 and len(values) == width + count * 16
        nodes = [values[width + i * 16:width + (i + 1) * 16].copy() for i in range(count)]
        for node in nodes:
            node[15] = pointer(node[15], count)
        if nodes:
            put(NODES, packed([v for node in nodes for v in node]))
        put(HEAD, packed([pointer(head, count)]))
        put(COUNTER, packed([counter]))
        if owned_phase:
            put(FREE, packed([pointer(free, count)]))
            put(ADDED, packed([added]))
        if kind in [1, 2]:
            assert argument < count
            u.mem_write(STACK, packed([STOP, NODES + argument * 64]))
            if kind == 1:
                assert run(0x120900) == STOP
                result = 0  # Original void routine; no invented AL result.
            else:
                u.reg_write(UC_X86_REG_ESI, NODES + argument * 64)
                stop[0] = 0x120a08
                assert run(0x1209d1) == stop[0]
                result = u.reg_read(UC_X86_REG_ESI)
        elif kind == 3:
            assert argument & 255 < count
            u.reg_write(UC_X86_REG_ESI, argument)
            u.mem_write(STACK, packed([0, STOP, extra]))  # Actual saved EBX and priority slot.
            stop[0] = 0x120d3c
            assert run(0x120cf0) == stop[0]
            result = u.reg_read(UC_X86_REG_EBX) & 255
        elif kind == 4:
            stop[0] = 0x120c18
            assert run(0x120c00) == stop[0]
            result = u.reg_read(UC_X86_REG_ESI)
        elif kind == 5:
            u.reg_write(UC_X86_REG_ESI, argument)
            stop[0] = 0x2d62c4
            assert run(0x120b60) == stop[0]
            result = u.reg_read(UC_X86_REG_EBX) & 255
        elif kind == 6:
            assert argument < count
            u.reg_write(UC_X86_REG_ESI, NODES + argument * 64)
            stop[0] = 0x2d62c4
            assert run(0x120a3d) == stop[0]
            result = 0  # Original void routine.
        elif kind == 7:
            stop[0] = 0x2d626c
            assert run(0x120a90) == stop[0]
            assert words(read(u.reg_read(UC_X86_REG_ESP) + 4, 12)) == [0, 0, 0]
            result = 0  # Initialization prefix has no original return here.
        elif kind == 8:
            u.reg_write(UC_X86_REG_ESI, 0)
            stop[0] = 0x2d62c4
            assert run(0x1208c0) == stop[0]
            result = ordinal(u.reg_read(UC_X86_REG_ESI), count)
        elif kind == 9:
            assert argument < count
            u.reg_write(UC_X86_REG_ESI, NODES + argument * 64)
            stop[0] = 0x2d62c4
            assert run(0x120ca2) == stop[0]
            result = 0  # Void ownership phase, not a completed worker iteration.
        else:
            raise ValueError('unknown queue fixture kind')
        assert not boundary
        final = []
        for i in range(count):
            node = words(read(NODES + i * 64, 64))
            node[15] = ordinal(node[15], count)
            final.extend(node)
        output = [result, ordinal(uint(read(HEAD, 4)), count), uint(read(COUNTER, 4))]
        if owned_phase:
            output.extend([ordinal(uint(read(FREE, 4)), count), uint(read(ADDED, 4))])
        output.extend(final)
        for address, size in writes:
            assert (NODES <= address and address + size <= NODES + count * 64) or address in [HEAD, COUNTER, FREE, ADDED]
        return output

    rows, labels, output, traces = [], [], [], []
    def add(row, label, trace=False):
        rows.append(row)
        labels.append(label)
        result = original(row)
        output.append(result)
        if trace:
            traces.append(dict(input=row, source_output=result, label=label, ordered_comparisons=callbacks.copy()))

    priorities = [0, 1, 2, NONE, 0x7fffffff, 0x80000000, 0x7fc00000, 0xff800000]
    for p in priorities:
        for q in priorities:
            for cb, other_cb in [(READ, READ), (READ, OTHER), (OTHER, READ), (OTHER, OTHER)]:
                for valid in [0, 1, 2, 255]:
                    add([0, p, q, cb, other_cb, 0xfffffffc, 3, 4, NONE, 2, 5, 0xaabbcc00 | valid], 'signed priority/callback/valid-byte matrix')
    edges = [0, 1, 0x7fffffff, 0x80000000, NONE]
    for base in edges:
        for incoming in edges:
            for other in edges:
                add([0, 2, 2, READ, READ, incoming, incoming, other, other, base, base, 1], 'unsigned file/offset modular edge')
                add([0, 2, 2, READ, READ, 4, incoming, 4, other, 4, base, 1], 'same-file unsigned offset modular edge')
    rng = random.Random(0x1209001a1720)
    for _ in range(2048):
        random_words = [rng.getrandbits(32) for _ in range(9)]
        add([0, random_words[0], random_words[1], rng.choice([READ, OTHER, 0, NONE]), rng.choice([READ, OTHER, 0, NONE]), *random_words[2:]], 'raw integer comparator controls')

    def node(index, priority=2, callback=READ, file=4, offset=0, state=NONE, category=3):
        record = words(bytes((index * 17 + i * 37 + (i >> 3)) & 255 for i in range(64)))
        record[0], record[1], record[2], record[5], record[12], record[13], record[15] = priority, state, file, offset, category, callback, NONE
        return record

    def bank_row(kind, bank, order, argument, extra=0, counter=actual_counter):
        bank = [v.copy() for v in bank]
        for a, b in zip(order, order[1:]):
            bank[a][15] = b
        if order:
            bank[order[-1]][15] = NONE
        return [kind, order[0] if order else NONE, argument, extra, counter, len(bank), 0, *[v for record in bank for v in record]]

    # Retained actual rows are currently free. Membership and exclusive node
    # leases are explicitly supplied; neither a wait nor a free-list pop is faked.
    for index in range(150):
        add(bank_row(1, actual_nodes, [], index), 'retained source node bytes / supplied exclusive insertion lease', index == 33)
    for incoming_offset in [50, 200, 400]:
        bank = [node(0, offset=900), node(1, offset=100), node(2, offset=300), node(3, offset=incoming_offset)]
        add(bank_row(1, bank, [0, 1, 2], 3), 'literal read-head barrier / local rotational state', True)
    for head_callback in [READ, OTHER]:
        for next_callback in [READ, OTHER]:
            for incoming_callback in [READ, OTHER]:
                bank = [node(0, callback=head_callback), node(1, callback=next_callback), node(2, callback=incoming_callback)]
                add(bank_row(1, bank, [0, 1], 2), 'equal-priority callback precedence below head', True)
    for _ in range(512):
        length = rng.randrange(0, 12)
        bank = [node(i, rng.choice(priorities), rng.choice([READ, OTHER, 0]), rng.getrandbits(32), rng.getrandbits(32)) for i in range(length + 1)]
        order = sorted(range(length), key=lambda i: signed(bank[i][0]), reverse=True)
        add(bank_row(1, bank, order, length), 'complete insertion over explicit descending-priority bank')

    for index in range(150):
        add(bank_row(2, actual_nodes, [], index), 'all150 source node ordinals / enqueue-ID critical section')
    for index in [0, 1, 33, 148, 149]:
        for counter in [0, 1, 0x00ffffff, 0x01000000, 0x7fffffff, 0xfffffffe, NONE]:
            add(bank_row(2, actual_nodes, [], index, counter=counter), 'counter shift/truncation/wrap controls', index == 33)
    for _ in range(128):
        bank = [node(i, rng.choice(priorities), rng.choice([READ, OTHER]), rng.getrandbits(32), rng.getrandbits(32)) for i in range(6)]
        order = sorted(range(5), key=lambda i: signed(bank[i][0]), reverse=True)
        add(bank_row(2, bank, order, 5, counter=rng.getrandbits(32)), 'enqueue-ID plus full insertion over explicit bank')

    for _ in range(256):
        count = rng.randrange(1, 13)
        counter = rng.getrandbits(32)
        bank = [node(i, rng.choice(priorities), rng.choice([READ, OTHER]), rng.getrandbits(32), rng.getrandbits(32), ((counter << 8) | i) & NONE, rng.choice([0, 3, NONE])) for i in range(count)]
        order = sorted(range(count), key=lambda i: signed(bank[i][0]), reverse=True)
        target = rng.randrange(count)
        job = bank[target][1]
        for stale in [False, True]:
            add(bank_row(3, bank, order, job ^ (0x100 if stale else 0), extra=rng.choice(priorities)), 'priority-change ID match' if not stale else 'stale priority-change ID rejected')
        add(bank_row(6, bank, order, target), 'unlink head/interior/tail preserves removed-next and clears ID')
    for length in range(151):
        bank = [node(i, category=i % 5) for i in range(length)]
        order = list(range(length))
        rng.shuffle(order)
        add(bank_row(4, bank, order, 0), 'complete owned count0..150')
        for category in [0, 4, NONE]:
            add(bank_row(5, bank, order, category), 'complete owned category found/absent')

    def ownership_row(kind, bank, work, free, argument=0, counter=actual_counter, added=actual_added):
        assert len(bank) == 150 and len(set(work + free)) == len(work + free)
        bank = [record.copy() for record in bank]
        for record in bank:
            record[15] = NONE
        for order in [work, free]:
            for a, b in zip(order, order[1:]):
                bank[a][15] = b
        if kind == 9:
            assert argument not in work + free
            bank[argument][1] = NONE
            # Actual120A30 preserves the removed head's old next pointer.
            bank[argument][15] = work[0] if work else NONE
        return [kind, work[0] if work else NONE, argument, free[0] if free else NONE,
            counter, 150, added, 0, *[v for record in bank for v in record]]

    rotation = lambda index: list(range(index, 150)) + list(range(index))
    add(ownership_row(7, actual_nodes, [], list(range(150))), 'literal retained bank / complete initialization prefix')
    counter_edges = [0, 1, 0x00ffffff, 0x01000000, 0x7fffffff, 0x80000000, NONE]
    for counter in counter_edges:
        for added in counter_edges:
            bank = [node(i, priority=rng.getrandbits(32), state=rng.getrandbits(32)) for i in range(150)]
            add(ownership_row(7, bank, [0], list(range(1, 150)), counter=counter, added=added),
                'initialization full-bank clear / both counters preserved')
    for head in range(150):
        order = rotation(head)
        bank = [node(i, priority=rng.getrandbits(32), state=rng.getrandbits(32)) for i in range(150)]
        add(ownership_row(7, bank, order[:37], order[37:], counter=rng.getrandbits(32), added=rng.getrandbits(32)),
            'initialization all150 work-head positions / full150 node outputs')

    add(ownership_row(8, actual_nodes, [], []), 'empty free take availability boundary / no whole return')
    add(ownership_row(8, actual_nodes, [], list(range(150))), 'literal retained free bank / owned take prefix')
    for head in range(150):
        order = rotation(head)
        add(ownership_row(8, actual_nodes, [], order, counter=rng.getrandbits(32), added=rng.getrandbits(32)),
            'owned take all150 full-chain free heads / exact payload preservation')
        add(ownership_row(8, actual_nodes, order[1:], [head], counter=rng.getrandbits(32), added=rng.getrandbits(32)),
            'owned take all150 single-node free heads / work bank unchanged')

    for node_index in range(150):
        others = rotation((node_index + 1) % 150)[:-1]
        assert node_index not in others
        add(ownership_row(9, actual_nodes, [], others, node_index, counter=rng.getrandbits(32), added=rng.getrandbits(32)),
            'post-unlink return all150 node/free-head positions / payload preserved')
        add(ownership_row(9, actual_nodes, others, [], node_index, counter=rng.getrandbits(32), added=rng.getrandbits(32)),
            'post-unlink return to empty free list / preserved removed-next overwritten')
    order = rotation(34)
    order.remove(33)
    add(ownership_row(9, actual_nodes, order[:75], order[75:], 33),
        'post-unlink return with disjoint nonempty work/free lists / full bank preserved')

    diagnostics = []
    for entry in [0x1209c0, 0x120ce0, 0x120bf0, 0x120b50, 0x120a30, 0x1208b0]:
        reset()
        u.reg_write(UC_X86_REG_EAX, NODES + 33 * 64)
        u.reg_write(UC_X86_REG_ESI, NODES + 33 * 64 if entry == 0x120a30 else actual_nodes[33][1])
        u.mem_write(STACK, packed([STOP, 2]))
        run(entry)
        assert len(boundary) == 1 and boundary[0]['entry'] == '80020F5A'
        assert boundary[0]['stack'] == [4 if entry == 0x1208b0 else 8, 1, 0, 0]
        diagnostics.append(dict(entry=f'{entry:08X}', completed=False, boundary=boundary.copy()))
    reset()
    assert read(NODES, 9600) == initial_bank
    assert uint(read(COUNTER, 4)) == actual_counter
    assert read(HEAD, 4) == snapshot.read(HEAD, 4)
    assert read(FREE, 4) == snapshot.read(FREE, 4)
    assert uint(read(ADDED, 4)) == actual_added

    encoded = [' '.join(f'{v:08X}' for v in row) for row in rows]
    if args.binary:
        result = subprocess.run([str(args.binary.resolve()), '--probe'], input='\n'.join(encoded) + '\n', text=True, capture_output=True, check=True)
        actual = [[int(v, 16) for v in line.split()] for line in result.stdout.splitlines()]
        assert len(actual) == len(output)
        for i, (native, source) in enumerate(zip(actual, output)):
            assert native == source, (i, labels[i], rows[i][:7], native[:20], source[:20])
    fixture = ROOT / 'tests/fixtures/retail_source_async_queue.tsv'
    fixture.write_text('# Kind0: comparator12words->bool+state3words. Kinds1..6: kind/head/indexOrID/arg/counter/count/0 +count*rawnode16words -> result/head/counter +nodes. Kinds7..9: kind/workHead/index/freeHead/counter/150/requestsAdded/0 +150*rawnode16words -> result/workHead/counter/freeHead/requestsAdded +nodes. Head/next useindex/NONE; kind8NONE is availability, notwhole1208B0return.\n' +
        '\n'.join(encoded[i] + ' | ' + ' '.join(f'{v:08X}' for v in output[i]) for i in range(len(rows))) + '\n')
    report = dict(boundary=__doc__, source_xbe_sha256=PIN, snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha((snapshot.directory / 'export.json').read_bytes()), tool_sha256=sha(Path(__file__).read_bytes()),
        fixture_sha256=sha(fixture.read_bytes()), native_binary_sha256=sha(args.binary.read_bytes()) if args.binary else None,
        helper_sha256=sha((ROOT / 'src/platform/source_async_jobs.cpp').read_bytes()),
        helper_header_sha256=sha((ROOT / 'include/halo2/source_async_jobs.hpp').read_bytes()),
        native_test_sha256=sha((ROOT / 'tests/source_async_jobs_tests.cpp').read_bytes()),
        cases=len(rows), checked_words=sum(len(row) for row in output), kind_counts=dict(collections.Counter(row[0] for row in rows)),
        label_counts=dict(collections.Counter(labels)), unique_instructions=len(instructions), source_entries=dict(counts),
        instruction_sha256=sha(b''.join(packed([pc]) + image.read(pc, n) for pc, n in sorted(instructions.items()))),
        fpcw='023F', mxcsr='1F80', supplied_configuration=True,
        retained_nodes_sha256=sha(initial_bank), retained_task_counter=actual_counter, retained_requests_added=actual_added,
        actual_callback_counts=dict(collections.Counter(f'{v[13]:08X}' for v in actual_nodes)),
        exact_traces=traces, whole_wait_boundaries=diagnostics, services_replaced=[], source_instruction_patches=[],
        node_layout={'bank': '004E0368', 'capacity': 150, 'stride': 64,
            '00': 'signed 32-bit priority', '04': 'raw 32-bit request ID/state',
            '08..2F': '40-byte task payload; read file handle at08, offset at14',
            '30': 'category', '34': 'callback', '38': 'done pointer/token', '3C': 'next pointer'},
        fixture_kind_scope={'0': 'complete 1A1720', '1': 'complete 120900',
            '2': '1209D1 through before120A08; explicit post-wait ownership',
            '3': '120CF0 through before120D3C; explicit post-wait ownership',
            '4': '120C00 through before120C18; explicit post-wait ownership',
            '5': '120B60 through before2D62C4 release; explicit post-wait ownership',
            '6': '120A3D through before2D62C4 release; explicit post-wait ownership',
            '7': '120A90 through before first2D626C CreateMutex; no kernel startup',
            '8': '1208C0 withESI0 through before2D62C4; explicit free-list ownership; empty result is availability boundary',
            '9': '120CA2 withESI node through before2D62C4; explicit free-list ownership and post-unlink admission'},
        abi={'1A1720': dict(registers={'EDX': 'new signed priority', 'ECX': 'next signed priority', 'EDI': 'new task pointer', 'EBX': 'next task pointer'}, stack=['state pointer', 'new callback', 'next callback'], returns='AL boolean; RET C hex'),
            '120900': dict(stack=['new node pointer'], returns='void; RET4'),
            '1209D1..120A08': dict(input='ESI node pointer; starts before complete120900 insertion; ends after request-ID/counter stores, before work mutex release', result='ESI request ID'),
            '120CF0..120D3C': dict(input='ESI request ID, BL0, priority atESP+8; post-wait pure core', result='BL match/result'),
            '120C00..120C18': dict(input='ESI0; post-wait pure count core', result='ESI count'),
            '120B60..release': dict(input='ESI category, BL0; post-wait pure search core', result='BL found'),
            '120A3D..release': dict(input='ESI queued node pointer; post-wait pure unlink core', result='void; node stateNONE, removed next preserved'),
            '120A90..firstCreateMutex': dict(input='Full admitted node bank; both counters explicit', result='All150 nodes zeroed except stateNONE/linked next; freehead0, workheadNONE; both counters preserved; no original return'),
            '1208C0..release': dict(input='ESI0; explicit post-wait free-list ownership', result='ESI leased node pointer or0; freehead popped and leasednext0; empty is notwholecallerreturn'),
            '120CA2..release': dict(input='ESI node pointer absent bothlists withstateNONE; explicit owned post-unlink phase', result='node.next=oldfreehead; freehead=node; no original wholeworkerreturn')},
        rules=dict(comparator='Signed priority descending. Equal priority read callback1A0E70 beats other callbacks. Bothreads compare unsigned modular file then offset distances only after valid previous-compared-read state. Any compared-next read updates state, even on differing priority.',
            insertion='Empty bank/head strictly lower priority permits newhead. Otherwise comparator begins head.next; no equal-priority task can pass the original head. State12bytes initializeszero on every insertion and is not seededfromhead.',
            request_id='For admitted0..149nodeordinals: ((counter<<8)|ordinal)&FFFFFFFF; counter incrementswith32-bitwrap. No semaphore release or caller return is represented by the core fixture.',
            priority='Low8 ID bits select node, complete32-bit state mustmatch. Match removes/reinsertswithnewpriority, preservingID/counter/taskdata/done/category/callback. No semaphorecount changes in this body.',
            ownership_phases='Initialization stops before firstCreateMutex. Free take/return start at literalowned instructions and stop before release; no wait result, release status or worker completion is supplied. All150 node bytes, bothheads and bothcounters are frozen. Empty free take means availabilityboundary, nottheoriginalretryingcallerreturn.',
            safety='Only explicit finite acyclic queue banks with owned nodes0..149 are admitted. UnlinkedmatchingID, corrupted links and out-of-bank ordinals are not given synthetic traversal or status outcomes.',
            authority='Frozen outputs are unchanged executable outputs; Python comparator transcription is an independent assertion only. Pure bodies have explicit exclusive-bank admission and do not prove successful mutex ownership. Whole caller prefixes stop before actualNtWaitForSingleObjectEx.'),
        protocol='See fixtureheader and tools/oracle_source_async_queue.py. Nodeword15/head normalize actualNODES pointers toordinal/NONE only; all15otherwords retain original bits.')
    path = ROOT / 'analysis/multiplayer-shading/source-async-queue-original.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps({k: v for k, v in report.items() if k not in ['exact_traces']}, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ['cases', 'checked_words', 'kind_counts', 'unique_instructions', 'source_entries', 'actual_callback_counts']}, indent=2))

if __name__ == '__main__':
    main()
