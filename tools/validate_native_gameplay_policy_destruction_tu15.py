#!/usr/bin/env python3
"""Qualify gameplay-policy destruction against unchanged physical TU1.5.

Allocator and base-lock calls stop at genuine entries; no release outcome is supplied.
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
BASE,WORLD,ISLAND=0x1000000,0,0
POLICY,ALLOCATOR,TABLE=BASE+0x500,BASE+0x1000,BASE+0x2000
ENTRIES={0x1d9c80:24}
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
 assert image.read(0x8fdb0,3)==bytes.fromhex('c20800')
 return dict(sites=sites,body_sites=list(ENTRIES.values()),listing_sha256=hashes,stop=reference.STOP)
def cases(image):
 rows=[]
 def make(flags,disposing,seed=False):
  raw=bytearray(0x4000);glob=bytearray(0x1000)
  def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(POLICY,0x12345678);put(POLICY+4,flags,2);put(POLICY+6,1,2)
  put(ALLOCATOR,TABLE);put(TABLE+0x14,0x149e00);struct.pack_into('<I',glob,0x938,ALLOCATOR)
  n=len(rows);gp=[0x5a010000+n*31+i*7 for i in range(8)];gp[4]=reference.SP+(n%4)*4;gp[1]=POLICY
  depth=(0,1,3,8)[n%4];live=[struct.pack('<QH',0x8000000000000000+(i<<58),0x3fff+i%3|(0x8000 if i%2 else 0)).hex()for i in range(depth)]
  c=dict(id=n,name=('fault_seed_'if seed else'policy_')+str(n),entry=0x1d9c80,world=0,island=0,body=POLICY,arguments=[disposing],gp=gp,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x48c000,before=glob.hex())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=live,alignment_fault=False)
  if flags&0x8000:c['required_owner']=dict(owner=0x2e12a0,argument_count=1)
  elif disposing&1:c['required_owner']=dict(owner=0x149e00,argument_count=3)
  rows.append(c)
 for flags in(0,0x60,0x7fff,0x8000,0x8060,0xffff):
  for disposing in(0,1,2,0xffffffff):make(flags,disposing)
 make(0x60,0,True);make(0x60,1,True);make(0xffff,1,True)
 return rows

def prepare(image,meta,scratch):
 owned=activation.prepare(image,meta,scratch)
 p=owned/'reference.s';s=p.read_text();assert s.count('.section .stack,"aw"')==1;p.write_text(s.replace('.section .stack,"aw"','.section .allocator,"aw"\n.space 4096\n.section .stack,"aw"'))
 p=owned/'link.ld';s=p.read_text();assert s.count('.stack 0x')==1;p.write_text(s.replace('.stack 0x','.allocator 0x48c000:{*(.allocator)} .stack 0x'))
 for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')]):subprocess.run(command,check=True,capture_output=True)
 return owned
def driver():
 text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({24})')
 start=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',start)
 text=text[:start]+r'''NativeTu15GameplayPolicyDestructionServices services;J required=nullptr;std::uint32_t returned{};
   services.release_base_lock=[&](auto policy,auto flags,auto&){required=J{{"owner",0x2e12a0},{"receiver",policy},{"arguments",J::array({flags})}};throw OwnerStop{};};
   services.allocation.release=[&](const auto&request,auto&){required=J{{"owner",request.owner},{"receiver",request.receiver},{"arguments",J::array({request.data,request.bytes,request.memory_class})}};throw OwnerStop{};};
'''+text[end:]
 start=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',start)
 text=text[:start]+'returned=native_tu15_destroy_gameplay_body_policy(m,body.identity,test.at("arguments")[0],fp,services,observer);\n'+text[end:]
 start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start);text=text[:start]+text[end:]
 text=text.replace('  }++cases;', '   if(test.at("completed").get<bool>())check(returned==test.at("boundary_gp")[0].get<unsigned>()&&test.at("boundary_gp")[4].get<unsigned>()==test.at("gp")[4].get<unsigned>()+8u,name+" original return differs");\n  }++cases;')
 return text
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 source=ROOT/'src/physics/native_tu15_gameplay_policy_destruction.cpp';files=[ROOT/'src/physics/native_tu15_activation_services.cpp',ROOT/'include/halo2/native_tu15_activation_services.hpp',source,ROOT/'include/halo2/native_tu15_gameplay_policy_destruction.hpp',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch);rows=reference.physical(owned,cases(image),'complete');faults=[]
  for seed in [c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'listeners.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_gameplay_policy_destruction.hpp"\n#include "halo2/native_tu15_activation_services.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in (('wrong_mask','flags&0x7fffu','flags&0x3fffu'),('wrong_table','0x45edf0u','0x45eddcu'),('wrong_class','0x26u','0x12u')):
   assert text.count(needle)>0,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects mutation:",r.stderr.strip()[:300])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeGameplayPolicyDestruction LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\n'
  for name in ('native',*variants):
   cmake+=f'add_executable(gameplay_policy_{name} $<TARGET_OBJECTS:driver> {name}.cpp "{ROOT}/src/physics/native_tu15_activation_services.cpp")\ntarget_include_directories(gameplay_policy_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(gameplay_policy_{name} PRIVATE cxx_std_20)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:gameplay_policy_{name}> "{fixture}"';cmake+=f'add_test(NAME gameplay_policy_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','gameplay_policy_native',*sum((['--target','gameplay_policy_'+n]for n in variants),[]),'--filter','^gameplay_policy_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(reference.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),required_owner_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_snapshot_unchanged=unchanged,negative_controls=list(variants),image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],remaining=['Genuine custom callback return composition and mutable callback cohorts','Full visible multiplayer binding','Unmasked platform exceptions'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
