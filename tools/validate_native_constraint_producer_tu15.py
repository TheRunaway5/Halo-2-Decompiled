#!/usr/bin/env python3
"""Full unchanged TU1.5 producer -> base -> schema-emitter composition.

The original performs every nested owner itself. Native services compose the
typed base and emitter, with no supplied rows, cursor, property or FP result.
All binaries and declared input/output banks have run_test.py ownership.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_constraint_base_tu15 as base
import validate_native_geometry_tu15 as geometry

OWNERS={0x31f770:57,0x31f860:125,0x325ef0:15,0x1d7630:20,0x3296e0:20,0x329720:921,0x32a390:15}
PROPERTIES,RECORDS,FACTORS=0x1000800,0x1000900,0x1000a00
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');pcs=set();counts={};names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for owner,count in OWNERS.items():
  body=complete_body(code,owner);assert len(body.code)==count;pcs.update(body.code);counts[hex(owner)]=count
 assert len(pcs)==1173
 for pc in sorted(pcs):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment;access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE);b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index);ops.append(dict(base=names[b]if b else-1,index=names[idx]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
  sites.append(dict(instruction=pc,bytes=i.bytes.hex(),operands=ops))
 return dict(sites=sites,stop=geometry.STOP,body_sites=counts,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
def cases(image):
 out=[];seeds=base.cases(image)
 def setup(seed,entry,property_kind='late',scale=(0x3f000000,0x3f800000)):
  c=copy.deepcopy(seed);c.pop('fault',None);c.update(id=len(out),name=hex(entry)+'_'+seed['name']+'_property_'+property_kind,entry=entry,arguments=[base.INPUT,base.BUILDER]);c['gp'][1]=base.FRAME
  def find(p,n):
   for bank in c['banks']:
    if bank['address']<=p and p+n<=bank['address']+len(bank['before'])//2:return bank
   raise AssertionError(hex(p))
  def word(p,v):
   bank=find(p,4);raw=bytearray.fromhex(bank['before']);struct.pack_into('<I',raw,p-bank['address'],v&0xffffffff);bank['before']=raw.hex()
  frame=find(base.FRAME,64);raw=bytearray.fromhex(frame['before']);raw[0x50:0x90]=raw[:64];frame['before']=raw.hex()
  for p,v in((base.FRAME+0xc,PROPERTIES),(base.FRAME+0x34,base.POINTS),(base.FRAME+0x38,seed['arguments'][1]),(base.FRAME+0x40,scale[0]),(base.FRAME+0x44,scale[1]),(base.FRAME+0x90,base.RUNTIME)):word(p,v)
  for address,size in((PROPERTIES,128),(RECORDS,64),(FACTORS,64)):c['banks'].append(dict(address=address,before=bytes(size).hex()))
  word(PROPERTIES+0x30,RECORDS);word(PROPERTIES+0x34,0xffffffff if property_kind=='negative'else 0 if property_kind=='none'else 3)
  for index in range(3):word(RECORDS+index*8,0x2003 if property_kind=='first'and index==0 or property_kind in('late','duplicate')and index==2 or property_kind=='duplicate'and index==0 else 0x1000+index);word(RECORDS+index*8+4,FACTORS+index*16)
  for index,bits in enumerate((0x3f000000,0xbf800000,0x3eaaaaab,0x40000000)*3):word(FACTORS+index*4,bits)
  c['banks'].sort(key=lambda b:b['address']);out.append(c)
 for entry in(0x31f770,0x31f860):
  for seed in seeds:
   if seed.get('fault')or seed['name'].startswith(('fault_seed','alias_')):continue
   setup(seed,entry)
  for kind in('none','missing','first','duplicate','negative'):
   for seed in(seeds[2],next(s for s in seeds if s['name']=='aggregate_2_1_4')):setup(seed,entry,kind)
  seed=next(s for s in seeds if s['name']=='alias_both_bodies');setup(seed,entry)
  for scale in((0,0x80000000),(0x3fe00000,0xbe800000),(0x7fa00021,0x7fc00341)):setup(seeds[2],entry,scale=scale)
  setup(next(s for s in seeds if s['name']=='aggregate_2_1_0'),entry);out[-1]['name']='fault_seed_'+hex(entry)
 for entry in(0x325ef0,0x3296e0):setup(seeds[2],entry)
 return out
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);owned=base.prepare(image,meta,scratch);rows=geometry.physical(owned,cases(image),'complete');prefix=[]
 for seed in(c for c in rows if c['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for index in range(len(events)):
    c=copy.deepcopy(seed);c.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(index),fault=dict(phase=phase,ordinal=index));prefix.append(c)
 rows+=geometry.physical(owned,prefix,'prefix');fixture=scratch/'producer.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,body_sites=1173,cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_constraint_producer.cpp';text=source.read_text();(project/'native.cpp').write_text(text);variants={}
 for name,old,new in(('factors','multiply(m.word(factors+n*4u))','multiply(m.word(factors))'),('restore','(n<4?first:second)+48u+(n%4u)*4u','(n<4?first:second)+48u'),('property','==0x2003u','==0x2004u')):
  assert text.count(old)==1;variants[name]=text.replace(old,new);(project/(name+'.cpp')).write_text(variants[name])
 (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in("base ordered store differs","base retained bytes differ","base complete FP differs","base read differs","base store count differs")),(r.returncode,r.stdout,r.stderr)\nprint("Physical original rejects producer mutation:",r.stderr.strip())\n');cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_producer LANGUAGES CXX)
include(CTest)
add_library(producer_driver OBJECT "{ROOT}/tests/native_tu15_constraint_producer_tests.cpp")
target_include_directories(producer_driver PRIVATE "{ROOT}/include")
target_compile_features(producer_driver PRIVATE cxx_std_20)
target_compile_options(producer_driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
 for name in('native',*variants):
  target=name+'_producer';cmake+=f'add_executable({target} $<TARGET_OBJECTS:producer_driver> "{project/(name+".cpp")}" "{ROOT}/src/physics/native_tu15_constraint_base.cpp" "{ROOT}/src/physics/native_tu15_constraint_schema_emitter.cpp")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';cmd=f'$<TARGET_FILE:{target}> "{fixture}"'if name=='native'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {cmd})\n'
 (project/'CMakeLists.txt').write_text(cmake);results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target',name+'_producer']for name in('native',*variants)),[]),'--filter','_producer$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=[source,ROOT/'include/halo2/native_tu15_constraint_producer.hpp',ROOT/'src/physics/native_tu15_constraint_base.cpp',ROOT/'include/halo2/native_tu15_constraint_base.hpp',ROOT/'src/physics/native_tu15_constraint_schema_emitter.cpp',ROOT/'include/halo2/native_tu15_constraint_schema_emitter.hpp',ROOT/'tests/native_tu15_constraint_producer_tests.cpp',ROOT/'tools/validate_native_constraint_base_tu15.py',Path(__file__)];report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('phase')=='before_read'for c in rows),rejected_store_prefixes=sum(c.get('fault',{}).get('phase')=='before_write'for c in rows),completed_observer_prefixes=sum(c.get('fault',{}).get('phase')=='after_write'for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),source_hashes={str(p.relative_to(ROOT)):sha(project/'native.cpp'if p==source else p)for p in files},owned_scratch=str(scratch),fixture_bytes=fixture.stat().st_size,temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[],native_child_outcomes_supplied=False,remaining=['Private transient x87 payload on exceptional return','Unmasked FP platform exceptions','Ordinary immutable value/private local ABI excludes guest control-frame aliases','Production live activation']);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
