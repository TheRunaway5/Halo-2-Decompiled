#!/usr/bin/env python3
"""Compare retained actual 313AB0, 314D80 and real 314A30 call/return transport."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess

import validate_native_solver_hull_tetrahedron_frame_tu15 as tetra
import validate_native_solver_hull_seed_facets_frame_tu15 as seed
import validate_native_solver_hull_facet_plane_frame_tu15 as plane
from validate_native_solver_hull_facet_plane_source_tu15 import ESTIMATE, ESTIMATE_PIN
from validate_native_tu15_prt_helpers import PIN
from research_mp_sandbox_update_functions import Xbe

ROOT=tetra.ROOT
FRAME='src/physics/runtime/native_tu15_solver_hull_tetrahedron_source_frame.hpp'
TEST='tests/native_tu15_solver_hull_tetrahedron_source_tests.cpp'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def exact_bodies():
    bodies=(tetra.actual_body(),seed.actual_body(),plane.actual_body())
    helper=ESTIMATE.findall(tetra.SOURCE.read_text())
    if len(helper)!=1 or tuple(digest(body.encode())for body in (*bodies,helper[0]))!=(
            tetra.BODY_PIN,seed.BODY_PIN,plane.BODY_PIN,ESTIMATE_PIN):
        raise ValueError('Complete actual tetrahedron/seed/Plane/estimate source drift')
    return *bodies,helper[0]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++')
    p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--ordered-plane-reference',action='store_true')
    p.add_argument('--ordered-seed-reference',action='store_true')
    p.add_argument('--ordered-plane-cross-reference',action='store_true')
    a=p.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    names=(FRAME,TEST,'tools/validate_native_solver_hull_tetrahedron_frame_tu15.py',
           'tools/validate_native_solver_hull_seed_facets_frame_tu15.py','tools/validate_native_solver_hull_facet_plane_frame_tu15.py',
           'tools/validate_native_solver_hull_facet_plane_source_tu15.py',
           'src/physics/runtime/native_tu15_solver_hull_seed_facets_frame.hpp',
           'src/physics/runtime/native_tu15_solver_hull_tetrahedron_frame.hpp',
           'src/physics/runtime/native_tu15_solver_hull_facet_plane_source_frame.hpp',
           'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
           'include/halo2/native_tu15_solver_hull_seed_facets_kernel.hpp',
           'include/halo2/native_tu15_solver_hull_tetrahedron_kernel.hpp','include/halo2/native_tu15_solver_hull_facet_plane_kernel.hpp',
           'tests/native_tu15_mopp_ray_frame_test_memory.hpp',str(Path(__file__).relative_to(ROOT)))
    pins={name:digest((ROOT/name).read_bytes())for name in names}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if not a.xbe.is_file():return 77
        if digest(a.xbe.read_bytes())!=PIN:raise ValueError('Wrong TU1.5 original image')
        parent,child_seed,child_plane,helper=exact_bodies()
        image=Xbe(a.xbe)
        ordered_helper=''
        if a.ordered_seed_reference and not a.ordered_plane_reference:
            raise ValueError('Ordered seed reference requires the independently authenticated Plane ordering')
        if a.ordered_plane_cross_reference and not a.ordered_seed_reference:
            raise ValueError('Ordered Plane cross comparison requires both previously authenticated orders')
        if a.ordered_plane_reference:
            from oracle_native_campaign_screen_lifecycle_tu15 import _Code
            operation=_Code(image,'.text').at(0x314aef)
            if (operation.mnemonic,operation.op_str,bytes(operation.bytes).hex())!=('mulps','xmm0, xmm1','0f59c1'):
                raise ValueError('Original Plane paired-NaN operand-order drift')
            anchor='if(s.services.instruction){s.services.instruction(0x314aefu);} s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[1]);'
            if child_plane.count(anchor)!=1:
                raise ValueError('Complete authored final Plane multiply drift')
            child_plane=child_plane.replace(anchor,anchor.replace('_mm_mul_ps','ordered_final_mul'),1)
            ordered_helper='static __m128 ordered_final_mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}'
            report['ordered_reference_site']=dict(pc='314aef',opcode=operation.mnemonic,operands=operation.op_str,
                                                bytes=bytes(operation.bytes).hex(),changed_plane_body_sha256=digest(child_plane.encode()))
        if a.ordered_seed_reference:
            operation=_Code(image,'.text').at(0x314e4f)
            if (operation.mnemonic,operation.op_str,bytes(operation.bytes).hex())!=('mulps','xmm0, xmm1','0f59c1'):
                raise ValueError('Original Seed paired-NaN operand-order drift')
            anchor='if(s.services.instruction){s.services.instruction(0x314e4fu);} s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[1]);'
            if child_seed.count(anchor)!=1:
                raise ValueError('Complete authored Seed projection multiply drift')
            child_seed=child_seed.replace(anchor,anchor.replace('_mm_mul_ps','ordered_final_mul'),1)
            report['ordered_seed_reference_site']=dict(pc='314e4f',opcode=operation.mnemonic,operands=operation.op_str,
                bytes=bytes(operation.bytes).hex(),changed_seed_body_sha256=digest(child_seed.encode()))
        if a.ordered_plane_cross_reference:
            operation=_Code(image,'.text').at(0x314a70)
            if (operation.mnemonic,operation.op_str,bytes(operation.bytes).hex())!=('mulps','xmm1, xmm2','0f59ca'):
                raise ValueError('Original Plane paired-NaN cross operand-order drift')
            anchor='if(s.services.instruction){s.services.instruction(0x314a70u);} s.xmm[1]=_mm_mul_ps(s.xmm[1],s.xmm[2]);'
            if child_plane.count(anchor)!=1:
                raise ValueError('Complete authored Plane cross multiply drift')
            child_plane=child_plane.replace(anchor,anchor.replace('_mm_mul_ps','ordered_final_mul'),1)
            report['ordered_plane_cross_reference_site']=dict(pc='314a70',opcode=operation.mnemonic,
                operands=operation.op_str,bytes=bytes(operation.bytes).hex(),changed_plane_body_sha256=digest(child_plane.encode()))
        parent_io=tetra.authored_io(parent);seed_io=seed.authored_io(child_seed)
        io=sorted(set(plane.IO).union(*parent_io,*seed_io))
        body='\n'.join(('static constexpr std::array io_sites{'+','.join(hex(pc)+'u'for pc in io)+'};',ordered_helper,
                        helper.replace('source_estimate(WorldState','source_estimate(TetraState',1),
                        child_plane.replace('run_314a30(WorldState','plane(TetraState',1),
                        child_seed.replace('run_314d80(WorldState','seed(TetraState',1),
                        parent.replace('run_313ab0(WorldState','tetrahedron(TetraState',1)))
        (scratch/'solver_hull_tetrahedron_source.inc').write_text(body+'\n')
        data=scratch/'inputs.json'
        data.write_text(json.dumps(seed.inputs(image)+tetra.inputs(image),separators=(',',':')))
        command=[a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),
                 str(ROOT/TEST),'-o',str(scratch/'tetrahedron-source')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report.update(command=command,input_sha256=digest(data.read_bytes()),tetrahedron_body_sha256=tetra.BODY_PIN,
                      seed_body_sha256=seed.BODY_PIN,plane_body_sha256=plane.BODY_PIN,estimate_helper_sha256=ESTIMATE_PIN)
        built=subprocess.run(command,capture_output=True,text=True,timeout=900)
        if built.returncode:raise ValueError('Private tetrahedron source compile: '+built.stderr[-9000:])
        with data.open('rb')as stream:result=subprocess.run([command[-1]],stdin=stream,capture_output=True,text=True,timeout=900)
        if result.returncode:raise ValueError(result.stderr[-9000:])
        report.update(passed=True,comparisons=json.loads(result.stdout),
                      scope='Retained ABI/publication including genuine Seed Facets and all five actual Plane calls; no supplied outcomes.')
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
