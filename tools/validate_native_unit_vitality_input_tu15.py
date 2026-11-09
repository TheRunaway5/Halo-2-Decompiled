#!/usr/bin/env python3
"""Physical originalD81E0/D80A0 plus the actual HS live-unit admission prefix.

Ordinary input scalars, object/tag/model DATA and caller FP are supplied before
unchanged source execution. Every retained bank, ordered operand/store and FP
is compared; original private CPU/stack postimages are outside this typed API.
Use tools/run_test.py; generated binaries/raw fixtures remain disposable.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_support_carrier_tu15 as support
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

SOURCE=ROOT
B=0x1000000
POOL,HEADERS,BODY,TAGS,OBJECT_TAG,MODEL_TAG,INFO,HEALTH,SHIELD=[B+n for n in(0x100,0x200,0x400,0x800,0x1000,0x1100,0x1200,0x1400,0x1404)]
BACKEND='unchanged-tu15-unit-maximum-vitality-v1'
OWNERS=(0xd81e0,0xd80a0)
def sha(path):
 with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
def metadata(image):
 original=support.OWNERS;support.OWNERS=OWNERS
 try:
  # Same authenticated operand extraction; parent-specific count assertion is separate.
  import inspect
  source=inspect.getsource(support.metadata).replace(" assert counts['0x184ec0']==94 and counts['0x1d8cd0']==51,counts",' assert list(counts.values())==[35,24],counts')
  env=dict(support.__dict__,OWNERS=OWNERS);exec(compile(source,__file__,'exec'),env);result=env['metadata'](image)
 finally:support.OWNERS=original
 code=_Code(image,'.text');body=complete_body(code,0x2adcc4)
 for pc in sorted(body.code):
  if not 0x2adcc4<=pc<0x2adcff:continue
  instruction=code.at(pc);ops=[]
  from capstone.x86_const import X86_OP_MEM
  from capstone import CS_AC_READ,CS_AC_WRITE
  registers={name:i for i,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
  for op in instruction.operands:
   if op.type!=X86_OP_MEM or instruction.mnemonic=='lea':continue
   ops.append(dict(base=registers.get(instruction.reg_name(op.mem.base),-1),index=registers.get(instruction.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
  result['sites'].append(dict(instruction=pc,operands=ops))
 result['body_sites']['hs_live_unit_admission_prefix']=sum(0x2adcc4<=site['instruction']<0x2adcff for site in result['sites'])
 return result
def cases(image):
 out=[]
 def add(name,live=False,unit=0,flags=0,count=1,model=1,health=0x42c80000,shield=0,
         use_health=True,use_shield=True,alias=None,fp=None,depth=0,seed=False):
  rng=random.Random(len(out));banks={B:bytearray(rng.randbytes(0x2000)),0x4ecbd0:bytearray(struct.pack('<I',POOL)),0x4f0414:bytearray(struct.pack('<I',TAGS)),0x46a0d0:bytearray(image.read(0x46a0d0,4))}
  def put(p,n,width=4):
   lo=next(lo for lo,data in banks.items()if lo<=p and p+width<=lo+len(data));banks[lo][p-lo:p-lo+width]=(n&((1<<(width*8))-1)).to_bytes(width,'little')
  put(POOL+0x44,HEADERS);put(HEADERS+8,BODY);put(BODY,0);put(BODY+0x10a,flags,1)
  put(TAGS+8,OBJECT_TAG);put(TAGS+24,MODEL_TAG);put(OBJECT_TAG+0x38,model);put(MODEL_TAG+0x60,count);put(MODEL_TAG+0x64,INFO)
  put(INFO+0x28,0x42c80000);put(INFO+0x8c,0x43480000);put(HEALTH,health);put(SHIELD,shield)
  h=HEALTH if use_health else 0;s=SHIELD if use_shield else 0
  if alias:h,s=alias
  if live:
   h,s=physical.SP+16,physical.SP+8;banks[s]=bytearray(12);put(s,shield);put(h,health)
  out.append(dict(name=('fault_seed_'if seed else'')+name,kind='live'if live else'direct',entry=0x2adcc4 if live else 0xd81e0,receiver=unit,arguments=[],
      entry_registers={'eax':unit,'edi':h,'ebx':s},unit=unit,health_source=h,shield_source=s,x87_depth=depth,
      banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=fp or[0x27f,0x453f,0x1f80],stop_pc=0x2adcff if live else physical.STOP,boundary=None))
 for live in(False,True):
  for rounding in range(4):
   for mode in(0,0x40,0x8000,0x8040):add(f'fp_{live}_{rounding}_{mode}',live,fp=[0x27f|(rounding<<10),0x453f,0x1f80|(rounding<<13)|mode])
  for value in(0,0x80000000,1,0x80000001,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
   add(f'health_{live}_{value:x}',live,health=value);add(f'shield_{live}_{value:x}',live,shield=value)
  for count in(0,1,0xffffffff,0x80000000):
   for h in(False,True):
    for s in(False,True):add(f'defaults_{live}_{count:x}_{h}_{s}',live,count=count,use_health=h,use_shield=s)
  add(f'no_model_{live}',live,model=0xffffffff,use_health=False,use_shield=False)
  for depth in range(1,8):add(f'x87_{live}_{depth}',live,depth=depth,seed=True)
  add(f'ordinary_{live}',live,seed=True)
 for flags in(0,1,3,4,5,255):add('live_flag_'+str(flags),True,flags=flags)
 add('live_none',True,unit=0xffffffff)
 for h,s in((BODY+0xe4,BODY+0xe8),(BODY+0xe8,BODY+0xe4),(INFO+0x28,INFO+0x8c),(HEALTH,HEALTH)):
  add(f'alias_{h:x}_{s:x}',alias=(h,s))
 return out
def driver_source():
 text=(ROOT/'tests/native_tu15_sphere_overlap_tests.cpp').read_text().replace('#include "halo2/native_tu15_convex_overlap_query.hpp"','#include "halo2/native_tu15_unit_vitality_input.hpp"')
 a=text.index(' for(unsigned depth=1;');b=text.index(' for(const auto&c:fixture.at("cases"))',a);text=text[:a]+text[b:]
 a=text.index('  NativeTu15PhysicsFrameFp fp{c.at');b=text.index('  const auto cw=',a)
 text=text[:a]+'''  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+text[b:]
 a=text.index('#if defined(HALO2_SPHERE_QUERY_BINDING)',text.index('  try{'));b=text.index('  }catch(const Fault&)',a)
 text=text[:a]+'''   const NativeTu15UnitMaximumVitalityInput input{c.at("health_source"),c.at("shield_source")};
   if(c.at("kind")=="live")native_tu15_set_live_unit_maximum_vitality(m,c.at("unit"),input,fp,observe);
   else native_tu15_set_unit_maximum_vitality(m,c.at("unit"),input,fp,observe);
'''+text[b:]
 text=text.replace('check(clock==c.at("timestamps").size(),name+" clock consumption differs");','')
 text=text.replace('name+" store count differs"','name+" store count differs native="+std::to_string(stores.size())+" original="+std::to_string(expected.size())+" expected="+expected.dump()')
 # Reuse the independently qualified actual hardware x87 preservation witness.
 witness=support.driver_source();a=witness.index('struct CallerX87');b=witness.index('std::uint16_t control()',a)
 text=text.replace('std::uint16_t control()',witness[a:b]+'std::uint16_t control()')
 text=text.replace('#include <fstream>','#include <fstream>\n#include <cstring>')
 text=text.replace('  const auto cw=control(),sw=status();','  CallerX87 occupied(c);const auto cw=control(),sw=status();')
 text=text.replace('check(control()==cw&&status()==sw&&_mm_getcsr()==mx,name+" host FP differs");','check(control()==cw&&status()==sw&&_mm_getcsr()==mx,name+" host FP differs");occupied.verify(name);')
 text=text.replace('const auto fault=c.value("fault",J{});','check(c.at("incoming_x87")==c.at("outgoing_x87")&&c.at("incoming_x87_tags")==c.at("outgoing_x87_tags"),name+" original x87 preservation differs");const auto fault=c.value("fault",J{});')
 return text.replace('unchanged-tu15-sphere-overlap-v1',BACKEND)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--no-prefixes',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],original_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);physical.observer_source=support.observer_source;physical.boundary=lambda c,d:None
  import inspect
  # The typed HS admission begins after its two scalar spills. Its EAX/ZF
  # entry formals are exactly the preceding2ADCBB CMP's unit/NONE relation.
  # This ordinary BEFORE setup does not supply any native branch/result.
  original_source=inspect.getsource(support.physical_with_x87)
  marker=r"  entry_setup+='\\n'+'\\n'.join(f' fldt x87_input+{n*10}'for n in range(depth))"
  assert original_source.count(marker)==1
  original_source=original_source.replace(marker,marker+"\n  if c.get('kind')=='live':entry_setup+='\\\\n cmpl $-1,%eax'")
  env=dict(support.__dict__);exec(compile(original_source,__file__,'exec'),env);original=env['physical_with_x87']()
  rows=original(image,meta,cases(image),scratch);prefixes=[];code=_Code(image,'.text')
  if not a.no_prefixes:
   for seed in(c for c in rows if c['name'].startswith('fault_seed_')):
    for category in('reads','writes'):
     seen=set();occurrences={}
     for e in seed[category]:
      key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occurrences[key]=occurrences.get(key,0)+1
      if e['instruction']in seen:continue
      seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefixes.append(c)
    for n,e in enumerate(seed['writes']):
     c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,observer_fault=n+1);prefixes.append(c)
   rows+=original(image,meta,prefixes,scratch)
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  header=SOURCE/'include/halo2/native_tu15_unit_vitality_input.hpp';kernel=SOURCE/'src/physics/native_tu15_unit_vitality_input.cpp';shutil.copyfile(header,project/'include/halo2'/header.name);shutil.copyfile(kernel,project/'native.cpp');driver=driver_source();(project/'driver.cpp').write_text(driver);(a.output/'driver.cpp.txt').write_text(driver);(a.output/'qualifier.py.txt').write_bytes(Path(__file__).read_bytes())
  source=kernel.read_text();mutants={'positive_includes_zero':('seta %0','setae %0'),'wrong_health_store':('destination+0xe4u,health','destination+0xe8u,health'),'missing_zero_shield':('has_shield?one:0u','one'),'unsigned_information_count':('std::bit_cast<std::int32_t>(count)<=0','count==0'),'missing_dead_guard':('if((read(m,body(m,unit)+0x10au,1)>>2u)&1u)','if(false)')}
  for name,(old,new)in mutants.items():assert source.count(old)==1;(project/(name+'.cpp')).write_text(source.replace(old,new))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differ"in r.stderr,r.stderr\nprint(r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(unit_vitality LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*mutants):
   unit='native.cpp'if name=='native'else name+'.cpp';cmake+=f'add_executable(vitality_{name} $<TARGET_OBJECTS:driver> {unit})\ntarget_include_directories(vitality_{name} PRIVATE include)\ntarget_compile_features(vitality_{name} PRIVATE cxx_std_20)\ntarget_compile_options(vitality_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';prefix=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME vitality_{name} COMMAND {prefix}$<TARGET_FILE:vitality_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--target','vitality_'+name]for name in('native',*mutants)),[]),'--filter','^vitality_'];result=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(result.stdout+result.stderr);results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-2000:]+result.stderr[-2000:]
  visited={pc for c in rows for pc in c['visited']};pins={str(path):sha(path)for path in(header,kernel,Path(__file__),ROOT/'tools/validate_native_support_carrier_tu15.py')}
  report.update(passed=True,original_sha256=NEW_PIN,cases_per_compiler=len(rows),fault_prefixes=sum('fault'in c for c in rows),observer_fault_prefixes=sum('observer_fault'in c for c in rows),preserved_x87_depths=sorted({c.get('x87_depth',0)for c in rows}),body_coverage={hex(owner):dict(covered=sum(pc in visited for pc in complete_body(code,owner).code),sites=len(complete_body(code,owner).code))for owner in OWNERS},hs_admission_prefix_sites=meta['body_sites']['hs_live_unit_admission_prefix'],negative_controls=list(mutants),compiler_results=results,source_hashes=pins,full_live_world_qualification=False)
 except BaseException as error:report['error']=str(error);raise
 finally:save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
