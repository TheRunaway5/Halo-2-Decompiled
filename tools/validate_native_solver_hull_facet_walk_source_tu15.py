#!/usr/bin/env python3
"""Compare private retained 314C40/3148E0 and genuine 3147D0 child transport."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess

import validate_native_solver_hull_clip_frame_tu15 as clip
import validate_native_solver_hull_facet_search_frame_tu15 as search
import validate_native_solver_hull_facet_visit_frame_tu15 as visit
from validate_native_tu15_prt_helpers import PIN
from research_mp_sandbox_update_functions import Xbe

ROOT=search.ROOT
FRAME='src/physics/runtime/native_tu15_solver_hull_facet_search_frame.hpp'
TEST='tests/native_tu15_solver_hull_facet_walk_source_tests.cpp'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def exact_bodies():
    bodies=(search.actual_body(),visit.actual_body(),clip.actual_body())
    if tuple(digest(body.encode())for body in bodies)!=(search.BODY_PIN,visit.BODY_PIN,clip.BODY_PIN):
        raise ValueError('Complete actual facet search/visit/clip source drift')
    return bodies


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++')
    p.add_argument('--sanitizers',action='store_true')
    a=p.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    names=(FRAME,TEST,'tools/validate_native_solver_hull_facet_search_frame_tu15.py',
           'tools/validate_native_solver_hull_facet_visit_frame_tu15.py','tools/validate_native_solver_hull_clip_frame_tu15.py',
           'src/physics/runtime/native_tu15_solver_hull_facet_visit_frame.hpp',
           'src/physics/runtime/native_tu15_solver_hull_facet_visit_source_frame.hpp',
           'src/physics/runtime/native_tu15_solver_hull_clip_source_frame.hpp',
           'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
           'include/halo2/native_tu15_solver_hull_facet_visit_kernel.hpp',
           'include/halo2/native_tu15_solver_hull_facet_search_kernel.hpp','include/halo2/native_tu15_solver_hull_clip_kernel.hpp',
           'tests/native_tu15_mopp_ray_frame_test_memory.hpp',str(Path(__file__).relative_to(ROOT)))
    pins={name:digest((ROOT/name).read_bytes())for name in names}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if not a.xbe.is_file():return 77
        if digest(a.xbe.read_bytes())!=PIN:raise ValueError('Wrong TU1.5 original image')
        parent,child_visit,child_clip=exact_bodies()
        parent_io=search.authored_io(parent);visit_io=visit.authored_io(child_visit)
        io=sorted(set(clip.IO_DEPTH).union(*parent_io,*visit_io))
        body='\n'.join(('static constexpr std::array io_sites{'+','.join(hex(pc)+'u'for pc in io)+'};',
                        child_clip.replace('run_3147d0(WorldState','clip(FacetState',1),
                        child_visit.replace('run_3148e0(WorldState','visit(FacetState',1),
                        parent.replace('run_314c40(WorldState','search(FacetState',1)))
        (scratch/'solver_hull_facet_walk_source.inc').write_text(body+'\n')
        data=scratch/'inputs.json'
        image=Xbe(a.xbe)
        data.write_text(json.dumps(visit.inputs(image)+search.inputs(image),separators=(',',':')))
        command=[a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),
                 str(ROOT/TEST),'-o',str(scratch/'facet-walk-source')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report.update(command=command,input_sha256=digest(data.read_bytes()),search_body_sha256=search.BODY_PIN,
                      visit_body_sha256=visit.BODY_PIN,clip_body_sha256=clip.BODY_PIN)
        built=subprocess.run(command,capture_output=True,text=True,timeout=900)
        if built.returncode:raise ValueError('Private facet walk source compile: '+built.stderr[-9000:])
        with data.open('rb')as stream:result=subprocess.run([command[-1]],stdin=stream,capture_output=True,text=True,timeout=900)
        if result.returncode:raise ValueError(result.stderr[-9000:])
        report.update(passed=True,comparisons=json.loads(result.stdout),
                      scope='Retained ABI/publication including genuine recursion and complete Clip child; no supplied outcomes.')
    except Exception as error:report['error']=repr(error)
    try:
        exact_bodies()
        report['sources_unchanged']=all(digest((ROOT/name).read_bytes())==value for name,value in pins.items())
    except (ValueError,OSError,StopIteration):report['sources_unchanged']=False
    report['passed'] &= report['sources_unchanged']
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','error')if k in report}))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
