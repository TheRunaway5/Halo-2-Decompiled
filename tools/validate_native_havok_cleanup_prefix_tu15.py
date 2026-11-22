#!/usr/bin/env python3
"""Compare the native Havok pressure cleanup prefix with unchanged TU1.5."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from unittest.mock import patch

import validate_native_havok_reclamation_selection_tu15 as selection
from audit_native_havok_statistics_tu15 import (NEW_DEFAULT, NEW_PIN, ROOT, STACK, WORLD,
                                                Xbe, observe_original, put, set_words, word)


CLEANUP = 0x1cb330
WORLD_STEP = 0x31b1a0
COMPONENT_DESTRUCTION = 0x1d7ca0
BODY_DESTRUCTION = 0x31b610
DIRTY_ISLANDS = 0x282c10
ITERATOR_STORES = {0x1cb6fe, 0x1cb702, 0x1cb70b, 0x1cb713, 0x1cb718,
                   0xbcf3d, 0xbcf4a, 0xbcf50, 0xbcf5b}


def make_cases(image):
    seed = next(case for case in selection.pressure_cases(image) if case['name'] == 'frame_free_1')
    rows = []
    layouts = set()

    def add(name, boundary, *, compose=False, guard=1, gate=2, references=0,
            flags=None, component=False, passes=0, limit=6, mask_fp=False,
            pending=0, following_component=False):
        index = len(rows)
        excluded = tuple((selection.PROPERTY+n*0x20, (0x2001, selection.NONE)) for n in range(4))
        row, _ = selection.build_case(name, index, seed, edits=excluded,
                                      pressure_boundary=boundary if compose else None)
        banks = {bank['address']: bytearray.fromhex(bank['before']) for bank in row['banks']}
        banks.update({0x52d000: bytearray(4096), 0x556000: bytearray(4096),
                      0x56dfcc: bytearray((pending,)),
                      0x46a18c: bytearray(image.read(0x46a18c, 4)),
                      0x4500b8: bytearray(image.read(0x4500b8, 4))})
        manager, queue, active_count = WORLD+0x700, WORLD+0x750, WORLD+0x760
        set_words(banks, 0x52d880, manager)
        set_words(banks, 0x52be44, active_count)
        set_words(banks, active_count, 1)
        set_words(banks, WORLD+0x24, gate)
        set_words(banks, WORLD+0x838, limit)
        set_words(banks, WORLD+0x123c, passes)
        put(banks, 0x48b873, bytes((0 if compose else guard,)))
        set_words(banks, manager+8, queue, int(bool(references)), 0, 0xffffffff)
        set_words(banks, queue, selection.BODY)
        put(banks, selection.BODY+6, struct.pack('<H', references))
        vtable = WORLD+0x140
        set_words(banks, selection.BODY, vtable)
        set_words(banks, vtable, BODY_DESTRUCTION)
        if flags is not None:
            body = selection.OBJECT_BODY + 3*0x200
            set_words(banks, body+0xb4, 0x12340003 if component else 0xffffffff)
            put(banks, selection.OBJECT_HEADER+3*12, struct.pack('<H', 0xe3a0))
            put(banks, body+0xc0, bytes((flags,)))
        if following_component:
            body = selection.OBJECT_BODY + 4*0x200
            set_words(banks, body+0xb4, 0x12340004)
            put(banks, selection.OBJECT_HEADER+4*12, struct.pack('<H', 0xe3a0))
            put(banks, body+0xc0, b'\x81')
        if mask_fp:
            # Unmask an exception with no pending fault. Original 32B3D6 and
            # LDMXCSR then make observable control-only changes at 31B1A0.
            row['before_fp'][0] &= ~0x2
            row['before_fp'][2] &= ~0x80
            row['allow_x87_cw_change'] = True
        row.update(entry=selection.PRESSURE if compose else CLEANUP, compose=compose,
                   stop_before_entries=[boundary],
                   banks=[dict(address=base, before=raw.hex()) for base, raw in sorted(banks.items())])
        rows.append(row)
        layouts.add(tuple(sorted((base, len(bank)) for base, bank in banks.items())))

    add('caller_no_candidate_empty_scan', WORLD_STEP, compose=True)
    add('caller_no_candidate_held_reference_component', COMPONENT_DESTRUCTION,
        compose=True, references=2, flags=0xc1, component=True)
    add('empty_scan_preserves_negative_world_gate', WORLD_STEP, gate=-3, mask_fp=True)
    add('scope_sets_guard_and_finishes_three_passes', WORLD_STEP, guard=0, flags=0x81, passes=1)
    add('initial_scan_bound_preserves_zero_world_gate', WORLD_STEP, gate=0, limit=0)
    add('cleanup_component_without_dirty_flag', COMPONENT_DESTRUCTION, flags=1, component=True)
    add('dirty_island_without_pending_work_resumes_scan', WORLD_STEP, flags=0x41)
    add('last_deferred_reference_dispatch', BODY_DESTRUCTION, references=1)
    add('live_pressure_holds_guard_across_pending_dirty_islands', COMPONENT_DESTRUCTION,
        compose=True, pending=1, flags=0x41, following_component=True)
    add('pending_dirty_islands_resumes_three_scans_under_owned_guard', WORLD_STEP,
        guard=0, pending=1, flags=0x40, passes=1)
    if len(layouts) != 1:
        raise ValueError('Cleanup fixtures disagree on declared backing')
    return rows, layouts.pop()


def observe(image, rows, scratch, regions):
    original_read_text = Path.read_text
    observer = ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp'
    control_check = 'fp.cwd==entry_fp.cwd&&((fp.swd^entry_fp.swd)&0x3800u)==0'

    def read_text(path, *args, **kwargs):
        text = original_read_text(path, *args, **kwargs)
        if path.resolve() != observer:
            return text
        if text.count(control_check) != 1:
            raise ValueError('Physical cleanup control observation anchor changed')
        return text.replace(control_check, '(fp.cwd==entry_fp.cwd||c.value("allow_x87_cw_change",false))'
                            '&&((fp.swd^entry_fp.swd)&0x3800u)==0')

    with patch.object(Path, 'read_text', read_text):
        return observe_original(image, rows, scratch, regions=regions,
            entries=(selection.PRESSURE, 0x149ad0, 0x2335b0, 0x1498a0, selection.SELECTION,
                     selection.PREDICATE, 0xb1870, CLEANUP, 0x1cb6b0, 0x282010, 0x283450,
                     0xbcee0, DIRTY_ISLANDS, 0x32b3d6, 0x32b26c, 0x32b2fe))


def expected(row, original):
    entry, = row['stop_before_entries']
    if not original['completed'] or original['stopped_at'] != entry:
        raise ValueError(f'{row["name"]} escaped its authenticated cleanup boundary')
    _, ecx, _, _, esp, ebp, esi, edi = original['boundary_gp']
    prefix = 'Native memory pressure requires actual'
    if entry == WORLD_STEP:
        packet, = struct.unpack('<I', word(original['banks'], esp+4))
        seconds, inverse = struct.unpack('<II', word(original['banks'], packet, 8))
        message = f'{prefix}31B1A0 cleanup world step for world {ecx:08X} and step {seconds:08X} {inverse:08X}'
    elif entry == COMPONENT_DESTRUCTION:
        message = f'{prefix}1D7CA0 cleanup component destruction for object {edi:08X}, component {ebp:08X} at {esi:08X}'
    else:
        message = f'{prefix}{entry:06X} deferred body destructor for body {ecx:08X}'
    banks = [before if before['address'] == STACK else {**before, 'after': after['after']}
             for before, after in zip(row['banks'], original['banks'], strict=True)]
    writes = [event for event in original['writes'] if not STACK <= event['address'] < STACK+32768]
    iterator = [event for event in original['writes'] if event['instruction'] in ITERATOR_STORES]
    first = iterator[0]['address'] if iterator else 0
    iterator_writes = [[event['address']-first, event['written']] for event in iterator]
    dirty_reads = []
    recording_dirty_return = False
    for event in original['reads']:
        if event['instruction'] == DIRTY_ISLANDS:
            dirty_reads.append([])
            recording_dirty_return = True
        if recording_dirty_return and not STACK <= event['address'] < STACK+32768:
            dirty_reads[-1].append([event['address'], event['bytes']])
            if event['instruction'] == 0x1cb81c:
                recording_dirty_return = False
    if recording_dirty_return:
        raise ValueError(f'{row["name"]} failed to refresh the scan owners after dirty handling')
    return dict(name=row['name'], compose=row['compose'], message=message, banks=banks,
                x87_depth=len(row['live_x87']), before_fp=row['before_fp'],
                after_fp=original['after_fp'], writes=writes,
                iterator_writes=iterator_writes, dirty_reads=dirty_reads)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Original cleanup authority changed')
    rows, regions = make_cases(image)
    proof, authority = observe(image, rows, scratch, regions)
    cases = [expected(row, original) for row, original in zip(rows, proof['cases'], strict=True)]
    fixture = scratch / 'cleanup-prefix-fixture.json'
    fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN, cases=cases), separators=(',', ':')))
    sources = ('src/physics/native_tu15_physics_frame.cpp', 'src/physics/native_tu15_memory_pressure.cpp',
               'src/physics/native_tu15_dirty_island_objects.cpp', 'src/physics/native_tu15_component_state.cpp',
               'tests/native_tu15_havok_cleanup_prefix_tests.cpp')
    executable = scratch / 'cleanup-prefix'
    command = ['g++', '-std=c++20', '-O1', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
               '-fno-fast-math', '-ffp-contract=off', '-frounding-math', '-Iinclude', '-Isrc',
               *sources, '-o', str(executable)]
    build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, check=False)
    result = build if build.returncode else subprocess.run([str(executable), str(fixture)],
        capture_output=True, text=True, check=False)
    evidence_sources = (*sources, 'include/halo2/native_tu15_physics_frame.hpp',
                        'include/halo2/native_tu15_dirty_island_objects_binding.hpp',
                        'include/halo2/native_tu15_object_scan_kernel.hpp',
                        str(Path(__file__).resolve().relative_to(ROOT)))
    report = dict(passed=result.returncode == 0, original_sha256=NEW_PIN, authority=authority,
                  original_instruction_patches=[], supplied_original_outcomes=False,
                  source_sha256={source: hashlib.sha256((ROOT/source).read_bytes()).hexdigest()
                                 for source in evidence_sources}, compiler_command=command,
                  cases=[{key: value for key, value in case.items() if key != 'banks'} for case in cases],
                  detail=(result.stdout+result.stderr)[-4000:], owned_scratch=str(scratch),
                  retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(report['detail'], end='')
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
