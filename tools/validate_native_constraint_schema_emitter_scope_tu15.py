#!/usr/bin/env python3
"""Actual3caller scoped native emission using exact old/current world headers.

Only the emitter function body is extracted from the actual generated source.
Frozen dependency headers are copied and hashed without a modern state refresh.
The typed native owner and synchronous bank/executor policy remain separate
from the unscoped offline literal/reference route.
"""
import argparse,copy,difflib,hashlib,json,os,re,shutil,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_constraint_initializer_tu15 import descriptors
import validate_native_constraint_schema_emitter_tu15 as original
BACKEND='unchanged-tu15-scoped-schema-emitter-real-parent-phase-v1'
OWNED=('include/halo2/native_constraint_schema_emitter_consumer_tu15.hpp','include/halo2/native_tu15_constraint_schema_emitter.hpp',
       'src/physics/native_constraint_schema_emitter_consumer_tu15.cpp','src/physics/native_tu15_constraint_schema_emitter.cpp',
       'src/physics/runtime/source_constraint_schema_emitter_native_tu15_internal.hpp','tests/native_constraint_schema_emitter_scope_tu15_tests.cpp')
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def copy_closure(root,project,initial):
    pending=[initial];seen={};before={}
    while pending:
        path=pending.pop().resolve()
        if path in seen:continue
        relative=path.relative_to(root);target=project/relative;target.parent.mkdir(parents=True,exist_ok=True);pin=sha(path);shutil.copyfile(path,target);assert sha(target)==pin and sha(path)==pin;seen[path]=pin;before[str(relative)]=pin
        for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"',path.read_text(),re.M):
            options=(path.parent/name,root/'include'/name,root/'src'/name,root/'src'/Path(name).name)
            child=next((p for p in options if p.is_file()),None)
            assert child is not None,(str(path),name,'Missing exact dependency')
            pending.append(child)
    return before
def stage(project,root,frozen,out):
    initial=root/('src/physics_world_runtime_internal.hpp'if frozen else'src/physics/runtime/physics_world_runtime_internal.hpp');pins=copy_closure(root,project,initial);owned={}
    for name in OWNED:
        path=ROOT/name;target=project/name;target.parent.mkdir(parents=True,exist_ok=True);pin=sha(path);shutil.copyfile(path,target);assert sha(path)==pin;owned[name]=pin
    # New semantic header dependencies can already exist in the source cohort;
    # copy only their exact closure, never a broad modern State header overlay.
    for header in ('include/halo2/native_tu15_constraint_step.hpp',):
        path=root/header
        if path.exists():pins.update(copy_closure(root,project,path))
    generated=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp';text=generated.read_text();needle='static void run_326820(ObservedWorldState &s) {';start=text.index(needle);opening=text.index('{',start);depth=1;end=opening+1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    body=text[start:end];hook='if(try_native_constraint_schema_emitter_tu15(s))return;';assert body.count(hook)==1
    (project/'emitter_driver.hpp').write_text(body+'\n');(project/'emitter_literal_driver.hpp').write_text(body.replace(hook,'')+'\n')
    (out/(('frozen'if frozen else'modern')+'-driver.diff')).write_text(''.join(difflib.unified_diff(body.replace(hook,'').splitlines(True),body.splitlines(True),fromfile='actual-unscoped-literal-emitter',tofile='actual-native-scoped-emitter')))
    assert all(sha(root/name)==pin for name,pin in pins.items()),'Dependency source changed while copying'
    return dict(dependency_source=str(root),exact_dependency_sha256=pins,root_source_sha256=owned,generated_cpp_sha256=sha(generated),extracted_driver_sha256=sha(project/'emitter_driver.hpp'),modern_state_wholesale_refresh_used=False)
def fixture(image,scratch):
    code=_Code(image,'.text');body=complete_body(code,0x32a390);meta=dict(sites=descriptors([code.at(p)for p in sorted(body.code)]),regions=[dict(address=original.BASE,bytes=0x1000),dict(address=original.SP,bytes=20)])
    out=[]
    for caller in(0x31f7c3,0x31f8a0,0x31f95e):
        original.STOP=caller+5;complete=original.cases()
        for c in complete:
            c['name']+=f'_caller_{caller:x}';c['actual_caller']=caller;c['stop_pc']=caller+5;arguments=c['banks'][1];arguments['address']=original.SP;arguments['before']=struct.pack('<I',caller+5).hex()+arguments['before']
        complete=original.physical(image,meta,complete,scratch)
        # Prefix builder expects the source seed name before caller labeling.
        seed=next(c for c in complete if c['name'].startswith('read_fault_seed_'));seed_copy=copy.deepcopy(seed);seed_copy['name']='read_fault_seed';prefix=original.prefixes(image,[seed_copy])
        for c in prefix:c['name']+=f'_caller_{caller:x}';c['actual_caller']=caller
        ret_fault=copy.deepcopy(seed);ret_fault.update(name=f'actual_late_RET_read_fault_caller_{caller:x}',stop_pc=0x32a3ba,fault=dict(address=original.SP,bytes=4,occurrence=1));prefix.append(ret_fault)
        out+=complete+original.physical(image,meta,prefix,scratch)
    path=scratch/'scope-original.json';path.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,cases=out),separators=(',',':')));return path,out
def qualify(project,fixture,out,compiler,frozen):
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_emitter_scope_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for kind in('original','negative'):
        target='native_emitter_scope_'+kind;test=project/'tests/native_constraint_schema_emitter_scope_tu15_tests.cpp'
        if kind=='negative':
            test=project/'negative-test.cpp';source=(project/'tests/native_constraint_schema_emitter_scope_tu15_tests.cpp').read_text();assert source.count('#include "emitter_driver.hpp"')==1;test.write_text(source.replace('#include "emitter_driver.hpp"','#include "emitter_literal_driver.hpp"'))
        cmake+=f'add_executable({target} "{test}" src/physics/native_constraint_schema_emitter_consumer_tu15.cpp src/physics/native_tu15_constraint_schema_emitter.cpp)\ntarget_include_directories({target} PRIVATE include src .)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
        if frozen:cmake+=f'target_compile_definitions({target} PRIVATE HALO2_FROZEN_WORLD_STATE=1)\n'
        command=f'$<TARGET_FILE:{target}> "{fixture}"'if kind=='original'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and "scoped native owner did not execute" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Physical scoped proof rejects bypassed native entry:",r.stderr.strip())\n')
    (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(project.parent/'builds'),'--output',str(out),'--target','native_emitter_scope_original','--target','native_emitter_scope_negative','--filter','^native_emitter_scope_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];result=subprocess.run(command);return dict(compiler=compiler,exit_code=result.returncode,command=command)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--frozen-source',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);original_fixture,cases=fixture(image,scratch);contexts=[];results=[]
    for frozen,root in((True,args.frozen_source.resolve()),(False,ROOT)):
        name='frozen'if frozen else'modern';project=scratch/name;project.mkdir();contexts.append(stage(project,root,frozen,args.output))
        for compiler in('g++','clang++'):results.append(dict(context=name,**qualify(project,original_fixture,args.output/name/compiler,compiler,frozen)))
    files=[ROOT/name for name in OWNED]+[ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp',ROOT/'tools/generate_physics_game_frame_runtime_20261011.py',ROOT/'tools/validate_native_constraint_schema_emitter_tu15.py',Path(__file__)];hashes={str(p.relative_to(ROOT)):sha(p)for p in files};report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend=BACKEND,image_sha256=NEW_PIN,original_cases=len(cases),actual_internal_callers=[0x31f7c3,0x31f8a0,0x31f95e],context_snapshots=contexts,ordered_original_reads=sum(len(c['reads'])for c in cases),ordered_original_stores=sum(len(c['writes'])for c in cases),original_instructions=sum(c['original_instructions']for c in cases),fixture_sha256=sha(original_fixture),fixture_bytes=original_fixture.stat().st_size,root_source_sha256=hashes,current_sources_match_snapshot=all(sha(ROOT/p)==pin for p,pin in hashes.items()),owned_scratch=str(scratch),retained_large_artifacts=[],admission_read_speculation=False,caller_provenance='Actual parent instruction cursor before leaf phase publication',negative_control='Minimal generated emitter seam removed; literal route cannot satisfy native owner/cursor gate',excluded=['whole constraint producer continuation math','visible multiplayer world scope wiring/participation','unknown indirect caller ABI'])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','ordered_original_reads','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
