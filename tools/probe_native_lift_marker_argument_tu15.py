#!/usr/bin/env python3
"""Trace the original lift marker argument when only its high bytes change."""
import argparse
import copy
import json
import os
from pathlib import Path

import generate_native_lift_velocity_parent_fixture_tu15 as parent
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def banks(row):
    return {region['address'] + offset: value
            for region in row['final_banks']
            for offset, value in enumerate(bytes.fromhex(region['hex']))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report_path = args.output.resolve()
    assert not report_path.exists()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    sha = parent.lift.sha
    report = {'passed': False, 'owned_scratch': str(scratch), 'retained_large_artifacts': []}
    original_observer = parent.OBSERVER
    try:
        assert sha(NEW_DEFAULT) == NEW_PIN
        image = Xbe(NEW_DEFAULT)
        code = _Code(image, '.text')
        sites = (0xbade2, 0xbae09, 0xbae0f, 0xbae18, 0x1e12f3, 0x1e1351, 0x1e135a)
        report['original_sites'] = {hex(pc): {'bytes': code.at(pc).bytes.hex(),
                                              'instruction': code.at(pc).mnemonic + ' ' + code.at(pc).op_str}
                                    for pc in sites}
        fixture, paired, _ = parent.lift.original(parent.controls(False))
        baseline = scratch / 'baseline'
        mutated = scratch / 'high-bytes'
        baseline.mkdir()
        mutated.mkdir()
        normal, _ = parent.physical(copy.deepcopy(fixture), paired, baseline, code)
        source = original_observer.read_text()
        anchor = '   const auto previous=std::uint32_t(r.rip);'
        assert source.count(anchor) == 1
        code_change = r'''
   if(actual==0xbae0fu){
    const auto p=std::uint32_t(regs(child).rsp);
    const auto original=read_hex(child,p,4);
    for(unsigned lane=1;lane<4;++lane){
     const auto address=std::uintptr_t(p+lane)&~7ul;
     const auto shift=((p+lane)&7u)*8u;
     const auto word=std::uint64_t(request(PTRACE_PEEKDATA,child,reinterpret_cast<void*>(address)));
     request(PTRACE_POKEDATA,child,reinterpret_cast<void*>(address),
         reinterpret_cast<void*>(word^(std::uint64_t(0xa5u) << shift)));
    }
    result["marker_controls"].push_back(J{{"step",step},{"argument",p},
        {"physical_before",original},{"declared_high_byte_control",read_hex(child,p,4)}});
   }
'''
        parent.OBSERVER = scratch / 'marker-control-observer.cpp'
        parent.OBSERVER.write_text(source.replace(anchor, code_change + anchor))
        changed, _ = parent.physical(copy.deepcopy(fixture), paired, mutated, code)
        assert normal['original_hardware_binary_sha256'] == changed['original_hardware_binary_sha256']
        trace = json.loads((mutated / 'hardware/trace.json').read_text())
        cases = {case['id']: case for case in trace['cases']}
        profiles = []
        for a, b in zip(normal['cases'], changed['cases'], strict=True):
            assert a['id'] == b['id']
            controls = cases[a['id']]['marker_controls']
            assert controls
            locations = set()
            for control in controls:
                before = bytes.fromhex(control['physical_before'])
                after = bytes.fromhex(control['declared_high_byte_control'])
                assert before[0] == after[0] and all((before[i] ^ after[i]) == 0xa5 for i in range(1, 4))
                locations.update(control['argument'] + i for i in range(1, 4))
            old, new = banks(a), banks(b)
            assert old.keys() == new.keys()
            final_diffs = {address for address in old if old[address] != new[address]}
            assert a['instructions'] == b['instructions'] and a['exit'] == b['exit']
            changed_preimages = set()
            for before_store, after_store in zip(a['writes'], b['writes'], strict=True):
                assert all(before_store[k] == after_store[k] for k in ('instruction', 'address', 'written')), (
                    'Changed marker argument bytes propagated into a later physical store', before_store, after_store)
                before_bytes = bytes.fromhex(before_store['before'])
                after_bytes = bytes.fromhex(after_store['before'])
                changed_preimages.update(before_store['address'] + lane for lane, (x, y) in
                                         enumerate(zip(before_bytes, after_bytes, strict=True)) if x != y)
            assert changed_preimages <= locations, 'Marker argument preimages changed outside their private cells'
            assert final_diffs <= locations, 'Changed marker argument bytes escaped their stack cells'
            profiles.append({'id': a['id'], 'retained_bytes': len(old), 'physical_control_events': controls,
                             'final_differing_stack_cells': sorted(final_diffs),
                             'different_private_store_preimages': sorted(changed_preimages),
                             'instruction_order_equal': True, 'full_exit_equal': True,
                             'all_physical_store_values_equal': True})
        report.update(passed=True, original_image_sha256=NEW_PIN, profiles=profiles,
                      baseline_observer_sha256=sha(original_observer), control_observer_sha256=sha(parent.OBSERVER),
                      scope='Diagnostic postinstruction high-byte controls in the old private marker argument. '
                            'Compares unchanged original code against itself. The mixed native parent raw failure remains.')
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        parent.OBSERVER = original_observer
        report['temporary_allocated_bytes_before_cleanup'] = sum(
            p.stat().st_blocks * 512 for p in scratch.rglob('*') if p.is_file())
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
