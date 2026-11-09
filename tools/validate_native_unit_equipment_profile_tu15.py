#!/usr/bin/env python3
"""Unchanged CF2E0 selected-profile parent and genuine empty-inventory clear.

Complete no-weapon profiles compare all external banks, ordered operands/stores,
FP and occupied x87. Positive weapon profiles stop at the genuine CF240 child
entry, compare its typed arguments, and supply no factory return. Positive
birth/carry/retirement completion requires the real World integration proof.
"""
import argparse,copy,hashlib,inspect,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_unit_vitality_input_tu15 as vitality
import validate_native_support_carrier_tu15 as support
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
SOURCE=ROOT
B=0x1000000
POOL,HEADERS,BODY,SCENARIO,PROFILE=[B+n for n in(0x100,0x200,0x400,0x800,0x1000)]
BACKEND='unchanged-tu15-unit-equipment-profile-v1'
OWNERS=(0xcf2e0,0xcf4d0,0xcf400)
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
def metadata(image):
 text=inspect.getsource(support.metadata).replace(" assert counts['0x184ec0']==94 and counts['0x1d8cd0']==51,counts",' assert list(counts.values())==[74,56,60],counts')
 env=dict(support.__dict__,OWNERS=OWNERS);exec(compile(text,__file__,'exec'),env);m=env['metadata'](image);m['boundaries']=[0xcf240];return m
def cases(image):
 rows=[]
 def add(name,clear=False,index=0,unit=0,health=0x3f800000,shield=0x3f800000,hurt=0,shield_hurt=0,grenades=(0,0),existing=(2,3),weapon=(),garbage=False,alias=None,fp=None,depth=0,seed=False):
  rng=random.Random(len(rows));banks={B:bytearray(rng.randbytes(0x2000)),0x4ecbd0:bytearray(struct.pack('<I',POOL)),0x4ecc20:bytearray(struct.pack('<I',SCENARIO)),0x46a0d0:bytearray(image.read(0x46a0d0,4))}
  def put(p,v,n=4):
   lo=next(lo for lo,d in banks.items()if lo<=p and p+n<=lo+len(d));banks[lo][p-lo:p-lo+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  row=PROFILE if alias is None else alias
  put(SCENARIO+0xfc,(row-index*0x44)&0xffffffff);put(POOL+0x44,HEADERS);put(HEADERS+8,BODY)
  put(BODY+0xec,health);put(BODY+0xf0,shield);put(BODY+0x212,255,1);put(BODY+0x213,255,1)
  for n in range(4):put(BODY+0x218+n*4,0xffffffff)
  put(BODY+0x23e,existing[0],1);put(BODY+0x23f,existing[1],1)
  put(row+0x20,hurt);put(row+0x24,shield_hurt)
  for n in range(2):put(row+0x2c+n*12,0xffffffff)
  for n in weapon:put(row+0x2c+n*12,0x100+n)
  put(row+0x40,grenades[0],1);put(row+0x41,grenades[1],1)
  rows.append(dict(name=('fault_seed_'if seed else'')+name,entry=0xcf2e0,receiver=0,entry_registers={'eax':index&0xffff},arguments=[unit,int(clear),int(garbage)],unit=unit,profile_index=index,clear=clear,garbage=garbage,profile=row,x87_depth=depth,banks=[dict(address=p,before=d.hex())for p,d in sorted(banks.items())],before_fp=fp or[0x27f,0x453f,0x1f80],stop_pc=physical.STOP,boundary=None))
 for clear in(False,True):
  for rounding in range(4):
   for mode in(0,0x40,0x8000,0x8040):add(f'fp_{clear}_{rounding}_{mode}',clear,fp=[0x27f|(rounding<<10),0x453f,0x1f80|(rounding<<13)|mode])
  for g in(0,1,2,4,127,128,255):
   for old in(0,1,255):add(f'grenades_{clear}_{g}_{old}',clear,grenades=(g,255-g),existing=(old,255-old))
  for v in(0,0x80000000,1,0x80000001,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):add(f'arithmetic_{clear}_{v:x}',clear,hurt=v,shield_hurt=v)
  for depth in range(1,8):add(f'x87_{clear}_{depth}',clear,depth=depth)
  add(f'empty_{clear}',clear,seed=True)
  for weapon in((0,),(1,),(0,1)):
   for garbage in(False,True):add(f'factory_{clear}_{weapon}_{garbage}',clear,weapon=weapon,garbage=garbage,seed=True)
 for index in(-32768,-2,-1,0,1,32767):add('index_'+str(index),index=index)
 add('none_unit',unit=0xffffffff,clear=True,weapon=(0,))
 # Aliases are ordinary before-input relationships; full banks remain compared.
 for alias in(BODY+0xbc,BODY+0xbe,BODY+0xc0):add('alias_'+hex(alias),alias=alias,grenades=(1,4),existing=(255,2))
 return rows
def driver_source():
 text=vitality.driver_source().replace('native_tu15_unit_vitality_input.hpp','native_tu15_unit_equipment_profile.hpp').replace(vitality.BACKEND,BACKEND)
 marker='struct Fault{};struct ObserverFault{};';assert text.count(marker)==1;text=text.replace(marker,marker+'struct ChildBoundary{};')
 a=text.index('   const NativeTu15UnitMaximumVitalityInput input');b=text.index('  }catch(const Fault&)',a)
 text=text[:a]+'''   const auto word=[&](std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8);return v;};
   struct Input:NativeTu15UnitEquipmentProfileInput {const J&c;const std::function<std::uint32_t(std::uint32_t,unsigned)>&word;
    bool selected()const override{return c.at("profile_index")!=-1;}
    std::uint32_t data()const override{const auto scenario=word(0x4ecc20u,4),rows=word(scenario+0xfcu,4);return rows+std::uint32_t(c.at("profile_index").get<std::int32_t>())*0x44u;}
    Input(const J&v,const std::function<std::uint32_t(std::uint32_t,unsigned)>&r):c(v),word(r){} };
   const std::function<std::uint32_t(std::uint32_t,unsigned)>reader=word;Input input(c,reader);
   const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t v){std::array<std::byte,1>b{std::byte(v)};SourcePhysicsContactRecordStore e{pc,p,{},{}};if(observe){e.before.resize(1);m.read(p,e.before);e.written.assign(b.begin(),b.end());}m.write(p,b);if(observe)observe(e);};
   NativeTu15UnitEquipmentProfileServices services;
   services.clear=[&](auto unit,auto&){
    const auto pool=word(0x4ecbd0u),headers=word(pool+0x44u),body=word(headers+(unit&65535u)*12u+8u);
    check(word(body+0x212u,1)==255u,name+" clear first inventory domain differs");store(0xcf4fdu,body+0x216u,255u);
    check(word(body+0x213u,1)==255u,name+" clear second inventory domain differs");store(0xcf545u,body+0x217u,255u);
    const auto p=word(0x4ecbd0u),h=word(p+0x44u),b=word(h+(unit&65535u)*12u+8u);
    for(unsigned n=0;n<4;++n)check(word(b+0x218u+n*4u)==0xffffffffu,name+" clear inventory domain differs");
   };
   services.create=[&](auto unit,auto request,bool garbage,auto&)->std::uint32_t{
    check(!c.at("boundary").is_null(),name+" unexpected child differs");const auto&boundary=c.at("boundary");
    check(unit==boundary.at("unit")&&request==boundary.at("weapon_input")&&garbage==boundary.at("garbage"),name+" child ABI differs");boundary_failed=true;throw ChildBoundary{};
   };
   native_tu15_apply_unit_equipment_profile(m,c.at("unit"),input,c.at("clear"),c.at("garbage"),fp,services,observe);
'''+text[b:]
 text=text.replace('bool failed{},observer_failed{};','bool failed{},observer_failed{},boundary_failed{};')
 text=text.replace('}catch(const ObserverFault&){observer_failed=true;}','}catch(const ObserverFault&){observer_failed=true;}catch(const ChildBoundary&){}')
 text=text.replace('check(failed==fault_case','check(boundary_failed==(!c.at("boundary").is_null()),name+" boundary completion differs");check(failed==fault_case')
 return text
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--no-prefixes',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],original_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);physical.observer_source=support.observer_source
  def boundary(c,d):
   if d['actual_stop']==physical.STOP:return None
   assert d['actual_stop']==0xcf240;return dict(unit=d['boundary_stack'][1],garbage=bool(d['boundary_stack'][2]&255),weapon_input=d['boundary_gp'][3])
  physical.boundary=boundary;original=support.physical_with_x87();rows=original(image,meta,cases(image),scratch);prefixes=[];code=_Code(image,'.text')
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
  header=SOURCE/'include/halo2/native_tu15_unit_equipment_profile.hpp';kernel=SOURCE/'src/physics/native_tu15_unit_equipment_profile.cpp';pins={str(p):sha(p)for p in(header,kernel,Path(__file__))};shutil.copyfile(header,project/'include/halo2'/header.name);shutil.copyfile(kernel,project/'native.cpp');driver=driver_source();(project/'driver.cpp').write_text(driver);(a.output/'driver.cpp.txt').write_text(driver);(a.output/'qualifier.py.txt').write_bytes(Path(__file__).read_bytes())
  source=kernel.read_text();mutants={'wrong_health_channel':('body+0xecu,subtract(health','body+0xf0u,subtract(health'),'grenade_assignment':('frag+frag_added','frag_added'),'wrong_grenade_order':('const auto plasma_added=read(m,profile+0x41u,1);','const auto plasma_added=frag_added;'),'omit_clear_health':('body+0xecu,one,4,stats','body+0xecu,0u,4,stats'),'wrong_second_stride':('n*12u;if(read','n*16u;if(read')}
  for name,(old,new)in mutants.items():assert source.count(old)==1;(project/(name+'.cpp')).write_text(source.replace(old,new))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differ" in r.stderr,r.stderr\nprint(r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(unit_profile LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*mutants):
   cmake+=f'add_executable(profile_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(profile_{name} PRIVATE include)\ntarget_compile_features(profile_{name} PRIVATE cxx_std_20)\ntarget_compile_options(profile_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';prefix=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME profile_{name} COMMAND {prefix}$<TARGET_FILE:profile_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--target','profile_'+name]for name in('native',*mutants)),[]),'--filter','^profile_'];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stdout[-2000:]+r.stderr[-2000:]
  visited={pc for c in rows for pc in c['visited']};report.update(passed=True,original_sha256=NEW_PIN,cases_per_compiler=len(rows),fault_prefixes=sum('fault'in c for c in rows),observer_fault_prefixes=sum('observer_fault'in c for c in rows),preserved_x87_depths=sorted({c.get('x87_depth',0)for c in rows}),body_coverage={hex(o):dict(covered=sum(pc in visited for pc in complete_body(code,o).code),sites=len(complete_body(code,o).code))for o in OWNERS},negative_controls=list(mutants),compiler_results=results,source_hashes=pins,full_live_world_qualification=False,positive_factory_returns_supplied=False)
 except BaseException as e:report['error']=str(e);raise
 finally:save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
