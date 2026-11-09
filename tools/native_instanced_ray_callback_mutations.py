"""Compiled defects for source-backed instanced callback stage qualification."""
import json
import subprocess
import sys
from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN

def qualify(scratch,output,rows,io):
    project=scratch/'negative-controls';project.mkdir()
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
    core='include/halo2/native_tu15_instanced_ray_callback_kernel.hpp'
    scale='src/physics/runtime/native_tu15_mopp_ray_scale_frame.hpp'
    caller='src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp'
    mutations={
        'unprotected_provider':(scale,'scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();'),
        'premature_push_commit':(caller,'store(pc,address,value,[&]{s_.gp[4]=address;});','at(pc);s_.gp[4]=address;store(pc,address,value,[]{},true);'),
        'unsigned_stamp_index':(core,'std::uint32_t(std::int32_t(std::int16_t(d.instance&0xffffu)))','d.instance&0xffffu'),
        'early_owner_read':(core,'frame.push_body(d);frame.fraction(d);\n    d.owner=frame.owner(d);','frame.push_body(d);d.owner=frame.owner(d);frame.fraction(d);\n    '),
    }
    wrapper=project/'expect_rejection.py';wrapper.write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nprint(json.dumps(dict(rejected=r.returncode!=0,exit_code=r.returncode,diagnostic=(r.stdout+r.stderr)[-4000:])))\nsys.exit(0 if r.returncode!=0 else 1)\n')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(callback_mutations LANGUAGES CXX)\ninclude(CTest)\n'
    for name,(path,old,new) in mutations.items():
        source=(ROOT/path).read_text();assert source.count(old)==1,(name,'Mutation boundary drift')
        overlay=project/name;target=overlay/path;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new))
        cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_instanced_ray_callback_frame_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'include'}" "{overlay/'src'}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(output/'negative-controls'),'--filter','^('+'|'.join(mutations)+')$']
    for name in mutations:command+=['--target',name]
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert result.returncode==0,'Callback compiled controls failed: '+result.stdout[-6000:]
    return dict(passed=True,mutations_rejected=list(mutations),reference_instructions_or_outcomes_modified=False)
