#!/usr/bin/env python3
"""Physical original key/channel visibility with native immutable input values."""
import argparse,copy,hashlib,inspect,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE=0x1000000
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');names={s:n for n,s in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
 for owner in(0x1f76a0,0x186e70):
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    # This Capstone version reports access0 for TEST memory,register at
    #1F774E/186EB9. The original instructions read memory and never write it.
    access=CS_AC_READ if i.mnemonic in('test','cmp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=names.get(i.reg_name(op.mem.base),-1),index=names.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[82,27];return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[],boundaries=[])
def cases():
 rows=[]
 def make(name,kind=1,index=1,part=0,channel=0,visible=False,flag=8,bsp=0,geometry_index=0,datum=0xffff0002,object_flag=0,generation=0,channel_owner=False,instance=0xffffffff):
  data=bytearray(0x6000)
  def put(p,v,n=4):
   assert 0<=p-BASE and p-BASE+n<=len(data),(hex(p),n)
   data[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(BASE+0x100+0x2c,BASE+0x200);surface=BASE+0x200+index*8;put(surface+4,flag,1);put(surface+5,channel,1)
  put(BASE+0x400+0x144,BASE+0x800);put(BASE+0x400+0x13c,BASE+0x1000);put(BASE+0x800+index*0x58+0x34,geometry_index,2);put(BASE+0x1000+geometry_index*0xc8+0x9c,BASE+0x1800);instance_surface=BASE+0x1800+part*8
  if part<64:put(instance_surface+4,flag,1);put(instance_surface+5,channel,1)
  put(BASE+0x2000+index*4,datum);put(BASE+0x2400+0x44,BASE+0x2800)
  if datum!=0xffffffff:
   header=BASE+0x2800+(datum&0xffff)*12;put(header+2,object_flag,1);put(header+8,BASE+0x4000);put(BASE+0x4000+0x11a,0x200,2);put(BASE+0x4200+(part&31),generation,1)
  word=(channel>>5)+bsp*8;address=BASE+0x3000+word*4+1
  if kind==5:address=BASE+0x3000+bsp*1024+index+0x201+(channel>>5)*4
  if channel_owner and instance!=0xffffffff:address=BASE+0x3000+bsp*1024+instance+0x201+(channel>>5)*4
  if channel!=0xffffffff:put(address,(1<<(channel&31))if visible else 0)
  key=(kind<<29)|((part&0x1fff)<<16)|index;entry=0x186e70 if channel_owner else 0x1f76a0
  globals=[(0x4ecc10,BASE+0x100,4),(0x4ecc18,BASE+0x400,4),(0x4ecbd0,BASE+0x2400,4),(0x52be70,BASE+0x2000,4),(0x4f9b50,BASE+0x3000,4),(0x474c24,bsp&0xffff,2)]
  banks=[dict(address=BASE,before=data.hex()),*[dict(address=p,before=v.to_bytes(n,'little').hex())for p,v,n in globals]];n=len(rows);rows.append(dict(name=name,entry=entry,receiver=instance if channel_owner else 0,minimum_input=channel if channel_owner else 0,initial_edx=key,key=key,channel=channel,instance=instance,arguments=[],banks=sorted(banks,key=lambda b:b['address']),before_fp=[0x27f|((n%4)<<10),(n%8)<<11,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 for kind in(1,5):
  for channel in(0,1,31,32,63,127,128,255):
   for visible in(False,True):
    for bsp in(0,1):make(f'key_{kind}_{channel}_{visible}_{bsp}',kind=kind,part=7 if kind==5 else 0,channel=channel,visible=visible,bsp=bsp)
 for kind in(1,5):make('flag_disabled_'+str(kind),kind=kind,flag=0);make('negative_bsp_'+str(kind),kind=kind,bsp=-1,visible=True)
 for kind in(3,4):
  for wanted in(0,1,127,128,255):
   for generation in(0,1,127,128,255):make(f'object_{kind}_{wanted}_{generation}',kind=kind,part=(wanted<<5)|31,generation=generation)
  make('object_inactive_'+str(kind),kind=kind,object_flag=0x10);make('object_sentinel_'+str(kind),kind=kind,datum=0xffffffff)
 make('kind2_unconditional',kind=2)
 for channel in(0,1,31,32,255,0xffffffff):
  for instance in(0xffffffff,0,2):
   for visible in(False,True):make(f'channel_{channel:x}_{instance:x}_{visible}',channel_owner=True,channel=channel,instance=instance,visible=visible)
 make('fault_seed_global',channel=255,visible=True);make('fault_seed_instance',kind=5,part=7,channel=63,visible=True);make('fault_seed_object',kind=3,part=(127<<5)|31,generation=127)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);preparer=inspect.getsource(physical.physical);needle=" mov ${c.get('minimum_input',0)},%eax";assert preparer.count(needle)==1;preparer=preparer.replace(needle,needle+"\n mov ${c.get('initial_edx',0)},%edx");exec(preparer,physical.__dict__);physical.boundary=lambda c,o:None;rows=physical.physical(image,meta,cases(),scratch);prefix=[]
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   seen=set();occurrences={}
   for event in seed['reads']:
    key=event['address'],event['bytes'];occurrences[key]=occurrences.get(key,0)+1
    if event['instruction']in seen:continue
    seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_read_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key]));prefix.append(c)
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'visibility.json';save(fixture,dict(image_sha256=NEW_PIN,cases=rows));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_collision_component.cpp';surface=ROOT/'src/physics/native_tu15_collision_surface_shape.cpp';test=ROOT/'tests/native_tu15_collision_component_visibility_tests.cpp';(project/'native.cpp').write_bytes(source.read_bytes());(project/'surface.cpp').write_bytes(surface.read_bytes());text=source.read_text();variants={}
  for name,needle,replacement in(('skip_channel255','const auto channel=m.word(surface+5u,1);','const auto channel=m.word(surface+5u,1);if(channel==255u)return true;'),('wrong_generation_signedness','const auto generation=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(object+offset+(part&31u),1)))));','const auto generation=m.word(object+offset+(part&31u),1);'),('wrong_bitmap_offset','visibility+word*4u+1u','visibility+word*4u')):
   assert text.count(needle)==(2 if name=='skip_channel255'else 1),(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);assert r.returncode!=0 and any(x in r.stderr for x in("visibility result", "visibility ordered read")),(r.returncode,r.stdout,r.stderr)\nprint("Original collision visibility rejects semantic mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_collision_component_visibility LANGUAGES CXX)
include(CTest)
add_library(driver OBJECT "{test}")
add_library(dependencies OBJECT surface.cpp)
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE "{ROOT}/include")
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
'''
  for mode in('native',*variants):
   cmake+=f'add_executable(visibility_{mode} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {mode}.cpp)\ntarget_include_directories(visibility_{mode} PRIVATE "{ROOT}/include")\ntarget_compile_features(visibility_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(visibility_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME visibility_{mode} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:visibility_{mode}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','visibility_native',*sum((['--target','visibility_'+name]for name in variants),[]),'--filter','^visibility_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  files=[source,ROOT/'include/halo2/native_tu15_collision_component.hpp',surface,test,Path(__file__)];report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},original_sha256=NEW_PIN,body_sites=[82,27],listing_sha256=meta['listing_sha256'],physical_cases=len(rows),complete_original_calls=len(rows)-len(prefix),read_fault_prefixes=len(prefix),ordered_reads=sum(len(c['reads'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),input_bootstrap_sha256=hashlib.sha256(preparer.encode()).hexdigest(),input_bootstrap_change='Initialize actual fastcallEDX input in test setup; original instructions unchanged',supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False)
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
