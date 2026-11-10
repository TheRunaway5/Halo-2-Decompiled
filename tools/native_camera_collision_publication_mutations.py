"""Compile and reject semantic/admission defects against genuine16B9A0 packets."""
import subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT

def qualify(scratch,output,fixture):
 project=scratch/'mutations';project.mkdir()
 core='include/halo2/native_tu15_camera_collision_publication_kernel.hpp'
 frame='src/physics/runtime/native_tu15_camera_collision_publication.hpp'
 mutations={
  'normal_nan_operand_order':(core,'N::multiply(d.term,d.plane_x)','N::multiply(d.plane_x,d.term)'),
  'plane_distance_nan_order':(core,'N::multiply(d.term,memory.scalar(Read::scale,d))','N::multiply(memory.scalar(Read::scale,d),d.term)'),
  'tail_clobbers_upper_ecx':(frame,'width=1;register_width=1;break;','width=1;register_width=4;break;'),
  'missing_early_ch_flags':(frame,'s_.flags.logic(s_.reg(1,1,8),1)','s_.flags.logic(s_.reg(1,4,0)&0x8000u,4)'),
  'unprotected_io':(frame,'scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();'),
  'premature_push_commit':('src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','store(pc,address,value,[&]{s_.gp[4]=address;});','at(pc);s_.gp[4]=address;store(pc,address,value,[]{},true);'),
 }
 wrapper=project/'expect_rejection.py';wrapper.write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nprint(json.dumps(dict(rejected=r.returncode!=0,exit=r.returncode,diagnostic=(r.stdout+r.stderr)[-2000:])))\nsys.exit(0 if r.returncode else 1)\n')
 cmake='cmake_minimum_required(VERSION 3.20)\nproject(admission_mutants LANGUAGES CXX)\ninclude(CTest)\n'
 for name,(path,old,new)in mutations.items():
  source=(ROOT/path).read_text();assert source.count(old)>0,(name,'Mutation witness drift')
  overlay=project/name;target=overlay/path;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
  cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_camera_collision_publication_tests.cpp'}")
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
