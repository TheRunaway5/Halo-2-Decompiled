#!/usr/bin/env python3
"""Qualify native rate admission against exact old owned frozen state headers.

Only the native include, two scalar-entry seams and diagnostic activity symbol
are added to a private copy of the old vehicle CPP. No broad modern state/header
refresh is imported. Native policy/data remain distinct from offline full trace.
"""
import argparse,copy,difflib,hashlib,itertools,json,math,os,shutil,struct,subprocess,sys
from pathlib import Path
import oracle_vehicle_runtime_20261011 as oracle
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,v):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def controls():
 records=oracle.definitions();rows=oracle.controls(records);sample=next(row for row in rows if row.get('scalar_control'))
 for entry,precision,rounding,top,depth in itertools.product((0x208d80,0x208e70),(0,0x200,0x300),range(4),range(8),(0,1,3)):
  row=dict(sample);row.update(entry=entry,direction=-1.,turn=.25,target=-.125,fp_control=0x7f|precision|(rounding<<10),mxcsr=0x1f80|(rounding<<13),top=top,depth=depth);rows.append(row)
 subnormal=struct.unpack('<f',bytes.fromhex('01000000'))[0]
 for entry in(0x208d80,0x208e70):
  for field in('turn','direction','target','dt'):
   for value in(float('nan'),float('inf'),-float('inf'),0.,-0.,subnormal,-subnormal):
    row=dict(sample);row.update(entry=entry,turn=.25,direction=1.,target=-.125,dt=1/30);row[field]=value;row['native_scalar_edge']=field;rows.append(row)
  for rates in((0.,0.,0.,0.),(4.,3.,float('nan'),5.),(float('nan'),3.,2.,5.),(4.,float('nan'),2.,5.)):
   for direction in(-1.,1.):
    row=dict(sample);row.update(entry=entry,turn=.25,direction=direction,target=-.125,scalar_rates=rates,native_scalar_edge='rates');rows.append(row)
 return records,rows

def stage(source,frozen,out):
 before={str(p.relative_to(frozen)):sha(p)for p in[frozen/'src/physics_vehicle_runtime_generated.cpp',*sorted((frozen/'src').glob('*.hpp')),*sorted((frozen/'include').rglob('*.hpp'))]}
 shutil.copytree(frozen/'include',source/'include');(source/'src').mkdir()
 for p in(frozen/'src').glob('*.hpp'):shutil.copyfile(p,source/'src'/p.name)
 originals=['include/halo2/native_vehicle_rate_consumer_tu15.hpp','include/halo2/source_vehicle_native_rates.hpp','src/physics/native_vehicle_rate_consumer_tu15.cpp','src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp','src/physics/vehicles/native_vehicle_rate_math_tu15.hpp','tests/native_vehicle_rate_scope_tu15_tests.cpp']
 pins={}
 for name in originals:
  p=ROOT/name;target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target);pins[name]=sha(p)
 shim=source/'src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp';shim.write_text(shim.read_text().replace('"physics/vehicles/physics_vehicle_runtime_internal.hpp"','"physics_vehicle_runtime_internal.hpp"'))
 cpp=source/'src/physics_vehicle_runtime_generated.cpp';baseline=(frozen/'src/physics_vehicle_runtime_generated.cpp').read_text();text=baseline;needle='#include "physics_vehicle_runtime_internal.hpp"';assert text.count(needle)==1;text=text.replace(needle,needle+'\n#include "physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp"',1)
 for entry,approach,body in(('208d80','false','accelerate'),('208e70','true','approach')):
  needle=f'static void run_{entry}(VehicleState &s) {{\n';assert text.count(needle)==1;text=text.replace(needle,needle+f'if(vehicle_rate_native_eligible(s,selected,{approach})){{vehicle_rate_{body}_native(s,selected);return;}}\n',1)
 for pc,offset in(('208da8','4'),('208db3','8')):
  needle=f'w(s.memory,s.observe,0x{pc}u,(0x{offset}u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();'
  assert text.count(needle)==1,(pc,'Requalify actual literal FSTP publication')
  text=text.replace(needle,f'vehicle_rate_publish_extended_reference(s,0x{pc}u,(0x{offset}u+s.gp[4]));',1)
 for pc,offset,reg in(('208da1','8','2'),('208da4','8','4'),('208dac','c','2'),('208daf','8','4')):
  needle=f's.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x{offset}u+s.gp[{reg}]),4))));'
  assert text.count(needle)>=1,(pc,'Requalify actual literal FMUL m32 operand')
  line=f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}} '+needle
  assert text.count(line)==1
  text=text.replace(line,f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}} vehicle_rate_multiply_extended_reference(s,(0x{offset}u+s.gp[{reg}]));',1)
 needle='void source_run_physics_vehicle_runtime(M &m,std::uint32_t entry,';assert text.count(needle)==1
 activity='''SourceVehicleNativeRateActivity source_vehicle_native_rate_activity() {
 return {physics_vehicle_runtime_detail::rate_acceleration_calls.load(std::memory_order_relaxed),physics_vehicle_runtime_detail::rate_approach_calls.load(std::memory_order_relaxed),physics_vehicle_runtime_detail::rate_cursor_calls.load(std::memory_order_relaxed)};
}
''';text=text.replace(needle,activity+needle,1);cpp.write_text(text)
 (out/'old-vehicle-minimal-seams.diff').write_text(''.join(difflib.unified_diff(baseline.splitlines(True),text.splitlines(True),fromfile='exact-old-frozen-vehicle-cpp',tofile='owned-native-rate-seams')))
 test=source/'tests/native_vehicle_rate_scope_tu15_tests.cpp';test.write_text(test.read_text().replace('"physics/runtime/physics_step_internal.hpp"','"physics_step_internal.hpp"'))
 assert all(sha(frozen/p)==h for p,h in before.items()),'Frozen source changed during read-only staging'
 return dict(root_source_sha256=pins,old_frozen_dependency_sha256=before,old_header_wholesale_refresh_used=False,private_vehicle_cpp_sha256=sha(cpp),private_shim_include_relocation_sha256=sha(shim),private_test_include_relocation_sha256=sha(test))

def stage_modern(source,out):
 pins={}
 paths=[*sorted((ROOT/'include').rglob('*.hpp')),*sorted((ROOT/'src/physics/runtime').glob('*.hpp'))]
 paths += [ROOT/p for p in ('src/physics/vehicles/physics_vehicle_runtime_internal.hpp','src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp','src/physics/vehicles/native_vehicle_rate_math_tu15.hpp','src/physics/native_vehicle_rate_consumer_tu15.cpp','tests/native_vehicle_rate_scope_tu15_tests.cpp')]
 for p in paths:
  name=p.relative_to(ROOT);target=source/name;target.parent.mkdir(parents=True,exist_ok=True);before=sha(p);shutil.copyfile(p,target);assert sha(target)==before and sha(p)==before; pins[str(name)]=before
 p=ROOT/'src/physics/vehicles/physics_vehicle_runtime_generated.cpp';target=source/'src/physics_vehicle_runtime_generated.cpp';before=sha(p);shutil.copyfile(p,target);assert sha(target)==before and sha(p)==before;pins[str(p.relative_to(ROOT))]=before
 return dict(root_source_sha256=pins,modern_context=True,private_vehicle_cpp_sha256=sha(target))

def qualify(source,fixture,out,compiler='g++'):
 (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(NativeVehicleRateScopeTu15 LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
add_executable(native_vehicle_rate_scope_tu15_tests tests/native_vehicle_rate_scope_tu15_tests.cpp src/physics_vehicle_runtime_generated.cpp src/physics/native_vehicle_rate_consumer_tu15.cpp)
target_include_directories(native_vehicle_rate_scope_tu15_tests PRIVATE include src)
target_compile_features(native_vehicle_rate_scope_tu15_tests PRIVATE cxx_std_20)
target_compile_options(native_vehicle_rate_scope_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_vehicle_rate_scope_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_vehicle_rate_scope_original_tu15 COMMAND native_vehicle_rate_scope_tu15_tests "'''+str(fixture)+'''")
''')
 command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--output',str(out),'--scratch-root',str(source.parent/'builds'),'--target','native_vehicle_rate_scope_tu15_tests','--filter','^native_vehicle_rate_scope_original_tu15$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);return dict(compiler=compiler,exit_code=r.returncode,command=command)

def sanitize(value):
 if isinstance(value,float)and not math.isfinite(value):return dict(float32_hex=struct.pack('<f',value).hex())
 if isinstance(value,dict):return{k:sanitize(v)for k,v in value.items()}
 if isinstance(value,(tuple,list)):return[sanitize(v)for v in value]
 return value

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--frozen-source',type=Path,required=True);p.add_argument('--modern-context',action='store_true');a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=False);source=scratch/'source';frozen=a.frozen_source.resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 save(a.output/'report.json',report)
 try:
  report.update(stage_modern(source,a.output)if a.modern_context else stage(source,frozen,a.output));paired_path=ROOT/'analysis/multiplayer/sandbox-source/vehicle-runtime-20261011/correspondence-verified/report.json';paired=json.loads(paired_path.read_text());records,rows=controls();cases=oracle.execute(oracle.NEW_DEFAULT,paired,True,records,rows)
  for c in cases:c['declared_input']=sanitize(c['declared_input'])
  image=oracle.Xbe(oracle.NEW_DEFAULT);fixture=dict(schema=1,image='1.5',cases=cases,scope_stack_begin=oracle.STACK-0x4000,scope_stack_end=oracle.STACK+0x1000,native_cursor_immutable_inputs=[dict(address=0x46a0d0,hex=image.read(0x46a0d0,4).hex())]);target=scratch/'tu15.json';target.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
  report.update(original_cases=len(cases),original_stores=sum(len(c['writes'])for c in cases),original_sha256=sha(oracle.NEW_DEFAULT),oracle_backend='unchanged-original-Unicorn-component-reference',fixture_sha256=sha(target),fixture_bytes=target.stat().st_size,correspondence_sha256=sha(paired_path),actual_scalar_pc_delta=0x4900,physical_paired_nan_authority_proven=False)
  save(a.output/'report.json',report);results=[qualify(source,target,a.output/c,compiler=c)for c in('g++','clang++')];report.update(compiler_results=results,passed=all(r['exit_code']==0 for r in results))
 except BaseException as error:report['error']=str(error);raise
 finally:
  files=[p for p in scratch.rglob('*')if p.is_file()];report.update(temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in files),temporary_logical_bytes_before_owner_cleanup=sum(p.stat().st_size for p in files));save(a.output/'report.json',report)
 print(json.dumps({k:report[k]for k in('passed','original_cases','original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
