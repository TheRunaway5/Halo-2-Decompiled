#!/usr/bin/env python3
"""Native BDA50/BCFB0/BE7E0 and existing B9390 against physical TU1.5."""
import argparse,copy,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_contact_entity_notification_tu15 as notification
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,POOL,ROWS,DATA,GLOBAL=0x1000000,0x1000100,0x1000800,0x1002800,0x4ec000
ENTRIES={0xbda50:30,0xbcfb0:13,0xbe7e0:33,0xb9390:26}
def cases():
    rows=[]
    def make(entry=0xbda50,depth=3,flags=1,kind=0,branch=False,seed=False,none=False):
        raw=bytearray(0x4000);glob=bytearray(0x1000)
        def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
        put(POOL+0x44,ROWS);glob[0xbd0:0xbd4]=POOL.to_bytes(4,'little')
        for k in range(5):
            header=ROWS+k*12;data=DATA+k*0x80;put(header+2,flags if k==depth-1 else flags&0x7f,1);put(header+3,(kind+k*7)&255,1);put(header+8,data)
            put(data+0xc,0xffffffff);put(data+0x10,k+1 if k+1<depth else 0xffffffff);put(data+0x14,k-1 if k else 0xffffffff)
        if branch:put(DATA+0x80+0xc,4);put(DATA+4*0x80+0x14,0)
        datum=0xffffffff if none else depth-1 if entry in (0xbda50,0xbcfb0,0xb9390) else 0
        n=len(rows);gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=reference.SP;gp[0]=datum;gp[1]=datum
        rows.append(dict(id=n,name=('fault_seed_'if seed else'publication_')+str(n),entry=entry,world=0,island=0,body=0,object=datum,
            arguments=[datum]if entry==0xbe7e0 else[],gp=gp,banks=[dict(address=BASE,before=raw.hex()),dict(address=GLOBAL,before=glob.hex())],
            before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*7%64),0x1f80|((n%4)<<13)],live_x87=[],alignment_fault=False))
    for depth in (1,2,4):
        for flags in (0,1,2,3,4,0x80,0x81,0x82,0xff):
            for kind in (0,1,2,12,31,32,44,255):make(depth=depth,flags=flags,kind=kind,branch=depth>1)
    for entry in (0xbcfb0,0xbe7e0,0xb9390):
        for depth in (1,2,4):
            for flags in (0,1,2,0x81):make(entry=entry,depth=depth,flags=flags,kind=depth,branch=depth>1)
    make(entry=0xbcfb0,none=True)
    make(depth=4,flags=0x81,kind=0,branch=True,seed=True);make(entry=0xbe7e0,depth=4,flags=0,kind=1,branch=True,seed=True)
    return rows
def prepare(image,meta,scratch):
    owned=activation.prepare(image,meta,scratch)
    p=owned/'reference.s';s=p.read_text();needle='.section .stack,"aw"';assert s.count(needle)==1;p.write_text(s.replace(needle,'.section .objects,"aw"\n.space 4096\n'+needle))
    p=owned/'link.ld';s=p.read_text();needle='.stack 0x';assert s.count(needle)==1;p.write_text(s.replace(needle,'.objects 0x4ec000:{*(.objects)} '+needle))
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')]):
        r=subprocess.run(command,capture_output=True,text=True);assert r.returncode==0,r.stderr[-3000:]
    return owned
def driver():
    text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({30,13,33,26})')
    start=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',start)
    text=text[:start]+'std::uint32_t result{};J required=nullptr;\n'+text[end:]
    start=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',start)
    text=text[:start]+'''switch(entry){
    case 0xbda50:native_tu15_publish_object_update(m,{test.at("object")},observer);break;
    case 0xbcfb0:result=native_tu15_publication_root(m,{test.at("object")});break;
    case 0xbe7e0:native_tu15_mark_publication_tree(m,{test.at("object")},observer);break;
    case 0xb9390:native_tu15_wake_object_ancestors(m,test.at("object"),observer);break;
    default:throw std::runtime_error("Unexpected publication owner");}
'''+text[end:]
    start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start)
    text=text[:start]+'''   if(test.at("entry")==0xbcfb0&&test.at("completed").get<bool>())check(result==test.at("boundary_gp")[0],name+" root datum differs");'''+text[end:]
    return text
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_object_publication.cpp'
    files=[source,ROOT/'include/halo2/native_tu15_object_publication.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp',ROOT/'include/halo2/native_tu15_activation_services.hpp',Path(__file__),ROOT/'tools/validate_native_contact_entity_notification_tu15.py',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp'];pins={str(p.relative_to(ROOT)):reference.sha(p)for p in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);old=notification.ENTRIES;notification.ENTRIES=ENTRIES
        try:meta=notification.metadata(image)
        finally:notification.ENTRIES=old
        assert all(op['read']or op['write']for s in meta['sites']for op in s['operands']);owned=prepare(image,meta,scratch);rows=reference.physical(owned,cases(),'complete');faults=[];unqualified=[]
        for seed in (c for c in rows if c['name'].startswith('fault_seed')):
            for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                for n,event in enumerate(events):
                    if phase=='before_write'and event['instruction']in (0xbda93,0xb93c8):
                        unqualified.append(dict(seed=seed['name'],phase=phase,ordinal=n,instruction=event['instruction'],reason='Atomic source RMW prewrite stop precedes its operand read; native memory API splits read and write'));continue
                    c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
        rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'publication.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
        project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_object_publication.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'services.cpp');text=source.read_text();mutants={}
        for name,needle,replacement in (('wrong_header_flag','word(m,p,1)|4u','word(m,p,1)|8u'),('wrong_root','result=object.datum;','if(!result)result=object.datum;'),('wrong_tree_types','&0x1003u','&0x1007u')):
            assert text.count(needle)==1;mutants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(mutants[name])
        (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects publication mutation:",r.stderr[-400:])\n')
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeObjectPublication LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
        for name in ('native',*mutants):
            cmake+=f'add_executable(publication_{name} $<TARGET_OBJECTS:driver> {name}.cpp services.cpp)\ntarget_include_directories(publication_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(publication_{name} PRIVATE cxx_std_20)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:publication_{name}> "{fixture}"';cmake+=f'add_test(NAME publication_{name} COMMAND {command})\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','publication_native',*sum((['--target','publication_'+n]for n in mutants),[]),'--filter','^publication_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
        unchanged=all(reference.sha(ROOT/p)==h for p,h in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),fault_prefixes=len(faults),unqualified_prefixes=unqualified,ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],negative_controls=list(mutants),sources_unchanged=unchanged,image_sha256=reference.NEW_PIN,instruction_patches=[],supplied_original_outcomes=False,remaining=['Atomic RMW prewrite observer contract','Callback-mutated bank/alias domains','Native frame binding and visible multiplayer','Actual platform page-fault delivery'])
    except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
    finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','original_cases','fault_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
