#!/usr/bin/env python3
"""Generate a full-return TU1.5 lift velocity caller fixture on real i386."""
import argparse
from collections import Counter, defaultdict
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from types import SimpleNamespace

sys.dont_write_bytecode = True

from capstone import CS_AC_READ

from export_mp_physics_contact_lifecycle import segments
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
import validate_native_lift_parent_tu15 as lift

OBSERVER = ROOT / 'tools/lift_velocity_parent_hardware_trace_tu15.cpp'
VELOCITY_OWNERS = (0x1d9080, 0x1d9100, 0x1d92c0, 0x315c90, 0x315cb0, 0x3239f0, 0x323a00)
CALL_SITES = (0x1db370, 0x1db64f)
EVENT_KEYS = ('instruction', 'address', 'before', 'written')
STACK = 4


def controls(quick):
    # Coefficient families, zero and noninteger powers, both point signs,
    # stationary and moving bodies, all roundings and supported precisions.
    choices = (
        (1, 2., .5, 0., 0, 3), (2, -3., 0., .5, 1, 2),
        (3, 2., .5, 3., 2, 0), (4, -3., .5, .5, 3, 3),
        (1, -3., .5, 3., 1, 3), (4, 2., 0., 0., 2, 2),
    )
    rows = lift.controls(quick)
    if quick:
        return rows
    keys = ('coefficients', 'point', 'velocity', 'exponent')
    by_profile = {
        (*[row[k] for k in keys], (row['fp_control'] >> 10) & 3,
         (row['fp_control'] >> 8) & 3): row for row in rows
    }
    return [dict(by_profile[choice], top=index) for index, choice in enumerate(choices)]


def add_read_metadata(path, code):
    data = json.loads(path.read_text())
    gp = {name: n for n, name in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}
    for row in data['instructions']:
        instruction = code.at(row['actual'])
        reads = []
        if instruction.mnemonic in ('pop', 'ret'):
            reads.append(dict(base=STACK, index=-1, scale=1, displacement=0, width=4))
        if instruction.mnemonic != 'lea':
            for operand in instruction.operands:
                if operand.type != 3 or not operand.access & CS_AC_READ:
                    continue
                reg_name = instruction.reg_name
                mem = operand.mem
                # The observed velocity routines use flat DATA only. Existing
                # CRT FS reads outside the delivery remain the old oracle's job.
                if mem.segment and reg_name(mem.segment) not in ('ds', 'es', 'ss'):
                    continue
                reads.append(dict(base=gp[reg_name(mem.base)] if mem.base else -1,
                                  index=gp[reg_name(mem.index)] if mem.index else -1,
                                  scale=mem.scale, displacement=mem.disp & 0xffffffff,
                                  width=operand.size))
        row['reads'] = reads
    path.write_text(json.dumps(data))


def physical(fixture, paired, scratch, code):
    # Reuse the authenticated input bridge and output ABI. Substitute only
    # the test observer, with extra metadata consumed by that observer.
    physical_module = lift.physical
    module_root = physical_module.ROOT
    process_module = physical_module.subprocess
    observer_root = scratch / 'observer'
    (observer_root / 'tools').mkdir(parents=True)
    observer = observer_root / 'tools/vehicle_motion_math_hardware_trace_20261011.cpp'
    shutil.copyfile(OBSERVER, observer)
    work = scratch / 'hardware'

    def run(argv, *args, **kwargs):
        if argv[0] == 'c++' and str(observer) in argv:
            add_read_metadata(work / 'metadata.json', code)
        return subprocess.run(argv, *args, **kwargs)

    physical_module.ROOT = observer_root
    physical_module.subprocess = SimpleNamespace(run=run, check_output=subprocess.check_output)
    try:
        hardware = lift.hardware(fixture, paired, work)
    finally:
        physical_module.ROOT = module_root
        physical_module.subprocess = process_module
    trace = json.loads((work / 'trace.json').read_text())
    assert hardware['trace_observer_sha256'] == lift.sha(OBSERVER)
    assert not hardware['unported_owner_capture_stubs']
    return hardware, {row['id']: row['velocity_deliveries'] for row in trace['cases']}


def fields(event, keys=EVENT_KEYS):
    return {key: event[key] for key in keys}


def bank_data(banks):
    return {
        row['address'] + i: value for row in banks
        for i, value in enumerate(bytes.fromhex(row['hex']))
    }


def classify(case, deliveries, actual, owner_sites, code):
    # Keep the pre-existing scalar private-frame classification intact. The
    # velocity exclusions are confined to actual nested CALL..RET intervals.
    scalar_semantic = iter(case['semantic_writes'])
    scalar_event = next(scalar_semantic, None)
    by_step = defaultdict(list)
    for event in case['writes']:
        by_step[event['step']].append(event)

    initial = bank_data(case['initial_banks'])
    current = dict(initial)
    private_final = set()
    semantic_writes = []
    private_before = []
    delivery_for_step = {}
    read_steps = defaultdict(list)
    for delivery in deliveries:
        begin, end = delivery['call_step'], delivery['return_step']
        assert case['instructions'][begin] == delivery['callsite']
        assert actual[case['instructions'][begin + 1]] == delivery['entry']
        assert actual[case['instructions'][end]] == delivery['continuation']
        sp = delivery['caller_sp']
        assert delivery['return_sp'] == sp + (4 if delivery['callsite'] == CALL_SITES[1] else 0)
        assert sp <= delivery['vector'] <= case['stack_end'] - 12
        assert case['stack_begin'] <= sp < case['stack_end']
        child_pcs = [actual[p] for p in case['instructions'][begin + 1:end]]
        assert all(p in owner_sites for p in child_pcs), set(child_pcs) - owner_sites.keys()
        for pc in child_pcs:
            instruction = code.at(pc)
            assert not any(op.type == 3 and op.mem.segment and
                           instruction.reg_name(op.mem.segment) not in ('ds', 'es', 'ss')
                           for op in instruction.operands), hex(pc)
        delivery['executed_owners'] = sorted({owner_sites[p] for p in child_pcs})
        delivery['stores'] = []
        delivery['private_stores'] = []
        delivery['retained_reads'] = []
        for read in delivery['reads']:
            assert begin < read['step'] < end
            read_steps[read['step']].append(read)
        for step in range(begin, end):
            assert step not in delivery_for_step
            delivery_for_step[step] = delivery

    def check_raw(p, raw):
        observed = bytes.fromhex(raw)
        assert all(current.get(p + i) == value for i, value in enumerate(observed)), (hex(p), raw)

    for step, canonical in enumerate(case['instructions']):
        delivery = delivery_for_step.get(step)
        if delivery and step == delivery['call_step']:
            check_raw(delivery['vector'], delivery['vector_hex'])
        for read in read_steps.get(step, ()):
            p, raw = read['address'], read['raw']
            check_raw(p, raw)
            if not case['stack_begin'] <= p < delivery['caller_sp']:
                delivery['retained_reads'].append(fields(read, ('instruction', 'address', 'raw')))
            else:
                assert p + len(bytes.fromhex(raw)) <= delivery['caller_sp']

        kept = []
        for event in by_step.get(step, ()):
            raw = fields(event)
            if raw == scalar_event:
                scalar_event = next(scalar_semantic, None)
                private = False
            else:
                private = True
                assert delivery is None, event
            p = event['address']
            written = bytes.fromhex(event['written'])
            check_raw(p, event['before'])
            if delivery:
                sp = delivery['caller_sp']
                if case['stack_begin'] <= p < sp:
                    assert p + len(written) <= sp
                    private = True
                    delivery['private_stores'].append(raw)
                else:
                    assert actual[canonical] == delivery['store_site'], event
                    delivery['stores'].append(raw)
            mask = sum(1 << i for i in range(len(written)) if p + i in private_final)
            for i, value in enumerate(written):
                current[p + i] = value
                if private:
                    private_final.add(p + i)
                else:
                    private_final.discard(p + i)
            if not private:
                kept.append((raw, mask))

        # The old generic observer split stores wider than eight bytes.
        # The physical setter executes one MOVAPS, so expose one event.
        if delivery and actual[canonical] == delivery['store_site']:
            assert len(kept) == 2, kept
            (first, lo_mask), (second, hi_mask) = kept
            assert second['address'] == first['address'] + 8
            assert len(first['written']) == len(second['written']) == 16
            kept = [(dict(first, before=first['before'] + second['before'],
                          written=first['written'] + second['written']), lo_mask | (hi_mask << 8))]
        for event, mask in kept:
            semantic_writes.append(event)
            private_before.append(mask)

    assert scalar_event is None
    assert current == bank_data(case['final_banks'])
    for delivery in deliveries:
        assert len(delivery['stores']) == 2
        entry_cell, *other = delivery['private_stores']
        assert other and entry_cell['instruction'] == delivery['callsite']
        assert entry_cell['address'] == delivery['caller_sp'] - 4
        assert int.from_bytes(bytes.fromhex(entry_cell['written']), 'little') == delivery['continuation']
        delivery.pop('reads')
    counts = Counter(delivery['callsite'] for delivery in deliveries)
    assert all(counts[pc] for pc in CALL_SITES), counts
    case['velocity_calls'] = [counts[pc] for pc in CALL_SITES]
    case['velocity_deliveries'] = deliveries
    case['semantic_writes'] = semantic_writes
    case['private_before_byte_masks'] = private_before
    case['original_private_leaf_final_bytes'] = [
        dict(address=p, bytes=len(raw)) for p, raw in segments({p: 0 for p in private_final})
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--quick-matrix', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report_path = args.report_output.resolve()
    assert not report_path.exists()
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report = dict(passed=False, scratch_cleanup_owner=str(scratch),
                  retained_large_artifacts=[], original_sha256=lift.NEW_PIN)
    try:
        image = Xbe(lift.NEW_DEFAULT)
        code = _Code(image, '.text')
        starts_file = ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
        starts = {int(row.split('\t')[0], 16) for row in starts_file.read_text().splitlines()[1:]}
        owner_sites = {}
        for owner in VELOCITY_OWNERS:
            body = _trace(code, owner, starts)
            assert not body.tables
            for pc in body.code:
                assert owner_sites.setdefault(pc, owner) == owner
        rows = controls(args.quick_matrix)
        fixture, paired, leaf_sites = lift.original(rows)
        assert len(fixture['cases']) == len(rows)
        hardware, calls = physical(fixture, paired, scratch, code)
        actual = {int(k, 16): v for k, v in paired['instruction_map'].items()}
        supplements = lift.qualify(fixture, paired, leaf_sites, hardware)
        for case in fixture['cases']:
            classify(case, calls[case['id']], actual, owner_sites, code)
        fixture_path = scratch / 'lift-velocity-parent.json'
        fixture_path.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
        deliveries = [d for c in fixture['cases'] for d in c['velocity_deliveries']]
        dependencies = {}
        for pc in CALL_SITES:
            selected = [d for d in deliveries if d['callsite'] == pc]
            dependencies[hex(pc)] = dict(
                deliveries=len(selected),
                changed_motion_deliveries=sum(any(e['before'] != e['written'] for e in d['stores']) for d in selected),
                retained_reads=sum(len(d['retained_reads']) for d in selected),
                retained_read_sites=sorted({(hex(e['instruction']), len(bytes.fromhex(e['raw'])))
                                            for d in selected for e in d['retained_reads']}),
                executed_owners=sorted({hex(owner) for d in selected for owner in d['executed_owners']}),
                private_store_sites=sorted({hex(e['instruction']) for d in selected for e in d['private_stores']}),
                private_bytes_written=sum(len(bytes.fromhex(e['written'])) for d in selected for e in d['private_stores']),
                physical_virtual_store_fragments=sum(len(d['stores']) for d in selected))
        assert all(value['changed_motion_deliveries'] for value in dependencies.values()), dependencies
        paths = [Path(__file__).resolve(), OBSERVER, Path(lift.__file__), Path(lift.parent.__file__),
                 Path(lift.physical.__file__), lift.CORRESPONDENCE, starts_file]
        report.update(
            passed=True, original_full_parent_returns=len(fixture['cases']),
            fixture_sha256=lift.sha(fixture_path), fixture_filename=fixture_path.name,
            physical_instructions=hardware['single_stepped_original_instructions'],
            cpu_model=hardware['cpu_model'], paired_instruction_bytes_changed=hardware['paired_instruction_bytes_changed'],
            original_hardware_binary_sha256=hardware['original_hardware_binary_sha256'],
            dependency_paths=dependencies,
            readonly_image_supplements=supplements,
            declared_inputs=[case['declared_input'] for case in fixture['cases']],
            semantic_stores=sum(len(c['semantic_writes']) for c in fixture['cases']),
            original_private_helper_final_bytes=sum(r['bytes'] for c in fixture['cases']
                                                    for r in c['original_private_leaf_final_bytes']),
            tool_source_sha256={str(p.relative_to(ROOT)): lift.sha(p) for p in paths},
            scope='Source-authenticated offline full parent fixture. Native and live comparisons are separate.')
    except BaseException as error:
        report['failure'] = repr(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
