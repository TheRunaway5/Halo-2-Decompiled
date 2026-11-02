#!/usr/bin/env python3
"""Source-derived24DC90 component entry and full native setup/core/lifetime.

Every captured BEFORE byte is strict. Original caller-private allocations are
not retained outputs; the native caller owns independent initialized DATA and
guards the unwritten ray field. No source GP or expected arithmetic is input.
"""
import argparse,hashlib,inspect,json,lzma,os,shutil,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
import validate_native_mopp_ray_traversal_tu15 as core
from observe_native_camera_query_mopp_entry_tu15 import reference_original
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
OWNERS=(0x24dc90,0x2e9790,0x30aa40,0x2e9750,0x2e96a0,0x30b890,0x2e96b0,0x30aa50,0x1f3600)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--stage',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_GP_or_after_consumed=False,captured_bank_exclusions=[])
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN;capture=json.loads(a.archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-camera-query-typed-before-v1'and not capture['physics_result_supplied']and not capture['guest_abi_fabricated'];typed=capture['input'];assert typed['preserved_x87_depth']==0
  hashes=json.loads(lzma.decompress(a.archive.with_name('all-file-hashes.json.xz').read_bytes()));wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=str(Path(member.name));name=name[2:]if name.startswith('./')else name
    if name not in wanted:continue
    r=wanted[name];raw=tar.extractfile(member).read();proof=hashes.get('./'+name,hashes.get(name));assert len(raw)==r['size']and hashlib.sha256(raw).hexdigest()==proof['sha256'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);captured_bytes=sum(map(len,banks.values()));image=Xbe(NEW_DEFAULT)
  def ensure(p,n):
   if not any(first<=p and p+n<=first+len(raw)for first,raw in banks.items()):banks[p]=image.read(p,n)
  for at,n in((0x41c500,16),(0x46a0d0,4),(0x46a0e8,4),(0x46a1bc,4),(0x46a520,4)):ensure(at,n)
  dispatch=[]
  for address,n in((0x30b738,108),(0x30b6b8,128)):
   raw=image.read(address,n);assert any(first<=address and address+n<=first+len(b)and b[address-first:address-first+n]==raw for first,b in banks.items());dispatch.append(dict(address=address,bytes=n,sha256=hashlib.sha256(raw).hexdigest()))
  witness=ROOT/'analysis/physics/native-camera-query-actual1477-first-boundary-20261102-first/report.json';proof=json.loads(witness.read_text());assert proof['passed'];clusters={r['gp'][0]for r in proof['actual_boundaries']};assert clusters=={46};cluster=next(iter(clusters))
  fp=typed['floating_point'];q=dict(cluster=cluster,mask=typed['mask'],bsp_filter=(typed['mask']>>23)&63,origin=typed['origin'],displacement=typed['displacement'],result=typed['result'])
  rows=[dict(name='source_derived_instanced_'+str(mode),entry=0x24dc90,receiver=0,arguments=[q['mask'],q['bsp_filter'],q['result']],request=q,before_fp_input=[(fp['x87_control_word']&~0xc00)|(mode<<10),fp['x87_status'],(fp['mxcsr']&~0x6000)|(mode<<13)])for mode in range(4)]
  base.OWNERS=OWNERS;meta_source=inspect.getsource(base.metadata).replace("assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'","assert not op.mem.segment or ins.reg_name(op.mem.segment)in('es','fs')").replace('read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)',"read=False if ins.mnemonic in('fst','fstp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp')");env=dict(base.__dict__);exec(meta_source,env)
  def metadata(image,banks):
   meta,authority=env['metadata'](image,banks);kernel=core.metadata(image);meta['sites']+=kernel['sites'];meta['regions']=[r for r in meta['regions']if r['address']!=base.UI];authority['core_listing_sha256']=kernel['listing_sha256'];return meta,authority
  original=reference_original().replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp")
  original=original.replace(" mov ${case['receiver']},%ecx\n int3",f" mov ${{case['receiver']}},%ecx\n mov ${q['cluster']},%eax\n mov ${q['origin']},%esi\n mov ${q['displacement']},%edi\n int3")
  env2=dict(base.__dict__,metadata=metadata);exec(original,env2);fixture,authority=env2['original'](image,banks,rows,scratch);save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');stage=a.stage.resolve()/'staged';units=('native_tu15_instanced_scene_ray.cpp','native_tu15_instanced_ray_lifetime.cpp','native_tu15_island_activation.cpp','native_tu15_mopp_ray_traversal.cpp','native_tu15_mopp_ray_setup.cpp')
  for name in(*units,'native_tu15_joint_constraint_numeric_internal.hpp'):shutil.copyfile((stage/'src/physics'/name)if name=='native_tu15_mopp_ray_setup.cpp'else(ROOT/'src/physics'/name),project/name)
  shutil.copyfile(stage/'include/halo2/native_tu15_mopp_private_ray_setup.hpp',project/'include/halo2/native_tu15_mopp_private_ray_setup.hpp');shutil.copyfile(ROOT/'tests/native_tu15_instanced_scene_ray_actual_tests.cpp',project/'driver.cpp')
  (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(actual_instanced LANGUAGES CXX)\ninclude(CTest)\nadd_executable(actual driver.cpp '+' '.join(units)+')\ntarget_include_directories(actual PRIVATE include)\ntarget_compile_features(actual PRIVATE cxx_std_20)\ntarget_compile_options(actual PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME actual_instanced COMMAND actual "'+str(fixture/'original.json')+'")\n');results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','actual','--filter','^actual_instanced$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];rc=subprocess.run(cmd).returncode;results.append(dict(compiler=compiler,exit_code=rc));assert rc==0,'Native actual instanced differs'
  report.update(passed=True,archive_sha256=a.archive_sha256,captured_before_bytes=captured_bytes,profiles=4,results=results,source_authority=authority,source_reads=sum(len(r['reads'])for r in rows),source_stores=sum(len(r['writes'])for r in rows),immutable_dispatch_CODE_tables=dispatch,source_derived_request=q,entry_witness=dict(path=str(witness.relative_to(ROOT)),sha256=sha(witness)),scope=__doc__,unprovided_field_reads_refused=True,actual_caller_input_boundary_captured=False,full_geometric_query_or_live_qualified=False,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'src/physics/native_tu15_instanced_scene_ray.cpp',ROOT/'include/halo2/native_tu15_instanced_scene_ray.hpp',Path(__file__),ROOT/'tests/native_tu15_instanced_scene_ray_actual_tests.cpp')})
 except BaseException as e:report['error']=str(e)or repr(e);report['compiler_diagnostic']=str(getattr(e,'stderr',b''))[-8192:];raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
