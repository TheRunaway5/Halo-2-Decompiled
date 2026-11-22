#!/usr/bin/env python3
"""Observe original 1CD130 and its real private formatter on physical i386."""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

from audit_native_diagnostic_stream_tu15 import DEFINITION, PRIMARY, ROOT
from generate_crt_float_tu15 import ENTRIES as FLOAT_ENTRIES
import oracle_native_campaign_screen_lifecycle_tu15 as original_observer
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_memory_pressure_tu15 import EMERGENCY, POOL, SECOND, cases
from validate_native_tu15_prt_helpers import physical_original
from xbe import Xbe


STOP = 0x63000000
WORLD = 0x61000000
STACK = 0x62000000
COUNTER = 0x52d888
FORMATTER_METHODS = 0x46ec80
STATISTICS_ENTRIES = (0x1496f0, 0x149ad0, 0x2335b0, 0x1cd130, 0x2e4d60,
                      0x2834b0, 0x2335f0, 0x233650, 0x233520, 0x232d90,
                      0x1dbb30, 0x1cd100)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def word(banks, address, width=4, key='after'):
    for bank in banks:
        offset = address - bank['address']
        data = bank[key]
        if offset >= 0 and (offset + width) * 2 <= len(data):
            return bytes.fromhex(data[2 * offset:2 * (offset + width)])
    raise ValueError(f'Original statistics bank missing {address:#x}/{width}')


def image_regions(image):
    return {section.va: bytearray(image.read(section.va, section.vsize))
            for section in image.sections if section.name in ('.rdata', '.data')}


def put(regions, address, data):
    for base, bank in regions.items():
        offset = address - base
        if offset >= 0 and offset + len(data) <= len(bank):
            bank[offset:offset + len(data)] = data
            return
    raise ValueError(f'TU1.5 statistics input outside declared bank at {address:#x}')


def set_words(regions, address, *words):
    put(regions, address, struct.pack(f'<{len(words)}I', *(word & 0xffffffff for word in words)))


def original_row(name, index, regions, entry, before_fp, argument=0,
                 live_x87=(), stop_before=None):
    gp = [0x51800001 + 17 * index + 0x10001 * n for n in range(8)]
    gp[4] = STACK + 0x7000 + (index & 7)
    row = dict(id=index, name=name, entry=entry, gp=gp, eflags=0x202, matrix=argument,
               source=0, before_fp=before_fp, live_x87=live_x87, max_steps=250000,
               strict_writes=True,
               banks=[dict(address=base, before=raw.hex()) for base, raw in sorted(regions.items())])
    if stop_before:
        row['stop_before_entries'] = [stop_before]
    return row


def observe_original(image, rows, scratch, **options):
    # Multiple source memmove jump entries share actual epilogues. Deduplicate
    # identical instruction metadata before passing it to the strict observer.
    original_metadata = original_observer.metadata
    read_text = Path.read_text
    observer_source = ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp'

    def metadata(image):
        result = original_metadata(image)
        sites = {}
        for site in result['sites']:
            prior = sites.setdefault(site['instruction'], site)
            if prior != site:
                raise ValueError('Overlapping original owners disagree on their instruction')
        result['sites'] = list(sites.values())
        return result

    def read_observer(path, *args, **kwargs):
        text = read_text(path, *args, **kwargs)
        if path.resolve() != observer_source:
            return text
        ceiling = 'max_steps<=100000u'
        last_site = 'check(finished||!boundary.is_null(),"Original geometry did not reach its declared boundary");'
        if text.count(ceiling) != 1 or text.count(last_site) != 1:
            raise ValueError('Reusable physical statistics observer anchors changed')
        return text.replace(ceiling, 'max_steps<=250000u').replace(
            last_site, 'if(!finished&&boundary.is_null())'
            'std::cerr<<"last original pc="<<std::hex<<regs.rip<<std::dec<<"\\n";' + last_site)

    original_observer.metadata = metadata
    Path.read_text = read_observer
    try:
        return physical_original(image, rows, scratch, **options)
    finally:
        original_observer.metadata = original_metadata
        Path.read_text = read_text


def observe_initialization(image, scratch):
    regions = image_regions(image)
    regions[STACK] = bytearray(32768)
    row = original_row('original_crt_float_startup', 0, regions, 0x32cbc0, [0x027f, 0, 0x1f80])
    proof, authority = observe_original(
        image, [row], scratch, entries=(0x32cbc0,),
        regions=sorted((base, len(bank)) for base, bank in regions.items()))
    actual, = proof['cases']
    methods = word(actual['banks'], FORMATTER_METHODS, 24)
    if not actual['completed'] or len(actual['writes']) != 6:
        raise ValueError('Original CRT float initialization did not publish six method slots')
    return methods, authority


def authored_formatter_entries(image):
    # Reuse the same direct formatter closure as the independent physical
    # integer and MOPP diagnostics. Float dispatch still executes image code.
    bodies = {int(match[1][4:], 16): match[0]
              for match in DEFINITION.finditer((ROOT / PRIMARY).read_text())}
    pending = [0x32ba80]
    observed = set()
    while pending:
        entry = pending.pop()
        if entry in observed:
            continue
        observed.add(entry)
        pending.extend(int(name, 16) for name in
                       re.findall(r'\brun_([0-9a-f]+)\(s\)', bodies[entry]))
    # Fixed-decimal formatting delegates padding/copying to the real CRT
    # and may consult the pinned default C-locale classifiers.
    memmove_tables = ((0x32a9f4, 3), (0x32aa70, 8), (0x32aadc, 4),
                      (0x32ab80, 3), (0x32ac0c, 8), (0x32ac78, 4))
    memmove_targets = {target for table, count in memmove_tables for target in
                       struct.unpack(f'<{count}I', image.read(table, count * 4))}
    return observed.union(FLOAT_ENTRIES, (0x32a990, 0x32b98d, 0x32f05c), memmove_targets)


def statistics_rows(image, methods, count):
    seeds = {row['name']: row for row in cases(image)}
    layouts = set()
    rows = []
    for index in range(count):
        seed = seeds[('frame_free_1', 'frame_free_4', 'frame_free_16',
                      'frame_free_128')[index % 4]]
        regions = image_regions(image)
        for bank in seed['banks']:
            address, raw = bank['address'], bytes.fromhex(bank['before'])
            if address < 0x1000000:
                if address >= 0x480000:
                    put(regions, address, raw)
                continue  # Use the authenticated .rdata, never a fixture result.
            regions[address] = bytearray(raw)
        put(regions, FORMATTER_METHODS, methods)
        set_words(regions, POOL + 0x1c, 48 + index, 1 + index)
        regions[WORLD] = bytearray(16384)
        regions[STACK] = bytearray(32768)
        set_words(regions, 0x52be48, WORLD)
        set_words(regions, 0x48c938, WORLD + 0x780)
        set_words(regions, WORLD + 0x780, 0x45f4cc)
        put(regions, COUNTER, struct.pack('<H', [0, 1, 0xfffe, 0xffff][index % 4]))

        active = [0, 1, 3, 3][index % 4]
        inactive = [0, 3, 1, 2][index % 4]
        set_words(regions, WORLD + 8, WORLD + 0x100, active)
        set_words(regions, WORLD + 0x14, WORLD + 0x120, inactive)
        set_words(regions, WORLD + 0x2c, WORLD + 0x700)
        for base, size in ((0x100, active), (0x120, inactive)):
            for ordinal in range(size):
                island = WORLD + 0x200 + (base - 0x100) * 8 + ordinal * 0x80
                set_words(regions, WORLD + base + 4 * ordinal, island)
                set_words(regions, island + 0x40, (ordinal + index) % 4)
        set_words(regions, WORLD + 0x740, index + 1)

        component_pool = WORLD + 0x900
        bitmap = WORLD + 0xa00
        component_rows = WORLD + 0x1000
        indices = (1, 4, 33) if index & 1 else ()
        set_words(regions, 0x52be5c, component_pool)
        set_words(regions, component_pool + 0x24, 0xa0)
        set_words(regions, component_pool + 0x38, 35)
        set_words(regions, component_pool + 0x44, component_rows, bitmap)
        set_words(regions, bitmap, 0x12 if indices else 0, 2 if indices else 0)
        for ordinal, number in enumerate(indices):
            component = component_rows + 0xa0 * number
            slots = WORLD + 0x2800 + ordinal * 0x100
            set_words(regions, component + 0x70, slots, 2, 0x80000002)
            set_words(regions, component + 0x84, 3 + ordinal)
            set_words(regions, component + 0x8c, 7 + ordinal)
            set_words(regions, slots + 0x4c, 4)
            set_words(regions, slots + 0x60 + 0x4c, 5 + ordinal)

        diagnostic_heap = WORLD + 0x3400
        set_words(regions, 0x486068, diagnostic_heap)
        set_words(regions, diagnostic_heap + 0x18, WORLD + 0x3500, 0x80, index & 1)
        set_words(regions, WORLD + 0x357c, 8191)
        for ordinal, address in enumerate((EMERGENCY, SECOND)):
            if index & (1 << ordinal):
                set_words(regions, 0x486070 + ordinal * 4, address)
                set_words(regions, address + 0x18, WORLD + 0x3600 + ordinal * 0x100, 0x80, 2)
                set_words(regions, WORLD + 0x3674 + ordinal * 0x100, 2615, 0, 4997)

        # The even cases finish 1CD130 itself. The odd cases start in the
        # actual pressure loop and stop at the original next owner entry.
        from_pressure = bool(index & 1)
        mode = (index // 2) % 4
        precision = (0x027f, 0x007f, 0x037f)[index % 3]
        status = ((index + 1) % 7) << 11 | (0, 0x0200, 0x4520, 0x4000)[index % 4]
        fp = [precision | mode << 10, status,
              0x1f80 | mode << 13]
        stack_values = ('0000000000000080ff3f',) if index > 3 else ()
        rows.append(original_row(
            f"{'pressure' if from_pressure else 'standalone'}_{seed['name']}_{index}",
            index, regions, 0x1496f0 if from_pressure else 0x1cd130, fp,
            argument=(1, 0, 0xffffffff)[index % 3], live_x87=stack_values,
            stop_before=0x1ccbe0 if from_pressure else None))
        layouts.add(tuple(sorted((base, len(bank)) for base, bank in regions.items())))
    if len(layouts) != 1:
        raise ValueError('Pressure cases cannot share the physical memory layout')
    return rows, layouts.pop()


def formatted_outputs(trace):
    writes = iter(trace['writes'])
    calls = (call for call in trace['calls'] if call['entry'] == 0x32ba80)
    returns = (call for call in trace['calls'] if call['entry'] == 0x1cd122)
    for call, returned in zip(calls, returns, strict=True):
        destination, capacity, fmt, _ = call['arguments']
        length = returned['gp'][0]
        if capacity != 254 or length >= capacity:
            raise ValueError('Original statistics did not fit its actual CRT buffer')
        contents = bytearray(length + 1)
        observed = set()
        for event in writes:
            offset = event['address'] - destination
            data = bytes.fromhex(event['written'])
            if 0 <= offset and offset + len(data) <= len(contents):
                contents[offset:offset + len(data)] = data
                observed.update(range(offset, offset + len(data)))
            if event['instruction'] == 0x32babe:
                if offset != length or len(observed) != length + 1 or contents[-1] != 0:
                    raise ValueError('Original CRT returned a buffer without its complete writes')
                yield dict(format=hex(fmt), bytes=length, sha256=sha(contents))
                break
        else:
            raise ValueError('Original CRT buffer did not reach its final terminator store')


def statistics_fixture(rows, originals):
    cases = []
    for row, actual in zip(rows, originals, strict=True):
        case = {key: row[key] for key in ('name', 'entry', 'before_fp')}
        case['after_fp'] = actual['after_fp']
        case['writes'] = [{key: event[key] for key in ('instruction', 'address', 'before', 'written')}
                          for event in actual['writes']
                          if not STACK <= event['address'] < STACK + 32768]
        case['banks'] = [{**before, 'after': after['after']} for before, after in
                         zip(row['banks'], actual['banks'], strict=True)
                         if before['address'] != STACK]
        if row['entry'] == 0x1496f0:
            eax, ecx, _, _, sp, *_ = actual['boundary_gp']
            aggressive, recent, excluded = struct.unpack(
                '<3I', word(actual['banks'], sp + 4, 12))
            case['candidate'] = dict(mode=ecx, final_pass=bool(eax & 255),
                                     aggressive=bool(aggressive & 255),
                                     recent_cleanup=bool(recent & 255), excluded=excluded)
        cases.append(case)
    return dict(image_sha256=NEW_PIN, required_backend='unchanged-tu15-havok-statistics-v1',
                cases=cases)


def qualify_native(scratch, fixture):
    path = scratch / 'statistics-fixture.json'
    path.write_text(json.dumps(fixture, separators=(',', ':')))
    project = scratch / 'native-statistics-project'
    project.mkdir()
    (project / 'CMakeLists.txt').write_text(f"""
cmake_minimum_required(VERSION 3.20)
project(OriginalHavokStatistics LANGUAGES CXX)
include(CTest)
add_executable(original_havok_statistics
    "{ROOT / 'tests/native_tu15_memory_pressure_tests.cpp'}"
    "{ROOT / 'src/physics/native_tu15_memory_pressure.cpp'}")
target_include_directories(original_havok_statistics PRIVATE "{ROOT / 'include'}")
target_compile_features(original_havok_statistics PRIVATE cxx_std_20)
target_compile_options(original_havok_statistics PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME original_havok_statistics
    COMMAND "{sys.executable}" "{ROOT / 'tools/run_test.py'}" --
        $<TARGET_FILE:original_havok_statistics> "{path}")
""")
    output = scratch / 'statistics-ctest'
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project),
               '--scratch-root', str(scratch / 'builds'), '--output', str(output),
               '--target', 'original_havok_statistics', '--filter', '^original_havok_statistics$',
               '--jobs', '1', '--test-jobs', '1']
    result = subprocess.run(command, check=False)
    details = {}
    for name in ('configure.log', 'build.log', 'tests.log'):
        log = output / name
        if log.is_file():
            details[name] = log.read_text()[-4000:]
    return dict(passed=result.returncode == 0, status=result.returncode, command=command,
                fixture_sha256=sha(path.read_bytes()), log_tails=details,
                ordered_retained_stores=sum(len(case['writes']) for case in fixture['cases']))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--cases', type=int, choices=(2, 4, 8, 12), default=2)
    parser.add_argument('--compare-native', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    if sha(image.data) != NEW_PIN:
        raise ValueError('Original TU1.5 image identity differs')
    initializer = scratch / 'format-initialization'
    initializer.mkdir()
    methods, initializer_authority = observe_initialization(image, initializer)
    rows, regions = statistics_rows(image, methods, args.cases)
    entries = sorted(set(STATISTICS_ENTRIES).union(authored_formatter_entries(image)))
    proof, authority = observe_original(
        image, rows, scratch, entries=entries, regions=regions, trace_extended=True,
        trace_calls=[dict(entry=pc, gp=True, xmm=True, arguments=count)
                     for pc, count in ((0x1cd130, 1), (0x1cd100, 8),
                                       (0x32ba80, 4), (0x1cd122, 0), (0x331e10, 5))])
    summaries = []
    for row, actual in zip(rows, proof['cases'], strict=True):
        expected_stop = row.get('stop_before_entries', [STOP])[0]
        if not actual['completed'] or actual.get('stopped_at', STOP) != expected_stop:
            raise ValueError(f"{row['name']} did not reach its genuine return or candidate boundary")
        retained = [event for event in actual['writes']
                    if not STACK <= event['address'] < STACK + 32768]
        changed = [bank['address'] for bank in actual['banks'] if bank['address'] != STACK and
                   bank['after'] != next(item['before'] for item in row['banks']
                                         if item['address'] == bank['address'])]
        calls = Counter(hex(call['entry']) for call in actual['calls'])
        formats = [hex(call['arguments'][0]) for call in actual['calls'] if call['entry'] == 0x1cd100]
        formatted = list(formatted_outputs(actual))
        if calls['0x1cd130'] != 1 or calls['0x331e10'] == 0 or len(formats) != len(formatted):
            raise ValueError('Original statistics did not exercise actual formatter returns')
        initial_count = int.from_bytes(word(row['banks'], COUNTER, 2, key='before'), 'little')
        expected = [(0x149711, 0x48b873, '01')] if row['entry'] == 0x1496f0 else []
        expected.extend((site, COUNTER, struct.pack('<H', value & 0xffff).hex())
                        for site, value in ((0x1cd178, initial_count + 1), (0x1cd1f2, initial_count)))
        retained_projection = [(item['instruction'], item['address'], item['written'])
                               for item in retained]
        summaries.append(dict(name=row['name'], stop=hex(expected_stop), retained_writes=retained,
                              counter_and_busy_stores_only=retained_projection == expected,
                              changed_region_starts=changed, before_fp=row['before_fp'],
                              after_fp=actual['after_fp'], live_x87_checked=actual['live_x87_unchanged'],
                              calls=calls, formatted_buffers=formatted,
                              original_instructions=actual['original_instructions']))
    report = dict(passed=True, native_qualified=False, owned_scratch=str(scratch),
                  original_xbe_sha256=NEW_PIN, original_float_methods=methods.hex(),
                  original_startup=initializer_authority, original_statistics=authority,
                  source_sha256={str(path.relative_to(ROOT)): sha(path.read_bytes()) for path in
                                 (Path(__file__).resolve(), ROOT / 'tools/validate_native_memory_pressure_tu15.py',
                                  ROOT / 'tools/validate_native_tu15_prt_helpers.py')},
                  cases=summaries, instruction_patches=[], supplied_original_outcomes=False,
                  retained_large_artifacts=[])
    if args.compare_native:
        qualification = qualify_native(scratch, statistics_fixture(rows, proof['cases']))
        report.update(passed=qualification['passed'], native_qualified=qualification['passed'],
                      qualification=qualification)
        for name in ('include/halo2/native_tu15_memory_pressure.hpp',
                     'src/physics/native_tu15_memory_pressure.cpp',
                     'tests/native_tu15_memory_pressure_tests.cpp'):
            report['source_sha256'][name] = sha((ROOT / name).read_bytes())
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(args.report)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
