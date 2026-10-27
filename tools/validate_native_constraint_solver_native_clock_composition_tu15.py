#!/usr/bin/env python3
"""Compose genuine whole TU1.5 solver initialization and built-in schema14.

Each unchanged original326A40 entry begins ready-zero and executes its actual
initializer, positive row math, built-in callback, secondary pass and exports.
Native execution composes the proper typed kernels from the same input banks.
No callback output, success flag or solver result is supplied.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import validate_native_constraint_solver_initialized_tu15 as initialized
import validate_native_constraint_custom_schema_tu15 as custom
core=initialized.core
BACKEND='unchanged-tu15-whole-solver-native-clock-initializer-custom-composition-v1'

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def cases(image):
    seeds={case['name']:case for case in initialized.cases(image)};out=[]
    for owner in custom.OWNERS:
        for row in range(2,14):
            for location in('before','after'):
                case=copy.deepcopy(seeds[f'client_finite_{row}_2_0']);case['name']=f'custom_{owner:x}_{row}_{location}'
                bank=next(b for b in case['banks']if b['address']==core.SCHEMA);raw=bytearray.fromhex(bank['before']);length={2:4,3:8,4:16,5:16,6:8,7:20,8:24,9:8,10:16,11:12,12:4,13:4}[row];cursor=0x18 if location=='before'else 0x18+length
                raw[cursor+0x38:cursor+0x38+length+4]=raw[cursor:cursor+length+4];raw[cursor:cursor+0x38]=bytes(0x38)
                struct.pack_into('<II',raw,cursor,14|(0x38<<8)|(2<<16),owner);payload=cursor+8
                for lane in range(8):struct.pack_into('<f',raw,payload+lane*4,.125*(lane+1))
                struct.pack_into('<ffII',raw,payload+0x20,.75,.625,core.ACCUM,core.ACCUM+0x80)
                if owner==0x31f560:struct.pack_into('<I',raw,payload+0x10,core.ROW)
                bank['before']=raw.hex();case['custom_owner']=owner;out.append(case)
        seed=next(c for c in out if c['name']==f'custom_{owner:x}_8_before')
        for value in(0x7fc12345,0x7f812345,0x80000000,1):
            case=copy.deepcopy(seed);case['name']+=f'_edge_{value:x}';core.put(case,core.SCHEMA+0x20,value);out.append(case)
    for number,name in enumerate(('date_full_8','date_full_7','date_prefix_P','date_empty_platform')):
        date=seeds[name];case=copy.deepcopy(next(c for c in out if c['name']==f'custom_{custom.OWNERS[number%3]:x}_8_before'));case['name']='native_clock_'+name
        for bank in date['banks']:
            if bank['address']in dict(initialized.EXTRA_BANKS):
                target=next(b for b in case['banks']if b['address']==bank['address']);target['before']=bank['before']
        core.put(case,0x450108,0);out.append(case)
    return out

def faults(complete):
    out=[]
    for owner in custom.OWNERS:
        seed=next(c for c in complete if c['name']==f'custom_{owner:x}_8_before');seen,occurrences=set(),{}
        for read in seed['reads']:
            key=read['address'],read['bytes'];occurrences[key]=occurrences.get(key,0)+1;pc=read['instruction']
            if not(owner<=pc<owner+0x100)or pc in seen:continue
            seen.add(pc);case=copy.deepcopy(seed);case.update(name=seed['name']+'_fault_'+hex(pc),stop_pc=pc,fault=dict(address=read['address'],bytes=read['bytes'],occurrence=occurrences[key]));out.append(case)
    return out

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
    meta=initialized.metadata(image,scratch);extra=custom.metadata(image);meta['sites']+=extra['sites'];meta['body_sites']+=extra['body_sites'];meta['listing_sha256'].update(extra['listing_sha256'])
    complete=initialized.physical(image,meta,cases(image),scratch);fault_cases=faults(complete);all_cases=complete+initialized.physical(image,meta,fault_cases,scratch)
    fixture=scratch/'custom-composition.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=all_cases),separators=(',',':')))
    project=scratch/'project';project.mkdir();names=('native_tu15_constraint_solver.cpp','native_tu15_constraint_solver_rows.cpp','native_tu15_constraint_solver_internal.hpp','native_tu15_constraint_solver_initializer.cpp','native_tu15_constraint_custom_schema.cpp','native_tu15_constraint_clock.cpp');sources=[ROOT/'src/physics'/n for n in names]
    for source in sources:(project/source.name).write_bytes(source.read_bytes())
    text=(project/names[-2]).read_text();assert text.count('if(n==2)')==1;(project/'mutated.cpp').write_text(text.replace('if(n==2)','if(false)'));(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and ("original solver read order differs" in r.stderr or "ordered solver store differs" in r.stderr),(r.returncode,r.stdout,r.stderr)\nprint("Whole original solver rejects changed native custom callback:",r.stderr.strip())\n')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_solver_custom_composition LANGUAGES CXX)\ninclude(CTest)\n'
    for kind in('original','negative'):
        target='native_solver_custom_'+kind;paths=[project/('mutated.cpp'if kind=='negative'and p.name==names[-2]else p.name)for p in sources if p.suffix=='.cpp'];cpp=' '.join('"'+str(p)+'"'for p in paths)
        cmake+=f'add_executable({target} "{ROOT}/tests/native_tu15_constraint_solver_native_clock_composition_tests.cpp" {cpp})\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=f'$<TARGET_FILE:{target}> "{fixture}"'if kind=='original'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','native_solver_custom_original','--target','native_solver_custom_negative','--filter','^native_solver_custom_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=sources+[ROOT/'include/halo2/native_tu15_constraint_custom_schema.hpp',ROOT/'tests/native_tu15_constraint_solver_native_clock_composition_tests.cpp',Path(initialized.__file__),Path(custom.__file__),ROOT/'tools/observe_native_constraint_initializer_tu15.cpp',Path(__file__)];hashes={str(p.relative_to(ROOT)):sha(project/p.name if p in sources else p)for p in files}
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend=BACKEND,original_cases=len(all_cases),complete_original_calls=len(complete),read_fault_prefixes=len(fault_cases),callback_owners=list(custom.OWNERS),primary_row_domain=list(range(2,14)),genuine_native_clock_compositions=sum(c['actual_time']is not None for c in all_cases),actual_clock_seconds_output_supplied=False,ready_zero_on_every_original_entry=True,core_or_callback_outcome_supplied=False,ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),image_sha256=NEW_PIN,listing_sha256=meta['listing_sha256'],source_hashes=hashes,current_sources_match_snapshot=all(sha(ROOT/p)==value for p,value in hashes.items()),fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,owned_scratch=str(scratch),retained_large_artifacts=[],negative_control='Original whole solver rejects changed source custom scale operand order',excluded=['unknown registered callback owners','full326200 with actual constraint builders','visible multiplayer closure','rejected floating destination write fault semantics'])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
