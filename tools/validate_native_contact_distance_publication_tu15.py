#!/usr/bin/env python3
"""Real TU1.5 30CF40 versus native distance publication, allocation and point removal."""
import argparse
import copy
import hashlib
import inspect
import json
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT, Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

U=base.UI
FIRST,SECOND,CONTEXT,SETTINGS,COUNT,OUTPUT,HEADER,MANAGER,TABLE,RECORDS,DEST,A,B=(U+x for x in (0x100,0x200,0x300,0x400,0x500,0x600,0x700,0x800,0x900,0x1000,0x3000,0x4000,0x4100))
UNITS=('native_tu15_contact_distance_publication.cpp','native_tu15_array_reserve.cpp',
       'native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp')

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def cases():
    result=[]
    for count in (0,1,2,4):
        for distance in (-2.,-.25,0.,.25,2.):
            for tolerance in (.05,1.):
                values={FIRST+8:A,SECOND+8:B,CONTEXT+8:bits(1.),CONTEXT+12:SETTINGS,
                        SETTINGS+8:bits(tolerance),COUNT:count,OUTPUT:HEADER,HEADER:DEST,
                        HEADER+4:2,HEADER+8:0x80000008,MANAGER:TABLE,TABLE+8:0x2e4d60}
                for motion,translation in ((A,0.),(B,.125)):
                    for row in range(4):
                        for lane in range(4):values[motion+32+row*16+lane*4]=bits(translation if row==3 and lane==0 else float(row==lane))
                for n in range(count):
                    first=(distance+float(n)*.1,.5 if n&1 else 0.,0.,.3+float(n)*.1)
                    second=(0.,0.,0.,0.)
                    normal=(1.,0.,0.,0.)
                    for column,vector in enumerate((first,second,normal)):
                        for lane,value in enumerate(vector):values[RECORDS+n*48+column*16+lane*4]=bits(value)
                    values[RECORDS+n*48+28]=0x3f000120+n
                result.append(dict(name=f'contact_{count}_{distance}_{tolerance}',entry=0x30cf40,receiver=0,
                    arguments=[FIRST,SECOND,CONTEXT,bits(-1.),RECORDS,COUNT,OUTPUT,MANAGER],
                    user_words=[(p-U,value)for p,value in values.items()]))
    # Actual generic2E31E0 reserve with genuine native provider/pool. Existing
    # storage is borrowed valid capacity2; source itself computes new capacity.
    for n in (1,2,4):
        row=next(c for c in result if c['name']==f'contact_{n}_0.25_1.0').copy()
        row['name']=f'growth_{n}';row['user_words']=[(p,0x80000002 if p==HEADER-U+8 else v)for p,v in row['user_words']];result.append(row)
    for value in (0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
        for address in (RECORDS,RECORDS+12,RECORDS+36,SETTINGS+8):
            row=copy.deepcopy(next(c for c in result if c['name']=='contact_2_0.25_1.0'))
            row['name']=f'ieee_{address:x}_{value:x}';row['user_words']=[(p,value if p==address-U else v)for p,v in row['user_words']];result.append(row)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,
                scope='Declared coherent current contact/motion/output DATA, genuine allocator/pool execution and genuine RET4 point no-op. Persistent banks, ordered operands/stores and FP strict. Source-private scalar lifetime and generic observers are separate domains; not production/full solver/live qualification.')
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
        base.OWNERS=(0x30cf40,0x2e4d60,0x2e31e0,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420)
        metadata_source=inspect.getsource(base.metadata).replace('read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)',
            'read=(ins.mnemonic not in("fst","fstp"))and(bool(op.access&CS_AC_READ)or ins.mnemonic in("test","cmp")),write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in("fst","fstp")')
        ns=dict(base.__dict__);exec(metadata_source,ns);base.metadata=ns['metadata']
        banks,inputs=base.reference_inputs(image);banks[0x41c500]=image.read(0x41c500,16)
        observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
        observer=observer.replace('unsigned count=0,fault_count=0;', 'unsigned count=0,fault_count=0,observer_count=0;')
        observer=observer.replace('    unsigned mutation_index=0;', '    if(m.contains("observer_stop")&&pc==m.at("observer_stop").at("instruction")&&++observer_count==m.at("observer_stop").at("occurrence")){declared_fault=true;break;}\n    unsigned mutation_index=0;')
        (scratch/'observer.cpp').write_text(observer)
        original_source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace("('fault','mutations')", "('fault','mutations','observer_stop')")
        ns=dict(base.__dict__);exec(original_source,ns);rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch)
        assert all(c['after_esp']==base.SP+4 for c in rows)
        full_rows=list(rows);prefix_rows=[];excluded_rmw=[]
        # Each real memory operand at its actual occurrence, rather than an
        # arbitrary missing bank, discriminates exact partial-FP/source order.
        seen=set()
        for original in (next(c for c in rows if c['name']==name)for name in
                         ('contact_2_0.25_1.0','contact_2_2.0_0.05','growth_2')):
            for kind in ('reads','writes'):
                occurrences={}
                for event in original[kind]:
                    p=event['address'];width=event.get('bytes',len(event.get('written',''))//2)
                    pair=(p,width);occurrences[pair]=occurrences.get(pair,0)+1
                    if not 0x30cf40<=event['instruction']<=0x30d190:continue
                    signature=(event['instruction'],kind)
                    if signature in seen:continue
                    seen.add(signature)
                    if kind=='writes'and event['instruction']==0x30d17b:
                        excluded_rmw.append(dict(instruction=event['instruction'],reason='Preinstruction oracle cannot model distinct RMW read-completed/write-fault phase.'));continue
                    row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')}
                    row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=p,bytes=width,occurrence=occurrences[pair],write=kind=='writes'))
                    prefix_rows.append(row)
        for original in (next(c for c in rows if c['name']==name)for name in ('contact_2_0.25_1.0','contact_2_2.0_0.05')):
            for event in original['writes']:
                pc=event['instruction'];signature=('observer',pc)
                if not 0x30cf40<=pc<=0x30d190 or signature in seen:continue
                seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')}
                row.update(name=f'observer_{pc:x}',observer_stop=dict(instruction=pc,occurrence=1));prefix_rows.append(row)
        prefix_root=scratch/'prefix';prefix_root.mkdir();shutil.copyfile(scratch/'observer.cpp',prefix_root/'observer.cpp')
        prefix_fixture,_=ns['original'](image,banks,prefix_rows,prefix_root)
        for row in prefix_rows:
            for bank in row['banks']:
                for key in ('before_file','after_file'):bank[key]=str(prefix_fixture/bank[key])
        rows+=prefix_rows
        (fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
        driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()
        driver='#include "halo2/native_tu15_contact_distance_publication.hpp"\n'+driver
        driver=driver.replace('  void write(std::uint32_t p,','  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}\n  void write(std::uint32_t p,')
        driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;',
            '      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;\n      if(read_pass&&c.contains("observer_stop"))continue;')
        driver=driver.replace('++index;}};', '++index;}if(c.contains("observer_stop")&&event.instruction==c.at("observer_stop").at("instruction"))throw Fault{};};')
        begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
        driver=driver[:begin]+'''      NativeTu15ContactDistanceServices services;services.allocation=growth;
      services.remove_point=[&](auto manager,auto owner,auto point,auto&){check(owner.identity==0x2e4d60u,"Unqualified contact removal owner");};
      NativeTu15ContactDistanceStats stats;const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
      try{native_tu15_publish_contact_distances(memory,{args[0]},{args[1]},{args[2]},args[3],args[4],args[5],{args[6]},args[7],fp,0,services,observe,&stats);
'''+driver[end:]
        (project/'driver.cpp').write_text(driver)
        source=(project/UNITS[0]).read_text();variants={}
        for name,needle,replacement in (('wrong_comparison','bool keep=odd_compare(bound);','bool keep=!odd_compare(bound);'),
                                       ('wrong_blend','const auto weight=m.word(record-4u);','const auto weight=m.word(record);'),
                                       ('wrong_count','m.word(0x30cfa6u,header+4u,needed);','m.word(0x30cfa6u,header+4u,needed+1u);')):
            assert source.count(needle)==1;variants[name]=source.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
        (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Unchanged original rejected native mutation")\n')
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(contact_distance LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp '+' '.join(UNITS[1:])+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
        for mode in ('native',*variants):
            unit=UNITS[0]if mode=='native'else mode+'.cpp';target='distance_'+mode
            cmake+=f'add_executable({target} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
            command=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
            cmake+=f'add_test(NAME native_contact_distance_{mode} COMMAND {command}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
        (project/'CMakeLists.txt').write_text(cmake)
        results=[]
        for compiler in ('g++','clang++'):
            cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),*sum((['--target','distance_'+mode]for mode in ('native',*variants)),[]),'--filter','^native_contact_distance_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1']
            r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native contact distance differs'
        parent_sites={pc for row in full_rows for pc in row['executed_sites']if 0x30cf40<=pc<=0x30d190}
        report.update(passed=True,profiles=len(rows),full_returns=len(full_rows),read_write_fault_prefixes=len(prefix_rows),unqualified_rmw_write_faults=excluded_rmw,
            parent_sites_covered=len(parent_sites),negative_controls=list(variants),source_authority=authority,input_provenance=inputs,original_sha256=NEW_PIN,
            ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,
            source_hashes={str(p.relative_to(ROOT)):sha(p)for p in (ROOT/'include/halo2/native_tu15_contact_distance_publication.hpp',*[ROOT/'src/physics'/n for n in UNITS],Path(__file__).resolve())})
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')

if __name__=='__main__':main()
