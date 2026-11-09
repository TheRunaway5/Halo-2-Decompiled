#!/usr/bin/env python3
"""Actual source-computed MOPP arithmetic entry versus original/native core.

The entry receipt contains ordinary initialized DATA, not captured GP inputs.
Original and native are independently invoked on that DATA and all captured
banks. Unwritten cache padding is explicitly declared poison, never inferred
from the offline ELF. Native30B890 caller composition remains a separate proof.
"""
import argparse,hashlib,inspect,json,lzma,os,shutil,struct,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
import validate_native_mopp_ray_traversal_tu15 as component
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
CACHE,VARS,BOUNDS=base.UI+0x3000,base.UI+0x3100,base.UI+0x3200
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_GP_or_after_consumed=False,captured_bank_exclusions=[])
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads(a.archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-camera-query-typed-before-v1'and not capture['physics_result_supplied']and not capture['guest_abi_fabricated']
  hashes=json.loads(lzma.decompress(a.archive.with_name('all-file-hashes.json.xz').read_bytes()));wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=str(Path(member.name));name=name[2:]if name.startswith('./')else name
    if name not in wanted:continue
    r=wanted[name];raw=tar.extractfile(member).read();proof=hashes.get('./'+name,hashes.get(name));assert len(raw)==r['size']and hashlib.sha256(raw).hexdigest()==proof['sha256'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);captured_bytes=sum(map(len,banks.values()));image=Xbe(NEW_DEFAULT)
  def ensure(p,n):
   if not any(first<=p and p+n<=first+len(raw)for first,raw in banks.items()):banks[p]=image.read(p,n)
  for at,n in((0x41c500,16),(0x46a0d0,4),(0x46a0e8,4)):ensure(at,n)
  dispatch_tables=[]
  for address,size in((0x30b738,108),(0x30b6b8,32*4)):
   original_bytes=image.read(address,size);candidates=[raw[address-first:address-first+size]for first,raw in banks.items()if first<=address and address+size<=first+len(raw)]
   assert len(candidates)==1 and candidates[0]==original_bytes,'Captured immutable dispatch CODE differs from authenticated XBE'
   dispatch_tables.append(dict(address=address,bytes=size,sha256=hashlib.sha256(original_bytes).hexdigest()))
  before_path=ROOT/'analysis/physics/native-camera-query-actual1477-mopp-entry-20261102-eighth/report.json';after_path=ROOT/'analysis/physics/native-camera-query-actual1477-mopp-first-leaf-20261102-second/report.json';before=json.loads(before_path.read_text());after=json.loads(after_path.read_text());assert before['passed']and after['passed'];rows=[]
  for mode,r in enumerate(before['actual_boundaries']):
   typed=r['typed_data'];expected=after['actual_boundaries'][mode];assert expected['watched_cache40_reads']==[]if 'watched_cache40_reads'in expected else True
   values={offset:0xa5a5a5a5 for offset in range(0,0x68,4)}
   for offset,value in typed['context_fields'].items():values[int(offset)]=value
   for offset,field in((0x20,'origin_hex'),(0x30,'endpoint_hex')):
    for n,value in enumerate(struct.unpack('<4I',bytes.fromhex(typed[field]))):values[offset+n*4]=value
   values[0x50]=(values[0x50]&0xffffff00)|typed['hit_byte'];values[0x40]=0xc0def00d
   words=[(CACHE-base.UI+off,value)for off,value in sorted(values.items())]
   for address,key in((VARS,'quantization_state_hex'),(BOUNDS,'axis_bounds_hex')):
    words.extend((address-base.UI+n*4,value)for n,value in enumerate(struct.unpack('<8I',bytes.fromhex(typed[key]))))
   rows.append(dict(name=f'actual_mopp_arithmetic_{mode}',entry=0x30aa70,receiver=CACHE,arguments=[VARS,2417268816,BOUNDS],variables=VARS,bounds=BOUNDS,code=2417268816,user_words=words,before_fp_input=r['fp'],expected_after_bounds=expected['typed_data']['axis_bounds_hex'],expected_after_fp=expected['fp']))
  def metadata(image,banks,implicit_stack=False):
   assert not implicit_stack,'This typed DATA cohort does not admit a retained CPU stack'
   meta=component.metadata(image);meta['regions']=[dict(address=p,bytes=len(raw))for p,raw in sorted(banks.items())];return meta,dict(original_sha256=NEW_PIN,listing_sha256=meta['listing_sha256'],instruction_patches=[])
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('if(pc==stop)break;','if(pc==stop)break;').replace('count<1000000','count<1000000');(scratch/'observer.cpp').write_text(observer)
  original=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp")
  env=dict(base.__dict__,metadata=metadata);exec(original,env);fixture,authority=env['original'](image,banks,rows,scratch)
  for row in rows:
   assert row['after_fp']==row['expected_after_fp'],(row['name'],row['after_fp'],row['expected_after_fp'])
   ui=next(b for b in row['banks']if b['address']==base.UI);raw=(fixture/ui['after_file']).read_bytes();assert raw[BOUNDS-base.UI:BOUNDS-base.UI+32].hex()==row['expected_after_bounds']
  save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_mopp_ray_traversal.cpp',project/'native.cpp');shutil.copyfile(ROOT/'tests/native_tu15_mopp_ray_traversal_actual_tests.cpp',project/'driver.cpp')
  (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(actual_mopp LANGUAGES CXX)\ninclude(CTest)\nadd_executable(actual driver.cpp native.cpp)\ntarget_include_directories(actual PRIVATE include)\ntarget_compile_features(actual PRIVATE cxx_std_20)\ntarget_compile_options(actual PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME actual_mopp COMMAND actual "'+str(fixture/'original.json')+'")\n');results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','actual','--filter','^actual_mopp$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];rc=subprocess.run(cmd).returncode;results.append(dict(compiler=compiler,exit_code=rc));assert rc==0,'Native actual MOPP differs'
  report.update(passed=True,archive_sha256=a.archive_sha256,captured_before_bytes=captured_bytes,profiles=4,results=results,source_authority=authority,source_reads=sum(len(r['reads'])for r in rows),source_stores=sum(len(r['writes'])for r in rows),immutable_dispatch_CODE_tables=dispatch_tables,dispatch_projection='Only the two authenticated opcode→class→handler CODE fetch instructions30AAA0/30AAA7 are represented by the native C++ asset-opcode switch. Every captured bank remains compared without exclusions.',typed_entry_receipt=dict(path=str(before_path.relative_to(ROOT)),sha256=sha(before_path)),typed_return_receipt=dict(path=str(after_path.relative_to(ROOT)),sha256=sha(after_path)),declared_unwritten_cache40_poison=0xc0def00d,full_parent_caller_composition_qualified=False,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'src/physics/native_tu15_mopp_ray_traversal.cpp',ROOT/'include/halo2/native_tu15_mopp_ray_traversal.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_slab_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_resume_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_crossing_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_branch_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_plane_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_primitive_key_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_scale_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_integer_program_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_traversal_kernel.hpp',ROOT/'include/halo2/native_tu15_mopp_ray_projection_kernel.hpp',ROOT/'tests/native_tu15_mopp_ray_traversal_actual_tests.cpp',Path(__file__))})
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
