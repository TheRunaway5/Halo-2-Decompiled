#!/usr/bin/env python3
"""Actual30CAA0 five-record reduction versus native current-DATA kernel."""
import argparse,copy,hashlib,inspect,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
U=base.UI;ARRAY=U+0x100;MANAGER=U+0x200;TABLE=U+0x300;RECORDS=U+0x1000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def cases():
 rows=[];rng=random.Random(3405)
 points=[[(0.,0.,0.)]*5,[(float(n),0.,0.)for n in range(5)],[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(1.,1.,0.),(.5,.5,0.)],[(rng.uniform(-2,2),rng.uniform(-2,2),rng.uniform(-2,2))for n in range(5)]]
 for profile,values in enumerate(points):
  for order in ((0,1,2,3,4),(4,3,2,1,0),(2,4,1,3,0)):
   for scale in (.1,1.,-2.):
    data={ARRAY+n*4:RECORDS+index*48 for n,index in enumerate(order)};data.update({MANAGER:TABLE,TABLE+8:0x2e4d60})
    for n,point in enumerate(values):
     for lane,value in enumerate(point):data[RECORDS+n*48+lane*4]=bits(value*scale)
     data[RECORDS+n*48+28]=0x3f000001+n
    rows.append(dict(name=f'points_{profile}_{order}_{scale}',entry=0x30caa0,receiver=0,arguments=[ARRAY,0,0,MANAGER],user_words=[(p-U,v)for p,v in data.items()]))
 for value in (0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  row=copy.deepcopy(rows[-1]);row['name']=f'ieee_{value:x}';row['user_words']=[(p,value if p==RECORDS-U else v)for p,v in row['user_words']];rows.append(row)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x30caa0,0x2e4d60)
  source=inspect.getsource(base.metadata).replace('read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)','read=(ins.mnemonic not in("fst","fstp"))and(bool(op.access&CS_AC_READ)or ins.mnemonic in("test","cmp")),write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in("fst","fstp")');ns=dict(base.__dict__);exec(source,ns);base.metadata=ns['metadata']
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')");ns=dict(base.__dict__);exec(source,ns)
  banks={0x46a0e8:image.read(0x46a0e8,4)};rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+4 for c in rows)
  full_rows=list(rows);extra=[];seen=set()
  for original in (rows[0],rows[27]):
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     address=event['address'];width=event.get('bytes',len(event.get('written',''))//2);pair=(address,width);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen:continue
     seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=address,bytes=width,occurrence=occurrences[pair],write=kind=='writes'));extra.append(row)
  # A current-record observer changes the next source-read vector after the
  # first completed copy. This tests rereading, rather than cached snapshots.
  row={k:copy.deepcopy(rows[0][k])for k in ('entry','receiver','arguments','user_words')};row.update(name='current_record_reread',mutations=[dict(instruction=0x30ccc0,address=RECORDS+4*48+16,written='78563412')]);extra.append(row)
  prefix=scratch/'prefix';prefix.mkdir();shutil.copyfile(scratch/'observer.cpp',prefix/'observer.cpp');prefix_fixture,_=ns['original'](image,banks,extra,prefix)
  for row in extra:
   for bank in row['banks']:
    for key in ('before_file','after_file'):bank[key]=str(prefix_fixture/bank[key])
  rows+=extra
  (fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_reduction.cpp',project/'native.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_reduction.hpp"\n'+driver;driver=driver.replace('  void write(std::uint32_t p,','  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}\n  void write(std::uint32_t p,')
  driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;', '      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;')
  begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin);driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15ContactReductionStats activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      NativeTu15ContactDistanceServices services;services.remove_point=[&](auto,auto owner,auto,auto&){check(owner.identity==0x2e4d60u,"Unqualified point owner");};
      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();result=native_tu15_reduce_contact_records(memory,args[0],args[3],fp,0,services,observe,&activity);
'''+driver[end:];driver=driver.replace('      check(calls==c.at("allocator_calls"),','      check(failed||required||result==c.at("return_word"),name+" result count differs");\n      check(calls==c.at("allocator_calls"),')
  (project/'driver.cpp').write_text(driver);variants={};text=(project/'native.cpp').read_text()
  for name,needle,replacement in (('wrong_selection','if(!(compare_metric(metric)&0x4100u))','if((compare_metric(metric)&0x4100u))'),('wrong_cross','const auto cross=subtract(left,right);','const auto cross=left;'),('skip_move','m.vector(0x30ccc0u,destination,value);',';')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(reduction LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n'
  for mode in ('native',*variants):
   source='native.cpp'if mode=='native'else mode+'.cpp';target='reduce_'+mode;wrapper=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_executable({target} $<TARGET_OBJECTS:driver> {source})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_reduce_{mode} COMMAND {wrapper}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake)
  results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','reduce_'+mode]for mode in ('native',*variants)),[]),'--filter','^native_reduce_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native contact reduction differs'
  sites={pc for c in rows for pc in c['executed_sites']if 0x30caa0<=pc<=0x30ccf8};report.update(passed=True,profiles=len(rows),full_returns=len(full_rows)+1,operand_fault_prefixes=len(extra)-1,negative_controls=list(variants),parent_sites_covered=len(sites),source_authority=authority,results=results,original_sha256=NEW_PIN,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),source_hashes={n:sha(ROOT/n)for n in('include/halo2/native_tu15_contact_reduction.hpp','src/physics/native_tu15_contact_reduction.cpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp')},scope='Declared coherent five-record DATA and genuine RET4 point no-op; all persistent banks/ordered source operands/stores/FP/RET0 and computed remaining-count. Source-private selection flags/scalars and arbitrary observers remain separate; no production binding or whole2F8B60 claim.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
