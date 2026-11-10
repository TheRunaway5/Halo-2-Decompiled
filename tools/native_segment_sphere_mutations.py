"""Compile and reject semantic/admission defects against genuine120FA0 packets."""
import subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT

def qualify(scratch,output,fixture):
 project=scratch/'mutations';project.mkdir()
 core='include/halo2/native_tu15_segment_sphere_kernel.hpp'
 frame='src/physics/runtime/native_tu15_segment_sphere.hpp'
 mutations={
  'missing_squared_y':(core,'d.distance=N::add(d.distance,d.term);','d.distance=N::add(d.distance,d.zero);'),
  'unordered_comparison':(core,'"comiss %4,%3;setb %0;sete %1;setp %2"','"ucomiss %4,%3;setb %0;sete %1;setp %2"'),
  'byte_return_clobbers_eax':(frame,'Result::byte_true?1u:4u','Result::byte_true?4u:4u'),
  'unprotected_io':(frame,'scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();'),
  'premature_return_argument':(frame,'pop(pc);s_.gp[4]+=4u;','s_.gp[4]+=4u;pop(pc);'),
  'radius_squared_add':(core,'d.distance=N::subtract(d.distance,d.radius_squared);','d.distance=N::add(d.distance,d.radius_squared);'),
 }
 wrapper=project/'expect_rejection.py';wrapper.write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nprint(json.dumps(dict(rejected=r.returncode!=0,exit=r.returncode,diagnostic=(r.stdout+r.stderr)[-2000:])))\nsys.exit(0 if r.returncode else 1)\n')
 cmake='cmake_minimum_required(VERSION 3.20)\nproject(admission_mutants LANGUAGES CXX)\ninclude(CTest)\n'
 for name,(path,old,new)in mutations.items():
  source=(ROOT/path).read_text();assert source.count(old)>0,(name,'Mutation witness drift')
  overlay=project/name;target=overlay/path;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
  cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_segment_sphere_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'include'}" "{overlay/'src'}" "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
 (project/'CMakeLists.txt').write_text(cmake)
 command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(output/'mutations'),'--filter','^('+'|'.join(mutations)+')$']
 for name in mutations:command+=['--target',name]
 result=subprocess.run(command,capture_output=True,text=True)
 assert result.returncode==0,'Compiled admission control failed: '+result.stdout[-6500:]+result.stderr[-1000:]
 return dict(passed=True,mutations_rejected=list(mutations),source_outcomes_or_oracles_modified=False)
