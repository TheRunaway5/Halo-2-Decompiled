#!/usr/bin/env python3
"""Build declared-before-input original1DA750 cases from every censused force family."""
import argparse
import collections
import copy
import inspect
import json
import os
from pathlib import Path
import struct

import generate_native_lift_marker_velocity_fixture_tu15 as markers
import generate_native_lift_velocity_parent_fixture_tu15 as velocity
import validate_native_lift_parent_tu15 as lift

ROOT = lift.ROOT
CENSUS = ROOT / 'build/test-results/gravlift-general-audit-20261109/census'
FAMILIES = ROOT / 'build/test-results/gravlift-general-audit-20261109/logic/native-force-ownership-followup.json'


def controls(quick):
    families = json.loads(FAMILIES.read_text())['authored_force_families']
    detail = {}
    result = []
    sample = next(r for r in lift.parent.controls() if r['entry'] == 0x1da750 and r['coefficients'] == 1 and r['members'] == 1)
    for index, family in enumerate(families):
        example = family['examples'][0]['id']
        name = example.split(':')[0]
        if name not in detail:
            detail[name] = json.loads((CENSUS / (name + '.json')).read_text())
        case = next(c for c in detail[name]['cases'] if c['case_id'] == example)
        raw = bytes.fromhex(case['force']['record_hex'])
        assert len(raw) == 0x68
        assert struct.unpack_from('<I', raw)[0] == family['flags']
        kinds = (0,) if quick else range(13)
        for kind in kinds:
            row = dict(sample, root_type=kind, fp_control=0x7f | ((0, 2, 3)[(index + kind) % 3] << 8) | (((index + kind) % 4) << 10),
                       mxcsr=0x1f80 | (((index + kind) % 4) << 13), top=(index + kind) % 8,
                       point=2. if (index + kind) % 2 else -3., velocity=.5 if (index + kind) % 3 else 0.,
                       authored_family=index, authored_case=example, authored_force_hex=raw.hex(), body_branch='active')
            result.append(row)
        if not quick:
            for branch in ('no_island', 'inactive_no_world', 'static', 'angular_component_gate',
                           'inactive_world', 'inactive_deferred', 'source_gate', 'zero_factor'):
                result.append(dict(result[-1], root_type=(0, 1, 11)[index % 3], body_branch=branch,
                                   flags=0 if branch == 'source_gate' else sample['flags'],
                                   factor=0. if branch == 'zero_factor' else 1.))
    assert len(families) == 54
    return result


def original(rows):
    capture = lift.parent.capture
    captures = 0
    def authored(g):
        nonlocal captures
        if captures % 2 == 0:
            row = rows[captures // 2]
            g.mem_write(lift.parent.PHDEF, bytes.fromhex(row['authored_force_hex']))
            branch = row['body_branch']
            body, island = lift.parent.HAVOK + 0x100, lift.parent.ISLAND + 0x100
            put = lambda p, value: g.mem_write(p, struct.pack('<I', value & 0xffffffff))
            if branch == 'no_island':
                put(body + 0x44, 0)
            elif branch in ('inactive_no_world', 'inactive_world', 'inactive_deferred'):
                put(body + 0x44, island)
                g.mem_write(island + 0x29, b'\0')
                put(body + 8, 0)
                if branch != 'inactive_no_world':
                    world = lift.parent.BASE + 0x40000
                    g.mem_write(world, bytes(0x1000))
                    put(body + 8, world)
                    put(world + 8, world + 0x600)
                    put(world + 0xc, 0)
                    put(world + 0x10, 2)
                    put(world + 0x14, world + 0x700)
                    put(world + 0x18, 1)
                    put(world + 0x700, island)
                    put(island + 0x24, 0)
                    if branch == 'inactive_deferred':
                        g.mem_write(body + 6, b'\x02\0')
                        put(world + 0x108, world + 0x800)
                        put(world + 0x10c, 1)
                        put(world + 0x800, body)
            elif branch == 'static':
                g.mem_write(body + 0x40, b'\x01')
            elif branch == 'angular_component_gate':
                put(lift.parent.COMPONENT + 0xa0 + 4, 2)
        captures += 1
        return capture(g)
    # Keep every declared parent case, including legitimate zero-force and
    # early-return paths. A filtered-out family must never count as qualified.
    source = inspect.getsource(lift.original)
    line = "    cases=[c for c in cases if c['complete'] and any(w['instruction'] in CALLS for w in c['writes'])]\n"
    assert source.count(line) == 1
    source = source.replace(line, '')
    # Admit these unchanged original instructions to the offline oracle, not
    # a result service. They are the independently qualified body wake roots.
    environment = dict(lift.__dict__, OWNERS=tuple(dict.fromkeys((*lift.OWNERS, 0x3165d0, 0x316d40, 0x322660))))
    exec(source, environment)
    lift.parent.capture = authored
    try:
        result = environment['original'](rows)
    finally:
        lift.parent.capture = capture
    assert captures == len(rows) * 2
    return result


def generalized_velocity_source():
    source = inspect.getsource(velocity.classify)
    source = markers.replace(source, '                    assert actual[canonical] == delivery[\'store_site\'], event',
                             '                    assert actual[canonical] in owner_sites, event')
    source = markers.replace(source, "        assert len(delivery['stores']) == 2\n", '')
    source = markers.replace(source, "        assert other and entry_cell['instruction'] == delivery['callsite']",
                             "        assert entry_cell['instruction'] == delivery['callsite']")
    source = markers.replace(source, '    assert all(counts[pc] for pc in CALL_SITES), counts\n', '')
    return source


def classify(case, deliveries, actual, owner_sites, code):
    source = inspect.getsource(markers.classify)
    source = markers.replace(source, '    source = inspect.getsource(base.classify)',
                             '    source = generalized_velocity_source()')
    environment = dict(markers.__dict__, generalized_velocity_source=generalized_velocity_source)
    exec(source, environment)
    environment['classify'](case, deliveries, actual, owner_sites, code)
    for delivery in case['velocity_deliveries']:
        merged = []
        source_stores = iter(delivery['stores'])
        for store in source_stores:
            if store['instruction'] in (0x3239f7, 0x323a07):
                second = next(source_stores)
                assert second['instruction'] == store['instruction'] and second['address'] == store['address'] + 8
                assert len(store['written']) == len(second['written']) == 16
                store = dict(store, before=store['before'] + second['before'], written=store['written'] + second['written'])
            merged.append(store)
        delivery['stores'] = merged
    case['velocity_store_calls'] = [sum(any(e['instruction'] == site for e in delivery['stores'])
        for delivery in case['velocity_deliveries']) for site in (0x3239f7, 0x323a07)]


def attach_delivery_models(fixture):
    prefixes = set()
    for case in fixture['cases']:
        current = velocity.bank_data(case['initial_banks'])
        events = iter(case['writes'])
        event = next(events, None)
        def publish(row):
            for offset, value in enumerate(bytes.fromhex(row['written'])):
                current[row['address'] + offset] = value
        def banks():
            return [dict(address=p, hex=raw.hex()) for p, raw in lift.segments(current)]
        for delivery in case['velocity_deliveries']:
            begin, end = delivery['call_step'], delivery['return_step']
            while event and event['step'] < begin:
                publish(event)
                event = next(events, None)
            delivery['ordinary_before_banks'] = banks()
            private = set()
            selected = []
            semantic_index = 0
            fps = {(entry['instruction'], entry['address']): entry['after_fp'] for entry in delivery['store_fp']}
            while event and event['step'] < end:
                step = event['step']
                group = []
                while event and event['step'] == step:
                    group.append(event)
                    publish(event)
                    if case['stack_begin'] <= event['address'] < delivery['caller_sp']:
                        assert event['address'] + len(bytes.fromhex(event['written'])) <= delivery['caller_sp']
                        private.update(range(event['address'], event['address'] + len(bytes.fromhex(event['written']))))
                    event = next(events, None)
                for row in group:
                    if semantic_index >= len(delivery['stores']):
                        continue
                    wanted = delivery['stores'][semantic_index]
                    if (row['instruction'], row['address']) != (wanted['instruction'], wanted['address']):
                        continue
                    semantic_index += 1
                    fp = fps[(row['instruction'], row['address'])]
                    key = (delivery['callsite'], wanted['instruction'], *delivery['entry_fp'],
                           case['declared_input']['body_branch'])
                    if key not in prefixes:
                        prefixes.add(key)
                        selected.append(dict(store_count=semantic_index, after_fp=fp, after_banks=banks()))
            assert semantic_index == len(delivery['stores'])
            delivery['ordinary_after_banks'] = banks()
            delivery['private_original_bytes'] = sorted(private)
            delivery['observer_prefixes'] = selected


def physical_fixture(rows, scratch, report):
    image = lift.Xbe(lift.NEW_DEFAULT)
    code = lift._Code(image, '.text')
    starts_path = ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
    starts = {int(line.split('\t')[0], 16) for line in starts_path.read_text().splitlines()[1:]}
    owner_sites = {}
    for owner in (*velocity.VELOCITY_OWNERS, 0x3165d0, 0x316d40, 0x322660):
        body = lift._trace(code, owner, starts)
        assert not body.tables
        for pc in body.code:
            assert owner_sites.setdefault(pc, owner) == owner
    fixture, paired, leaf_sites = original(rows)
    assert len(fixture['cases']) == len(rows) and all(c['complete'] for c in fixture['cases'])
    observer = velocity.OBSERVER
    observed_source = observer.read_text()
    observed_source = markers.replace(observed_source, 'struct Write {',
        'J numeric(pid_t child){user_fpregs_struct f{};request(PTRACE_GETFPREGS,child,nullptr,&f);return J::array({f.cwd,f.swd,f.mxcsr});}\n'
        'bool empty_x87(pid_t child){user_fpregs_struct f{};request(PTRACE_GETFPREGS,child,nullptr,&f);return f.ftw==0;}\nstruct Write {')
    observed_source = markers.replace(observed_source, '    active["return_step"]=step;',
        '    active["return_fp"]=numeric(child);active["return_step"]=step;')
    observed_source = markers.replace(observed_source, '{"reads",J::array()}};',
        '{"reads",J::array()},{"entry_fp",numeric(child)},{"entry_x87_empty",empty_x87(child)},{"store_fp",J::array()}};')
    observed_source = markers.replace(observed_source,
        's.erase("size");result["writes"].push_back(std::move(s));',
        's.erase("size");if(!active.is_null())active["store_fp"].push_back(J{{"instruction",actual},{"address",s.at("address")},{"after_fp",numeric(child)}});result["writes"].push_back(std::move(s));')
    private_observer = scratch / 'delivery-fp-observer.cpp'
    private_observer.write_text(observed_source)
    velocity.OBSERVER = private_observer
    try:
        hardware, calls = markers.physical(fixture, paired, scratch, code)
    finally:
        velocity.OBSERVER = observer
    report['delivery_fp_observer_sha256'] = lift.sha(private_observer)
    actual = {int(k, 16): v for k, v in paired['instruction_map'].items()}
    supplements = lift.qualify(fixture, paired, leaf_sites, hardware)
    for case in fixture['cases']:
        classify(case, calls[case['id']], actual, owner_sites, code)
        assert all(d['entry_x87_empty'] for d in case['velocity_deliveries'])
    attach_delivery_models(fixture)
    report.update(original_hardware_compared=True, original_sha256=lift.NEW_PIN,
                  hardware_instructions=hardware['single_stepped_original_instructions'],
                  instruction_bytes_changed=hardware['paired_instruction_bytes_changed'],
                  readonly_image_supplements=supplements,
                  physical_tool_source_sha256={str(p.relative_to(ROOT)): lift.sha(p) for p in (
                      Path(markers.__file__), Path(velocity.__file__), velocity.OBSERVER,
                      Path(lift.physical.__file__), starts_path)},
                  unported_owner_stubs=hardware['unported_owner_capture_stubs'])
    assert not report['instruction_bytes_changed'] and not report['unported_owner_stubs']
    return fixture


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-output', required=True, type=Path)
    parser.add_argument('--quick', action='store_true')
    parser.add_argument('--discover-only', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    output = args.report_output.resolve()
    assert not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    pins = {str(p.relative_to(ROOT)): lift.sha(p) for p in (FAMILIES, Path(__file__), Path(lift.__file__), Path(lift.parent.__file__))}
    report = dict(passed=False, scope=__doc__, original_hardware_compared=False, live_admission=False,
                  owned_scratch=str(scratch), retained_large_artifacts=[], source_sha256=pins)
    try:
        rows = controls(args.quick)
        if args.discover_only:
            fixture, paired, leaf_sites = original(rows)
        else:
            fixture = physical_fixture(rows, scratch, report)
            target = scratch / 'authored-parent.json'
            target.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
            report['fixture_sha256'] = lift.sha(target)
            deliveries = [d for c in fixture['cases'] for d in c['velocity_deliveries']]
            report['physical_deliveries'] = len(deliveries)
            report['physical_delivery_fp_records'] = sum(len(d['store_fp']) for d in deliveries)
            assert all(len(d['entry_fp']) == len(d['return_fp']) == 3 for d in deliveries)
        cases = fixture['cases']
        report.update(declared_cases=len(rows), observed_cases=len(cases), force_families=len({r['authored_family'] for r in rows}),
            completed=sum(c['complete'] for c in cases),
            incomplete=[{'id': c['id'], 'family': rows[c['id']]['authored_family'], 'body_branch': rows[c['id']]['body_branch'],
                         'exit_entry': hex(c['exit_entry'])} for c in cases if not c['complete']],
            body_types=sorted({r['root_type'] for r in rows}), body_branches=dict(collections.Counter(r['body_branch'] for r in rows)))
        assert len(cases) == len(rows), 'Declared families/cases disappeared'
        report['passed'] = all(c['complete'] for c in cases)
        report['scope'] += (' Discovery only; no arithmetic/native handoff/live qualification.' if args.discover_only else
                            ' Original physical fixture and FP observation only; native and live comparisons remain separate.')
        assert all(lift.sha(ROOT / name) == pin for name, pin in pins.items())
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'declared_cases', 'force_families', 'completed')}))


if __name__ == '__main__':
    main()
