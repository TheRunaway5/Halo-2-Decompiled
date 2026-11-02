#!/usr/bin/env python3
"""Native2E5320 with real type/getter/inverse bodies, stopped at real children.

Declared current DATA inputs are separate from actual multiplayer captures.
No original child return, contact outcome, CPU postimage or expected preimage
is installed in native execution. All compiler/raw products are owned.
"""
import argparse,copy,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE=0x1000000
AGENT,FIRST,SECOND,CONFIG,OUTPUT=BASE+0x100,BASE+0x300,BASE+0x340,BASE+0x400,BASE+0x500
SHAPE_A,SHAPE_B,TRANSFORM_A,TRANSFORM_B=BASE+0x600,BASE+0x680,BASE+0x800,BASE+0x900
BODY_A,BODY_B,MOTION_A,MOTION_B=BASE+0xb00,BASE+0xc00,BASE+0xd00,BASE+0xe00
RELATIVE=((physical.SP-4)&~15)-0x70+0x30
OWNERS=(0x2e5320,0x2e4550,0x283380,0xa9520,0xab4b0,0x31dd70)
BOUNDARIES=(0x2f8b60,0x309060,0x309150)
BACKEND='unchanged-tu15-native-moving-shape-process-prefix-v1'
EXECUTED=set()
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
def metadata(image,output):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];pins={}
 for entry in OWNERS:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');ops=[]
   for op in ins.operands:
    if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
    assert not op.mem.segment;base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
    access=CS_AC_WRITE if ins.mnemonic in('fst','fstp')else CS_AC_READ if ins.mnemonic in('test','cmp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),ins.mnemonic,ins.op_str)
    ops.append(dict(base=registers.get(base,-1),index=registers.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops,timestamp=ins.mnemonic=='rdtsc'))
  text=''.join(listing);pins[hex(entry)]=hashlib.sha256(text.encode()).hexdigest();(output/f'{entry:08x}.txt').write_text(text)
 assert counts[0]==274,counts
 return dict(sites=sites,body_sites=counts,listing_sha256=pins,boundaries=list(BOUNDARIES),regions=[])

def cases(image):
 rows=[]
 def make(name,*,classes=(23,24),velocities=(0,0),thresholds=(0x3f800000,0x3f800000),cached=0,parents=0,eligible=True,rounding=0,precision=2,edge_w=0,alias_scale=False):
  banks={BASE:bytearray(0x10000),0x48c974:bytearray(8),0x52be5c:bytearray(4),0x4ecbd0:bytearray(4),0x51daf0:bytearray(4),RELATIVE:bytearray((n*11+len(rows))&255 for n in range(64))}
  def put(at,value,width=4):
   lo=next(lo for lo,b in banks.items()if lo<=at and at+width<=lo+len(b));banks[lo][at-lo:at-lo+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
  def vector(at,words):
   for n,w in enumerate(words):put(at+n*4,w)
  config=SHAPE_A if alias_scale else CONFIG
  put(config+12,BASE+0x1000);put(BASE+0x1000,0x93,1);put(config+16,0x3f000001);put(AGENT+0xf0,cached)
  for index,(payload,shape,transform,body,motion)in enumerate(((FIRST,SHAPE_A,TRANSFORM_A,BODY_A,MOTION_A),(SECOND,SHAPE_B,TRANSFORM_B,BODY_B,MOTION_B))):
   table=BASE+0x1100+index*0x40;put(payload,shape);put(shape,table);put(table+0x14,{23:0xa9520,24:0xab4b0,5:0x31dd70}[classes[index]])
   put(payload+8,transform);put(shape+0x20,thresholds[index]);put(payload+0x20,body)
   for depth in range(parents):
    parent=BASE+0x1200+index*0x100+depth*0x30;put(payload+0xc if depth==0 else parent-0x30+0xc,parent);put(parent+0x20,body)
   put(body+0x3c,motion);vector(motion+0x40,(velocities[index],0x3e000001,0xbe800001,edge_w))
   put(body+0x34,1);put(body+0x30,BASE+0x1600+index*8);put(BASE+0x1600+index*8,0x2001);put(BASE+0x1604+index*8,0xffffffff if eligible else index)
   basis=((0x3f800000,0x3e000001,0xbe800001,0x3e800001),(0xbe000001,0x3f800000,0x3d800001,0xbe800001),(0x3e800001,0xbe000001,0x3f800000,0x3f000001),(0x3f800000,0xc0000000,0x40400000,0))
   for lane,values in enumerate(basis):vector(transform+0x20+lane*16,values)
  if not eligible:
   put(0x52be5c,BASE+0x2000);put(BASE+0x2044,BASE+0x2100);put(0x4ecbd0,BASE+0x2400);put(BASE+0x2444,BASE+0x2500);put(0x51daf0,BASE+0x3000)
   for n in range(2):put(BASE+0x2100+n*160+8,0x12340000+n);put(BASE+0x2500+n*12+8,BASE+0x2800+n*0x100);put(BASE+0x2800+n*0x100+0xc0,6,2);put(BASE+0x2800+n*0x100+0xb8,0x12345678)
   put(BASE+0x3000+0x1d80,1);put(BASE+0x3000+0x1d00,0x12345678)
  n=len(rows);rows.append(dict(name=name,entry=0x2e5320,receiver=AGENT,arguments=[FIRST,SECOND,config,OUTPUT],relative=RELATIVE,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=physical.STOP,boundary=None))
 for classes in((24,24),(23,24),(24,23),(23,23),(24,5),(23,5)):
  for velocities in((0,0),(0x40000001,0),(0,0x40000001),(0x40000001,0x40000001)):
   for cached in(0,3):make(f'types_{classes}_vel_{velocities}_cache_{cached}',classes=classes,velocities=velocities,cached=cached)
 for parents in(1,2,3):make('parents_'+str(parents),classes=(23,23),velocities=(0x40000001,0x40000001),parents=parents)
 make('property_parent_rejected',classes=(23,23),velocities=(0x40000001,0x40000001),eligible=False)
 make('configuration_scale_alias_shape_vector',classes=(23,23),velocities=(0x40000001,0x40000001),alias_scale=True)
 for precision in(0,2,3):
  for rounding in range(4):
   for values in((0x3f800001,0),(0x40000001,0x40000001),(0x7fc12345,0),(0x7f812345,0)):
    make(f'fp_{precision}_{rounding}_{values}',classes=(23,23),velocities=values,precision=precision,rounding=rounding)
 for value in(0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  make('threshold_'+hex(value),classes=(23,23),velocities=(0x40000001,0x40000001),thresholds=(value,value));make('W_'+hex(value),classes=(23,23),velocities=(0x40000001,0x40000001),edge_w=value)
 for name,classes,velocities in(('clear',(23,23),(0,0)),('motion',(23,23),(0x40000001,0x40000001)),('rejected',(23,23),(0x40000001,0x40000001))):make('fault_seed_'+name,classes=classes,velocities=velocities,eligible=name!='rejected')
 return rows

def observer_source():
 text=physical_original_observer();needle='const auto site=sites.find(pc);';assert text.count(needle)==1
 text=text.replace('user_regs_struct regs{};unsigned count=0;','J executed=J::array();user_regs_struct regs{};unsigned count=0;').replace(needle,'executed.push_back(pc);'+needle)
 return text.replace('{"banks",banks},{"actual_stop"','{"executed_sites",executed},{"banks",banks},{"actual_stop"')
physical_original_observer=physical.observer_source

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--case-filter');p.add_argument('--no-faults',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image,a.output);physical.observer_source=observer_source
  def boundary(c,actual):
   owner=actual['actual_stop'];assert owner in BOUNDARIES,hex(owner)
   return dict(owner=owner,receiver=actual['boundary_gp'][1],arguments=actual['boundary_stack'][1:5 if owner==0x2f8b60 else 4])
  physical.boundary=boundary;declared=cases(image)
  if a.case_filter:declared=[c for c in declared if a.case_filter in c['name']];assert declared
  rows=physical.physical(image,meta,declared,scratch);prefix=[];code=_Code(image,'.text')
  if not a.no_faults:
   for seed in(c for c in rows if c['name'].startswith('fault_seed')):
    for category in('reads','writes'):
     seen=set();occurrences={}
     for e in seed[category]:
      key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occurrences[key]=occurrences.get(key,0)+1
      if e['instruction']in seen:continue
      seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefix.append(c)
    seen=set()
    for n,e in enumerate(seed['writes']):
     if e['instruction']in seen:continue
     seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,observer_fault=n+1);prefix.append(c)
   rows+=physical.physical(image,meta,prefix,scratch)
  for c in rows:EXECUTED.update(c['executed_sites'])
  fixture=scratch/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_moving_shape_process.cpp';text=source.read_text();(project/'native.cpp').write_text(text);shutil.copyfile(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',project/'driver.cpp')
  variants={}
  for name,needle,replacement in(('wrong_vector_order','return add(add(xterm,yterm),zterm)','return add(add(zterm,yterm),xterm)'),('wrong_cache_class','cache_class==5u?','cache_class==24u?'),('wrong_clear','shape+0x10u,_mm_setzero_ps()','shape+0x10u,broadcast(0x3f800000u)')):
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
  units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')
  for name in units:shutil.copyfile(ROOT/'src/physics'/name,project/name)
  shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(MovingShapeProcess LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(moving_{name} $<TARGET_OBJECTS:driver> {name}.cpp '+ ' '.join(units)+f')\ntarget_include_directories(moving_{name} PRIVATE "{project}/include")\ntarget_compile_features(moving_{name} PRIVATE cxx_std_20)\ntarget_compile_options(moving_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options(moving_{name} PRIVATE -Wl,--gc-sections)\n';command=(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:moving_{name}> "{fixture}"';cmake+=f'add_test(NAME moving_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--jobs','1','--test-jobs','1','--filter','^moving_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   for name in('native',*variants):command+=['--target','moving_'+name]
   result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
  files=[source,ROOT/'include/halo2/native_tu15_moving_shape_process.hpp',ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',Path(__file__),*[ROOT/'src/physics'/name for name in units]]
  parent=set(complete_body(code,0x2e5320).code);report.update(passed=all(r['exit_code']==0 for r in results),source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],compiler_results=results,cases=len(rows),genuine_cache_entries=sum(c['boundary']is not None and c['boundary']['owner']in(0x309060,0x309150)for c in rows),genuine_process_entries=sum(c['boundary']is not None and c['boundary']['owner']==0x2f8b60 for c in rows),executed_parent_sites=len(parent&EXECUTED),unexecuted_parent_sites=sorted(parent-EXECUTED),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),negative_controls=list(variants),supplied_original_outcomes=False,declared_inputs_only=True,remaining=['Returned cache and2F8B60 child closure, parent epilogue','Active profiling timestamp nondeterminism','Actual multiplayer owner input/production seam/live proof','Incoming nonempty x87 and unmasked exception domains'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
