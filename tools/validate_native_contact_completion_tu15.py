#!/usr/bin/env python3
"""Qualify contact completion and notification cohorts against unchanged physical TU1.5.

Real default callbacks execute; custom owners stop at their genuine entry.
No listener result, memory postimage or guest register result is supplied.
Compiler products and operand banks belong to run_test.py scratch.
"""
import argparse, copy, json, os, shutil, struct, subprocess, sys
from pathlib import Path
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,WORLD,MANAGER,FIRST,SECOND=0x1000000,0x1000100,0x1000500,0x1000800,0x1000a00
META1,META2,HEADER,OUTPUT,RECORDS,MAPPING,DEST=BASE+0xc00,BASE+0xe00,BASE+0x1000,BASE+0x1020,BASE+0x1100,BASE+0x1800,BASE+0x2000
ARRAY,LISTENER,TABLE=BASE+0x2800,BASE+0x3000,BASE+0x3500
ENTRIES={0x3230c0:74,0x322560:47,0x31ff50:47,0x2e4d60:1}
def metadata(image):
 code=reference._Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];hashes={}
 for entry,count in ENTRIES.items():
  body=reference.complete_body(code,entry);assert len(body.code)==count;listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    b,x=i.reg_name(op.mem.base),i.reg_name(op.mem.index)
    ops.append(dict(base=names.get(b,-1),index=names.get(x,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  hashes[hex(entry)]=reference.hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert image.read(0x2e4d60,3)==bytes.fromhex('c20400')
 return dict(sites=sites,body_sites=list(ENTRIES.values()),listing_sha256=hashes,stop=reference.STOP)
def cases(image):
 rows=[]
 def make(entry=0x3230c0,count=2,countdown=1,patterns=((0,1,0,1),(1,0,1),(0,1)),periods=(0,3),alias=False,custom=False,seed=False):
  raw=bytearray((n*19+7)&255 for n in range(0x4000))
  def put(p,v,bytes=4):raw[p-BASE:p-BASE+bytes]=(v&((1<<(bytes*8))-1)).to_bytes(bytes,'little')
  put(OUTPUT,HEADER);put(HEADER,RECORDS);put(HEADER+4,count);put(MANAGER+8,WORLD);put(MANAGER+0xc,countdown);put(MANAGER+0x30,MAPPING);put(MANAGER+0x44,RECORDS+16 if alias else DEST)
  put(FIRST+0x20,META1);put(SECOND+0x20,META2);put(META1+0x5c,periods[0],2);put(META2+0x5c,periods[1],2)
  for n in range(5):put(RECORDS+n*48+0x20,3+n,2);put(MAPPING+3+n,n%3,1)
  put(TABLE+8,0x316790 if custom else 0x2e4d60)
  for subject,ao,co,pattern,offset in ((WORLD,0xb4,0xb8,patterns[0],0),(META1,0x60,0x64,patterns[1],0x100),(META2,0x60,0x64,patterns[2],0x200)):
   put(subject+ao,ARRAY+offset);put(subject+co,len(pattern))
   for n,kind in enumerate(pattern):put(ARRAY+offset+n*4,LISTENER+offset+n*0x10 if kind else 0);put(LISTENER+offset+n*0x10,TABLE)
  n=len(rows);depth=(0,1,3,8)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=MANAGER;gp[4]=reference.SP
  packet=reference.SP-0x14 if entry==0x3230c0 else BASE+0x3800
  arguments=[FIRST,SECOND,BASE+0x700,OUTPUT]if entry==0x3230c0 else[WORLD if entry==0x322560 else META1,packet]
  banks=[dict(address=BASE,before=raw.hex())]
  if entry==0x3230c0:banks.append(dict(address=packet,before=bytes(range(20)).hex()))
  row=dict(id=n,name=('fault_seed_'if seed else'complete_')+f'{entry:x}_{n}',entry=entry,world=WORLD,island=0,body=META1,manager=MANAGER,first=FIRST,second=SECOND,output=OUTPUT,packet=packet,arguments=arguments,gp=gp,before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)|((0,0x40,0x8000,0x8040)[n%4])],live_x87=live,banks=banks,alignment_fault=False)
  if custom:row['required_owner']=dict(owner=0x316790,argument_count=1)
  rows.append(row)
 patterns=((),(0,),(1,),(0,1,0,1),(1,0,1,0),(1,1,1))
 for repeat in range(4):
  for count in(0,1,3,0xffffffff,0x80000001):
   for countdown in(0,1,3):make(count=count,countdown=countdown,periods=((0,0),(0xffff,7),(13,0xffff),(0x8000,0x8001))[repeat])
 for n,p in enumerate(patterns):
  make(patterns=(p,patterns[(n+2)%6],patterns[(n+4)%6]))
  for entry in(0x322560,0x31ff50):make(entry=entry,patterns=(p,p,p))
 for count in(1,3):make(count=count,countdown=2,alias=True)
 for entry in(0x3230c0,0x322560,0x31ff50):make(entry=entry,patterns=((1,),(1,),(1,)),custom=True)
 for entry in(0x3230c0,0x322560,0x31ff50):make(entry=entry,seed=True)
 return rows
def driver():
 text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({74,47,47,1})')
 begin=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',begin)
 text=text[:begin]+'''NativeTu15ContactCompletionStats stats;NativeTu15ContactCompletionServices services;J required=nullptr;
   services.notify=[&](auto listener,auto owner,auto packet,auto&){required=J{{"owner",owner.identity},{"receiver",listener.identity},{"arguments",J::array({packet.identity})}};throw OwnerStop{};};
'''+text[end:]
 begin=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',begin)
 text=text[:begin]+'''switch(entry){
    case 0x3230c0:native_tu15_complete_contact_manager(m,{test.at("manager")},{test.at("first")},{test.at("second")},{test.at("output")},{test.at("packet")},fp,services,observer,&stats);break;
    case 0x322560:native_tu15_notify_world_contact_completion(m,world,{test.at("packet")},fp,services,observer,&stats);break;
    case 0x31ff50:native_tu15_notify_body_contact_completion(m,body,{test.at("packet")},fp,services,observer,&stats);break;
    default:throw std::runtime_error("Unexpected actual completion entry");}
'''+text[end:]
 start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start);text=text[:start]+text[end:]
 return text.replace('activation','completion')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 source=ROOT/'src/physics/native_tu15_contact_completion.cpp';files=[source,ROOT/'include/halo2/native_tu15_contact_completion.hpp',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=activation.prepare(image,meta,scratch);rows=reference.physical(owned,cases(image),'complete');faults=[];excluded=[]
  for seed in [c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     if phase=='before_write'and event['instruction']==0x323107:excluded.append(dict(seed=seed['name'],instruction=event['instruction'],reason='Atomic original RMW prewrite stops before its input read; separate native data operations cannot expose the same prefix.'));continue
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'completion.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_contact_completion.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in (('wrong_second_vector','m.vector(record+16u)','m.vector(record)'),('wrong_countdown','m.word(manager.identity+0xcu)-1u','m.word(manager.identity+0xcu)+1u'),('wrong_copy','m.word(current+cursor*4u+4u)','m.word(current+cursor*4u)')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects mutation:",r.stderr.strip()[:300])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeContactCompletion LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\n'
  for name in ('native',*variants):
   cmake+=f'add_executable(completion_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(completion_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(completion_{name} PRIVATE cxx_std_20)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:completion_{name}> "{fixture}"';cmake+=f'add_test(NAME completion_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','completion_native',*sum((['--target','completion_'+n]for n in variants),[]),'--filter','^completion_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(reference.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),required_owner_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_snapshot_unchanged=unchanged,negative_controls=list(variants),image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],excluded_rmw_prewrite_observer_prefixes=excluded,complete_fault_contract_qualified=False,remaining=['Genuine custom callback return composition and mutable callback cohorts','Full visible multiplayer binding','RMW prewrite atomicity and unmasked platform exceptions'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
