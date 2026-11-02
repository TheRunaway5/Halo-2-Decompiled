#!/usr/bin/env python3
"""Actual camera BEFORE DATA→nativeBSP wrapper/recursive/surface composition.

Independent formal BSP entry is source-derived from current16C620 inputs.
No captured guest caller ABI, supplied after image or native CPU child exists.
All captured bytes plus independent output/context DATA are compared strictly.
"""
import argparse,bisect,hashlib,inspect,json,lzma,os,shutil,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
OWNERS=(0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_GP_or_after_consumed=False,captured_bank_exclusions=[])
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads(a.archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-camera-query-typed-before-v1'and not capture['physics_result_supplied']and not capture['guest_abi_fabricated'];typed=capture['input'];assert typed['preserved_x87_depth']==0
  hashes=json.loads(lzma.decompress(a.archive.with_name('all-file-hashes.json.xz').read_bytes()));wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=str(Path(member.name));name=name[2:]if name.startswith('./')else name
    if name not in wanted:continue
    r=wanted[name];raw=tar.extractfile(member).read();proof=hashes.get('./'+name,hashes.get(name));assert len(raw)==r['size']and sha_bytes(raw)==proof['sha256'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);captured_bytes=sum(map(len,banks.values()));image=Xbe(NEW_DEFAULT)
  def word(p,n=4):
   starts=sorted(banks);i=bisect.bisect_right(starts,p)-1
   if i>=0:
    first=starts[i]
    if p+n<=first+len(banks[first]):return int.from_bytes(banks[first][p-first:p-first+n],'little')
   raw=image.read(p,n);banks[p]=raw;return int.from_bytes(raw,'little')
  q=dict(flags=(typed['mask']>>23)&63,descriptor=word(0x4ecc10),output=base.UI+0x2000,epsilon=word(0x45695c),limit=256,bitmap=word(0x4f9b50)+((word(0x474c24,2)+32768)%65536-32768)*32+1,origin=typed['origin'],displacement=typed['displacement'])
  for at,n in((0x46a0d0,4),(0x46a0e8,4),(0x44ac8c,36)):word(at,n)
  context=base.SP-40;banks[context]=bytes(40)
  assert not any(first<=base.UI<first+len(raw)for first,raw in banks.items())
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');(scratch/'observer.cpp').write_text(observer)
  base.OWNERS=OWNERS;meta_source=inspect.getsource(base.metadata).replace("if ins.mnemonic!='lea':","if ins.mnemonic!='lea':").replace('read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)','read=bool((CS_AC_WRITE if ins.mnemonic in(\'fst\',\'fstp\')else op.access)&CS_AC_READ),write=bool((CS_AC_WRITE if ins.mnemonic in(\'fst\',\'fstp\')else op.access)&CS_AC_WRITE)');meta_env=dict(base.__dict__);exec(meta_source,meta_env)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp").replace('if p<0x800000:continue','if p<0x800000 or STACK<=p<STACK+0x10000:continue')
  source=source.replace(" mov ${case['receiver']},%ecx\n int3",f" mov ${{case['receiver']}},%ecx\n mov ${q['output']},%eax\n mov ${q['descriptor']},%edx\n mov ${q['epsilon']},%edi\n movd %edi,%xmm0\n int3")
  fp=typed['floating_point'];rows=[dict(name='actual_bsp_'+str(mode),entry=0x1e68e0,receiver=q['flags'],arguments=[q['limit'],q['bitmap'],q['origin'],q['displacement']],user_words=[],before_fp_input=[(fp['x87_control_word']&~0xc00)|(mode<<10),fp['x87_status'],(fp['mxcsr']&~0x6000)|(mode<<13)])for mode in range(4)]
  env=dict(base.__dict__,metadata=meta_env['metadata']);exec(source,env);fixture,authority=env['original'](image,banks,rows,scratch);assert all(row['after_esp']==base.SP+20 for row in rows);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
  report.update(actual_bsp_query=q,actual_source_return=[r['return_word']&255 for r in rows],executed_sites=sorted(set(pc for r in rows for pc in r['executed_sites'])),source_reads=sum(len(r['reads'])for r in rows),source_stores=sum(len(r['writes'])for r in rows));save(a.output/'original-receipt.json',report)
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');units=('native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp')
  for name in(*units,'native_tu15_joint_constraint_numeric_internal.hpp'):shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_bsp_segment_query.hpp"\n'+driver
  begin=driver.index('      NativeTu15ArrayGrowthMemory array(');end=driver.index('      const auto host_sw=',begin);driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15BspSegmentQueryStats activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  invoke='      try{result=native_tu15_query_bsp_segment(memory,{'+','.join(str(q[k])+'u'for k in('flags','descriptor','output','epsilon','limit','bitmap','origin','displacement'))+'},'+str(context)+'u,fp,0,observe,&activity);\n'
  driver=driver[:begin]+invoke+driver[end:];driver=driver.replace('      check(calls==','      check(activity.calls==1&&activity.completed==1&&bool(c.at("return_word").get<unsigned>()&255u)==result,"Actual BSP completion/return differs");\n      check(calls==');driver=driver.replace('check(cases>20,','check(cases==4,');(project/'driver.cpp').write_text(driver)
  (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(actual_bsp LANGUAGES CXX)\ninclude(CTest)\nadd_executable(actual driver.cpp '+' '.join(units)+')\ntarget_include_directories(actual PRIVATE include)\ntarget_compile_features(actual PRIVATE cxx_std_20)\ntarget_compile_options(actual PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME actual_bsp COMMAND actual "'+str(fixture/'original.json')+'")\n');results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','actual','--filter','^actual_bsp$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];rc=subprocess.run(cmd).returncode;results.append(dict(compiler=compiler,exit_code=rc));assert rc==0,'Native actual BSP differs'
  report.update(passed=True,archive_sha256=a.archive_sha256,actual_query_input=typed,captured_before_bytes=captured_bytes,profiles=4,results=results,source_authority=authority,scope=__doc__,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'src/physics/native_tu15_bsp_segment_query.cpp',ROOT/'include/halo2/native_tu15_bsp_segment_query.hpp',Path(__file__))})
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
def sha_bytes(raw):return hashlib.sha256(raw).hexdigest()
if __name__=='__main__':raise SystemExit(main())
