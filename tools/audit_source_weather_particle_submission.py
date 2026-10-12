#!/usr/bin/env python3
"""Read-only weather particle draw/visibility inspection and actual resource query.

391F0 and its draw setup are source-inspected only. This tool never executes
391F0, chooses an active view, advances simulation, accesses a session, or
supplies device/residency/wait/I/O outcomes. Complete unchanged12DE70 mode3
resource queries are replayed over literal immutable Lockout banks. A captured
36B50 entry additionally executes its original clock/admission caller on an
owned clone, stopping before any admitted wind/simulation/draw call. Unknown
original services stop at entry without replacement. The live original and
immutable input snapshots are never advanced or changed.
"""
from pathlib import Path
import argparse, collections, csv, hashlib, json, re, struct, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_OP_MEM, CS_OP_IMM
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed
from audit_rendering import Cache
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe, FUNCTIONS_CSV
from disasm import listing


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def shorts(raw):
    return list(struct.unpack('<' + 'h' * (len(raw) // 2), raw))


def floats(raw):
    return list(struct.unpack('<' + 'f' * (len(raw) // 4), raw))


def original_resource_query(snapshot, image, descriptor, geometry):
    synth, stack, stop = 0x20000000, 0x200fe000, 0x200ff000
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(synth, 0x100000)
    pages, dirty, seen, calls, changes, boundaries = {}, set(), {}, collections.Counter(), [], []
    regions = [('descriptor', descriptor, 36), ('manager', geometry['manager'], 112),
        ('physical_pool', geometry['physical_pool'], 76), ('cache_pool', geometry['cache_pool'], 76),
        ('physical_rows', geometry['physical_rows'], geometry['physical_count'] * 24),
        ('cache_rows', geometry['cache_rows'], geometry['cache_count'] * 12)]
    allowed = [(at, at + n) for _, at, n in regions]
    watch = {0x12de70, 0x12e3a0, 0x13d370, 0x16b840, 0x16b910, 0x223c70,
        0x213760, 0x1208b0, 0x120ce0, 0x125d60, 0x2d2084}

    def page(address):
        at = address & ~4095
        if at not in pages:
            raw = snapshot.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw

    def read(address, size):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            if not synth <= at < synth + 0x100000:
                page(at)
        return bytes(u.mem_read(address, size))

    def missing(g, access, address, size, value, _):
        try:
            read(address, size)
            return True
        except ValueError as error:
            boundaries.append(dict(kind='uncaptured original memory/MMIO; not replaced',
                pc=f'{g.reg_read(UC_X86_REG_EIP):08X}', address=f'{address:08X}', reason=str(error)))
            g.emu_stop()
            return False

    def code(g, pc, size, _):
        if pc == stop:
            g.emu_stop()
            return
        try:
            source = image.read(pc, size)
        except (ValueError, KeyError):
            boundaries.append(dict(kind='actual kernel/external instruction not executed', pc=f'{pc:08X}',
                eax=f'{g.reg_read(UC_X86_REG_EAX):08X}', ecx=f'{g.reg_read(UC_X86_REG_ECX):08X}',
                stack=words(read(g.reg_read(UC_X86_REG_ESP) + 4, 24))))
            g.emu_stop()
            return
        assert read(pc, size) == source, f'changed original instruction {pc:08X}'
        assert not any(at < pc + size and pc < end for at, end in allowed)
        seen[pc] = size
        if pc in watch:
            calls[f'{pc:08X}'] += 1

    def writing(g, access, address, size, value, _):
        if not synth <= address < synth + 0x100000:
            assert any(at <= address and address + size <= end for at, end in allowed)
            changes.append(dict(address=f'{address:08X}', bytes=size, before=read(address, size).hex(),
                after=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))
            dirty.update(range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096))

    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',
        snapshot.report['registers_before']).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for reg, value in [(UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16),
        (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)]:
        u.reg_write(reg, value)
    for reg in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
        UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]:
        u.reg_write(reg, 0)
    # Actual weather caller supplies flags3 and descriptor=system+40.
    # These are explicit fresh-call inputs, not paused CPU register contents.
    u.reg_write(UC_X86_REG_EAX, 3)
    u.reg_write(UC_X86_REG_ECX, descriptor)
    u.reg_write(UC_X86_REG_ESP, stack)
    u.reg_write(UC_X86_REG_EFLAGS, 2)
    u.reg_write(UC_X86_REG_FPCW, 0x023f)
    u.reg_write(UC_X86_REG_MXCSR, 0x1fa0)
    u.mem_write(stack, packed([stop]))
    before = {name: read(at, n) for name, at, n in regions}
    try:
        u.emu_start(0x12de70, stop, count=2000000)
    except UcError:
        if not boundaries:
            raise
    complete = not boundaries
    if complete:
        assert u.reg_read(UC_X86_REG_EIP) == stop and u.reg_read(UC_X86_REG_ESP) == stack + 4
    else:
        assert u.reg_read(UC_X86_REG_EIP) != stop
    after = {name: read(at, n) for name, at, n in regions}
    result = dict(descriptor=f'{descriptor:08X}', EAX_input='00000003', ECX_input=f'{descriptor:08X}',
        completed=complete, AL=u.reg_read(UC_X86_REG_EAX) & 255 if complete else None,
        boundaries=boundaries, source_calls=dict(calls), source_writes=changes,
        regions={name: dict(address=f'{at:08X}', bytes=n, before_sha256=sha(before[name]),
            after_sha256=sha(after[name])) for name, at, n in regions},
        after_descriptor=after['descriptor'].hex(),
        verified_original_instructions=len(seen),
        instruction_sha256=sha(b''.join(packed([pc]) + image.read(pc, n) for pc, n in sorted(seen.items()))),
        supplied_fpcw='023F', supplied_mxcsr='1FA0', source_instructions_patched=[], services_replaced=[],
        original_readiness_or_completion_result_supplied=False, live_original_advanced=False)
    for at in dirty:
        u.mem_write(at, pages[at])
    for at, raw in pages.items():
        assert bytes(u.mem_read(at, 4096)) == raw
    result['restored_pages_checked'] = len(pages)
    return result


def original_admission(snapshot, image):
    registers = snapshot.report['registers_before']
    pc = int(re.search(r'EIP=([0-9a-fA-F]+)', registers)[1], 16)
    if pc != 0x36b50:
        return dict(executed=False, captured_PC=f'{pc:08X}',
            reason='Captured phase is not36B50 entry. No active view is invented and no admission caller is replayed.')
    synth, stack, stop = 0x21000000, 0x210fe000, 0x210ff000
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(synth, 0x100000)
    pages, dirty, seen, calls, changes, boundaries = {}, set(), {}, collections.Counter(), [], []
    command = uint(snapshot.read(0x50942c, 4))
    mutable = [(0x509418, 0x509430), (0x4670fc, 0x467104)]
    if command:
        mutable.append((command, command + 24))
    admission = [None]
    stop_calls = {0x4b3d0, 0x37260, 0x38450, 0x37a60, 0x391f0}

    def page(address):
        at = address & ~4095
        if at not in pages:
            raw = snapshot.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw

    def read(address, size):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            if not synth <= at < synth + 0x100000:
                page(at)
        return bytes(u.mem_read(address, size))

    def missing(g, access, address, size, value, _):
        try:
            read(address, size)
            return True
        except ValueError as error:
            boundaries.append(dict(kind='uncaptured original memory/MMIO; not replaced',
                pc=f'{g.reg_read(UC_X86_REG_EIP):08X}', address=f'{address:08X}', reason=str(error)))
            g.emu_stop()
            return False

    def code(g, address, size, _):
        if address == stop:
            g.emu_stop()
            return
        if address in stop_calls:
            boundaries.append(dict(kind='Actual admitted wind/simulation/draw entry; callee not executed',
                pc=f'{address:08X}', eax=f'{g.reg_read(UC_X86_REG_EAX):08X}',
                ecx=f'{g.reg_read(UC_X86_REG_ECX):08X}', ebx=f'{g.reg_read(UC_X86_REG_EBX):08X}',
                esi=f'{g.reg_read(UC_X86_REG_ESI):08X}', stack=words(read(g.reg_read(UC_X86_REG_ESP) + 4, 16))))
            g.emu_stop()
            return
        try:
            original = image.read(address, size)
        except (ValueError, KeyError):
            boundaries.append(dict(kind='Actual kernel/external instruction not executed', pc=f'{address:08X}'))
            g.emu_stop()
            return
        assert read(address, size) == original
        assert not any(at < address + size and address < end for at, end in mutable)
        seen[address] = size
        if address in {0x36b50, 0x398d0}:
            calls[f'{address:08X}'] += 1
        if address == 0x36dd9:
            esp = g.reg_read(UC_X86_REG_ESP)
            admission[0] = dict(palette_flags=list(read(esp + 0x30, 16)),
                distance_words=[f'{w:08X}' for w in words(read(esp + 0x40, 64))],
                distances=floats(read(esp + 0x40, 64)),
                phase='Complete original clipped-cluster/portal admission outputs before simulation or drawing.')

    def writing(g, access, address, size, value, _):
        if not synth <= address < synth + 0x100000:
            assert any(at <= address and address + size <= end for at, end in mutable)
            changes.append(dict(address=f'{address:08X}', bytes=size, before=read(address, size).hex(),
                after=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))
            dirty.update(range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096))

    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', registers).groups())
    read(ga, gl + 1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for reg, value in [(UC_X86_REG_CS, 8), (UC_X86_REG_SS, 16), (UC_X86_REG_DS, 16),
        (UC_X86_REG_ES, 16), (UC_X86_REG_FS, 32)]:
        u.reg_write(reg, value)
    for reg, name in [(UC_X86_REG_EAX, 'EAX'), (UC_X86_REG_EBX, 'EBX'), (UC_X86_REG_ECX, 'ECX'),
        (UC_X86_REG_EDX, 'EDX'), (UC_X86_REG_ESI, 'ESI'), (UC_X86_REG_EDI, 'EDI'), (UC_X86_REG_EBP, 'EBP')]:
        u.reg_write(reg, int(re.search(name + r'=([0-9a-fA-F]+)', registers)[1], 16))
    u.reg_write(UC_X86_REG_ESP, stack)
    u.reg_write(UC_X86_REG_EFLAGS, 2)
    u.reg_write(UC_X86_REG_FPCW, 0x023f)
    u.reg_write(UC_X86_REG_FPTAG, 0xffff)
    u.reg_write(UC_X86_REG_FPSW, 0)
    mxcsr = int(re.search(r'MXCSR=([0-9a-fA-F]+)', registers)[1], 16)
    u.reg_write(UC_X86_REG_MXCSR, mxcsr)
    u.mem_write(stack, packed([stop]))
    before = {f'{at:08X}': read(at, end - at) for at, end in mutable}
    try:
        u.emu_start(0x36b50, stop, count=2000000)
    except UcError:
        if not boundaries:
            raise
    complete = not boundaries
    if complete:
        assert u.reg_read(UC_X86_REG_EIP) == stop and u.reg_read(UC_X86_REG_ESP) == stack + 4
    assert admission[0] is not None, 'Actual admission must finish before any subsequent boundary.'
    after = {f'{at:08X}': read(at, end - at) for at, end in mutable}
    result = dict(executed=True, captured_PC=f'{pc:08X}', completed_whole36B50=complete,
        admission=admission[0], source_calls=dict(calls), original_writes=changes, boundaries=boundaries,
        mutable_banks={key: dict(before_hex=raw.hex(), after_hex=after[key].hex()) for key, raw in before.items()},
        verified_original_instructions=len(seen),
        instruction_sha256=sha(b''.join(packed([at]) + image.read(at, n) for at, n in sorted(seen.items()))),
        supplied_fpcw='023F', supplied_mxcsr=f'{mxcsr:04X}', fresh_empty_x87_stack=True,
        captured_GPR_values_preserved=True, synthetic_return_and_stack=True,
        source_instructions_patched=[], services_replaced=[], simulation_or_draw_executed=False,
        live_original_advanced=False)
    for at in dirty:
        u.mem_write(at, pages[at])
    for at, raw in pages.items():
        assert bytes(u.mem_read(at, 4096)) == raw
    result['restored_pages_checked'] = len(pages)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, default=ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-state1')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer-shading/weather-particle-draw-inspection.json')
    parser.add_argument('--compare', type=Path)
    args = parser.parse_args()
    directory = args.snapshot.resolve()
    snapshot = Snapshot(directory)
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    assert sha(snapshot.ram) == snapshot.report['physical_ram_sha256']
    assert snapshot.report['registers_before'] == snapshot.report['registers_after']
    rows = {int(r['va'], 16): r for r in csv.DictReader(Path(FUNCTIONS_CSV).open())}
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    functions, lines = {}, []
    # Complete function extents come from source inventory; this is static
    # inspection, never an executed instruction or whole-draw proof count.
    for pc in [0x36b50, 0x391f0, 0x39780, 0x12de70, 0x1c590, 0x1cf80,
        0x3fb3e0, 0x3f5730, 0x3f5cf0, 0x3f54f0, 0x400fa0, 0x3fa680, 0x3faae0,
        0x132d0, 0x131ff0, 0x1320f0, 0x132b80, 0x134980]:
        r = rows[pc]
        size = int(r['size'])
        raw = image.read(pc, size)
        instructions = list(md.disasm(raw, pc))
        functions[f'{pc:08X}'] = dict(bytes=size, sha256=sha(raw), source=r['source'],
            static_decoded_instructions=len(instructions), static_extent_records_are_not_execution_counts=True,
            direct_calls=[dict(pc=f'{i.address:08X}', target=f'{i.operands[0].imm:08X}') for i in instructions
                if i.mnemonic == 'call' and i.operands[0].type == CS_OP_IMM],
            absolute_memory=sorted({f'{o.mem.disp & 0xffffffff:08X}' for i in instructions for o in i.operands
                if o.type == CS_OP_MEM and not o.mem.base and not o.mem.index}))
        lines.extend([f'{pc:08X} size {size} sha256 {sha(raw)} source {r["source"]}',
            *listing(raw, pc, rows), ''])

    raw_dir = args.output.parent / (args.output.stem + '-inputs')
    raw_dir.mkdir(exist_ok=True)
    retained = {}

    def bank(name, at, size):
        raw = snapshot.read(at, size)
        path = raw_dir / (name + '.bin')
        path.write_bytes(raw)
        retained[name] = dict(address=f'{at:08X}', bytes=size, sha256=sha(raw),
            path=str(path.relative_to(ROOT)))
        return raw

    bsp = uint(snapshot.read(0x4e0348, 4))
    palette_count, palette_pointer = words(snapshot.read(bsp + 0x84, 8))
    cluster_count, cluster_pointer = words(snapshot.read(bsp + 0x9c, 8))
    assert palette_count == 1 and cluster_count == 16
    palette = bank('palette', palette_pointer, palette_count * 136)
    clusters = bank('clusters', cluster_pointer, cluster_count * 176)
    tables = uint(snapshot.read(0x4e3b44, 4))
    datum = uint(palette, 0x24)
    root = uint(snapshot.read(tables + (datum & 65535) * 16 + 8, 4))
    root_raw = bank('weat-root', root, 176)
    system_count, system_pointer = words(root_raw[:8])
    assert datum == 0xe77d0608 and system_count == 1
    system = bank('particle-system', system_pointer, 140)
    group = uint(system, 0x68)
    group_raw = bank('published-group', group, 16)
    count, render, state_count, state = words(group_raw)
    assert count == 1024 and count == state_count
    rendered = bank('particle-render', render, count * 20)
    states = bank('particle-state', state, count * 32)
    ages = [struct.unpack_from('<f', states, i * 32 + 28)[0] for i in range(count)]
    sizes = [struct.unpack_from('<f', rendered, i * 20 + 12)[0] for i in range(count)]
    colors = collections.Counter(uint(rendered, i * 20 + 16) for i in range(count))
    visibility = bank('visibility-context', 0x547f88, 0x2ae0)
    current = struct.unpack_from('<h', visibility, 8)[0]
    view_pointer = uint(visibility, 12)
    view = bank('clipped-view-header', view_pointer, 20)
    visible_count = struct.unpack_from('<h', view, 4)[0]
    visible_pointer = uint(view, 12)
    visible = words(bank('clipped-visible-clusters', visible_pointer, visible_count * 4))
    admitted = [i for i in range(cluster_count) if struct.unpack_from('<h', clusters, i * 176 + 0x76)[0] == 0]
    assert admitted == [0, 2, 7, 8, 9]
    current_view = struct.unpack('<h', snapshot.read(0x485600, 2))[0]
    players = uint(snapshot.read(0x4e8c20, 4))
    player_raw = bank('players-header', players, 16)
    view_count = struct.unpack_from('<h', player_raw, 8)[0]
    clocks = bank('weather-clock', 0x509418, 12)
    rng = uint(snapshot.read(0x4e7408, 4))
    rng_raw = bank('auxiliary-rng', rng, 8)
    device = uint(snapshot.read(0x407488, 4))
    point_state = bank('SDK-point-state', 0x407668, 32)
    raster_height = bank('SDK-raster-height', device + 0xeec, 4)

    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    _, authored_root = cache.tag(datum, 'weat')
    authored_system = cache.block(authored_root, 140)[0]
    authored = bytes(cache.data[authored_system:authored_system + 140])
    location, resource_size, section = uint(authored, 0x40), uint(authored, 0x44), uint(authored, 0x48)
    selector = location >> 30
    path = (ROOT / 'assets/disc/maps') / ['', 'mainmenu.map', 'shared.map', 'single_player_shared.map'][selector]
    if not selector:
        path = ROOT / 'assets/disc/maps/lockout.map'
    with path.open('rb') as stream:
        stream.seek(location & 0x3fffffff)
        resource = stream.read(resource_size)
    assert len(resource) == resource_size
    raw_dir.joinpath('authored-particle-system.bin').write_bytes(authored)
    payloads = {}
    for address in cache.block(authored_system + 0x50, 16):
        descriptor_raw = bytes(cache.data[address:address + 16])
        kind, offset, size = cache.u16(address + 4), cache.u32(address + 12), cache.u32(address + 8)
        start = section + 8 + offset
        payload = resource[start:start + size]
        assert len(payload) == size and kind not in payloads
        raw_dir.joinpath(f'authored-resource-{kind}.bin').write_bytes(payload)
        runtime = rendered if kind == 0 else states if kind == 8 else b''
        payloads[kind] = dict(descriptor=descriptor_raw.hex(), resource_offset=start, bytes=size,
            sha256=sha(payload), runtime_sha256=sha(runtime), runtime_equal=payload == runtime,
            differing_runtime_bytes=sum(a != b for a, b in zip(payload, runtime)), lengths_equal=len(payload) == len(runtime))
    assert set(payloads) == {0, 8}
    cache.close()

    manager = uint(snapshot.read(0x4e649c, 4))
    manager_raw = bank('geometry-manager', manager, 112)
    physical_pool = uint(manager_raw, 0x64)
    physical_pool_raw = bank('geometry-physical-pool', physical_pool, 76)
    cache_pool = uint(snapshot.read(0x4e648c, 4))
    cache_pool_raw = bank('geometry-cache-pool', cache_pool, 76)
    handle = uint(system, 0x60)
    geometry = dict(manager=manager, physical_pool=physical_pool, physical_rows=uint(physical_pool_raw, 0x44),
        physical_count=uint(physical_pool_raw, 0x20), cache_pool=cache_pool,
        cache_rows=uint(cache_pool_raw, 0x44), cache_count=uint(cache_pool_raw, 0x20))
    physical_row = bank('particle-physical-row', geometry['physical_rows'] + (handle & 65535) * 24, 24)
    cache_row = bank('particle-cache-row', geometry['cache_rows'] + (handle & 65535) * 12, 12)
    resource_query = original_resource_query(snapshot, image, system_pointer + 0x40, geometry)
    assert resource_query['completed'] and resource_query['AL'] == 1
    assert resource_query['source_calls'] == {'0012DE70': 1}
    assert resource_query['source_writes'] == [dict(address=f'{geometry["physical_rows"] + (handle & 65535) * 24 + 20:08X}',
        bytes=4, before=physical_row[20:24].hex(), after=manager_raw[0x38:0x3c].hex())]
    admission_query = original_admission(snapshot, image)

    dependencies = ['tools/differential_source_weather_geometry.py', 'tools/generate_weather_vertex_fixture.py',
        'tests/fixtures/retail_source_weather_geometry.tsv', 'tests/fixtures/retail_weather_vertex_analytic.tsv',
        'analysis/multiplayer-shading/weather-geometry-differential.json',
        'analysis/multiplayer-shading/weather-vertex-validation.json',
        'analysis/multiplayer-shading/weather-gpu-assets/inventory.json',
        'analysis/multiplayer-shading/weather-12fa0-producer-inspection.json',
        'analysis/multiplayer-shading/weather-12fa0-producer-inspection.md']
    report = dict(authority=__doc__, source_xbe_sha256=PIN,
        snapshot=str(directory.relative_to(ROOT)), snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha(directory.joinpath('export.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()), functions=functions, raw_inputs=retained,
        instruction_execution_scope='Only complete12DE70 and, when the capturedPC is36B50, actual admission replay below execute. All draw/control disassembly counts are static inspection.',
        dependencies={path: sha(ROOT.joinpath(path).read_bytes()) for path in dependencies},
        particle=dict(palette_datum=f'{datum:08X}', palette_root=f'{root:08X}', system=f'{system_pointer:08X}',
            authored_system_sha256=sha(authored), published_group=f'{group:08X}', count=count,
            render=f'{render:08X}', state=f'{state:08X}', type=struct.unpack_from('<H', system, 0x6c)[0],
            sprite_datum=f'{uint(system, 4):08X}', authored_maximum=uint(system, 0x2c),
            opacity=floats(system[0x70:0x78]), streak_scale=floats(system[0x78:0x7c])[0],
            line_width=floats(system[0x7c:0x80])[0], elapsed_accumulator=floats(system[0x80:0x84])[0],
            published_group_count=uint(system, 0x64), raw_84_word=f'{uint(system, 0x84):08X}',
            raw_86_byte=system[0x86], raw_88_float=floats(system[0x88:0x8c])[0],
            runtime_linked=system[0x5e], descriptor_flags=system[0x5f], physical_handle=f'{handle:08X}',
            cache_done=cache_row[2], cache_priority=cache_row[3], cache_job=f'{uint(cache_row, 4):08X}',
            cache_owner_descriptor=f'{uint(cache_row, 8):08X}',
            age=dict(positive=sum(a > 0 for a in ages), zero=sum(a == 0 for a in ages),
                negative=sum(a < 0 for a in ages), minimum=min(ages), maximum=max(ages)),
            size=dict(minimum=min(sizes), maximum=max(sizes)),
            colors={f'{color:08X}': n for color, n in sorted(colors.items())},
            baseline_resources=payloads,
            baseline_comparison_scope='Authored decoded resource bytes precede39A80/simulation. Runtime differences do not infer update history, initialization calls, or readiness chronology.'),
        readiness_original_query=resource_query,
        admission_original_query=admission_query,
        visibility=dict(context='00547F88', current_cluster=current, clipped_header=f'{view_pointer:08X}',
            clipped_visible_clusters=visible, authored_palette_clusters=admitted,
            actual_current_cluster_has_palette=current in admitted, visible_palette_clusters=[i for i in visible if i in admitted],
            current_view=current_view, source_player_count=view_count,
            phase='Actual36B50 entry inputs.' if admission_query['executed'] else 'Paused outside active render-view setup. View-1 is not supplied to391F0 and no replacement view0 is chosen.',
            producer='132D0 copies664-byte caller bank to485600;1320F0 owns clipped visibility bank547F88 through132B80/1348E0/173130or132D60or1733E0/134390/132220.',
            PVS_substitution=False),
        clocks=dict(source_elapsed=floats(clocks[:4])[0], prior_weather_time=floats(clocks[4:8])[0],
            rendering_disabled=uint(clocks, 8), source_absolute_double=struct.unpack('<d', snapshot.read(0x4858a0, 8))[0],
            intensity=floats(snapshot.read(0x4670fc, 4))[0], auxiliary_pointer=f'{rng:08X}',
            auxiliary_header=f'{uint(rng_raw):08X}', auxiliary_state=f'{uint(rng_raw, 4):08X}',
            phase='Retained snapshot values. Any original caller clock writes occur only in the separately recorded owned replay; no live advancement or update history is inferred.'),
        immediate_setup=dict(type2='Lines; no143C0 sprite binding;391F0 calls3F5730 withsystem+7C.',
            other_types='Points;143C0 EAX=system+4,ECX0,XMM2=0,stackstage3;desired/current4-stage textures appliedby1CF80.',
            vertex_shader='391F0 passes485A80->+5C->+B4 (ordinal22), index0, shader cache51F0F0 to1C590.',
            camera='391F0 passesEAX48568C/ECX485618,scaled0,mode4858B8 to12FA0.',
            pixel='39780 creates60-worddefinition;BL=(systemWORD+6C==2);pixelbuilder returnsAL1.',
            point_input='render positionXYZ,render size*45DD40(16777215),raw packedcolor;state age+1C>0 ordered.',
            rain_input='start=position-(state velocity*509418)*system+78;end=position;v0.W1;opacity+70/+74*256 x87__ftol2,shift24,rawRGB retained.',
            point_sprite_globals='407670=1,407674=1,407678=0,40767C=0,407680=1;dirty407490|900 fornonrain. Cleanupsets407670/674=0,dirty|900.',
            states=[['00040304', 1], ['00040344', 0x302], ['00040348', 0x303], ['00040300', 1],
                ['00040340', 0], ['0004033C', 0x206], ['0004035C', 0], ['3F54F0', 2], ['00040354', 0x203]],
            cleanup='End417FC=0;device+8 maskFFFFE7FF;conditional3FA680 if oldbit1000;407670/407674=0;method40304 and40300 values0;407584/407588=0;16B10 with4670BC1. No prior-state restoration,3F54F0 disable,or3F5730 reset is present.',
            command_color='Complete3FB3E0 method41940+attribute*4;attribute1=41944;word swapsR/B:((color&FF)<<16)|((color>>16)&FF)|(color&FF00FF00).',
            vertex_command='391F0 writes101518 followedXYZW. Begin417FC1(points)or2(lines),end417FC0. Devicebufferfull branchescall3FAAE0.',
            SDK_line_width='3F5730 computesx87(width*device+964*8+.5), storesfloat,3FCF00CVTTSS2SI,unsignedcap511,method40380;raw40770Cretained.',
            SDK_point_parameters='Complete source-inspected3FF920 consumes407668/66C/684 size/min/max,407674 scale enable,407670 sprite enable,407678/67C/680 coefficients,device+964 and unsigneddevice+EEC rasterheight. Scale-enabled method200A30 emits eightpointparameters;fixed4043C is separately encoded fromminimum,notshaderoPts. Inspectiononly; hardwarecoverageunproved.',
            SDK_stage_selection='Source-inspected403400 usesstage3 when407670!=0,else0;device+784 nonNULLreturnsincomingflags beforethatselection. Parentownscompletebindingbridge.',
            retained_point_state=dict(size=floats(point_state[:4])[0], minimum=floats(point_state[4:8])[0],
                maximum=floats(point_state[28:32])[0], sprite_enable=uint(point_state, 8),
                scale_enable=uint(point_state, 12), coefficients=floats(point_state[16:28]),
                unsigned_raster_height=uint(raster_height),
                chronology='Raw capturedSDKstate;notassumed391F0 entryorhistoricaldrawinputs.'),
            actual_device=f'{device:08X}', actual_line_scale=floats(snapshot.read(device + 0x964, 4))[0],
            draw_execution_claimed=False),
        earlier_oracle_scope=dict(cases=754, words=10274,
            service_hooks=['12DE70','16B10','1C590','12FA0during391F0','143C0','1CF80','400FA0','3F5730',
                '3F54F0','3F5CF0','3FA3D0','3FA680','12D50','3F7330','3FB3E0'],
            synthetic_color_method='C010C010 withcopiedrawcolorinsteadactual3FB3E0',
            complete_original_draw_or_residency_proof=False,
            authority='Existing bounded immediateinput/pixel/clippedpalette/c10-c11 arithmetic only; hashes preserveimmutable dependency identity.'),
        minimum_safe_seam='Explicit actual active-view/rawcamera/projection and clipped visibility;owned already-initialized/publishedparticle arrays;complete resourcequery outcome or unresolvedservice;sourceelapsed/viewpartition;actualsprite selector/cache/stage bank;sourcepixel/VS22 payload;typedordered draw-state/vertexevents. Backendpoint/line rasterization andcommandallocation/pump remainseparateunprovedservices.',
        unknown_inputs=['Actual391F0 entry bank and historicaldraw chronology remainunknown; capturedview=' + str(current_view) + ' belongs onlyto thedeclaredsnapshotphase.',
            'Actual geometry/resource pendingservices if12DE70 isnotlinked;onlyretained linkedbranch completeshere.',
            'Whole391F0 title+SDK commandallocation/dirtyflush/pump effects;existingoracle replacescallees.',
            'Stage3 completebitmap/cache/bind branch andpriorfour-stagetexturebank (parent independentaudit).',
            'ActualNV2A point/line rastercoverage,point limits,reciprocals andsamplerbehavior.'],
        live_session_access=False, GPU_or_live_guest_execution=False, native_activation=False,
        full_391F0_executed=False, source_or_assets_mutated=False, source_calls_replaced=[])
    if args.compare:
        prior = json.loads(args.compare.read_text())
        report['phase_comparison'] = dict(prior_report=str(args.compare.relative_to(ROOT)),
            prior_report_sha256=sha(args.compare.read_bytes()), prior_snapshot=prior['snapshot'],
            prior_snapshot_ram_sha256=prior['snapshot_ram_sha256'],
            prior_visibility=prior['visibility'], current_visibility=report['visibility'],
            prior_clocks=prior['clocks'], current_clocks=report['clocks'],
            prior_particle=prior['particle'], current_particle=report['particle'],
            raw_bank_hashes={name: dict(before=value['sha256'], after=retained[name]['sha256'],
                equal=value['sha256'] == retained[name]['sha256']) for name, value in prior['raw_inputs'].items() if name in retained},
            history_inferred=False)
    out = args.output
    out.write_text(json.dumps(report, indent=2) + '\n')
    out.with_name(out.stem + '-disasm.txt').write_text('\n'.join(lines))
    print(json.dumps({key: report[key] for key in ['particle', 'readiness_original_query', 'admission_original_query', 'visibility', 'clocks']}, indent=2))


if __name__ == '__main__':
    main()
