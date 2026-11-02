#!/usr/bin/env python3
"""Source73+29+26+32 spatial owners, aliased packets/copies and host pipeline."""
import argparse,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from build_native_tu15_constraint_before_reference import sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1];U=base.UI
MANAGER=U+0x100;HEADERS=U+0x1000;OBJECT=U+0x2000;OBJECT2=U+0x3000;ROOTOBJ=U+0x400;INNER=U+0x600;OUTER=U+0x700;HEADS=U+0x5000;OUTROWS=U+0x7000;INROWS=U+0x8000;SOURCE=U+0x9000;OUTPUT=U+0xa000;KEY=0xaabbccdd
def common():
 words=[(0x144,HEADERS),(0x1008,OBJECT),(0x1014,OBJECT2),(0x2004,0x200),(0x20a8,0x020b0000),(0x212c,2),
  (0x400,HEADS),(0x404,INNER),(0x408,OUTER),(0x624,64),(0x644,INROWS),(0x724,16),(0x744,OUTROWS),
  (0x5000,0xcafe0000),(0x5004,0xbeef0002),(0x7000,KEY),(0x7004,0),(0x7008,0xabcd0001),(0x7014,1),(0x7018,0xffffffff),
  (0x8004,KEY^1),(0x8008,0xabcd0001),(0x8044,KEY),(0x8048,0xffffffff),(0x8084,KEY),(0x8088,0xffffffff)]
 words.extend((0x2040+i*4,v)for i,v in enumerate((0x3f800000,0x80000000,0x7fc12345,0x40000000)))
 words.extend((0x9000+i*4,(0x12345678+i*0x11111111)&0xffffffff)for i in range(16))
 return words
def cases():
 rows=[]
 def add(name,entry,*,eax=0,edx=0,edi=0,ebx=0,ecx=0,args=(),words=(),**extra):
  row=dict(name=name,entry=entry,receiver=ecx,incoming_eax=eax,incoming_edx=edx,incoming_edi=edi,incoming_ebx=ebx,arguments=list(args),user_words=[*common(),*words],**extra);rows.append(row);return row
 for typ in(0,11,1,255):
  for flags in(0,1,1<<20,1<<22,1<<31,0x80500201):
   for header in(0,0x10):
    for detail in(0,1):
     add(f'flags_{typ}_{flags}_{header}_{detail}',0x16e290,eax=0xabcd0000,words=[(0x1000,header<<16),(0x2004,flags),(0x20a8,(typ|0x200)<<16),(0x2108,detail<<18),(0x2348,detail<<3),(0x212c,detail<<1)])
 for typ in(0,11,128,255):
  for flags in(0,0x201):
   for offset in(0xa000,0x2040,0x203c,0x20a8,0x2002,0x1006):
    add(f'packet_{typ}_{flags}_{offset}',0xba910,edx=0x12340000,edi=U+offset,words=[(0x2004,flags),(0x20a8,(typ|0x200)<<16),(0x2108,4<<16),(0x2348,8),(0x212c,2)])
 for count in(0,1,2,3,4,5,7,8,19,20,21,23,36):
  for source in(SOURCE,SOURCE+1,INROWS+12,INROWS+13,INROWS+8):
   add(f'record_{count}_{source}',0x29fff0,eax=HEADS,edx=INNER,ecx=KEY,args=(count,0xffffffff if count==0 else source))
 for kind,head,key in(('empty',0xffffffff,KEY),('first',0,KEY^1),('missing',0,KEY^2)):
  add('record_'+kind,0x29fff0,eax=HEADS,edx=INNER,ecx=key,args=(20,SOURCE),words=[(0x5000,head)])
 for count in(0,1,3,4,7,19,20,21):
  add('chain_'+str(count),0x1d30a0,eax=0x12340000,edi=ROOTOBJ,ebx=SOURCE,args=(KEY,count))
 add('chain_empty',0x1d30a0,eax=0xffffffff,edi=0xffffffff,ebx=0xffffffff,args=(KEY,20))
 add('chain_signed_slot',0x1d30a0,eax=0,edi=ROOTOBJ,ebx=SOURCE,args=(KEY,20),words=[(0x7004,0xffff),(0x4ffc,2)])
 # Destination overlaps the outer link: the caller must reread it after child.
 add('chain_changed_outer_link',0x1d30a0,eax=0,edi=ROOTOBJ,ebx=SOURCE,args=(KEY,20),words=[(0x644,OUTROWS-4),(0x5000,0),(0x9000,0xffffffff)])
 # The key is a true DATA formal reread every outer row. Mutate only that cell.
 add('chain_key_argument_reread',0x1d30a0,eax=0,edi=ROOTOBJ,ebx=SOURCE,args=(KEY,20),retained_arguments=True,
  mutations=[dict(instruction=0x2a0030,address=base.SP+4,written=struct.pack('<I',KEY^2).hex())])
 for typ in(0,11,128,255):
  for flags in(0,0x201):
   add(f'host_pipeline_{typ}_{flags}',0xba910,edx=0x12340000,edi=base.SP-0x100,args=(),words=[(0x2004,flags),(0x20a8,(typ|0x200)<<16)],
    pipeline=dict(root=ROOTOBJ,datum=0x12340000,key=KEY,count=20,source=base.SP-0x100),host_pipeline=True)
 for name,entry,p,width,nth,writing in(
  ('flags_test_byte',0x16e290,OBJECT+4,1,1,False),('flags_late_word',0x16e290,OBJECT+4,4,1,False),
  ('packet_flags',0xba910,OUTPUT+2,2,1,True),('packet_type',0xba910,OBJECT+0xaa,1,2,False),
  ('record_word',0x29fff0,SOURCE+4,4,1,False),('record_byte',0x29fff0,SOURCE+4,1,1,False),
  ('chain_link',0x1d30a0,OUTROWS+8,4,1,False)):
  if entry==0x16e290:add('fault_'+name,entry,eax=0,args=(),words=[(0x2004,0)],fault=dict(address=p,bytes=width,occurrence=nth,write=writing))
  elif entry==0xba910:add('fault_'+name,entry,edx=0,edi=OUTPUT,fault=dict(address=p,bytes=width,occurrence=nth,write=writing))
  elif entry==0x29fff0:add('fault_'+name,entry,eax=HEADS,edx=INNER,ecx=KEY,args=(7,SOURCE),fault=dict(address=p,bytes=width,occurrence=nth,write=writing))
  else:add('fault_'+name,entry,eax=0,edi=ROOTOBJ,ebx=SOURCE,args=(KEY,20),fault=dict(address=p,bytes=width,occurrence=nth,write=writing))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['include/halo2/native_tu15_object_spatial_packet.hpp','src/physics/native_tu15_object_spatial_packet.cpp','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp',str(Path(__file__).relative_to(ROOT))];pins={n:sha(ROOT/n)for n in names};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x16e290,0xba910,0x1d30a0,0x29fff0)
  banks={0x4ecbd0:struct.pack('<I',MANAGER)};metadata=base.metadata
  def correct_metadata(image,banks):
   m,proof=metadata(image,banks);code=_Code(image,'.text')
   for site in m['sites']:
    i=code.at(site['instruction'])
    if i.mnemonic=='test':
     for op in site['operands']:op['read']=True
   return m,proof
  base.metadata=correct_metadata
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()
  observer=observer.replace('(pc==0x2e2e43u||pc==0x2e2e4au)','(pc==0x2e2e43u||pc==0x2e2e4au||pc==0x2a0030u||pc==0x2a0037u)')
  observer=observer.replace('if(pc==stop)break;','''if(pc==stop){if(m.contains("pipeline")&&!pipeline_done){pipeline_done=true;const auto&p=m.at("pipeline");std::array<std::uint32_t,3>args{std::uint32_t(stop),p.at("key"),p.at("count")};iovec local{args.data(),12},remote{reinterpret_cast<void*>(std::uintptr_t(pipeline_sp)),12};check(process_vm_writev(child.pid,&local,1,&remote,1,0)==12,"Pipeline formal DATA unavailable");regs.rip=0x1d30a0u;regs.rax=p.at("datum");regs.rdi=p.at("root");regs.rbx=p.at("source");regs.rsp=pipeline_sp;request(PTRACE_SETREGS,child.pid,nullptr,&regs);continue;}else break;}''')
  observer=observer.replace('  for(;count<100000;', '  const std::uint32_t pipeline_sp=0x200f000u;bool pipeline_done=false;\n  for(;count<100000;')
  observer=observer.replace('"allocator_calls",allocator_calls}', '"allocator_calls",allocator_calls},{"pipeline_packet",m.contains("pipeline")?hex(memory(child.pid,m.at("pipeline").at("source"),20)):std::string{}}')
  observer_path=scratch/'observer.cpp';observer_path.write_text(observer)
  original=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)')
  original=original.replace(" mov ${case['receiver']},%ecx", " mov ${case['incoming_eax']},%eax\n mov ${case['incoming_edx']},%edx\n mov ${case['incoming_edi']},%edi\n mov ${case['incoming_ebx']},%ebx\n mov ${case['receiver']},%ecx")
  original=original.replace("('fault','mutations')", "('fault','mutations','pipeline')")
  env=dict(base.__dict__,observer_path=observer_path);exec(original,env);rows=cases();fixture,authority=env['original'](image,banks,rows,scratch);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');kernel=(ROOT/'src/physics/native_tu15_object_spatial_packet.cpp').read_text();(project/'kernel.cpp').write_text(kernel)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_object_spatial_packet.hpp"\n#include "halo2/native_tu15_physics_frame.hpp"\n'+driver
  begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)
  driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15ObjectSpatialPacketStats activity;
      const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8u);return v;};
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  body='''      try{const auto entry=c.at("entry").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(c.value("host_pipeline",false)){
          const auto packet=native_tu15_build_object_spatial_packet(memory,c.at("incoming_edx"),&activity);check(hex(packet.bytes)==c.at("pipeline_packet").get<std::string>(),name+" computed host packet differs from original");
          const auto&p=c.at("pipeline");native_tu15_update_object_spatial_chain(memory,p.at("root"),p.at("datum"),p.at("key"),packet,observe,&activity);
        }else if(entry==0x16e290u)result=native_tu15_object_spatial_flags(memory,c.at("incoming_eax"),&activity);
        else if(entry==0xba910u)native_tu15_write_object_spatial_packet(memory,c.at("incoming_edx"),c.at("incoming_edi"),observe,&activity);
        else if(entry==0x29fff0u)result=native_tu15_update_spatial_record(memory,c.at("incoming_eax"),c.at("receiver"),c.at("incoming_edx"),{[&]{return args[0];},[&]{return args[1];}},observe,&activity);
        else if(entry==0x1d30a0u){
          const bool retained=c.value("retained_arguments",false);native_tu15_update_object_spatial_chain(memory,c.at("incoming_edi"),c.at("incoming_eax"),c.at("incoming_ebx"),{[&]{return retained?word(0x200f004u):args[0];},[&]{return retained?word(0x200f008u):args[1];}},observe,&activity);
        }else throw std::runtime_error("Unqualified spatial owner");
''';driver=driver[:begin]+body+driver[end:]
  driver=driver.replace('name+" actual allocator request order differs"','name+" unexpected allocator call"')
  anchor='      check(calls==';driver=driver.replace(anchor,'      if(!failed&&!required&&(c.at("entry")==0x16e290u||c.at("entry")==0x29fff0u))check(result==c.at("return_word"),name+" typed flags/record result differs");\n'+anchor)
  (project/'driver.cpp').write_text(driver)
  (project/'wrong_flag.cpp').write_text(kernel.replace('flags&(1u<<20u)','flags&(1u<<19u)'))
  (project/'wrong_tail.cpp').write_text(kernel.replace('(bytes&3u)','(bytes&1u)'))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);assert r.returncode!=0 and any(s in r.stderr for s in("differs","completion")),(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeSpatial LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for variant in('kernel','wrong_flag','wrong_tail'):
   cmake+=f'add_executable(spatial_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp)\ntarget_include_directories(spatial_{variant} PRIVATE include)\ntarget_compile_features(spatial_{variant} PRIVATE cxx_std_20)\n';negative=''if variant=='kernel'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME spatial_{variant} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:spatial_{variant}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','spatial_kernel','--target','spatial_wrong_flag','--target','spatial_wrong_tail','--filter','^spatial_','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  code=_Code(image,'.text');executed={p for r in rows for p in r['executed_sites']};report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==v for f,v in pins.items()),compiler_results=results,original_cases=len(rows),host_pipeline_cases=sum(r.get('host_pipeline',False)for r in rows),fault_prefixes=sum(r['declared_fault']for r in rows),body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in base.OWNERS},ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),source_authority=authority,
   scope='Whole4sourceowners: typedflags,aliasfaithful20Bpacket,forwardDWORD/bytecopy,latekey/rereadchain. Declared validDATA; realoriginalBA910 then1D30A0 computesprivatepacketindependently fordisjointhostPODpipeline. Allpersistentbanks/orderedreads/stores/FP,typedflags/recordreturns; no GP/postimages supplied. NativeDF0 and zeroECX REP semantics.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
