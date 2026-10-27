#!/usr/bin/env python3
"""Recover whole native TU1.5 multiplayer engine callbacks and dependencies.

Registration inputs, objects and all 51 methods come from the authenticated
original image. Existing named production dispatch owners are reused. Prior
canonical alias routes replaced by direct TU1.5 bodies are audited separately.
Direct callees
inside the original mode/common-engine clusters are recovered transitively;
other services retain the full ABI at the existing required-owner boundary.
This generates C++, never a runtime opcode decoder or a supplied service result.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from types import SimpleNamespace

from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate
from objective_native_tu15_injection import inject_native_objective_owners

REGISTRATION = ((1, 'CTF', 0x47d174), (2, 'Slayer', 0x4822a0),
                (3, 'Oddball', 0x4822a4), (4, 'King', 0x4822a8),
                (7, 'Juggernaut', 0x47a2f8), (8, 'Territories', 0x4822ac),
                (9, 'Assault', 0x47d174))
# Function clusters are a recovery boundary, not a semantic mode classifier.
# Every root is selected by the actual original vtable, independently of these
# ranges. Every included helper must be reached by an actual direct code edge.
CLUSTERS = ((0x1981e0, 0x199000), (0x249440, 0x24dc00),
            (0x25bb50, 0x25c500), (0x2c5d20, 0x2c90d0),
            (0x2c90d0, 0x2cac00))
# Shared equipment, score/stat and event producers reached by the mode owners.
# These complete original bodies have no existing native dispatch owner.
SHARED_ENTRIES = (0x1a8fd0, 0x15e350, 0x1a9080, 0x1a9880, 0x19bc10, 0x19bca0)
# Original direct dependencies reached by the mode callbacks/shared engine
# producers. Recover their remaining .text closure, retaining real native SDK
# boundaries outside that section. No SDK response is supplied here.
DEPENDENCY_ROOTS = (0x2b3e0, 0x2b440, 0x306f0, 0x41480, 0x69cb0,
                    0x13b340, 0x13b360, 0x13bd5e, 0x1508e0, 0x1628a0,
                    0x1628f0, 0x164fd0, 0x25c8f7, 0x25cbde, 0x25d02c)
# Reached from the existing shared CTF/Assault lifecycle owners. Preserve
# their already-routed canonical world/heap/renderer callees while recovering
# the genuinely missing direct .text closure, including later-round reset.
CTF_DEPENDENCY_ROOTS = (0xb2270, 0xb93e0, 0xd0a00, 0x150930, 0x15c840,
                        0x1799f0, 0x208a73, 0x24b800)
# Original13AAC0 invokes this shared engine frame before object movement.
# It schedules real player callbacks, the registered mode tick and phase work.
FRAME_DEPENDENCY_ROOTS = (0x15eba0,)
# Whole2486F0 publishes these comparison callbacks to the original sorting
# owner. Their indirect calls are not direct CFG edges of the shared frame.
FRAME_CALLBACK_PUBLISHERS = ((0x248798, 0x248ec0), (0x248971, 0x248ef0))
# Original13AAC0 invokes this later shared phase after object/light updates.
# It consumes requested phases, advances round timers and publishes results.
POST_FRAME_DEPENDENCY_ROOTS = (0x15f450,)
# Original13AAC0 follows finalization with this player phase. It owns player
# action/cooldown expiry, current-unit checks and nearby-object interactions.
LATE_PLAYER_DEPENDENCY_ROOTS = (0x1503d0, 0x13b990)
# Native DatumPool owns the real allocated datum. Continue the original fresh
# player constructor immediately after16E7F0, with its saved-ESI caller frame.
FRESH_PLAYER_CONTINUATION = (0x14e737, 0x16e7f0, 0x14e73c)
# Already implemented production services needed by isolated engine tests.
# Compile their unchanged whole bodies into a test-only composition; no return
# value or side effect is substituted for an unavailable runtime service.
TEST_SERVICES = (0x154ef0, 0x15dd10, 0x15e1e0, 0x15e070, 0x15ddc0,
                 0x19b1d0, 0x19b180, 0x19baa0, 0x19bb20, 0x19de80, 0x1a8740, 0x14e9e0,
                 0x1f20d0, 0x19b2b0, 0xb1e80, 0x13b4a0, 0x16e940,
                 0x164ab0, 0x1a79a0, 0x1a7a30, 0x1a83b0, 0x1f1fc0, 0x1a87a0,
                 0x1601b0, 0x160570, 0xb1c30, 0x150990, 0xce230, 0x106350,
                 0x15e3a0, 0xbbe00, 0x1a87f0, 0x150900, 0xfce50, 0xbcee0,
                 0x1617a0, 0x1a6830, 0x15b470, 0x248e40, 0x248a70, 0x161530,
                 0x15a1a0, 0x140910, 0x140bc0, 0x14ebe0, 0x1d03a0,
                 0x14e7b0, 0x15ab10, 0x15abc0, 0x15ad50, 0x15ae50, 0x32bde4, 0x1f1f70,
                 0x19ae70, 0x32a660)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def registered_engines(image):
    result = []
    for source_type, name, cell in REGISTRATION:
        obj, = struct.unpack('<I', image.read(cell, 4))
        vtable, = struct.unpack('<I', image.read(obj, 4))
        raw = image.read(vtable, 51 * 4)
        result.append(dict(source_type=source_type, name=name, cell=cell,
                           object=obj, vtable=vtable, methods=struct.unpack('<51I', raw),
                           methods_sha256=hashlib.sha256(raw).hexdigest()))
    assert result[0]['object'] == result[-1]['object']
    return result


def existing_owners(exclude_predicates=()):
    """Entries declared by named owns_entry predicates in the live dispatcher.

    The older canonical alias table is a separate ownership mechanism; callers
    must audit it when classifying a compiled body as a new runtime route.
    """
    dispatcher = (ROOT / 'src/physics/physics_live_world_tu15.cpp').read_text()
    names = set(re.findall(r'\b(\w+_owns_entry)\(c\.target\)', dispatcher))
    names.discard('source_multiplayer_engines_tu15_owns_entry')
    names.difference_update(exclude_predicates)
    entries, inputs = set(), {}
    for path in sorted((ROOT / 'src').rglob('*.cpp')):
        if path.name == 'source_multiplayer_engines_tu15_generated.cpp':
            continue
        source = path.read_text()
        for match in re.finditer(r'bool\s+(\w+_owns_entry)\s*\([^)]*\)\s*\{', source):
            if match[1] not in names:
                continue
            depth, end = 1, match.end()
            while depth:
                depth += (source[end] == '{') - (source[end] == '}')
                end += 1
            body = source[match.end():end]
            # Generated ownership switches enumerate exact entries. Refuse a
            # changed predicate rather than silently losing a service owner.
            cases = re.findall(r'case 0x([0-9a-f]+)u:', body)
            if not cases:
                continue
            entries.update(int(p, 16) for p in cases)
            inputs[str(path.relative_to(ROOT))] = sha(path)
    return entries, inputs


def existing_aliases():
    dispatcher = (ROOT / 'src/physics/physics_live_world_tu15.cpp').read_text()
    assert 'physics_live_entries_tu15::entries' in dispatcher
    path = ROOT / 'src/physics/physics_live_entries_tu15.hpp'
    source = path.read_text()
    rows = re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,Module::(\w+)\}', source)
    assert len(rows) == int(re.search(r'array<Entry,(\d+)>', source)[1])
    aliases = {int(entry, 16):dict(canonical=f'{int(canonical, 16):08x}', module=module)
               for entry, canonical, module in rows}
    assert len(aliases) == len(rows)
    return aliases, {str(path.relative_to(ROOT)):sha(path)}


def inject_native_objective_kernels(source):
    """Semantic native rule math; retain literal bodies for traced controls."""
    include='#include "gameplay/multiplayer/source_objective_native_tu15_internal.hpp"\n'
    if include not in source:source=include+source
    for pc,call,continuation in ((0x249866,'arm',0x24989a),(0x24991c,'decay',0x249934)):
        marker=f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}'
        guard=f'if(source_objective_native_tu15_detail::{call}(s))goto L{continuation:x};\n'
        assert source.count(marker)==1
        if guard not in source:source=source.replace(marker,guard+marker,1)
        target=f'if(s.services.instruction){{s.services.instruction(0x{continuation:x}u);}}'
        if f'L{continuation:x}: '+target not in source:
            assert source.count(target)==1
            source=source.replace(target,f'L{continuation:x}: '+target,1)
    return source


def inject_player_status_phase_observer(source):
    """Observe the unchanged161610 completion; do not replace instructions."""
    include='#include "halo2/source_player_status_phase_tu15.hpp"\n'
    if include not in source:source=include+source
    marker='if(s.services.instruction){s.services.instruction(0x15ed4fu);}'
    call='source_observe_player_status_phase_tu15(s.memory,s.save());\n'
    assert source.count(marker)==1
    if call not in source:source=source.replace(marker,call+marker,1)
    gate='L15e6bc: if(s.services.instruction){s.services.instruction(0x15e6bcu);}'
    observation='L15e6bc: source_observe_player_scoreboard_gate_tu15(s.memory,s.save());\n'
    if observation not in source:
        assert source.count(gate)==1
        source=source.replace(gate,observation+gate[len('L15e6bc: '):],1)
    include='#include "halo2/source_gameplay_fade_phase_tu15.hpp"\n'
    if include not in source:source=include+source
    for address,kind in (('15ed59',0),('15a7c9',1)):
        marker=f'if(s.services.instruction){{s.services.instruction(0x{address}u);}}'
        observation=f'source_observe_gameplay_fade_tu15(s.memory,{kind}u,s.save());\n'
        assert source.count(marker)==1
        if observation not in source:source=source.replace(marker,observation+marker,1)
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--test-services', action='store_true',
                        help='Compile existing shared production services for standalone parity tests')
    args = parser.parse_args()
    prefix = 'multiplayer_engine_services_tu15' if args.test_services else 'multiplayer_engines_tu15'
    if args.output is None:
        args.output = ROOT / ('tests' if args.test_services else 'src') / f'{"" if args.test_services else "source_"}{prefix}_generated.cpp'
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert scratch.is_dir()
    if not args.directory.resolve().is_relative_to(scratch):
        raise ValueError('Generation intermediates must stay under HALO2_TEST_SCRATCH')
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    engines = registered_engines(image)
    existing, reused = existing_owners()
    aliases, alias_inputs = existing_aliases()
    roots = {p for engine in engines for p in engine['methods']}
    starts = {int(row['address'], 16) for row in csv.DictReader(
        (ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),
        delimiter='\t')} | roots
    dependency_bodies, sdk_targets = {}, set()
    dependency_pending = set() if args.test_services else set(DEPENDENCY_ROOTS) - existing
    starts.update(DEPENDENCY_ROOTS)
    while dependency_pending:
        entry = min(dependency_pending)
        dependency_pending.remove(entry)
        if not code.inside(entry):
            sdk_targets.add(entry)
            continue
        body = _trace(code, entry, starts)
        dependency_bodies[entry] = body
        dependency_pending.update((body.calls | body.tail_jumps) - existing - dependency_bodies.keys())
    ctf_dependency_entries = set()
    dependency_pending = set() if args.test_services else set(CTF_DEPENDENCY_ROOTS) - existing - aliases.keys()
    starts.update(CTF_DEPENDENCY_ROOTS)
    while dependency_pending:
        entry = min(dependency_pending)
        dependency_pending.remove(entry)
        if not code.inside(entry):
            sdk_targets.add(entry)
            continue
        body = _trace(code, entry, starts)
        dependency_bodies[entry] = body
        ctf_dependency_entries.add(entry)
        dependency_pending.update((body.calls | body.tail_jumps) - existing - aliases.keys() - dependency_bodies.keys())
    if args.test_services:
        assert set(TEST_SERVICES) <= existing, 'Test services must already have production owners'
        pending = (roots & existing) | set(TEST_SERVICES)
    else:
        pending = (roots | set(SHARED_ENTRIES) | dependency_bodies.keys()) - existing
    bodies = {}
    while pending:
        entry = min(pending)
        pending.remove(entry)
        body = dependency_bodies.get(entry) or _trace(code, entry, starts)
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        bodies[entry] = body
        for target in (() if args.test_services else body.calls | body.tail_jumps):
            if target not in bodies and target not in existing and any(
                    low <= target < high for low, high in CLUSTERS):
                pending.add(target)
    frame_dependency_entries = set()
    for site, target in FRAME_CALLBACK_PUBLISHERS:
        instruction = code.at(site)
        assert instruction.mnemonic == 'push' and instruction.operands[0].imm == target
    frame_roots = set(FRAME_DEPENDENCY_ROOTS) | {target for _, target in FRAME_CALLBACK_PUBLISHERS}
    pending = set() if args.test_services else frame_roots - existing - aliases.keys() - bodies.keys()
    starts.update(frame_roots)
    while pending:
        entry = min(pending)
        pending.remove(entry)
        if not code.inside(entry):
            sdk_targets.add(entry)
            continue
        body = _trace(code, entry, starts)
        bodies[entry] = body
        dependency_bodies[entry] = body
        frame_dependency_entries.add(entry)
        pending.update((body.calls | body.tail_jumps) - existing - aliases.keys() - bodies.keys() - sdk_targets)
    post_frame_dependency_entries = set()
    pending = set() if args.test_services else set(POST_FRAME_DEPENDENCY_ROOTS) - existing - aliases.keys() - bodies.keys()
    starts.update(POST_FRAME_DEPENDENCY_ROOTS)
    while pending:
        entry = min(pending)
        pending.remove(entry)
        if not code.inside(entry):
            sdk_targets.add(entry)
            continue
        body = _trace(code, entry, starts)
        bodies[entry] = body
        dependency_bodies[entry] = body
        post_frame_dependency_entries.add(entry)
        pending.update((body.calls | body.tail_jumps) - existing - aliases.keys() - bodies.keys() - sdk_targets)
    late_player_dependency_entries = set()
    pending = set() if args.test_services else set(LATE_PLAYER_DEPENDENCY_ROOTS) - existing - aliases.keys() - bodies.keys()
    starts.update(LATE_PLAYER_DEPENDENCY_ROOTS)
    while pending:
        entry = min(pending)
        pending.remove(entry)
        if not code.inside(entry):
            sdk_targets.add(entry)
            continue
        body = _trace(code, entry, starts)
        bodies[entry] = body
        dependency_bodies[entry] = body
        late_player_dependency_entries.add(entry)
        pending.update((body.calls | body.tail_jumps) - existing - aliases.keys() - bodies.keys() - sdk_targets)
    allocation_site, allocation_owner, continuation = FRESH_PLAYER_CONTINUATION
    allocation_call = code.at(allocation_site)
    assert allocation_call.mnemonic == 'call' and allocation_call.operands[0].imm == allocation_owner
    assert allocation_site + allocation_call.size == continuation
    assert code.at(0x14e736).mnemonic == 'push' and code.at(0x14e736).op_str == 'esi'
    assert code.at(continuation).op_str == 'esi, eax'
    assert code.at(0x14e7a6).mnemonic == 'pop' and code.at(0x14e7a6).op_str == 'esi'
    assert code.at(0x14e7a7).mnemonic == 'ret'
    fresh_player_dependency_entries = set()
    pending = set() if args.test_services else {continuation} - existing - aliases.keys() - bodies.keys()
    starts.add(continuation)
    while pending:
        entry = min(pending)
        pending.remove(entry)
        if not code.inside(entry):
            sdk_targets.add(entry)
            continue
        body = _trace(code, entry, starts)
        bodies[entry] = body
        dependency_bodies[entry] = body
        fresh_player_dependency_entries.add(entry)
        pending.update((body.calls | body.tail_jumps) - existing - aliases.keys() - bodies.keys() - sdk_targets)
    args.directory.mkdir(parents=True, exist_ok=True)
    sites, instructions, addresses, routines, listings = {}, {}, {}, [], []
    for entry, body in sorted(bodies.items()):
        rows, raw = [], bytearray()
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert bytes(instruction.bytes) == image.read(pc, instruction.size)
            rows.append(f'{pc:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
            raw.extend(pc.to_bytes(4, 'little') + bytes(instruction.bytes))
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
        listing = args.directory / f'{entry:08x}.asm'
        listing.write_text(''.join(rows))
        listings.append(dict(entry=f'{entry:08x}', instructions=len(rows),
            instruction_bytes_sha256=hashlib.sha256(raw).hexdigest(),
            listing_sha256=sha(listing),
            direct_callees=[f'{p:08x}' for p in sorted(body.calls)],
            tail_targets=[f'{p:08x}' for p in sorted(body.tail_jumps)]))
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, width, n in body.tables if width == 4],
            byte_tables=[dict(address=p, width=width, count=n,
                             raw_hex=image.read(p, width * n).hex())
                         for p, width, n in body.tables if width == 1]))
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in bodies}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}':value for p,value in addresses.items()}, routines=routines)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest,
        correspondence=correspondence, additional_entries=Path(__file__)), paired,
        complete_listings=True, code_override=code,
        runtime_header=(f'{prefix}.hpp' if args.test_services else f'halo2/source_{prefix}.hpp'),
        runtime_function=f'source_run_{prefix}')
    args.output.write_text(args.output.read_text().replace(
        'source_physics_live_support_tu15_owns_entry',
        f'source_{prefix}_owns_entry').replace(
        'source_run_physics_live_support_tu15',
        f'source_run_{prefix}'))
    if not args.test_services:
        args.output.write_text(inject_player_status_phase_observer(inject_native_objective_owners(inject_native_objective_kernels(args.output.read_text()))))
    report = json.loads(manifest.read_text())
    alias_replacements = {f'{entry:08x}':dict(**aliases[entry], callback=entry in roots)
                          for entry in sorted(bodies.keys() & aliases.keys())} if not args.test_services else {}
    alias_callbacks = sum(row['callback'] for row in alias_replacements.values())
    report.update(generator_source_sha256=sha(__file__),
        generated_cpp_sha256=sha(args.output), engines=engines,
        callback_roots=len(roots), new_callbacks=0 if args.test_services else len(roots & bodies.keys())-alias_callbacks,
        compiled_callback_targets=len(roots & bodies.keys()),
        prior_alias_callback_replacements=alias_callbacks,
        prior_alias_bindings_replaced=alias_replacements,
        prior_alias_owner_source_hashes=alias_inputs,
        reused_callback_entries=[f'{p:08x}' for p in sorted(roots & existing)],
        reused_owner_source_hashes=reused, instruction_listings=listings,
        gameplay_admission_changed=False, supplied_service_results=[], test_only=args.test_services,
        dependency_closure_entries=[f'{p:08x}' for p in sorted(dependency_bodies)],
        ctf_dependency_closure_entries=[f'{p:08x}' for p in sorted(ctf_dependency_entries)],
        frame_dependency_closure_entries=[f'{p:08x}' for p in sorted(frame_dependency_entries)],
        post_frame_dependency_closure_entries=[f'{p:08x}' for p in sorted(post_frame_dependency_entries)],
        late_player_dependency_closure_entries=[f'{p:08x}' for p in sorted(late_player_dependency_entries)],
        fresh_player_dependency_closure_entries=[f'{p:08x}' for p in sorted(fresh_player_dependency_entries)],
        continuation_entries=[dict(entry=f'{continuation:08x}', allocation_site=f'{allocation_site:08x}',
            allocation_owner=f'{allocation_owner:08x}', eax='genuine allocated player datum',
            ebx='creation record', stack='saved ESI followed by caller return address')],
        frame_indirect_callback_publishers=[dict(site=f'{site:08x}', target=f'{target:08x}')
                                           for site,target in FRAME_CALLBACK_PUBLISHERS],
        dependency_sdk_targets=[f'{p:08x}' for p in sorted(sdk_targets)],
        scope='Whole compiled engine callbacks, mode-local callees and the '
              'remaining .text direct dependency closure; external native SDK, '
              'world/services and complete match acceptance remain required.')
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(bodies=len(bodies), instructions=len(instructions),
        callback_roots=len(roots), new_callbacks=report['new_callbacks'],
        reused_callbacks=len(report['reused_callback_entries']),
        generated_cpp_sha256=sha(args.output))), flush=True)


if __name__ == '__main__':
    main()
