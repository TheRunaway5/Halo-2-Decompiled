#!/usr/bin/env python3
"""Physical unchanged TU1.5 distinct-NaN vehicle-rate authority.

Uses a fully declared raw scalar ABI/input corpus, never a supplied output.
Original20D680/20D770 instructions run unchanged; no unknown callee is resumed.
All binaries, raw inputs/output and fixture banks remain in run_test.py scratch.
"""
import argparse,copy,itertools,json,os,struct,subprocess,sys
from pathlib import Path
import oracle_vehicle_runtime_20261011 as oracle
import oracle_vehicle_motion_math_hardware_20261011 as physical
from validate_vehicle_native_rate_scope_tu15 import ROOT,sha,save,stage
VALUE,RATES,STACK=0x1006000,0x1006100,0x100d000
STOP=0x100f000

def inputs(paired):
 records=oracle.definitions();seed=next(r for r in oracle.controls(records)if r.get('scalar_control'));seed=dict(seed,entry=0x208d80,turn=.25,direction=1.,target=-.125,dt=.25,fp_control=0x27f,mxcsr=0x1f80,top=0,depth=0)
 abi=oracle.execute(oracle.NEW_DEFAULT,paired,True,records,[seed])[0]['entry_abi'];out=[];image=oracle.Xbe(oracle.NEW_DEFAULT)
 names=('current','target','direction','seconds','positive_limit','negative_limit','acceleration','deceleration')
 flows=(('current','target'),('current','acceleration'),('current','deceleration'),('current','positive_limit'),('current','negative_limit'),('direction','acceleration'),('direction','deceleration'),('seconds','acceleration'),('seconds','deceleration'),('acceleration','deceleration'))
 def make(entry,values,rounding,denorm,precision,alias=None):
  state=copy.deepcopy(abi);state['gp'][1]=VALUE;state['gp'][2]=RATES;state['gp'][4]=STACK;state['x87_control_word']=0x7f|(precision<<8)|(rounding<<10);state['mxcsr']=0x1f80|(rounding<<13)|denorm;state['x87_status']=((len(out)%8)<<11)|((len(out)%8)<<8)|((len(out)%2)<<14)|(len(out)*7%64);state['x87_tag']=0xffff;state['x87_values']=[]
  if alias=='current-direction':state['gp'][1]=STACK+4
  if alias=='rates-arguments':state['gp'][2]=STACK+4
  if alias=='current-coefficient':state['gp'][1]=RATES+8
  raw={}
  def put(p,b):
   for i,v in enumerate(b):raw[p+i]=v
  put(STACK-32,bytes(96));put(VALUE,struct.pack('<I',values['current']));put(RATES,struct.pack('<4I',*[values[k]for k in('positive_limit','negative_limit','acceleration','deceleration')]))
  put(state['gp'][2],struct.pack('<4I',*[values[k]for k in('positive_limit','negative_limit','acceleration','deceleration')]));put(state['gp'][1],struct.pack('<I',values['current']));put(STACK,struct.pack('<3I',STOP,values['direction'],values['seconds']));put(0x46a0d0,image.read(0x46a0d0,4))
  state['xmm'][3]=f'{values["target"]:032x}';banks=[]
  for p in sorted(raw):
   if banks and banks[-1][0]+len(banks[-1][1])==p:banks[-1][1].append(raw[p])
   else:banks.append([p,bytearray([raw[p]])])
  out.append(dict(id=len(out),entry=entry,entry_abi=state,exit=copy.deepcopy(state),exit_entry=STOP,return_address=STOP,complete=True,initial_banks=[dict(address=p,hex=b.hex())for p,b in banks],final_banks=[],instructions=[],writes=[],estimates=[],timestamps=[],declared_input=dict(raw_float32_bits=values,rounding=rounding,denorm_bits=denorm,precision=precision,alias=alias)))
 baseline=dict(current=0x3e800000,target=0xbe000000,direction=0x3f800000,seconds=0x3e800000,positive_limit=0x40800000,negative_limit=0x40400000,acceleration=0x40000000,deceleration=0x40a00000)
 for entry,(a,b),flow,rounding,denorm in itertools.product((0x208d80,0x208e70),((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)),flows,range(4),(0,0x40,0x8000,0x8040)):
  values=dict(baseline);values[flow[0]]=a;values[flow[1]]=b;make(entry,values,rounding,denorm,(0,2,3)[len(out)%3])
 for entry,rounding,denorm,field in itertools.product((0x208d80,0x208e70),range(4),(0,0x40,0x8000,0x8040),('current','direction','seconds')):
  values=dict(baseline);values[field]=1;make(entry,values,rounding,denorm,(0,2,3)[len(out)%3])
 for entry,alias in itertools.product((0x208d80,0x208e70),('current-direction','rates-arguments','current-coefficient')):make(entry,baseline,0,0,3,alias)
 return dict(schema=1,image='1.5',cases=out,scope_stack_begin=STACK-0x4000,scope_stack_end=STACK+0x1000)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--frozen-source',type=Path,required=True);p.add_argument('--negative-control',action='store_true');a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=False);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);save(a.output/'report.json',report)
 try:
  source=scratch/'source';report.update(stage(source,a.frozen_source.resolve(),a.output));test=ROOT/'tests/native_vehicle_rate_physical_tu15_tests.cpp';target=source/'tests/native_vehicle_rate_physical_tu15_tests.cpp';target.write_text(test.read_text().replace('"physics/runtime/physics_step_internal.hpp"','"physics_step_internal.hpp"'));report['physical_test_sha256']=sha(test)
  if a.negative_control:
   shim=source/'src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp';text=shim.read_text();needle='    vehicle_rate_native_publication_complete(s,status);\n    if(s.observe)s.observe(event);';assert text.count(needle)==1
   before=sha(shim);shim.write_text(text.replace(needle,'    if(s.observe)s.observe(event);\n    vehicle_rate_native_publication_complete(s,status);'));report['private_mutation']=dict(path=str(shim.relative_to(source)),before=before,after=sha(shim),kind='Defer completed FSTP pop/C1 until after observer delivery')

  paired_path=ROOT/'analysis/multiplayer/sandbox-source/vehicle-runtime-20261011/correspondence-verified/report.json';paired=json.loads(paired_path.read_text());paired['instruction_map']={p:q for p,q in paired['instruction_map'].items()if 0x208d80<=int(p,16)<0x208ed0};paired['entries']={p:q for p,q in paired['entries'].items()if int(p,16)in(0x208d80,0x208e70)}
  observer=ROOT/'tools/vehicle_rate_hardware_trace_tu15.cpp';observer_root=scratch/'physical-observer';(observer_root/'tools').mkdir(parents=True);(observer_root/'tools/vehicle_motion_math_hardware_trace_20261011.cpp').write_bytes(observer.read_bytes());physical.ROOT=observer_root
  f=inputs(paired);observed=physical.execute(f,paired,oracle.NEW_DEFAULT,scratch/'hardware');assert observed['paired_instruction_bytes_changed']is False and not observed['unported_owner_capture_stubs']
  actual={c['id']:c for c in observed['cases']};snapshots={c['id']:c.get('rate_fp_snapshots',[]) for c in json.loads((scratch/'hardware/trace.json').read_text())['cases']};differences=0
  for c in f['cases']:
   r=actual[c['id']];c['rate_fp_snapshots']=snapshots[c['id']]
   for k in('exit','final_banks','instructions','writes'):c[k]=r[k]
  f['oracle_backend']='original_i386_hardware_single_step';fixture=scratch/'physical.json';fixture.write_text(json.dumps(f,separators=(',',':'))+'\n');report.update(original_sha256=observed['original_sha256'],original_cases=len(f['cases']),original_stores=sum(len(c['writes'])for c in f['cases']),physical_instructions=observed['single_stepped_original_instructions'],cpu_model=observed['cpu_model'],paired_instruction_bytes_changed=False,unported_owner_capture_stubs=[],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,physical_tool_sha256=observed['tool_sha256'],rate_physical_observer_source_sha256=sha(observer),original_rate_fp_snapshots=sum(len(c['rate_fp_snapshots'])for c in f['cases']),publication_pre_and_post_fault_cases=sum(x['instruction']in(0x208da8,0x208db3)for c in f['cases']for x in c['rate_fp_snapshots']),rounded_up_publications=sum(x['instruction']in(0x208da8,0x208db3)and bool(x['after']['x87_status']&0x200)for c in f['cases']for x in c['rate_fp_snapshots']),physical_observer_sha256=observed['trace_observer_sha256']);save(a.output/'report.json',report)
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeVehicleRatePhysicalTu15 LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
add_executable(native_vehicle_rate_physical_tu15_tests tests/native_vehicle_rate_physical_tu15_tests.cpp src/physics_vehicle_runtime_generated.cpp src/physics/native_vehicle_rate_consumer_tu15.cpp)
target_include_directories(native_vehicle_rate_physical_tu15_tests PRIVATE include src)
target_compile_features(native_vehicle_rate_physical_tu15_tests PRIVATE cxx_std_20)
target_compile_options(native_vehicle_rate_physical_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_vehicle_rate_physical_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_vehicle_rate_physical_original_tu15 COMMAND native_vehicle_rate_physical_tu15_tests "'''+str(fixture)+'")\n';(source/'CMakeLists.txt').write_text(cmake)
  results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--output',str(a.output/compiler),'--scratch-root',str(scratch/'builds'),'--target','native_vehicle_rate_physical_tu15_tests','--filter','^native_vehicle_rate_physical_original_tu15$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'];r=subprocess.run(command,check=False);row=dict(compiler=compiler,exit_code=r.returncode,command=command)
   if a.negative_control:
    validation=json.loads((a.output/compiler/'report.json').read_text());tests=(a.output/compiler/'tests.log').read_text();row['expected_publication_order_mutation_rejected']=r.returncode!=0 and 'physical publication fault depth/SW differs' in tests and all(step['exit_code']==0 for step in validation['steps']if step['name']!='tests')
   results.append(row)
  report.update(compiler_results=results,passed=all(c.get('expected_publication_order_mutation_rejected',False)if a.negative_control else c['exit_code']==0 for c in results))
 except BaseException as error:report['error']=str(error);raise
 finally:
  files=[p for p in scratch.rglob('*')if p.is_file()];report.update(temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in files),temporary_logical_bytes_before_owner_cleanup=sum(p.stat().st_size for p in files));save(a.output/'report.json',report)
 print(json.dumps({k:report.get(k)for k in('passed','original_cases','original_stores','physical_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
