#!/usr/bin/env python3
"""Physical original2E8120/306170 composition, controlled inputs and prefixes.

Known children execute complete native semantic kernels. All original code
remains unchanged. No child result, source output or admission is supplied.
"""
import argparse,copy,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_shape_bounds_tu15 as reference
ROOT=reference.ROOT
NEW=(0x2e8120,0x306170)
COMPOUND,LIST,ENDPOINT=reference.BASE+0xc00,reference.BASE+0xd00,reference.BASE+0xe00

def metadata(image):
 result=reference.metadata(image);code=reference._Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
 for entry in NEW:
  body=reference.complete_body(code,entry);result['body_sites'].append(len(body.code));listing=[]
  for p in sorted(body.code):
   i=code.at(p);listing.append(f'{p:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment;access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;b,idx=i.reg_name(op.mem.base),i.reg_name(op.mem.index)
    ops.append(dict(base=names[b]if b else-1,index=names[idx]if idx else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   result['sites'].append(dict(instruction=p,bytes=i.bytes.hex(),operands=ops))
  result['listing_sha256'][hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert result['body_sites']==[48,13,25,3,48,40,54,50];return result

def inputs(image):
 rows=[];seeds=[c for c in reference.cases(image)if c['entry']==0x306760 and not c['alignment_fault']]
 for seed in seeds:
  for entry in NEW:
   c=copy.deepcopy(seed);c.update(id=len(rows),entry=entry,name=f'{entry:x}_'+c['name']);c['shape']=reference.CONVEX if entry==0x2e8120 else COMPOUND;c['gp'][1]=c['shape'];bank=next(b for b in c['banks']if b['address']==reference.BASE);raw=bytearray.fromhex(bank['before'])
   def put(p,v):struct.pack_into('<I',raw,p-reference.BASE,v)
   put(ENDPOINT,0x461518);put(COMPOUND,0x461de0);put(COMPOUND+12,LIST);put(COMPOUND+16,3);put(LIST,reference.CONVEX);put(LIST+8,ENDPOINT);put(LIST+16,reference.CONVEX)
   bank['before']=raw.hex()
   if entry==0x306170:
    scratch=((c['gp'][4]-4)&~15)-0x20;c['banks'].append(dict(address=scratch,before=bytes(range(32)).hex()))
   rows.append(c)
 for count in(0,1,2,0xffffffff):
  c=copy.deepcopy(next(c for c in rows if c['entry']==0x306170));c.update(id=len(rows),name='compound_count_'+str(count));bank=next(b for b in c['banks']if b['address']==reference.BASE);raw=bytearray.fromhex(bank['before']);struct.pack_into('<I',raw,COMPOUND+16-reference.BASE,count);bank['before']=raw.hex();rows.append(c)
 for entry in NEW:
  c=copy.deepcopy(next(c for c in rows if c['entry']==entry));c.update(id=len(rows),name=f'output_alignment_{entry:x}',output=reference.OUTPUT+4,alignment_fault=True);c['arguments'][-1]=c['output'];rows.append(c)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_compound_shape_bounds.cpp';header=ROOT/'include/halo2/native_tu15_compound_shape_bounds.hpp';files=[source,header,ROOT/'src/physics/native_tu15_shape_bounds.cpp',ROOT/'src/physics/native_tu15_geometry.cpp',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[])
 def save():(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 save()
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);reference.CONSTANTS=(*reference.CONSTANTS,(0x461518,0x1c),(0x461de0,0x1c));meta=metadata(image);owned=reference.prepare(image,meta,scratch);rows=reference.geometry.physical(owned,inputs(image),'complete');prefix=[];excluded=[]
  for seed in(c for c in rows if 'fault_seed'in c['name']):
   for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     if phase=='before_read'and event['instruction']in(0x2e812f,0x2e8135,0x2e8138,0x2e813c,0x306768):excluded.append(dict(seed=seed['name'],instruction=event['instruction'],reason='source private unbalanced radius intermediate remains outside typed FP payload transport'));continue
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(c)
  rows+=reference.geometry.physical(owned,prefix,'prefix');fixture=scratch/'compound.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();test=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text();test=test.replace('#include "halo2/native_tu15_shape_bounds.hpp"','#include "halo2/native_tu15_compound_shape_bounds.hpp"');test=test.replace('J::array({48,13,25,3,48,40})','J::array({48,13,25,3,48,40,54,50})');needle='case 0x306690:';assert test.count(needle)==1;test=test.replace(needle,'case 0x2e8120:native_tu15_endpoint_shape_bounds(m,shape,transform,margin,out,fp,depth,observer);break;case 0x306170:native_tu15_compound_shape_bounds(m,shape,transform,margin,out,{((sp-4u)&~15u)-0x20u},fp,services,observer);break;'+needle);needle='owner==0x2e8870u?';assert test.count(needle)==1;test=test.replace(needle,'owner==0x306170u?aligned-0x40u:'+needle);test=test.replace('alignment=message==','alignment=message=="Native TU1.5 compound bounds requires actual aligned vector storage"||message==');(project/'test.cpp').write_text(test);shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={};needle='maximum(endpoints[0],endpoints[1])';assert text.count(needle)==1;variants['endpoint_order']=text.replace(needle,'maximum(endpoints[1],endpoints[0])');needle='maximum(current_maximum,child_maximum)';assert text.count(needle)==1;variants['compound_order']=text.replace(needle,'maximum(child_maximum,current_maximum)')
  for name,body in variants.items():(project/(name+'.cpp')).write_text(body)
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in("shape store differs","retained shape bytes differ","shape FP differs","ordered shape read differs")),(r.returncode,r.stdout,r.stderr)\nprint("Physical original rejects bounds operand-order mutation:",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeCompoundBounds LANGUAGES CXX)\ninclude(CTest)\n';cmake+=f'add_library(driver OBJECT test.cpp)\ntarget_include_directories(driver PRIVATE "{ROOT}/include")\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for variant in('native',*variants):
   cmake+=f'add_executable(bounds_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp "{ROOT}/src/physics/native_tu15_shape_bounds.cpp" "{ROOT}/src/physics/native_tu15_geometry.cpp")\ntarget_include_directories(bounds_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(bounds_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(bounds_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:bounds_{variant}> "{fixture}"';cmake+=f'add_test(NAME bounds_{variant} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','bounds_native',*sum((['--target','bounds_'+n]for n in variants),[]),'--filter','^bounds_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(reference.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],complete_original_calls=sum(c['completed']for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_snapshot_unchanged=unchanged,negative_controls=list(variants),excluded_private_radius_faults=excluded,supplied_original_outcomes=False,instruction_patches=[],live_activation_proven=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save()
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
