"""Compiled sphere-ray defects against untouched original result packets."""
import json
from pathlib import Path
import subprocess
import sys
from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN

def qualify(scratch,output,rows,io,compiler,sanitizers):
    project=scratch/('sphere-mutants-'+Path(compiler).name);project.mkdir()
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows[:36])))
    path='src/physics/runtime/native_tu15_sphere_ray_cast_frame.hpp'
    defects=(
        ('accept_tangent','return pair==0u||pair==0x41u;','return pair==0u||pair==0x40u||pair==0x41u;'),
        ('omit_late_radius','divide(read(0x2e8f91u,g[1]+12u));','divide(0x3f800000u);'),
        ('cached_ray_end','x[1]=Base::vector(0x2e8f5au,g[2]+16u);','x[1]=x[3];'),
        ('wide_boolean','event{pc,address,{},{std::byte(value)}};','event{pc,address,{},std::vector<std::byte>(4,std::byte(value))};'),
    )
    wrapper=project/'reject.py';wrapper.write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode==1 and any(t in r.stderr for t in ("sphere full packet differs", "sphere ordered reads differ", "sphere ordered writes differ", "sphere store observer differs")),r.stdout+r.stderr
print("Rejected genuine sphere defect")
''')
    original=(ROOT/path).read_text()
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(sphere_mutants LANGUAGES CXX)\ninclude(CTest)\n'
    for name,needle,replacement in defects:
        if original.count(needle)!=1:raise ValueError('Sphere exact defect seam drift '+name)
        overlay=project/name;header=overlay/path;header.parent.mkdir(parents=True);header.write_text(original.replace(needle,replacement,1))
        cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_sphere_ray_cast_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'src'}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
        if sanitizers:cmake+=f'target_compile_options({name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({name} PRIVATE -fsanitize=address,undefined)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
             '--output',str(output/(Path(compiler).name+'-sphere-mutants')),'--filter','^('+'|'.join(d[0]for d in defects)+')$',
             '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
    for name,*_ in defects:command+=['--target',name]
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:raise ValueError('Sphere compiled defects failed: '+(result.stdout+result.stderr)[-6000:])
    return dict(passed=True,compiler=compiler,rejected=[d[0]for d in defects],oracle_outcomes_changed=False)
