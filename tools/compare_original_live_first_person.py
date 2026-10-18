#!/usr/bin/env python3
"""Compare existing compiled native owners with an original live call.

Native code receives only captured entry CPU state and physical RAM. Actual
page tables retain address aliases. No expected poses or game returns are fed
to the native execution. The selected library build is explicit provenance,
not a claim of a complete current-root rebuild or full installed runtime parity.
Run via tools/run_test.py; compiler products and full native RAM are disposable.
"""
import argparse
import hashlib
import json
import mmap
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
from original_xbox_pose_case import ROOT,cpu,physical_ram,PageTableMemory,sha

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case',type=Path,required=True);p.add_argument('--oracle',type=Path,required=True)
    p.add_argument('--library-build',type=Path,default=ROOT/'build-multiplayer')
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);out=args.output.absolute();out.mkdir(parents=True,exist_ok=False)
    case=args.case.absolute();before=cpu(case,'before');after=cpu(case,'after')
    original=json.loads((args.oracle/'report.json').read_text());assert original['passed']
    assert original.get('write_and_read_trace_address_space','virtual')=='virtual', 'Native comparison requires explicitly normalized virtual/physical store traces'
    call=json.loads((case/'call.json').read_text())
    input_json=scratch/'entry-cpu.json';input_json.write_text(json.dumps(before))
    with physical_ram(case,'before')as original_ram:(scratch/'original-ram.bin').write_bytes(original_ram[:])
    cpp=ROOT/'tools/replay_original_live_first_person.cpp';build=args.library_build.absolute()
    link_file=build/'CMakeFiles/halo2_native.dir/link.txt';tokens=shlex.split(link_file.read_text())
    tail=tokens[tokens.index('-o')+2:];libraries=[];missing_archives=[]
    for token in tail:
        if token.startswith('-Wl,-rpath'):continue
        if not token.startswith('-') and not Path(token).is_absolute():token=str(build/token)
        if token.endswith('.a') and not Path(token).is_file():
            missing_archives.append(token);continue
        libraries.append(token)
    library_files=sorted({Path(token)for token in libraries if not token.startswith('-') and Path(token).is_file()})
    library_hashes={str(path):sha(path)for path in library_files}
    binary=scratch/'native-pose';dependency=scratch/'native-pose.d'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MMD','-MF',str(dependency),str(cpp),
        '-Wl,--gc-sections','-Wl,--as-needed','-Wl,--start-group',*libraries,'-Wl,--end-group','-o',str(binary)]
    (out/'build-command.json').write_text(json.dumps(dict(command=command,runner_scratch=str(scratch),
        absent_baseline_archives=missing_archives,policy='Absent archive names are omitted; every referenced native symbol must still link without replacement'),indent=2)+'\n')
    compile_result=subprocess.run(command,capture_output=True,text=True)
    (out/'compile.log').write_text((compile_result.stdout+compile_result.stderr)[-65536:])
    assert compile_result.returncode==0,compile_result.stderr[-3000:]
    native_report=scratch/'native.json';native_ram=scratch/'native-ram.bin'
    run_command=[str(binary),str(input_json),str(scratch/'original-ram.bin'),str(native_report),str(native_ram),hex(call['entry'])]
    dynamic_dirs=sorted({str(path.parent)for path in library_files if '.so' in path.name})
    native_env=dict(os.environ,LD_LIBRARY_PATH=':'.join(dynamic_dirs+[os.environ.get('LD_LIBRARY_PATH','')]).rstrip(':'))
    run=subprocess.run(run_command,env=native_env,capture_output=True,text=True,timeout=60)
    (out/'run.log').write_text((run.stdout+run.stderr)[-65536:])
    assert native_report.is_file(),run.stderr
    actual=json.loads(native_report.read_text());errors=[]
    if not actual['returned']:errors.append(dict(kind='native_execution',error=actual['error'],last_instruction=actual['last_instruction']))
    expected_trace=json.loads((args.oracle/'instructions.json').read_text());native_trace=actual['instructions']
    if native_trace!=expected_trace:
        first=next((i for i,(a,b)in enumerate(zip(native_trace,expected_trace))if a!=b),min(len(native_trace),len(expected_trace)))
        errors.append(dict(kind='instruction_trace',first=first,native=native_trace[max(0,first-3):first+8],original=expected_trace[max(0,first-3):first+8]))
    for key in ('gp','xmm','fs_base','x87_control','x87_status','mxcsr'):
        if actual['CPU'].get(key)!=after[key]:errors.append(dict(kind=key,native=actual['CPU'].get(key),original=after[key]))
    if actual['CPU']['eflags']&0xcd5!=after['eflags']&0xcd5:errors.append(dict(kind='defined_flags'))
    if actual['CPU']['x87_depth']!=0 or actual['CPU']['active_mmx']:errors.append(dict(kind='native_x87_tags_not_empty'))
    original_writes=json.loads((args.oracle/'writes.json').read_text())
    def byte_writes(rows):
        result=[]
        for row in rows:
            old=bytes.fromhex(row['before']);written=bytes.fromhex(row['written']);assert len(old)==len(written)
            result.extend((row['instruction'],row['address']+i,a,b)for i,(a,b)in enumerate(zip(old,written)))
        return result
    expected_stores=byte_writes(original_writes);native_stores=byte_writes(actual['memory_writes'])
    if expected_stores!=native_stores:
        first=next((i for i,(a,b)in enumerate(zip(native_stores,expected_stores))if a!=b),min(len(native_stores),len(expected_stores)))
        errors.append(dict(kind='ordered_source_byte_writes',first=first,native=native_stores[max(0,first-2):first+3],original=expected_stores[max(0,first-2):first+3]))
    models=[];state_equal=None;object_results=[]
    with native_ram.open('rb')as file,mmap.mmap(file.fileno(),0,access=mmap.ACCESS_READ)as memory,physical_ram(case,'after')as original_after:
        mapped=PageTableMemory(memory,before);expected=PageTableMemory(original_after,after)
        for index,model in enumerate(call['models']):
            address=call['arguments'][3]+index*0xd0c;raw=mapped.read(address+12,model['node_count']*52)
            reference=(case/model['nodes_file']).read_bytes()
            descriptor=struct.unpack('<3I',mapped.read(address,12))
            equal=raw==reference and descriptor==(model['model'],model['owner'],model['mode'])
            row=dict(model=model['model'],node_count=model['node_count'],exact_nodes_and_identity=equal,
                native_sha256=hashlib.sha256(raw).hexdigest(),original_sha256=model['nodes_sha256'])
            models.append(row)
            if not equal:errors.append(dict(kind='model_nodes',**row))
            (out/f'native-model-{index}-nodes.bin').write_bytes(raw)
        final_differences=[]
        for address,size in sorted({(w['address'],w['bytes'])for w in original_writes}):
            if mapped.read(address,size)!=expected.read(address,size):final_differences.append(dict(address=address,bytes=size))
        if final_differences:errors.append(dict(kind='final_original_owned_bytes',different_spans=len(final_differences),examples=final_differences[:8]))
        state_bank=call.get('first_person_state')
        if state_bank:
            state_equal=mapped.read(state_bank['address'],state_bank['bytes'])==(case/'after-first-person-state.bin').read_bytes()
            if not state_equal:errors.append(dict(kind='complete_first_person_state'))
        for obj in call.get('observed_objects',[]):
            raw=mapped.read(obj['address'],obj['bytes']);equal=raw==(case/('after-'+obj['name']+'-body.bin')).read_bytes()
            object_results.append(dict(name=obj['name'],identity=obj['identity'],bytes=obj['bytes'],exact=equal))
            if not equal:errors.append(dict(kind='complete_object_body',name=obj['name']))
    for path,digest in library_hashes.items():assert sha(path)==digest
    for source in (cpp,Path(__file__),Path(__file__).with_name('original_xbox_pose_case.py')):
        shutil.copyfile(source,out/source.name)
    dependencies=dependency.read_text().replace('\\\n',' ').split(':',1)[1].split()
    source_hashes={str(Path(name).absolute()):sha(name)for name in dependencies if Path(name).is_file()}
    report=dict(passed=not errors,scope=__doc__,errors=errors,models=models,entry=call['entry'],complete_first_person_state_equal=state_equal,complete_object_bodies=object_results,
        native_instructions=len(native_trace),original_instructions=len(expected_trace),
        native_ordered_store_observations=len(actual['memory_writes']),original_ordered_store_observations=len(original_writes),
        native_source_byte_writes=len(native_stores),original_source_byte_writes=len(expected_stores),
        actual_host_CPU_estimates=actual['actual_host_CPU_estimates'],native_expected_data_used=False,
        compile_command=command,run_command=run_command,native_binary_sha256=sha(binary),
        existing_native_library_files_sha256=library_hashes,adapter_source_files_sha256=source_hashes,
        absent_baseline_archives=missing_archives,
        original_case=str(case),original_call_sha256=sha(case/'call.json'),original_oracle_sha256=sha(args.oracle/'report.json'),
        baseline_link_recipe_sha256=sha(link_file),runner_scratch=str(scratch),new_persistent_build_tree=False,
        loader_search_directories=dynamic_dirs,
        complete_current_root_rebuild=False,full_native_runtime_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    (out/'native-observations.json').write_text(json.dumps(actual,separators=(',',':'))+'\n')
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in('scope','existing_native_library_files_sha256','adapter_source_files_sha256','compile_command','run_command','actual_host_CPU_estimates')}
        |dict(host_CPU_estimate_count=len(report['actual_host_CPU_estimates']))),flush=True)
    assert report['passed'],str(out/'report.json')

if __name__=='__main__':main()
