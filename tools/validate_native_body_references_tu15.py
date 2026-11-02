#!/usr/bin/env python3
"""Physical original147-site body reference compaction qualification.

Reserve stops at the actual2E31E0 entry. No array/result bytes are supplied.
All generated code, operand banks and compiler builds belong to run_test.py.
"""
import argparse,copy,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,BODY,ARRAY=0x1000000,0x1000100,0x1001000
ENTRIES={0x3174a0:53,0x3173c0:47,0x317430:47}
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
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 return dict(sites=sites,body_sites=list(ENTRIES.values()),listing_sha256=hashes,stop=reference.STOP)
def cases(image):
 rows=[]
 def make(entry,pattern,capacity=16,count=None,seed=False):
  raw=bytearray((n*23+5)&255 for n in range(0x4000))
  def put(p,v):struct.pack_into('<I',raw,p-BASE,v&0xffffffff)
  primary=0 if entry==0x3174a0 else(1 if entry==0x3173c0 else 2)
  for cohort,offset in enumerate((0x7c,0x88,0x94)):
   values=pattern if cohort==primary else(0,0x5a800000+cohort,0,0x6b900000+cohort)
   put(BODY+offset,ARRAY+cohort*0x100);put(BODY+offset+4,len(values)if count is None or cohort!=primary else count);put(BODY+offset+8,capacity if cohort==primary else 0x80000010)
   for n,value in enumerate(values):put(ARRAY+cohort*0x100+n*4,value)
  n=len(rows);depth=(0,1,3,8)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=BODY;gp[4]=reference.SP
  row=dict(id=n,name=('fault_seed_'if seed else'cohort_')+f'{entry:x}_{n}',entry=entry,world=0,island=0,body=BODY,arguments=[],gp=gp,before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)|((0,0x40,0x8000,0x8040)[n%4])],live_x87=live,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x41d000,before=image.read(0x41d000,0x1000).hex()),dict(address=0x556000,before=bytes(0x1000).hex())],alignment_fault=False)
  if count is None and len([v for v in pattern if v])>(capacity&0x7fffffff):row['required_owner']=dict(owner=0x2e31e0,argument_count=3)
  rows.append(row)
 patterns=((),(0,),(1,),(1,2,3),(0,1,0,2),(1,0,2,0),(0,0,0,0),(1,2,0,3,0,4),(0,0,5,6,0))
 for entry in ENTRIES:
  for repeat in range(4):
   for pattern in patterns:
    for capacity in(16,0x80000010):make(entry,pattern,capacity)
  for count in(0,0xffffffff,0x80000000,0x80000001):make(entry,(),count=count)
  for capacity in(0,1,2,0x80000002):make(entry,(1,2,3),capacity)
  make(entry,(0,1,0,2),seed=True)
 return rows
def driver():
 text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({53,47,47})');begin=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',begin)
 text=text[:begin]+'''NativeTu15BodyReferenceStats stats;NativeTu15BodyReferenceServices services;J required=nullptr;
   services.reserve_array=[&](auto header,auto capacity,auto stride,auto&){required=J{{"owner",0x2e31e0},{"receiver",0},{"arguments",J::array({header.identity,capacity,stride})}};throw OwnerStop{};};
'''+text[end:];begin=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',begin)
 text=text[:begin]+'''switch(entry){
    case 0x3174a0:native_tu15_compact_body_references(m,body,fp,services,observer,&stats);break;
    case 0x3173c0:native_tu15_compact_body_reference_cohort(m,body,NativeTu15BodyReferenceCohort::second,fp,services,observer,&stats);break;
    case 0x317430:native_tu15_compact_body_reference_cohort(m,body,NativeTu15BodyReferenceCohort::third,fp,services,observer,&stats);break;
    default:throw std::runtime_error("Unexpected actual reference entry");}
'''+text[end:];start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start);return(text[:start]+text[end:]).replace('activation','body-reference').replace('required.at("owner")!=0x2e3250','required.at("owner")!=0x2e31e0')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_body_references.cpp';files=[source,ROOT/'include/halo2/native_tu15_body_references.hpp',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=activation.prepare(image,meta,scratch);rows=reference.physical(owned,cases(image),'complete');faults=[]
  for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'references.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')));project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_body_references.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_copy','m.word(data+index*4u)','m.word(data+(index+1u)*4u)'),('wrong_count','header+4u,compacted','header+4u,compacted+1u'),('wrong_stride','s.reserve_array({header},requested,4u,fp)','s.reserve_array({header},requested,8u,fp)')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects reference mutation:",r.stderr.strip()[:300])\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeBodyReferences LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\n'
  for name in('native',*variants):
   cmake+=f'add_executable(references_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(references_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(references_{name} PRIVATE cxx_std_20)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:references_{name}> "{fixture}"';cmake+=f'add_test(NAME references_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','references_native',*sum((['--target','references_'+n]for n in variants),[]),'--filter','^references_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(reference.sha(ROOT/f)==v for f,v in pins.items()),compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),required_owner_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],remaining=['Native reserve return and mutable observer composition','Full visible multiplayer binding','Unmasked platform exceptions'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
