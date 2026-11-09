"""Compiled source defects for the private slab proof; no reference changes."""
import json
from pathlib import Path
import subprocess
import sys
from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN


def qualify(scratch,output,rows,io):
    project=scratch/'negative-controls';project.mkdir()
    # Actual original packets, banks and return/prefix outcomes are reused.
    # No mutated reference or expected numerical value is introduced.
    selected=[row for row in rows if row['phase']=='slab_boundary' or row['name'].startswith('slab_26_basic_0_')]
    assert any(row['phase']=='before_write' for row in selected)
    assert any('dual_nan' in row['name'] for row in selected)
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=selected)))
    frame='src/physics/runtime/native_tu15_mopp_ray_slab_frame.hpp'
    core='include/halo2/native_tu15_mopp_ray_slab_kernel.hpp'
    scale='src/physics/runtime/native_tu15_mopp_ray_scale_frame.hpp'
    sources={name:(ROOT/name).read_text() for name in (frame,core,scale)}
    mutations={
        'premature_spill_commit':(frame,((
            'Base::store(pc,s_.gp[4]+offset,pending.bits,[&]{scope_.commit(pending);},true);',
            'scope_.commit(pending);Base::store(pc,s_.gp[4]+offset,pending.bits,[]{},true);'),)),
        'unprotected_provider':(scale,((
            'scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();'),)),
        'hoisted_high_byte':(core,((
            'd.middle=frame.scalar(Read::high_middle,d);frame.store(Write::decode_low,d.low,d);',
            'd.middle=frame.scalar(Read::high_middle,d);d.high=frame.scalar(Read::high_high,d);frame.store(Write::decode_low,d.low,d);'),(
            'd.high=frame.scalar(Read::high_high,d);d.high=(d.high<<8u)+d.middle;',
            'd.high=(d.high<<8u)+d.middle;'))),
        'reversed_simd_add':(core,((
            'frame.add(first_term,d.last_term,VectorMath::result,d)',
            'frame.add(d.last_term,first_term,VectorMath::result,d)'),)),
    }
    wrapper=project/'expect_rejection.py'
    wrapper.write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nprint(json.dumps(dict(rejected=r.returncode!=0,exit_code=r.returncode,diagnostic=(r.stdout+r.stderr)[-4000:])))\nsys.exit(0 if r.returncode!=0 else 1)\n')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(slab_mutations LANGUAGES CXX)\ninclude(CTest)\n'
    for name,(path,replacements) in mutations.items():
        text=sources[path]
        for old,new in replacements:
            assert text.count(old)==1,(name,'Mutation boundary drift')
            text=text.replace(old,new)
        overlay=project/name;target=overlay/path;target.parent.mkdir(parents=True);target.write_text(text)
        cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_mopp_ray_slab_frame_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'include'}" "{overlay/'src'}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
        '--output',str(output/'negative-controls'),'--filter','^('+'|'.join(mutations)+')$']
    for name in mutations:command+=['--target',name]
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert result.returncode==0,'Slab compiled controls failed: '+result.stdout[-6000:]
    return dict(passed=True,compiled_mutations=list(mutations),actual_reference_cases=len(selected),reference_instructions_or_outcomes_modified=False)
