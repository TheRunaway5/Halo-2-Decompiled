"""Reject real returned-collector/default-control defects without changing the oracle."""
import json
import subprocess
import sys

from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN

def qualify(scratch,output,rows,io):
    project=scratch/'returned-callback-mutations';project.mkdir()
    selected=[]
    for pc in (0x24df6c,0x24df8d,0x24dfa0):
        selected.append(next(row for row in rows if row['phase']=='normal'and any(write['instruction']==pc for write in row['writes'])))
    selected.append(next(row for row in rows if row['phase']=='normal'and not any(write['instruction']==0x24df40 for write in row['writes'])))
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=selected)))
    core='include/halo2/native_tu15_instanced_ray_callback_kernel.hpp'
    frame='src/physics/runtime/native_tu15_instanced_ray_callback_frame.hpp'
    binding='src/physics/runtime/native_tu15_instanced_ray_callback.hpp'
    changes={
        'wrong_any_hit_fraction':(core,'frame.write(Write::fraction,0u,d);','frame.write(Write::fraction,0x3f800000u,d);'),
        'reversed_closest_comparison':(frame,'return s_.flags.cf||s_.flags.zf;','return !(s_.flags.cf||s_.flags.zf);'),
        'missing_duplicate_return':(core,'if(d.stamp==d.epoch)return Return::duplicate;','if(false)return Return::duplicate;'),
        'ignored_reference_control':(binding,'return value;','return false;'),
    }
    wrapper=project/'expect_rejection.py'
    wrapper.write_text('''import json,pathlib,subprocess,sys
result=subprocess.run(sys.argv[2:],capture_output=True,text=True)
diagnostic=(result.stdout+result.stderr)[-3500:]
accepted=result.returncode==1 and ('differ' in diagnostic or 'no retained statements' in diagnostic)
pathlib.Path(sys.argv[1]).write_text(json.dumps(dict(passed=accepted,rejected=accepted,exit_code=result.returncode,diagnostic=diagnostic),indent=2)+'\\n')
sys.exit(0 if accepted else 1)
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(callback_return_mutants LANGUAGES CXX)\ninclude(CTest)\n'
    for name,(path,old,new)in changes.items():
        source=(ROOT/path).read_text();assert source.count(old)==1,(name,'Mutation boundary drift')
        overlay=project/name;target=overlay/path;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new))
        environment='"${CMAKE_COMMAND}" -E env HALO2_REFERENCE_INSTANCED_RAY_CALLBACKS=1 HALO2_INSTANCED_RAY_CALLBACK_REFERENCE_TEST=1 'if name=='ignored_reference_control'else ''
        cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_mopp_ray_leaf_query_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'include'}" "{overlay/'src'}" "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_definitions({name} PRIVATE HALO2_CALLBACK_SOURCE_ROUTES=1)
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options({name} PRIVATE -fsanitize=address,undefined)
add_test(NAME {name} COMMAND {environment}"{sys.executable}" "{wrapper}" "${{CMAKE_CURRENT_BINARY_DIR}}/{name}.json" $<TARGET_FILE:{name}> "{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
        '--scratch-root',str(scratch/'builds'),'--output',str(output/'returned-callback-mutations'),
        '--filter','^('+'|'.join(changes)+')$','--cmake-arg=-DCMAKE_CXX_COMPILER=g++']
    for name in changes:command+=['--target',name,'--retain-report',name+'.json']
    result=subprocess.run(command,capture_output=True,text=True)
    assert result.returncode==0,'Returned callback controls failed: '+(result.stdout+result.stderr)[-6000:]
    reports={name:json.loads((output/'returned-callback-mutations/evidence'/f'{name}.json').read_text())for name in changes}
    assert all(report['passed']for report in reports.values())
    return dict(passed=True,mutations_rejected=reports,original_instructions_and_outcomes_modified=False)
