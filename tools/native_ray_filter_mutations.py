"""Real compiled filter/collection defects against unchanged original cases."""
import json
from pathlib import Path
import subprocess
import sys
from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN


def qualify(scratch,output,rows,io,compiler,sanitizers):
    project=scratch/('filter-mutants-'+Path(compiler).name);project.mkdir()
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,
        cases=[row for row in rows[:40]],hostile_providers=True)))
    defects=(
        ('ignore_same_group','include/halo2/native_tu15_ray_filter_kernel.hpp',
         'return ((first^second)&0xffff0000u)==0u&&(first&0xffff0000u)!=0u;','return false;'),
        ('wrong_mask_index','src/physics/runtime/native_tu15_ray_filter_frame.hpp',
         'g[2]=s_.flags.logic(first&31u,4);','g[2]=s_.flags.logic(first&15u,4);'),
        ('wrong_row_stride','include/halo2/native_tu15_shape_collection_kernel.hpp',
         'rows+key*8u+view.offset()','rows+key*16u+view.offset()'),
        ('wrong_info_field','src/physics/runtime/native_tu15_shape_collection_frame.hpp',
         'return Kind==Field::shape?0u:4u;','return 0u;'),
    )
    wrapper=project/'reject.py';wrapper.write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode==1 and any(text in r.stderr for text in ("primitive-key complete packets differ", "primitive-key boundary ABI differs", "ordered reads differ", "ordered writes differ")),r.stdout+r.stderr
print("Rejected genuine filter/collection defect")
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(ray_filter_mutants LANGUAGES CXX)\ninclude(CTest)\n'
    for name,path,needle,replacement in defects:
        source=(ROOT/path).read_text()
        if source.count(needle)!=1:raise ValueError('Exact filter mutant seam drift '+name)
        overlay=project/name;target=overlay/path;target.parent.mkdir(parents=True);target.write_text(source.replace(needle,replacement,1))
        cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_mopp_ray_leaf_query_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'include'}" "{overlay/'src'}" "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
        if sanitizers:cmake+=f'target_compile_options({name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({name} PRIVATE -fsanitize=address,undefined)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
        '--output',str(output/(Path(compiler).name+'-filter-mutants')),'--filter','^('+'|'.join(d[0]for d in defects)+')$',
        '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
    for name,*_ in defects:command+=['--target',name]
    run=subprocess.run(command,capture_output=True,text=True)
    if run.returncode:raise ValueError('Filter mutation controls failed: '+(run.stdout+run.stderr)[-6000:])
    return dict(passed=True,compiler=compiler,rejected=[d[0]for d in defects],oracle_outcomes_changed=False)
