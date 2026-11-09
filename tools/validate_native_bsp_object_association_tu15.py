#!/usr/bin/env python3
"""Real i386 TU1.5 BSP/object association graph, with unavailable native locals."""
import argparse,copy,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_native_tu15_constraint_before_reference import sha,save

STAGE=ROOT/'analysis/physics/native-bsp-object-association-stage-v2-20261102/staged'
PREFIX='native_tu15_bsp_object_association'
SCENARIO=base.UI+0x1000
POOL=base.UI+0x2000
ROWS=base.UI+0x2800
OBJECTS=base.UI+0x3000
HEADER=base.UI+0x5000
RECORDS=base.UI+0x5100
MASK=base.UI+0x6000
SCENERY=base.UI+0x7000
ITERATOR=base.UI+0x8000
OWNERS=(0x2190c0,0x219150,0x2191d0,0x2190f0,0xbcee0)

def cases():
    rows=[]
    def create(name,entry=0x2190c0,count=4,records=3,scenery=36,**more):
        words={POOL+0x38:count,POOL+0x44:ROWS,SCENARIO+0x220:HEADER,
          HEADER:records,HEADER+4:RECORDS,SCENARIO+0x5c:scenery,SCENARIO+0x60:SCENERY,
          ITERATOR:0x80,ITERATOR+4:0,ITERATOR+8:0xffffffff,ITERATOR+12:0x86868686}
        # Each row describes an ordinary authored object, not a callee outcome.
        for i in range(8):
            obj=OBJECTS+i*0x300
            words.update({ROWS+i*12:((7 if i!=1 else 3)<<24)|(i+1),ROWS+i*12+8:obj,
              obj+0xa4:0x12340000+(i%3),obj+0xa8:((i%2)<<24)|((i%3)<<16)|i,
              obj+0x1d8:0x12345678,obj+0x1dc:0xdeadbeef})
        for i in range(4):
            words.update({RECORDS+i*12:0x12340000+(i%3),
              RECORDS+i*12+4:((i%2)<<24)|((i%3)<<16)|i,
              RECORDS+i*12+8:((0x20+i)<<16)|(0x10+i)})
        for i in range(66):words[SCENERY+i*0x24+0x18]=8 if i%3==0 else 0
        words.update(more.pop('words',{}))
        row=dict(name=name,entry=entry,receiver=0,arguments=[ITERATOR]if entry==0xbcee0 else [],
          user_words=[(p-base.UI,v&0xffffffff)for p,v in sorted(words.items())],**more)
        rows.append(row);return row
    for entry in OWNERS:
        for count in (0,1,4,8):
            create(f'base_{entry:x}_{count}',entry,count=count)
    for records in (0,0xffffffff,1,2,4):
        for scenery in (0,1,31,32,33,65):create(f'counts_{records:x}_{scenery}',records=records,scenery=scenery)
    for requested,mask,kind in ((2,0x80,7),(1,0x80,7),(0,0,7),(0,1,32),(0,0x80000000,31)):
        create(f'iterator_filter_{requested}_{mask}_{kind}',0xbcee0,count=1,
          words={ITERATOR+4:requested,ITERATOR:mask,ROWS:(kind<<24)|(2<<16)|0x8001})
    for cursor,count in ((0xffff,0),(0x8000,0xffff8000),(4,4),(5,4),(0x7fff,0x7fff)):
        create(f'iterator_bound_{cursor:x}_{count:x}',0xbcee0,count=count,
          words={ITERATOR+4:cursor<<16})
    create('iterator_empty_generation',0xbcee0,words={ROWS:0,ROWS+12:0,ROWS+24:0,ROWS+36:0})
    create('iterator_null_object',0xbcee0,words={ROWS+8:0})
    create('association_no_match',words={RECORDS:0x99999999,RECORDS+12:0x99999999,RECORDS+24:0x99999999})
    create('association_part_mismatch',words={RECORDS+4:0x9999})
    create('association_variant_bypasses_part',words={ROWS+24:0x07000003,OBJECTS+0x600+0xa8:0x010200ff,RECORDS:0x12340002,RECORDS+4:0x010200aa})
    # Exact after-store mutations verify cached/reloaded original DATA choices.
    def mutation(pc,p,v):return dict(instruction=pc,address=p,written=struct.pack('<I',v).hex())
    create('reread_authored_record_count',mutations=[mutation(0x2192bc,HEADER,1)])
    create('reread_scenery_count',mutations=[mutation(0x219137,SCENARIO+0x5c,1)])
    create('reread_object_rows',0x219150,mutations=[mutation(0x2191ac,POOL+0x44,ROWS+12)])
    create('reread_authored_record_pointer',mutations=[mutation(0x2192bc,HEADER+4,RECORDS+12)])
    create('rep_iteration_mutation',mutations=[mutation(0x2190cf,MASK+4,0x12345678)])
    alias=create('iterator_result_read_after_alias',0xbcee0,count=1,
      words={ROWS:0x07000081,ROWS+4:0})
    alias['arguments']=[ROWS]
    for name,p,n,occurrence,writing in (
      ('global_mask',0x505e7c,4,1,False),('global_pool',0x4ecbd0,4,1,False),
      ('limit',POOL+0x38,4,1,False),('row',ROWS,2,1,False),
      ('definition',OBJECTS+0xa4,4,1,False),('record_bsp',RECORDS+8,2,1,False),
      ('record_count',HEADER,4,2,False),('object_reset',OBJECTS+0x1da,2,1,True),
      ('object_assignment',OBJECTS+0x1da,2,2,True),('mask_fill',MASK,4,1,True),
      ('mask_exclude',MASK,4,2,True),('scenario_rows',SCENARIO+0x60,4,1,False)):
        create('fault_'+name,fault=dict(address=p,bytes=n,occurrence=occurrence,write=writing))
    return rows

def driver_text():
    text='#include "halo2/'+PREFIX+'.hpp"\n#include "halo2/native_tu15_retained_store_observation.hpp"\n'+(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()
    text=text.replace('struct Memory:SourcePhysicsContactMemory {','''struct Memory:SourcePhysicsContactMemory,NativeTu15RetainedStoreObservationDomain {
  mutable std::array<bool,64>defined{};
  static bool private_data(std::uint32_t p,std::size_t n){return p>=0x74000000u&&std::uint64_t(p)+n<=0x74000040ull;}
  bool observes_retained_store(std::uint32_t,std::uint32_t p,std::size_t n)const override{return !private_data(p,n);}
''')
    text=text.replace('if(log_reads)reads.push_back', '''if(private_data(p,out.size()))for(std::size_t k=0;k<out.size();++k)check(defined[p-0x74000000u+k],"Unprovided private iterator DATA");if(log_reads&&!private_data(p,out.size()))reads.push_back''')
    text=text.replace('const auto i=at(p,in.size());std::copy', '''if(private_data(p,in.size()))for(std::size_t k=0;k<in.size();++k)defined[p-0x74000000u+k]=true;const auto i=at(p,in.size());std::copy''')
    text=text.replace('memory.fault=c.value("fault",J{});','memory.banks[0x74000000u]=std::vector<std::byte>(64,std::byte(0xa5));memory.fault=c.value("fault",J{});')
    begin=text.index('      NativeTu15ArrayGrowthMemory array');end=text.index('      const auto host_sw=',begin)
    text=text[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+text[end:]
    text=text.replace('NativeTu15ArrayGrowthActivity activity;','')
    begin=text.index('      try{const auto entry=');end=text.index('      }catch(const Fault&)',begin)
    text=text[:begin]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>();
        const NativeTu15BspObjectAssociationScratch scratch{{0x74000000u},{0x74000020u}};
        if(entry==0xbcee0u)result=native_tu15_next_scanned_object(memory,{c.at("arguments")[0]},observe);
        else if(entry==0x219150u)native_tu15_reset_bsp_object_associations(memory,scratch.reset_scan,observe);
        else if(entry==0x2191d0u)native_tu15_apply_bsp_object_associations(memory,scratch,observe);
        else if(entry==0x2190f0u)native_tu15_exclude_noncollidable_scenery(memory,observe);
        else native_tu15_initialize_bsp_object_associations(memory,scratch,observe);
'''+text[end:]
    marker='      check(calls==c.at("allocator_calls"),'
    text=text.replace(marker,'''      if(!failed&&c.at("entry")==0xbcee0u)check(result==c.at("return_word"),name+" object DATA identity differs");
'''+marker)
    return text

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    files=[STAGE/'include/halo2'/f'{PREFIX}.hpp',STAGE/'src/physics'/f'{PREFIX}.cpp',STAGE/'include/halo2/native_tu15_retained_store_observation.hpp',Path(__file__),ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp']
    pins={str(p.relative_to(ROOT)):sha(p)for p in files}
    report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,source_outcomes_supplied=False,
      original_control_state_supplied_to_native=False,live_integration_claimed=False,retained_large_artifacts=[])
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=OWNERS
        banks={p:struct.pack('<I',v)for p,v in ((0x4ecbd0,POOL),(0x4ecc18,SCENARIO),(0x505e7c,MASK))}
        metadata=base.metadata
        def exact_metadata(*args):
            result,authority=metadata(*args)
            # Capstone reports no access for TEST [ebp],ebx. The actual TEST
            # instruction consumes that DWORD; keep this read in the oracle.
            site=next(s for s in result['sites']if s['instruction']==0xbcf2e)
            assert len(site['operands'])==1 and site['operands'][0]['bytes']==4
            site['operands'][0]['read']=True
            return result,authority
        base.metadata=exact_metadata
        rows=cases();fixture,authority=base.original(image,banks,rows,scratch)
        assert all(r['declared_fault']for r in rows if 'fault'in r),'Declared operand fault was not reached'
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for p in files[:3]:shutil.copyfile(p,project/'include/halo2'/p.name)if p.suffix=='.hpp'else shutil.copyfile(p,project/'native.cpp')
        (project/'driver.cpp').write_text(driver_text())
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(bspObjectAssociations LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(bsp driver.cpp native.cpp)\ntarget_include_directories(bsp PRIVATE include)\ntarget_compile_features(bsp PRIVATE cxx_std_20)\ntarget_link_libraries(bsp PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME bsp_associations COMMAND bsp "'+str(fixture/'original.json')+'")\n')
        def run(label,compiler):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/label),'--target','bsp','--filter','^bsp_associations$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1']
            result=subprocess.run(command);report.setdefault('results',[]).append(dict(label=label,command=command,exit_code=result.returncode));return result.returncode
        for compiler in ('g++','clang++'):assert run(compiler,compiler)==0,'Native BSP/object graph differs'
        original=(project/'native.cpp').read_text()
        mutants=(('mask_width','n<16u','n<15u'),('association_part','matches=part==actual','matches=part!=actual'),('iterator_kind','kind&31u','kind&15u'))
        for label,before,after in mutants:
            assert original.count(before)==1;(project/'native.cpp').write_text(original.replace(before,after))
            for compiler in ('g++','clang++'):
                name='negative-'+label+'-'+compiler
                assert run(name,compiler)!=0,'Original failed to reject '+label
                log=(args.output/name/'tests.log').read_text()
                assert any(reason in log for reason in ('array store differs','complete array/pool bank differs','object DATA identity differs')),'Mutation did not fail on actual DATA/result: '+label
            (project/'native.cpp').write_text(original)
        assert all(sha(ROOT/p)==v for p,v in pins.items()),'Qualified source changed'
        code=_Code(image,'.text');executed={p for c in rows for p in c['executed_sites']}
        report.update(passed=True,original_sha256=NEW_PIN,authority=authority,profiles=len(rows),
          fault_prefixes=sum(r['declared_fault']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),
          ordered_stores=sum(len(r['writes'])for r in rows),body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in OWNERS},
          negative_controls=[m[0]for m in mutants],private_iterator_bytes_initially_unavailable=True,
          authenticated_metadata_access_corrections=['BCF2E TEST DWORD [ebp],ebx reads its memory operand; Capstone access=0'],
          scope='Selected full native five-function BSP/object association graph; retained DATA/read/store/SourcePC/order/FP/result and fault prefixes. Source stack/control preimages absent. Host integration remains separate.')
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
        save(args.output/'report.json',report)
    return 0
if __name__=='__main__':raise SystemExit(main())
