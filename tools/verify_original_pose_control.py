#!/usr/bin/env python3
"""Verify the real pose owner, retaining the raw reserved x87 bit difference.

Intel253665-021 Figure8-6 identifies control-word bit6 as reserved. Probe real
FLDCW/FNSTCW normalization and compare every other bit, every source instruction,
ordered store byte, live CPU field and produced model node. No game code changes.
Run via tools/run_test.py; compiler/probe products remain in owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from original_xbox_pose_case import cpu,sha

PROBE=r'''
#include <cstdint>
#include <iostream>
int main(){
 for(std::uint16_t requested:{0x023fu,0x027fu,0x033fu,0x037fu}){
  std::uint16_t saved,actual;
  asm volatile("fnstcw %0":"=m"(saved));
  asm volatile("fldcw %1; fnstcw %0":"=m"(actual):"m"(requested):"memory");
  asm volatile("fldcw %0"::"m"(saved):"memory");
  std::cout<<requested<<" "<<actual<<"\n";
 }
}
'''

def accepts_control(actual,expected):
    return ((actual^expected)&~0x40)==0

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case',type=Path,required=True);p.add_argument('--oracle',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    native=json.loads((args.native/'report.json').read_text());actual=json.loads((args.native/'native-observations.json').read_text())
    original=json.loads((args.oracle/'report.json').read_text());expected=cpu(args.case,'after')
    assert original['passed']and actual['returned']and not native['native_expected_data_used']
    native_control=actual['CPU']['x87_control'];original_control=expected['x87_control']
    control_errors=[]if native_control==original_control else[dict(kind='x87_control',native=native_control,original=original_control)]
    assert native['errors']==control_errors
    source=scratch/'control.cpp';source.write_text(PROBE);binary=scratch/'control-probe'
    compile_command=['c++','-std=c++20','-O2',str(source),'-o',str(binary)]
    compile_result=subprocess.run(compile_command,capture_output=True,text=True)
    assert compile_result.returncode==0,compile_result.stderr[-2000:]
    samples=[list(map(int,line.split()))for line in subprocess.check_output([str(binary)],text=True).splitlines()]
    assert samples==[[575,639],[639,639],[831,895],[895,895]]
    assert accepts_control(639,575)
    negative_controls=[dict(changed_bit=bit,rejected=not accepts_control(639^(1<<bit),575))for bit in (0,5,8,9,10,11,12,15)]
    assert all(row['rejected']for row in negative_controls)
    assert accepts_control(actual['CPU']['x87_control'],expected['x87_control'])
    for key in ('gp','xmm','fs_base','x87_status','mxcsr'):assert actual['CPU'][key]==expected[key],key
    assert actual['CPU']['eflags']&0xcd5==expected['eflags']&0xcd5
    assert actual['CPU']['x87_depth']==0 and not actual['CPU']['active_mmx']and expected['x87_tag']==65535
    original_trace=json.loads((args.oracle/'instructions.json').read_text())
    assert actual['instructions']==original_trace
    source_writes=json.loads((args.oracle/'writes.json').read_text())
    def flattened(rows):
        result=[]
        for row in rows:
            a,b=bytes.fromhex(row['before']),bytes.fromhex(row['written']);assert len(a)==len(b)
            result.extend((row['instruction'],row['address']+i,old,new)for i,(old,new)in enumerate(zip(a,b)))
        return result
    assert flattened(actual['memory_writes'])==flattened(source_writes)
    call=json.loads((args.case/'call.json').read_text())
    for index,model in enumerate(call['models']):
        assert sha(args.native/f'native-model-{index}-nodes.bin')==model['nodes_sha256']
    assert all(m['exact_nodes_and_identity']for m in native['models'])
    if call.get('first_person_state'):assert native['complete_first_person_state_equal']and original['complete_first_person_state_equal']
    assert actual['actual_host_CPU_estimates']==[]
    sources=args.output.with_name(args.output.stem+'-sources');sources.mkdir(exist_ok=False)
    (sources/'control-probe.cpp').write_text(PROBE)
    for path in (Path(__file__),Path(__file__).with_name('original_xbox_pose_case.py')):
        shutil.copyfile(path,sources/path.name)
    pins={}
    for folder in (args.case,args.oracle,args.native):
        for path in folder.iterdir():
            if path.is_file():pins[str(path.absolute())]=sha(path)
    for path in sources.iterdir():pins[str(path.absolute())]=sha(path)
    report=dict(passed=True,scope=__doc__,entry=call['entry'],source_tick=call['tick_before'],
        native_instructions=len(actual['instructions']),original_instructions=len(original_trace),
        ordered_store_records=len(source_writes),source_store_bytes=len(flattened(source_writes)),
        exact_model_nodes=sum(m['node_count']for m in call['models']),models=native['models'],
        complete_first_person_state_equal=native.get('complete_first_person_state_equal'),
        native_expected_data_used=False,supplied_gameplay_returns=[],host_CPU_estimates_needed=0,
        all_defined_CPU_FP_fields_equal=True,raw_x87_control_equal=native_control==original_control,
        raw_x87_control=dict(original=original_control,native=native_control,xor=native_control^original_control,
            comparison_ignores_only_bit=6,raw_discrepancy_preserved_in_native_report=True),
        host_control_probe=samples,nonreserved_negative_controls=negative_controls,
        reserved_bit_authority=dict(document='Intel253665-021 Volume1',section='8.1.5',figure='8-6',page='8-11',
            url='https://www.intel.com/content/dam/support/us/en/documents/processors/pentium4/sb/25366521.pdf#page=223'),
        compiler_command=compile_command,control_probe_binary_sha256=sha(binary),linked_files_sha256=pins,
        native_library_build_scope='Existing selected native library build; not a complete current-root rebuild',
        native_library_files_sha256=native['existing_native_library_files_sha256'],
        complete_current_root_rebuild=False,complete_native_runtime_verified=False,
        whole_original_physical_RAM_equal=original['whole_physical_RAM_equal'],
        whole_original_RAM_boundary='Original page-table/device/asynchronous differences are retained in the oracle report; API-owned stores and outputs compare exactly',
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'],physical_xbox_verified=False,full_weapon_parity=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,instructions=report['native_instructions'],ordered_stores=report['ordered_store_records'],
        source_store_bytes=report['source_store_bytes'],exact_model_nodes=report['exact_model_nodes'],
        reserved_x87_bit_difference_retained=True,report=str(args.output.absolute()))),flush=True)

if __name__=='__main__':main()
