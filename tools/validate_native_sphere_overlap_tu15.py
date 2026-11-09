#!/usr/bin/env python3
"""Physical original2F9CA0/native comparison, including actual30BA80 collector.

Declared ordinary sphere inputs, all retained bytes, ordered reads/stores and
FP are compared. Genuine measured RDTSC values are external clock inputs only;
no original collision result or AFTER bank is supplied to the native routine.
Run through tools/run_test.py. Compiler products and raw fixtures are disposable.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE=0x1000000
FIRST,SECOND,A,B,SA,SB,COLLECTOR,TABLE=map(lambda p:BASE+p,(0x100,0x200,0x300,0x400,0x500,0x600,0x700,0x800))
DATA=physical.SP-32
BACKEND='unchanged-tu15-sphere-overlap-v1'
ORIGINAL_OBSERVER=physical.observer_source
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2)+'\n')
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites={};pins={};counts={}
 for owner in (0x2f9ca0,0x30ba80):
  body=complete_body(code,owner);listing=[];counts[hex(owner)]=len(body.code)
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for o in i.operands:
    if o.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not o.mem.segment
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else o.access
    if i.mnemonic in('cmp','test'):access|=CS_AC_READ
    ops.append(dict(base=registers.get(i.reg_name(o.mem.base),-1),index=registers.get(i.reg_name(o.mem.index),-1),scale=o.mem.scale,displacement=o.mem.disp&0xffffffff,bytes=o.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites[pc]=dict(instruction=pc,operands=ops,timestamp=i.mnemonic=='rdtsc')
  pins[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert list(counts.values())==[64,2],counts
 return dict(sites=list(sites.values()),body_sites=counts,listing_sha256=pins,regions=[],boundaries=[])
def cases():
 out=[]
 def add(name,distance=0x3f800000,radius_a=0x3f800000,radius_b=0x3f800000,profile=False,fp=None,seed=False,**kw):
  rng=random.Random(len(out));banks={BASE:bytearray(rng.randbytes(0x1000)),0x48c974:bytearray(8),DATA+12:bytearray(rng.randbytes(4))}
  def put(p,n):
   lo=next(lo for lo,b in banks.items()if lo<=p and p+4<=lo+len(b));struct.pack_into('<I',banks[lo],p-lo,n)
  for p,n in ((FIRST,SA),(FIRST+8,A),(SECOND,SB),(SECOND+8,B),(SA+12,radius_a),(SB+12,radius_b),(COLLECTOR,TABLE),(TABLE+4,0x30ba80),(0x48c974,BASE+0xe00 if profile else 0),(0x48c978,BASE+0xe40 if profile else 0)):
   put(p,n)
  for p in (A+0x50,B+0x50):
   for n in range(4):put(p+n*4,0)
  put(B+0x50,distance)
  for p,n in kw.get('changes',()):put(p,n)
  out.append(dict(name=('fault_seed_'if seed else'')+name,entry=0x2f9ca0,receiver=0xdeadbeef,arguments=[FIRST,SECOND,0xbad0bad0,COLLECTOR],first=FIRST,second=SECOND,collector=COLLECTOR,data=DATA,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=fp or[0x27f,0x453f,0x1f80],stop_pc=physical.STOP,boundary=None))
 for precision in (0,2,3):
  for rounding in range(4):
   for sse in (0,0x40,0x8000,0x8040):
    for distance in (0x3f800000,0x40000000,0x40400000):
     add(f'fp_{precision}_{rounding}_{sse}_{distance:x}',distance,fp=[0x7f|(precision<<8)|(rounding<<10),0x453f,0x1f80|(rounding<<13)|sse])
 for value in (0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0x3f800001,0x7f7fffff):
  add(f'position_{value:x}',value)
  add(f'radius_a_{value:x}',radius_a=value)
  add(f'radius_b_{value:x}',radius_b=value)
 add('negative_radii',radius_a=0xbf800000,radius_b=0xc0000000)
 add('zero_distance',0)
 add('three_axes',0x3e800000,changes=((B+0x54,0x3f000000),(B+0x58,0x3f800000)))
 add('unused_w_lane',changes=((A+0x5c,0x7f812345),))
 # No collector reads for separation/equality/unordered, even if invalid.
 for distance in (0x40000000,0x40400000,0x7fc12345):add('no_collector_'+hex(distance),distance,changes=((COLLECTOR,0),))
 for profile in (False,True):
  for distance in (0,0x3f800000,0x40000000,0x40400000):add(f'profile_{profile}_{distance:x}',distance,profile=profile)
 add('overlap',seed=True);add('profile_overlap',profile=True,seed=True)
 return out
def observer_source():
 source=ORIGINAL_OBSERVER()
 source=source.replace('unsigned count=0;','unsigned count=0;J visited=J::array();')
 marker='const auto registers=gp(regs);J pending=J::array();';assert source.count(marker)==1
 source=source.replace(marker,'visited.push_back(pc);'+marker)
 marker='{"timestamps",timestamps}';assert source.count(marker)==1
 return source.replace(marker,marker+',{ "visited",visited}')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--binding',action='store_true',help='Also use the production typed query and collector binding');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,scope=__doc__,typed_binding=a.binding,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);physical.observer_source=observer_source;physical.boundary=lambda c,d:None
  rows=physical.physical(image,meta,cases(),scratch);prefixes=[];code=_Code(image,'.text')
  for seed in (c for c in rows if c['name'].startswith('fault_seed_')):
   for category in ('reads','writes'):
    seen=set();occurrences={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;occurrences[key]=occurrences.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefixes.append(c)
   for n,event in enumerate(seed['writes']):
    c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=n+1);prefixes.append(c)
  rows+=physical.physical(image,meta,prefixes,scratch)
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  units=('native_tu15_convex_overlap_query.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')
  if a.binding:units+=('native_tu15_wrapped_contact_query.cpp','native_tu15_contact_pair_collectors.cpp')
  pins={}
  for name in units:
   src=ROOT/'src/physics'/name;shutil.copyfile(src,project/name);pins[str(src.relative_to(ROOT))]=sha(src)
  driver=ROOT/'tests/native_tu15_sphere_overlap_tests.cpp';shutil.copyfile(driver,project/'driver.cpp');pins[str(driver.relative_to(ROOT))]=sha(driver)
  if a.binding:shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  kernel=project/units[0];source=kernel.read_text();mutants={
   'include_equal':('if(std::popcount(unsigned((relation>>8u)&5u))&1u){','if((relation&0x4000u)||(std::popcount(unsigned((relation>>8u)&5u))&1u)){'),
   'wrong_radius':('m.word(first_shape+0xcu);','m.word(first_shape+8u);'),
   'wrong_distance_axis':('auto y=_mm_shuffle_ps(b,b,0x55),z=_mm_shuffle_ps(b,b,0xaa);','auto y=_mm_shuffle_ps(b,b,0x55),z=_mm_shuffle_ps(b,b,0x55);'),
   'missing_collect':('services.collect(collector,owner,first,second,fp);});\n   cursor=m.word','(void)collector;(void)owner;});\n   cursor=m.word'),
   'wrong_profile_stride':('0x2f9ccfu,0x48c974u,cursor+12u);','0x2f9ccfu,0x48c974u,cursor+8u);')}
  for name,(before,after)in mutants.items():
   assert source.count(before)==1,(name,source.count(before));(project/(name+'.cpp')).write_text(source.replace(before,after))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in ("differs","differ")),r.stderr\nprint("Original sphere query rejects mutation",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(sphere_query LANGUAGES CXX)\ninclude(CTest)\nadd_library(deps OBJECT driver.cpp '+' '.join(units[1:])+')\ntarget_include_directories(deps PRIVATE include)\ntarget_compile_features(deps PRIVATE cxx_std_20)\ntarget_compile_options(deps PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  if a.binding:cmake+='target_compile_definitions(deps PRIVATE HALO2_SPHERE_QUERY_BINDING=1)\n'
  for name in ('native',*mutants):
   source_name=units[0]if name=='native'else name+'.cpp';cmake+=f'add_executable(sphere_{name} $<TARGET_OBJECTS:deps> {source_name})\ntarget_include_directories(sphere_{name} PRIVATE include)\ntarget_compile_features(sphere_{name} PRIVATE cxx_std_20)\ntarget_compile_options(sphere_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';prefix=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME sphere_{name} COMMAND {prefix}$<TARGET_FILE:sphere_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--target','sphere_'+name]for name in ('native',*mutants)),[]),'--filter','^sphere_']
   r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stdout[-1800:]+r.stderr[-1800:]
  for name in ('include/halo2/native_tu15_convex_overlap_query.hpp','tools/validate_native_sphere_overlap_tu15.py','tools/validate_native_island_agent_phase_tu15.py','tools/observe_native_constraint_step_tu15.cpp'):pins[name]=sha(ROOT/name)
  if a.binding:
   for name in ('include/halo2/native_tu15_overlap_query_binding.hpp','include/halo2/native_tu15_contact_pair_collectors.hpp','include/halo2/native_tu15_wrapped_contact_query.hpp','src/physics/native_tu15_contact_unwind_internal.hpp'):pins[name]=sha(ROOT/name)
  assert all(sha(ROOT/n)==h for n,h in pins.items())
  visited=set(pc for c in rows for pc in c['visited']);report.update(passed=True,original_sha256=NEW_PIN,body_coverage={n:dict(covered=sum(pc in visited for pc in complete_body(code,int(n,16)).code),sites=count)for n,count in meta['body_sites'].items()},listing_sha256=meta['listing_sha256'],cases_per_compiler=len(rows),fault_prefixes=sum('fault'in c for c in rows),observer_fault_prefixes=sum('observer_fault'in c for c in rows),genuine_clock_inputs=sum(len(c['timestamps'])for c in rows),nonempty_x87_refusal_controls_per_compiler=7,negative_controls=list(mutants),compiler_results=results,source_hashes=pins,full_live_world_qualification=False)
 except BaseException as e:report['error']=str(e);raise
 finally:save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
