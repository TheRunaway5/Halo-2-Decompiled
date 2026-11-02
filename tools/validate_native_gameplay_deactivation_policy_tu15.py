#!/usr/bin/env python3
"""Physical TU1.5 gameplay/body deactivation composition; owned scratch only."""
import argparse,copy,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_contact_entity_notification_tu15 as notify
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as ref
ROOT=ref.ROOT
BASE,POLICY,BODY,MOTION,OUTPUT=0x1000000,0x1000100,0x1000300,0x1000500,0x1000900
UNIT,POOL,ROWS,COMP,COMP_ROWS,INST,TAGS,TAG,GLOBALS=[BASE+x for x in(0x1000,0x1800,0x1900,0x1a00,0x1b00,0x2000,0x2800,0x2900,0x3000)]
ENTRIES={0x1d9b20:27,0x1d9b70:27,0x1d9bc0:78,0xf20c0:60,0xe7790:17,0xe65a0:35,0x31e5c0:66,0x31e690:66}
def cases(image):
 rows=[]
 def make(kind=1,attached=0,flags=0,field=0x354,bits=0,mode=0xff,biped=0,disable=0,state=0,ticks=30,object_flags=0,active_flags=1,distance=0,angle=0,period=0,seed=False):
  raw=bytearray(0x4000);banks={a:bytearray(0x1000)for a in(0x4ec000,0x4f0000,0x51d000,0x52b000)}
  def put(p,v,n=4):
   b=raw if BASE<=p<BASE+len(raw)else banks[p&~0xfff];offset=p-BASE if b is raw else p&0xfff;b[offset:offset+n]=(v&((1<<(8*n))-1)).to_bytes(n,'little')
  def number(p,v):put(p,struct.unpack('<I',struct.pack('<f',v))[0])
  put(0x4ecbd0,POOL);put(POOL+0x44,ROWS);put(ROWS+12+8,UNIT);put(ROWS+12+3,kind,1)
  put(0x52be5c,COMP);put(COMP+0x44,COMP_ROWS);put(UNIT+0xb4,0xabcd0001);put(COMP_ROWS+160+0x70,INST);put(INST+96+0x44,attached,1)
  put(POLICY+0x60,0x12340001);put(POLICY+0x64,1);put(BODY+0x3c,MOTION);put(OUTPUT,0xaabbccdd)
  for off in(0x10,0x20,0x30,0x40):number(POLICY+off+12,1)
  for off in(0x50,0x54,0x58,0x5c,8):number(POLICY+off,1)
  number(MOTION+0xb0,distance);number(MOTION+0xbc,1);number(MOTION+0x30,angle);number(MOTION+0x3c,1)
  put(UNIT,0x77770001);put(0x4f0414,TAGS);put(TAGS+16+8,TAG);put(TAG+0x1ec,flags)
  put(UNIT+0x38b,mode,1);put(UNIT+field,bits);put(UNIT+0x3dc,biped,1);put(UNIT+0x34b,disable,1)
  put(UNIT+0xc1,object_flags,1);put(UNIT+0x348,active_flags,1);put(UNIT+0x399,state,1);put(UNIT+0x14,0xffffffff)
  put(0x51df54,GLOBALS);put(GLOBALS+2,ticks,2)
  n=len(rows);gp=[0x5a000000+n*23+k*7 for k in range(8)];gp[1]=POLICY;gp[4]=ref.SP
  rows.append(dict(id=n,name=('fault_seed_'if seed else'policy_')+str(n),entry=0x1d9b70 if period else 0x1d9b20,period=period,world=0,island=0,body=BODY,policy=POLICY,output=OUTPUT,arguments=[OUTPUT,BODY],gp=gp,banks=[dict(address=BASE,before=raw.hex()),*[dict(address=a,before=b.hex())for a,b in banks.items()],dict(address=0x46a000,before=image.read(0x46a000,0x1000).hex())],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[],alignment_fault=False))
 for period in(0,1):
  for kind in(0,1,2,0xff):
   for attached in(0,1,2):
    for distance,angle in((0,0),(2,.5)):make(kind=kind,attached=attached,distance=distance,angle=angle,period=period)
  for flag,field in((1,0x354),(2,0x35c),(4,0x25c),(8,0x260),(32,0x358)):
   for bits in(0,0x80000000,1,0x80000001,0x3f800000,0xbf800000,0x7f800000,0x7fc12345,0x7f812345):make(flags=flag,field=field,bits=bits,period=period)
  for mode in(0,1,0xfe,0xff):make(flags=1,mode=mode,period=period)
  for disable in(0,1):
   for biped in(0,1,3):
    for ticks in(-30,30,45,32767):
     for state in(-128,0,1,127):make(kind=0,disable=disable,biped=biped,ticks=ticks,state=state,period=period)
  for object_flags,active in((1,1),(0,0),(1,0)):make(kind=0,biped=1,object_flags=object_flags,active_flags=active,period=period)
 make(flags=1,bits=0x3f800000,seed=True);make(kind=0,biped=1,ticks=45,state=127,seed=True);make(kind=2,distance=2,angle=.5,seed=True)
 return rows
def prepare(image,meta,scratch):
 p=activation.prepare(image,meta,scratch)
 asm=(p/'reference.s').read_text();link=(p/'link.ld').read_text()
 for n,a in enumerate((0x4ec000,0x4f0000,0x51d000,0x52b000,0x46a000)):
  asm=asm.replace('.section .stack,"aw"',f'.section .extra{n},"aw"\n.space 4096\n.section .stack,"aw"')
  link=link.replace('.stack 0x',f'.extra{n} 0x{a:x}:{{*(.extra{n})}} .stack 0x')
 (p/'reference.s').write_text(asm);(p/'link.ld').write_text(link)
 observer=p/'observer.cpp';text=observer.read_text()
 old='check(fp.cwd==entry_fp.cwd&&fp.swd==entry_fp.swd&&fp.ftw==entry_fp.ftw&&!std::memcmp(fp.st_space,entry_fp.st_space,sizeof(fp.st_space)),"Original geometry unexpectedly changed live x87 values/status");'
 new='check(fp.cwd==entry_fp.cwd,"Original gameplay policy changed x87 control");if(finished){check(((fp.swd^entry_fp.swd)&0x3800u)==0&&fp.ftw==entry_fp.ftw,"Original gameplay policy changed balanced x87 ownership");for(unsigned n=0;n<depth;++n)check(!std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),"Original gameplay policy changed a live x87 value");}'
 assert text.count(old)==1;observer.write_text(text.replace(old,new))
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')]):subprocess.run(command,check=True,capture_output=True)
 subprocess.run(['c++','-std=c++20','-O2',str(observer),'-o',str(p/'observer')],check=True,capture_output=True)
 return p
def driver():
 s=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({27,27,78,60,17,35,66,66})')
 start=s.index('NativeTu15IslandActivationStats stats;');end=s.index('   Host host;',start)
 s=s[:start]+'''NativeTu15GameplayDeactivationPolicyStats stats;J required=nullptr;
   m.banks.emplace(0x2000000,std::vector<std::byte>(0x10000));
'''+s[end:]
 start=s.index('switch(entry){');end=s.index('   }catch(const Prefix&)',start)
 s=s[:start]+'''predicate=native_tu15_gameplay_deactivation_policy(m,test.at("policy"),body,
      test.at("period")==0?NativeTu15DeactivationPeriod::short_period:NativeTu15DeactivationPeriod::long_period,
      test.at("output"),0x200efff,fp,0,observer,&stats);
'''+s[end:]
 start=s.index('   if(test.at("entry")==0x283450');end=s.index('\n',start)
 s=s[:start]+'''   if(test.at("completed").get<bool>())check(predicate==(bytes(test.at("banks")[0].at("after"))[test.at("output").get<unsigned>()-0x1000000]!=std::byte{}),name+" gameplay policy bool differs");'''+s[end:]
 # Compiler locals replace private source frames. Neither aliases persistent DATA.
 # Ignore only the declared source stack in this external-operand proof.
 s=s.replace('observer=[&](const auto&s){stores.push_back(s);','observer=[&](const auto&s){if(s.address>=0x2000000&&s.address<0x2010000)return;stores.push_back(s);')
 return s
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_gameplay_deactivation_policy.cpp';files=[source,ROOT/'include/halo2/native_tu15_gameplay_deactivation_policy.hpp',ROOT/'src/physics/native_tu15_body_deactivation_policy.cpp',Path(__file__)]
 pins={str(f.relative_to(ROOT)):ref.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert ref.sha(ref.NEW_DEFAULT)==ref.NEW_PIN;image=ref.Xbe(ref.NEW_DEFAULT);old=notify.ENTRIES;notify.ENTRIES=ENTRIES
  try:meta=notify.metadata(image)
  finally:notify.ENTRIES=old
  # Capstone omits access flags for this authenticated TEST m8,r8 form.
  # Record its real data read instead of silently omitting the operand.
  missing=[(s,o)for s in meta['sites']for o in s['operands']if not(o['read']or o['write'])]
  assert len(missing)==1 and missing[0][0]['instruction']==0xe65c7
  assert image.read(0xe65c7,6)==bytes.fromhex('849048030000')
  missing[0][1]['read']=True
  meta['operand_access_corrections']=[dict(instruction=0xe65c7,bytes='849048030000',meaning='TEST reads its one-byte memory operand')]
  owned=prepare(image,meta,scratch);rows=ref.physical(owned,cases(image),'complete');faults=[]
  for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=ref.physical(owned,faults,'prefix');fixture=scratch/'policy.json';fixture.write_text(json.dumps(dict(image_sha256=ref.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
  needle='void read(std::uint32_t p,std::span<std::byte>b)const override{';prefix=prefix.replace(needle,needle+'if(p>=0x2000000&&p+b.size()<=0x2010000){auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());return;}')
  needle='void write(std::uint32_t p,std::span<const std::byte>b)override{';prefix=prefix.replace(needle,needle+'if(p>=0x2000000&&p+b.size()<=0x2010000){auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);return;}')
  (project/'driver.cpp').write_text('#include "halo2/native_tu15_gameplay_deactivation_policy.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_attachment','==1u;\n    const auto kind','!=1u;\n    const auto kind'),('unordered_as_zero','return different||unordered;','return different;'),('wrong_period','period==NativeTu15DeactivationPeriod::long_period;','period!=NativeTu15DeactivationPeriod::long_period;')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects:",r.stderr[-500:])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeGameplayPolicy LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(policy_{name} $<TARGET_OBJECTS:driver> {name}.cpp "{ROOT}/src/physics/native_tu15_body_deactivation_policy.cpp")\ntarget_include_directories(policy_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(policy_{name} PRIVATE cxx_std_20)\ntarget_compile_options(policy_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:policy_{name}> "{fixture}"';cmake+=f'add_test(NAME policy_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','policy_native',*sum((['--target','policy_'+n]for n in variants),[]),'--filter','^policy_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(ref.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],negative_controls=list(variants),source_snapshot_unchanged=unchanged,image_sha256=ref.NEW_PIN,remaining=['Live binding','Private stack aliases','Unmasked FP exceptions'],instruction_patches=[],supplied_original_outcomes=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
