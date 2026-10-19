#!/usr/bin/env python3
"""Execute an unchanged captured weapon prefix on the host i386 CPU.

Only original entry banks and CPU state are supplied. Stop before the declared
unexecuted callee, with no replacement return. Retain bounded CPU/trace/store
evidence, not the generated ELF or bank copies. Run through tools/run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
from original_xbox_pose_case import ROOT, cpu, physical_ram, PageTableMemory, sha
from oracle_sandbox_first_person_hardware_tu15 import execute
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from probe_sound_binding_tu15 import defined_flag_mask
from audit_damage_delivery_observation_pcs_tu15 import mapping as damage_pc_mapping


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute()
    output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch), argv=os.sys.argv))+'\n')
    original = json.loads((args.prefix/'report.json').read_text())
    call = json.loads((args.case/'call.json').read_text())
    trace = json.loads((args.prefix/'instructions.json').read_text())
    stores = json.loads((args.prefix/'writes.json').read_text())
    mappings = json.loads((args.prefix/'mapped-pages.json').read_text())
    native = json.loads((args.native/'native-observations.json').read_text())
    assert original['declared_prefix_stop_reached'] and not original['passed']
    assert not call['guest_code_patches'] and not call['guest_memory_writes'] and not call['guest_register_writes']
    assert not original['source_instruction_patches'] and not original['synthetic_memory_banks']
    assert not original['supplied_gameplay_returns'] and not original['kernel_instructions']
    assert len(trace) == original['instruction_count'] and len(stores) == original['ordered_source_writes']
    assert sha(NEW_DEFAULT) == NEW_PIN
    initial = cpu(args.case, 'before')
    assert initial['x87_tag'] == 65535
    pages = sorted(map(int, mappings))
    assert len({row['physical'] for row in mappings.values()}) == len(pages), 'Shared physical aliases require their own host bank owner'
    ranges = []
    for page in pages:
        assert page % 4096 == 0
        if ranges and ranges[-1][1] == page:
            ranges[-1][1] += 4096
        else:
            ranges.append([page, page+4096])
    with physical_ram(args.case, 'before') as bank:
        memory = PageTableMemory(bank, initial)
        for page in pages:
            assert memory.translate(page)[0] == mappings[str(page)]['physical']
        banks = [dict(address=left, hex=memory.read(left, right-left).hex()) for left, right in ranges]
    abi = dict(gp=initial['gp'], eflags=initial['eflags'], mxcsr=initial['mxcsr'], fs_base=initial['fs_base'],
        x87_control_word=initial['x87_control'], x87_status=initial['x87_status'], x87_tag=initial['x87_tag'],
        x87_values=initial['x87_register_bytes'],
        xmm=[struct.pack('<4I', *words)[::-1].hex() for words in initial['xmm']])
    code = _Code(Xbe(NEW_DEFAULT), '.text')
    addresses = {pc:pc for pc in set(trace)|{call['entry'],call['return_address'],original['declared_prefix_stop']}}
    for pc in set(trace):
        instruction = code.at(pc)
        assert instruction.mnemonic not in ('rdtsc','rdtscp','cpuid','cli','sti','in','out')
        addresses[pc+instruction.size] = pc+instruction.size
    paired = dict(entries={f'{call["entry"]:08x}':call['entry']},
        instruction_map={f'{pc:08x}':pc for pc in sorted(set(trace))},
        address_map={f'{pc:08x}':pc for pc in sorted(addresses)})
    fixture = dict(image='1.5', cases=[dict(id=1, entry=call['entry'], entry_abi=abi,
        return_address=call['return_address'], exit_entry=original['declared_prefix_stop'], exit={},
        original_instructions=trace, initial_banks=banks)])
    actual = execute(fixture, paired, scratch/'original-host')
    assert not actual['original_instruction_patches'] and not actual['native_expected_data_used']
    host = actual['cases'][0]
    observed = host['exit']
    converted = dict(gp=observed['gp'], xmm=[list(struct.unpack('<4I',bytes.fromhex(value)[::-1])) for value in observed['xmm']],
        eflags=observed['eflags'], x87_control=observed['x87_control_word'], x87_status=observed['x87_status'],
        x87_tag=observed['x87_tag'], mxcsr=observed['mxcsr'])
    mask = defined_flag_mask(host['instructions'])
    damage_source = ROOT/'src/source_damage_delivery_runtime_generated.cpp'
    pc_map, owners, store_sites = damage_pc_mapping(damage_source)
    domains = native['canonical_damage_instruction_ranges']
    actual_native_trace = native['instructions'].copy()
    previous_end = 0
    for interval in domains:
        begin, end = interval['begin'], interval['end']
        assert previous_end <= begin < end <= len(actual_native_trace)
        for index in range(begin,end):
            assert actual_native_trace[index] in pc_map
            actual_native_trace[index] = pc_map[actual_native_trace[index]]
        previous_end = end
    trace_equal = actual_native_trace == host['instructions']
    comparisons = {key:dict(equal=native['CPU'][key]==converted[key],native=native['CPU'][key],original_host=converted[key],
        independent_emulator=original['actual_CPU'][key]) for key in ('gp','xmm','x87_control','x87_status','mxcsr')}
    comparisons['defined_flags'] = dict(equal=native['CPU']['eflags']&mask == converted['eflags']&mask,
        mask=mask, native=native['CPU']['eflags'], original_host=converted['eflags'], independent_emulator=original['actual_CPU']['eflags'])
    assert observed['x87_tag'] == 65535 and native['CPU']['x87_depth'] == 0
    normalized = lambda row:(row['address'],row['before'],row['written'])
    host_writes = host['writes']
    native_writes = native['memory_writes']
    native_stores_equal = len(native_writes)==len(host_writes) and all(normalized(a)==normalized(b) for a,b in zip(native_writes,host_writes))
    emulator_stores_equal = len(stores)==len(host_writes) and all(normalized(a)==normalized(b) for a,b in zip(stores,host_writes))
    differences = []
    for index,(left,right) in enumerate(zip(native_writes,host_writes)):
        if normalized(left)!=normalized(right):
            differences.append(dict(index=index,native=left,original_host=right))
            if len(differences)==8:
                break
    # The unchanged host forces x87 reserved CW bit6. These original CRT guards
    # compare against027F; the independent emulator retains023F and takes the
    # extra save/restore paths. Keep both raw observations, without patching
    # instructions or changing native math to fit the emulator.
    assert code.at(0x32ccfc).bytes.hex()=='66813c247f02'
    assert code.at(0x332133).bytes.hex()=='663d7f02'
    report = dict(passed=all(row['equal'] for row in comparisons.values()) and trace_equal and native_stores_equal,
        scope='Complete unchanged original prefix versus native raw CPU, domain-authenticated trace and every ordered write byte',
        entry=call['entry'], stop=original['declared_prefix_stop'], original_host_instructions=len(host['instructions']),
        independent_emulator_trace_equal=host['instructions']==trace, native_instructions=len(native['instructions']),
        native_actual_instruction_trace_equal=trace_equal,
        canonical_damage_observer_domains=domains,
        canonical_damage_whole_routines_authenticated=len(owners), canonical_damage_static_sites_authenticated=len(pc_map),
        canonical_damage_source_sha256=sha(damage_source),
        all_other_observer_domains_use_unchanged_actual_PCs=True,
        original_CRT_control_word_guards=[0x32ccfc,0x332133],
        independent_emulator_extra_instructions=len(trace)-len(host['instructions']),
        independent_emulator_control_xor=converted['x87_control']^original['actual_CPU']['x87_control'],
        independent_emulator_status_xor=converted['x87_status']^original['actual_CPU']['x87_status'],
        original_host_ordered_writes=len(host_writes),native_ordered_writes=len(native_writes),
        native_ordered_write_data_equal=native_stores_equal, independent_emulator_ordered_write_data_equal=emulator_stores_equal,
        native_store_difference_examples=differences, boundary_CPU_comparisons=comparisons,
        original_instruction_patches=[], original_gameplay_returns_or_native_expected_bytes_supplied=False,
        native_expected_data_used=False, original_state_or_bank_patches=[], physical_xbox_verified=False,
        full_native_or_original_firing_verified=False, full_weapon_parity=False,
        actual_original_hardware_binary_sha256=actual['original_hardware_binary_sha256'],
        input_reports_sha256={str(path):sha(path) for path in [args.case/'call.json',args.prefix/'report.json',args.native/'report.json']},
        source_files_sha256={str(path):sha(path) for path in [Path(__file__),ROOT/'tools/oracle_sandbox_first_person_hardware_tu15.py',ROOT/'tools/sandbox_first_person_hardware_trace_tu15.cpp',ROOT/'tools/audit_damage_delivery_observation_pcs_tu15.py']},
        runner_scratch=str(scratch), large_artifacts_retained=[])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    (output/'original-host-observations.json').write_text(json.dumps(dict(exit=observed,instructions=host['instructions'],writes=host_writes),separators=(',',':'))+'\n')
    (output/'damage-observer-correspondence.json').write_text(json.dumps(dict(source_sha256=sha(damage_source),original_TU15_sha256=NEW_PIN,
        whole_routines=owners,instruction_map={f'{pc:08x}':actual for pc,actual in sorted(pc_map.items())},
        canonical_memory_store_sites=sorted(store_sites)),indent=2)+'\n')
    shutil.copyfile(Path(__file__),output/Path(__file__).name)
    print(json.dumps(dict(passed=report['passed'],instructions=len(host['instructions']),boundary_CPU_equal=all(row['equal']for row in comparisons.values()),native_write_data_equal=native_stores_equal,runner_scratch=str(scratch))),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
