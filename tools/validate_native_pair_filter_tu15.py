#!/usr/bin/env python3
"""Qualify native2E9A00 against unchanged physical TU1.5 instructions."""
import argparse,copy,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_contact_entity_notification_tu15 as notification
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,FILTER,FIRST,SECOND,RESULT=0x1000000,0x1000100,0x1000500,0x1000600,0x1000800
def cases():
    rows=[]
    def make(first,second,mask,alias=0,seed=False):
        raw=bytearray((k*19+7)&255 for k in range(0x4000))
        def put(p,v):raw[p-BASE:p-BASE+4]=(v&0xffffffff).to_bytes(4,'little')
        for k in range(32):put(FILTER+0x14+k*4,mask if k==(first&31) else (mask^0xffffffff))
        put(FIRST+0x1c,first);put(SECOND+0x1c,second)
        decision=(RESULT,FIRST+0x1c,SECOND+0x1c,FILTER+0x14+(first&31)*4)[alias]
        n=len(rows);gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=reference.SP;gp[1]=FILTER
        rows.append(dict(id=n,name=('fault_seed_'if seed else'filter_')+str(n),entry=0x2e9a00,world=0,island=0,body=0,
            filter=FILTER,first=FIRST,second=SECOND,decision=decision,arguments=[decision,FIRST,SECOND],gp=gp,
            banks=[dict(address=BASE,before=raw.hex())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*7%64),0x1f80|((n%4)<<13)],
            live_x87=[],alignment_fault=False))
    # Every matrix row/bit, both outcomes, and high low-word bits independent
    # of the source's five-bit shift/index operands.
    for first in range(32):
        for second in range(32):
            make(first|0xffe0,second|0xaae0,(1<<second) if (first+second)%2 else ~(1<<second))
    for first_group,second_group in ((0,0),(1,1),(0xffff,0xffff),(1,2),(0xffff,0),(0,0xffff)):
        for first,second in ((0,0),(31,31),(0,31),(31,0)):
            for mask in (0,0xffffffff,0x80000001,0x7ffffffe):
                make((first_group<<16)|first,(second_group<<16)|second,mask)
    for alias in range(4):
        for first,second in ((3,7),(0x12340003,0x12340007),(0x12340003,0x43210007)):
            for mask in (0,0xffffffff):make(first,second,mask,alias)
    make(3,7,0xffffffff,seed=True);make(0x12340003,0x12340007,0,seed=True)
    return rows
def driver():
    text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({27})')
    start=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',start)
    text=text[:start]+'std::uint32_t result{};J required=nullptr;\n'+text[end:]
    start=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',start)
    text=text[:start]+'''result=native_tu15_activation_group_filter(m,test.at("filter"),test.at("decision"),
        {test.at("first")},{test.at("second")},observer);
'''+text[end:]
    start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start)
    text=text[:start]+'''   if(test.at("completed").get<bool>())check(result==test.at("boundary_gp")[0],name+" actual filter result DATA pointer differs");'''+text[end:]
    return text
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source=ROOT/'src/physics/native_tu15_activation_services.cpp';files=[source,ROOT/'include/halo2/native_tu15_activation_services.hpp',Path(__file__),
        ROOT/'tools/validate_native_contact_entity_notification_tu15.py',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp']
    pins={str(p.relative_to(ROOT)):reference.sha(p)for p in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT)
        old=notification.ENTRIES;notification.ENTRIES={0x2e9a00:27}
        try:meta=notification.metadata(image)
        finally:notification.ENTRIES=old
        assert all(op['read']or op['write']for s in meta['sites']for op in s['operands'])
        owned=activation.prepare(image,meta,scratch);rows=reference.physical(owned,cases(),'complete');faults=[]
        for seed in (c for c in rows if c['name'].startswith('fault_seed')):
            for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                for n,event in enumerate(events):
                    c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
        rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'pair-filter.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
        project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
        (project/'driver.cpp').write_text('#include "halo2/native_tu15_activation_services.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();mutants={}
        for name,needle,replacement in (('wrong_group','&&(first_group&0xffff0000u)','&&!(first_group&0xffff0000u)'),('wrong_row','(first_group&31u)*4u','(second_group&31u)*4u'),('wrong_bit','1u<<(second_group&31u)','1u<<((second_group+1u)&31u)')):
            assert text.count(needle)==1;mutants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(mutants[name])
        (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Unchanged original rejects filter mutation:",r.stderr[-400:])\n')
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativePairFilter LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
        for name in ('native',*mutants):
            cmake+=f'add_executable(filter_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(filter_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(filter_{name} PRIVATE cxx_std_20)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:filter_{name}> "{fixture}"'
            cmake+=f'add_test(NAME filter_{name} COMMAND {command})\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','filter_native',*sum((['--target','filter_'+n]for n in mutants),[]),'--filter','^filter_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1']
            r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
        unchanged=all(reference.sha(ROOT/p)==h for p,h in pins.items())
        report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),fault_prefixes=len(faults),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],negative_controls=list(mutants),sources_unchanged=unchanged,image_sha256=reference.NEW_PIN,instruction_patches=[],supplied_original_outcomes=False,remaining=['Composed pair-consumer original full-return proof','Live World activation and visible multiplayer','Actual platform page-fault delivery'])
    except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','original_cases','fault_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
