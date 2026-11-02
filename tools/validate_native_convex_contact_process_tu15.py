#!/usr/bin/env python3
"""Native2F8B60 fast fullreturns and genuine simplex-entry boundaries."""
import argparse,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
U=base.UI;AGENT,FIRST,SECOND,SHAPE_A,SHAPE_B,MOTION_A,MOTION_B,CONTEXT,MANAGER,TABLE=map(lambda x:U+x,(0x100,0x400,0x500,0x800,0x900,0x1000,0x1100,0x2000,0x2200,0x2300))
OWNERS=(0x2f8b60,0x2e4550,0x30ca60,0x2e4d60)
UNITS=('native_tu15_convex_contact_process.cpp','native_tu15_convex_contact_query.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp','native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp')
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def cases():
 rows=[]
 for distance in(3.,10.,1.,0.,-1.):
  for delta in(-.25,0.,2.):
   for count in(0,1,3):
    words=[(AGENT-U,0x41cd08),(AGENT-U+8,MANAGER),(AGENT-U+0xc,0),(AGENT-U+0x10,0xffffffff),(AGENT-U+0x14,0x02020101),(AGENT-U+0x18,count),(AGENT-U+0x20,bits(1.)),(AGENT-U+0x24,0),(AGENT-U+0x28,0),(AGENT-U+0x2c,bits(distance)),
     (FIRST-U,SHAPE_A),(FIRST-U+8,MOTION_A),(SECOND-U,SHAPE_B),(SECOND-U+8,MOTION_B),(SHAPE_A-U,0x4613a0),(SHAPE_B-U,0x4613a0),(SHAPE_A-U+0xc,bits(.5)),(SHAPE_B-U+0xc,bits(.5)),(CONTEXT-U+8,bits(.01)),(CONTEXT-U+0xc,U+0x2100),(0x2100,0),(MANAGER-U,TABLE),(TABLE-U+8,0x2e4d60)]
    for k in range(count):words.append((AGENT-U+0x30+k*48+0x1c,0xabc00000+k))
    for p in(MOTION_A,MOTION_B):
     for offset,value in enumerate((0.,0.,0.,.5,delta if p==MOTION_B else 0.,0.,0.,.5)):
      words.append((p-U+offset*4,bits(value)))
     for row in range(4):
      for column in range(4):words.append((p-U+0x20+row*16+column*4,bits(float(row==column))))
    prefix=distance<=0 or distance-(1.+delta)<=0
    item=dict(name='process_'+str(distance)+'_'+str(delta)+'_'+str(count),entry=0x2f8b60,receiver=AGENT,arguments=[FIRST,SECOND,CONTEXT,U+0x3000],user_words=words)
    if prefix:item.update(stop_boundary=0x30e930,required_owner=True)
    rows.append(item)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,production_native_only_closure=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=OWNERS
  source=inspect.getsource(base.metadata).replace('read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)','read=(ins.mnemonic not in("fst","fstp"))and(bool(op.access&CS_AC_READ)or ins.mnemonic in("test","cmp")),write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in("fst","fstp")');namespace=dict(base.__dict__);exec(source,namespace);base.metadata=namespace['metadata']
  banks={0x48c974:bytes(8),0x46a0e8:image.read(0x46a0e8,4),0x46a180:image.read(0x46a180,4),0x46a190:image.read(0x46a190,4)}
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();observer=observer.replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
  (scratch/'observer.cpp').write_text(observer);source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace('hex(STOP)]','hex(case.get("stop_boundary",STOP))]');environment=dict(base.__dict__);exec(source,environment);rows=cases();fixture,authority=environment['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+20 for c in rows if not c.get('required_owner'));(fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in UNITS:shutil.copyfile(ROOT/'src/physics'/name,project/name)
  shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_convex_contact_process.hpp"\n#include "halo2/native_tu15_phantom_pair_delete.hpp"\n'+driver
  driver=driver.replace('struct Fault{};struct Required{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
  driver=driver.replace('  void write(std::uint32_t p,','  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}\n  void write(std::uint32_t p,')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back').replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,std::byte(0xa5)));J actual_stores=J::array(),calls=J::array();').replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      NativeTu15ConvexProcessServices services;NativeTu15ConvexProcessStats stats;
      services.geometry.solve_simplex=[&](auto,auto,auto,auto,auto,auto,auto&)->std::uint32_t{throw Required{};};
      services.cleanup.remove_point=[&](auto,auto owner,auto,auto&){check(owner.identity==0x2e4d60u,"Unqualified removal owner");};
      try{native_tu15_process_convex_contact_agent(memory,{c.at("receiver")},{c.at("arguments")[0]},{c.at("arguments")[1]},{c.at("arguments")[2]},{c.at("arguments")[3]},{PRIVATE,PRIVATE+0x300u},fp,0,services,observe,&stats);
'''+driver[end:]
  (project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(convex_process LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(process driver.cpp '+' '.join(UNITS)+')\ntarget_include_directories(process PRIVATE include)\ntarget_compile_features(process PRIVATE cxx_std_20)\ntarget_compile_options(process PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(process PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME native_convex_process COMMAND process "'+str(fixture/'original.json')+'")\n');results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','process','--filter','^native_convex_process$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native convex process differs'
  report.update(passed=True,original_sha256=NEW_PIN,source_authority=authority,profiles=len(rows),full_returns=sum(not c.get('required_owner',False)for c in rows),genuine_solver_prefixes=sum(c.get('required_owner',False)for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,source_hashes={n:sha(ROOT/'src/physics'/n)for n in UNITS},scope=__doc__+' Declaredcoherent motion/shape/cacheDATA andactualRET4 noopchild. Alloriginalbanks/strictoperands/stores/preimages/FP, SourceRET16 checked. Notfullslowparent/publication/fault/production/livequalification.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
