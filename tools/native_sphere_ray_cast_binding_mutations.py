"""Actual compiled source-route/admission defects, without oracle changes."""
from pathlib import Path
import subprocess
import sys
from oracle_smg_firing_1_1 import ROOT

def qualify(scratch,output,fixture,compiler,sanitizers):
    project=scratch/('sphere-binding-mutants-'+Path(compiler).name);project.mkdir()
    path='src/physics/runtime/native_tu15_sphere_ray_cast.hpp'
    header=(ROOT/path).read_text();bodies=(scratch/'sphere_source_bodies.inc').read_text()
    prefix='if(halo2::native_sphere_ray_cast_binding_detail::bridge(s))return;'
    if bodies.count(prefix)!=4:raise ValueError('Sphere source-prefix mutation seam drift')
    defects=(('disabled_source_prefix',None,None),
        ('excess_live_capacity','s.fp.count>5u','s.fp.count>6u'),
        ('wrong_reference_control','std::getenv("HALO2_REFERENCE_SPHERE_RAY_CAST")','std::getenv("HALO2_REFERENCE_INSTANCED_RAY_CHILD")'))
    wrapper=project/'reject.py';wrapper.write_text('''import os,subprocess,sys
name=sys.argv[1];env=dict(os.environ)
if name=="wrong_reference_control":env["HALO2_REFERENCE_SPHERE_RAY_CAST"]="1"
r=subprocess.run(sys.argv[2:],capture_output=True,text=True,env=env)
expected={"disabled_source_prefix":("Sphere source literal instruction routing changed","sphere actual source route differs"),"excess_live_capacity":("Refused sphere source bridge wrote a bank",),"wrong_reference_control":("Reference control was not set",)}[name]
assert r.returncode==1 and any(t in r.stderr for t in expected),r.stdout+r.stderr
print("Rejected actual sphere binding defect "+name)
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(sphere_binding_mutants LANGUAGES CXX)\ninclude(CTest)\n'
    for name,needle,replacement in defects:
        overlay=project/name;overlay.mkdir()
        if name=='disabled_source_prefix':
            (overlay/'sphere_source_bodies.inc').write_text(bodies.replace(prefix,prefix.replace('if(','if(false&&',1)))
            source=ROOT/'tests/native_tu15_sphere_ray_cast_source_tests.cpp';arguments=f'"{fixture}"'
        else:
            if header.count(needle)!=1:raise ValueError('Sphere bridge mutation seam drift '+name)
            target=overlay/path;target.parent.mkdir(parents=True);target.write_text(header.replace(needle,replacement,1))
            source=ROOT/'tests/native_tu15_sphere_ray_cast_admission_tests.cpp';arguments='--reference'if name=='wrong_reference_control'else''
        cmake+=f'''add_executable({name} "{source}")
target_include_directories({name} PRIVATE "{overlay}" "{overlay/'src'}" "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" {name} $<TARGET_FILE:{name}> {arguments})
'''
        if sanitizers:cmake+=f'target_compile_options({name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({name} PRIVATE -fsanitize=address,undefined)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    cmd=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
         '--output',str(output/(Path(compiler).name+'-binding-mutants')),'--filter','^('+'|'.join(d[0]for d in defects)+')$',
         '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
    for name,*_ in defects:cmd+=['--target',name]
    result=subprocess.run(cmd,capture_output=True,text=True)
    if result.returncode:raise ValueError('Sphere binding defect controls failed: '+(result.stdout+result.stderr)[-6000:])
    return dict(passed=True,compiler=compiler,rejected=[d[0]for d in defects],oracle_outcomes_changed=False)
