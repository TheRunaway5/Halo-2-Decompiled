"""Compile semantic/ABI/protocol defects against unchanged inverse packets."""
import subprocess,sys
from oracle_smg_firing_1_1 import ROOT

def qualify(scratch,output,fixture):
 project=scratch/'mutations';project.mkdir()
 core='include/halo2/native_tu15_affine_inverse_kernel.hpp'
 frame='src/physics/runtime/native_tu15_affine_inverse.hpp'
 mutations={
  'short_zero_clear':(core,'n<13','n<12'),
  'transpose_wrong_axis':(frame,'address=s_.gp[1]+20u;integer=2','address=s_.gp[1]+24u;integer=2'),
  'missing_position_scale':(core,'d.pz=mul(d.reciprocal,d.pz);','d.pz=d.pz;'),
  'reversed_nan_operand':(core,'mul(d.output_fx,d.px)','mul(d.px,d.output_fx)'),
  'unprotected_provider_fp':(frame,'scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();'),
  'premature_push_commit':(frame,'Base::store(pc(0x144097u),address,s_.gp[7],[&]{s_.gp[4]=address;});','s_.gp[4]=address;Base::store(pc(0x144097u),address,s_.gp[7],[]{});'),
  'lost_saved_destination':(frame,'s_.gp[7]=pop(cleared?0x1441b9u:0x1441aeu);','pop(cleared?0x1441b9u:0x1441aeu);'),
 }
 wrapper=project/'expect_rejection.py';wrapper.write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nprint(json.dumps(dict(rejected=r.returncode!=0,exit=r.returncode,diagnostic=(r.stdout+r.stderr)[-2000:])))\nsys.exit(0 if r.returncode else 1)\n')
 cmake='cmake_minimum_required(VERSION 3.20)\nproject(inverse_mutants LANGUAGES CXX)\ninclude(CTest)\n'
 for name,(path,old,new)in mutations.items():
  source=(ROOT/path).read_text();assert source.count(old)==1,(name,'Mutation witness drift')
  overlay=project/name;target=overlay/path;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
  cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_affine_inverse_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'include'}" "{overlay/'src'}" "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
 (project/'CMakeLists.txt').write_text(cmake)
 command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(output/'mutations'),'--filter','^('+'|'.join(mutations)+')$']
 for name in mutations:command+=['--target',name]
 result=subprocess.run(command,capture_output=True,text=True)
 assert result.returncode==0,'Compiled inverse control failed: '+result.stdout[-6500:]+result.stderr[-1000:]
 return dict(passed=True,mutations_rejected=list(mutations),source_outcomes_or_oracles_modified=False)
