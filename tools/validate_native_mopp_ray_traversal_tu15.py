#!/usr/bin/env python3
"""Compare semantic native MOPP traversal to unchanged English TU1.5 30AA70.

Declared asset programs exercise the arithmetic and dispatch independently.
Private arithmetic variables/bounds are a complete typed 32-byte projection,
not a CPU frame. Every persistent bank and FP field is compared unchanged.
Shape/filter services stop at real original entries; no result is supplied.
"""
import argparse,re,copy,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import validate_native_island_agent_phase_tu15 as physical

BACKEND='unchanged-tu15-native-mopp-ray-traversal-v1'
BASE,CACHE,CODE,VARS,BOUNDS,CODE_OBJECT,COLLECTION,TABLE,FILTER=0x1000000,0x1000100,0x1001000,0x1002000,0x1002100,0x1002300,0x1002400,0x1002500,0x1002600
BOUNDARIES=(0x2e8cf0,0x31edf0)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def expanded_source():
 # Expand the real shared-header DAG once for compiled mutation controls.
 # In particular, Plane includes Projection, which Traversal also includes.
 names={'native_tu15_mopp_ray_traversal_kernel.hpp','native_tu15_mopp_ray_projection_kernel.hpp',
        'native_tu15_mopp_integer_program_kernel.hpp','native_tu15_mopp_ray_scale_kernel.hpp',
        'native_tu15_mopp_primitive_key_kernel.hpp','native_tu15_mopp_ray_plane_kernel.hpp',
        'native_tu15_mopp_ray_branch_kernel.hpp','native_tu15_mopp_ray_crossing_kernel.hpp','native_tu15_mopp_ray_resume_kernel.hpp','native_tu15_mopp_ray_slab_kernel.hpp'}
 seen=set()
 pattern=re.compile(r'#include "halo2/([^"\n]+)"\n')
 def expand(text):
  def include(match):
   name=match.group(1)
   if name not in names:return match.group(0)
   if name in seen:return ''
   seen.add(name)
   return expand((ROOT/'include/halo2'/name).read_text().removeprefix('#pragma once\n'))
  return pattern.sub(include,text)
 source=expand((ROOT/'src/physics/native_tu15_mopp_ray_traversal.cpp').read_text())
 assert seen==names,(seen,names)
 return source
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[]
 listing=ROOT/'analysis/physics/native-camera-query-dependency-inventory-20261102/0030aa70.txt'
 for line in listing.read_text().splitlines():
  pc=int(line.split()[0],16);ins=code.at(pc);ops=[]
  assert ins.bytes.hex()==line.split()[1],line
  for op in ins.operands:
   if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
   assert not op.mem.segment;base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
   ops.append(dict(base=registers.get(base,-1),index=registers.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=False if ins.mnemonic in('fst','fstp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp')))
  sites.append(dict(instruction=pc,operands=ops))
 assert len(sites)==896
 return dict(sites=sites,body_sites=[896],listing_sha256={'0x30aa70':sha(listing)},boundaries=list(BOUNDARIES),regions=[])
def cases(image):
 rows=[]
 def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
 def make(name,program,start=(0.,0.,0.,0.),end=(255.,255.,255.,0.),rounding=0,fraction=1.,filter=False,quant=None,raw=False,precision=2):
  banks={BASE:bytearray(0x3000),0x41c500:bytearray(image.read(0x41c500,16)),0x46a0d0:bytearray(image.read(0x46a0d0,4)),0x46a0e8:bytearray(image.read(0x46a0e8,4))}
  def put(p,v,n=4):banks[BASE][p-BASE:p-BASE+n]=int(v&((1<<(8*n))-1)).to_bytes(n,'little')
  program=bytes(program);banks[BASE][CODE-BASE:CODE-BASE+len(program)]=program
  for n,v in enumerate(start):put(BOUNDS+4*n,v if raw else bits(v));put(CACHE+0x20+4*n,v if raw else bits(v))
  for n,v in enumerate(end):put(BOUNDS+16+4*n,v if raw else bits(v));put(CACHE+0x30+4*n,v if raw else bits(v))
  for n,v in enumerate(quant or [0,0,0,0,0,bits(1.),0x12340000,0x12345678]):put(VARS+n*4,v)
  put(CACHE+0x10,CODE_OBJECT);put(CACHE+0x14,bits(1.));put(CACHE+0x54,bits(fraction));put(CACHE+0x64,COLLECTION);put(COLLECTION,TABLE);put(TABLE+0x34,BOUNDARIES[0]);put(FILTER,TABLE+0x80);put(TABLE+0x80,BOUNDARIES[1]);put(CACHE+0x44,FILTER if filter else 0)
  rows.append(dict(name=name,entry=0x30aa70,receiver=CACHE,arguments=[VARS,CODE,BOUNDS],variables=VARS,bounds=BOUNDS,code=CODE,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x7f|(precision<<8)|(rounding<<10),0x4500,0x1f80|(rounding<<13)],stop_pc=physical.STOP,boundary=None))
 make('return',[0])
 for rounding in range(4):
  for op in range(1,5):make(f'scale_{op}_{rounding}',[op,12,13,14,0],(1.25,-2.5,3.,4.),(255.,128.,0.,-4.),rounding)
  for op in range(5,8):make(f'jump_{op}_{rounding}',[op]+[0]*(op-5)+[2,255,255,0],rounding=rounding)
  for op,size in((9,1),(10,2),(11,4)):
   make(f'base_{op}_{rounding}',[op]+[255]*size+[0],rounding=rounding)
  for op in range(0x60,0x6c):
   size=1 if op<0x64 else 2 if op<0x68 else 4
   make(f'property_{op}_{rounding}',[op]+[255]*size+[0],rounding=rounding)
  # Each program has two genuine terminating branches; their relative entry
  # offsets are asset bytes, never patched original instructions.
  for op in list(range(0x10,0x1d))+list(range(0x20,0x26)):
   program=[op,64,192,1,0,0] if op<0x20 else [op,128,1,0,0]if op<0x23 else[op,64,192,0,0,0,1,0,0]
   for direction in range(4):
    a,b=((0.,255.),(255.,0.),(100.,101.),(200.,201.))[direction]
    make(f'split_{op:x}_{direction}_{rounding}',program,(a,a,a,1.),(b,b,b,-1.),rounding)
  for op in range(0x26,0x2c):
   program=[op,64,192,0]if op<0x29 else[op,0,0,64,0,0,192,0]
   for direction in range(4):
    a,b=((0.,255.),(255.,0.),(100.,101.),(200.,201.))[direction]
    make(f'slab_{op:x}_{direction}_{rounding}',program,(a,a,a,1.),(b,b,b,-1.),rounding)
 for op in range(0x30,0x54):
  program=[op]+([0xaa]*(op-0x4f)if op>=0x50 else[])
  make(f'leaf_{op:x}',program);make(f'filtered_leaf_{op:x}',program,filter=True)
 # Observe copied arithmetic variables through genuine subsequent child
 # formals, rather than merely testing dead private-data writes before RET.
 for rounding in range(4):
  for op,size in((9,1),(10,2),(11,4)):
   make(f'chained_base_{op}_{rounding}',[op]+[255]*size+[0x53,0x12,0x34,0x56,0x78],rounding=rounding)
   make(f'chained_wrap_{op}_{rounding}',[11,255,255,255,255,op]+[255]*size+[0x4f],rounding=rounding)
  for op in range(0x60,0x6c):
   size=1 if op<0x64 else 2 if op<0x68 else 4
   make(f'chained_property_{op}_{rounding}',[op]+[255]*size+[9,7,0x30],rounding=rounding)
  make(f'chained_scale_slab_{rounding}',[1,1,2,3,0x26,0,255,9,17,0x30],rounding=rounding)
  for n,left in enumerate(([9,17,0],[10,255,255,0],[11,255,255,255,255,0],[1,1,2,3,0],[0x60,12,9,17,0])):
   make(f'recursive_COW_{n}_{rounding}',[0x10,64,192,len(left),*left,0x30],rounding=rounding)
 for top in range(8):
  for modes in(0,0x40,0x8000,0x8040):
   for n,program in enumerate(([0],[1,1,2,3,0x30])):
    make(f'FP_top{top}_denorm{modes:x}_{n}',program,(1,0x80000001,0x00800000,0x80800000),(0x80000001,1,0x80800000,0x00800000),raw=True)
    rows[-1]['before_fp'][1]=0x473f|(top<<11);rows[-1]['before_fp'][2]|=modes
 patterns=[(0x7fc12345,0x7fc23456,0x3f800000,0x80000000),(0x7f812345,0x3f800000,0xbf800000,0),(0x7f800000,0xff800000,0,0x7f800000),(0x7f7fffff,0xff7fffff,0x3f800000,0x80000001),(1,0x80000001,0x00800000,0x80800000),(0x4b800000,0xcb800000,0x3f800000,0x3f800000),(0x80000000,0,0x80000000,0)]
 for precision in(0,2,3):
  for rounding in range(4):
   for op in list(range(0x13,0x1d))+list(range(0x26,0x2c))+[1]:
    program=[op,64,192,1,0,0]if op<0x20 and op!=1 else[op,64,192,0]if op<0x29 and op!=1 else[op,0,0,64,0,0,192,0]if op!=1 else[1,1,2,3,0]
    for n,start in enumerate(patterns):make(f'numeric_{op:x}_{precision}_{rounding}_{n}',program,start,patterns[(n+1)%len(patterns)],rounding,raw=True,precision=precision)
 # Genuine declared closer collector fractions exercise recomputation from
 # all four original endpoint lanes after recursive branches return.
 for op in list(range(0x10,0x1d))+list(range(0x20,0x26)):
  program=[op,64,192,1,0,0]if op<0x20 else[op,128,1,0,0]if op<0x23 else[op,64,192,0,0,0,1,0,0]
  make(f'fraction_{op:x}',program,(0.,0.,0.,5.),(255.,255.,255.,-5.),fraction=.5)
 for op,program in((0x10,[0x10,64,192,1,0,0]),(0x29,[0x29,0,0,64,0,0,192,0]),(1,[1,1,2,3,0])):make(f'fault_seed_{op:x}',program)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--xbe',type=Path,default=NEW_DEFAULT);p.add_argument('--limit',type=int);p.add_argument('--pattern');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 source=ROOT/'src/physics/native_tu15_mopp_ray_traversal.cpp'
 files=[source,ROOT/'include/halo2/native_tu15_mopp_ray_slab_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_resume_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_crossing_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_branch_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_plane_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_primitive_key_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_scale_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_integer_program_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_traversal_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_projection_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_traversal.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_setup.hpp',ROOT/'tests/native_tu15_mopp_ray_traversal_tests.cpp',Path(__file__),ROOT/'tools/observe_native_constraint_step_tu15.cpp',ROOT/'tools/validate_native_island_agent_phase_tu15.py']
 report['source_before']={str(f.relative_to(ROOT)):sha(f)for f in files}
 try:
  if not a.xbe.is_file():report['skipped']='Original TU1.5 reference image is unavailable';return 77
  assert sha(a.xbe)==NEW_PIN;image=Xbe(a.xbe);meta=metadata(image);rows=cases(image)
  if a.pattern:rows=[r for r in rows if a.pattern in r['name']]
  if a.limit:rows=rows[:a.limit]
  def boundary(c,o):
   if o['actual_stop']==physical.STOP:return None
   owner=o['actual_stop'];assert owner in BOUNDARIES
   gp,args=o['boundary_gp'],o['boundary_stack']
   return dict(owner=owner,receiver=gp[1],key=args[1]if owner==BOUNDARIES[0]else args[4],ray=None if owner==BOUNDARIES[0]else args[2],collection=gp[1]if owner==BOUNDARIES[0]else args[3])
  physical.boundary=boundary
  original=physical.observer_source
  def observer_source():
   text=original().replace('user_regs_struct regs{};unsigned count=0;','J executed=J::array();user_regs_struct regs{};unsigned count=0;').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"banks",banks},{"actual_stop"','{"executed_sites",executed},{"banks",banks},{"actual_stop"')
   return text
  physical.observer_source=observer_source;rows=physical.physical(image,meta,rows,scratch)
  prefixes=[]
  for seed in(r for r in rows if r['name'].startswith('fault_seed')):
   seen=set();occurrences={}
   for e in seed['reads']:
    p,n=e['address'],e['bytes']
    if VARS<=p<VARS+32 or BOUNDS<=p<BOUNDS+32:continue
    key=p,n;occurrences[key]=occurrences.get(key,0)+1
    if e['instruction']in seen:continue
    seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_read_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=p,bytes=n,occurrence=occurrences[key]));prefixes.append(c)
  save(a.output/'fault-prefix-inputs.json',[dict(name=r['name'],stop_pc=r['stop_pc'],fault=r['fault'])for r in prefixes]);rows+=physical.physical(image,meta,prefixes,scratch)
  fixture=scratch/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_mopp_ray_traversal.cpp';source_text=expanded_source();(project/'native.cpp').write_text(source_text);shutil.copyfile(ROOT/'tests/native_tu15_mopp_ray_traversal_tests.cpp',project/'driver.cpp')
  variants={}
  for name,needle,replacement in [('scale','const auto scale_integer=1u<<opcode','const auto scale_integer=1u<<(opcode+1u)'),('base','key+=variables.fields10_1c[2]','key+=variables.fields10_1c[2]+1u'),('clip','if(d.reverse!=bool(d.clipping))bounds.start=value;else bounds.end=value','if(d.reverse==bool(d.clipping))bounds.start=value;else bounds.end=value'),('projection_sign','{{1,2,0},2,{false,true, false}}','{{1,2,0},2,{false,false,false}}'),('provider_fp','environment_.io([&]{memory_.read(p,std::span(b).first(width));});','memory_.read(p,std::span(b).first(width));'),('encoded_order','d.encoded=(d.encoded<<8u)+d.value','d.encoded=(d.encoded>>8u)+d.value'),('primitive_key','d.encoded=(d.encoded<<8u)+value','d.encoded=(d.encoded<<7u)+value'),('plane_boundary','triple?3*(value-bias):2*value-bias','triple?4*(value-bias):2*value-bias'),('branch_direction','if(start_high.less())','if(!start_high.less())'),('crossing_fraction','frame.reverse_divide(frame.scalar(Scalar::fraction_numerator,d));','frame.reverse_divide(frame.scalar(Scalar::fraction_end,d));'),('resume_direction','if(increasing?relation.less():relation.greater())return true;','if(increasing?relation.greater():relation.less())return true;')]:
   assert needle in source_text;variants[name]=source_text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(MoppTraversal LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+='add_executable(mopp_'+name+' $<TARGET_OBJECTS:driver> '+name+'.cpp)\ntarget_include_directories(mopp_'+name+' PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(mopp_'+name+' PRIVATE cxx_std_20)\ntarget_compile_options(mopp_'+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=(''if name=='native'else'"'+sys.executable+'" "'+str(project/'negative.py')+'" ')+'$<TARGET_FILE:mopp_'+name+'> "'+str(fixture)+'"';cmake+='add_test(NAME mopp_'+name+' COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- '+command+')\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--jobs','2','--test-jobs','1','--filter','^mopp_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   for name in('native',*variants):cmd+=['--target','mopp_'+name]
   r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode,command=cmd))
  executed=set().union(*(set(r['executed_sites'])for r in rows));sites={r['instruction']for r in meta['sites']}
  report['source_after']={str(f.relative_to(ROOT)):sha(f)for f in files};assert report['source_before']==report['source_after'],'Ray proof inputs changed during validation'
  report.update(native_provider_modes=['ordinary','deliberately-clobbered-x87-and-mxcsr'],native_comparisons_per_compiler=2*len(rows),native_provider_exception_fp_preservation=True)
  report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,cases=len(rows),full_returns=sum(r['boundary']is None and not r.get('fault')for r in rows),read_fault_prefixes=len(prefixes),negative_controls=list(variants),real_first_child_boundaries=sum(r['boundary']is not None for r in rows),physically_executed_parent_sites=len(sites&executed),unexecuted_parent_sites=sorted(sites-executed),source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},original_sha256=NEW_PIN,listing_sha256=meta['listing_sha256'],instruction_patches=[],supplied_child_results=False,typed_arithmetic_projection='All32 variable and32 input/output bounds bytes; nativecomputes currentoutputs, neverconsumes originalafter; no savedCPU/control bytes are inputs.',remaining=['Malformed MOPP diagnostic domain and private arithmetic input-address faults','Returned native filter/shape/collector services and actual30B890 caller composition','Whole16C620 query and visible multiplayer camera acceptance'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
