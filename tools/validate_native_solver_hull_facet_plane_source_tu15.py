#!/usr/bin/env python3
"""Qualify private 314A30 source publication and its actual reciprocal-estimate request."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess

from research_mp_sandbox_update_functions import Xbe
from validate_native_solver_hull_facet_plane_frame_tu15 import BODY_PIN, IO, SOURCE, actual_body, inputs
from validate_native_tu15_prt_helpers import PIN

ROOT = Path(__file__).resolve().parents[1]
FRAME = 'src/physics/runtime/native_tu15_solver_hull_facet_plane_source_frame.hpp'
TEST = 'tests/native_tu15_solver_hull_facet_plane_source_tests.cpp'
ESTIMATE = re.compile(r'std::uint32_t source_estimate\(WorldState &s,std::uint32_t input\) \{.*?\n\}',re.S)
ESTIMATE_PIN = 'bb50fdcd1d22250d871032ff227a462ef42c30981de20b9f23137bb1c60efd89'
JOURNAL = ('src/runtime/source_execution_inputs.cpp','src/runtime/source_runtime_memory.cpp',
           'src/runtime/source_runtime_snapshot_bytes.cpp','src/assets/cache_file_snapshot.cpp',
           'src/runtime/source_runtime_write_protection.inc')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def sha(path):
    return digest(path.read_bytes())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--ordered-reference', action='store_true')
    p.add_argument('--mutations', action='store_true')
    a = p.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        p.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686') or not a.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    paths = (SOURCE, ROOT/FRAME, ROOT/TEST, ROOT/'src/physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp',
             ROOT/'include/halo2/native_tu15_solver_hull_facet_plane_kernel.hpp',
             ROOT/'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
             ROOT/'tools/validate_native_solver_hull_facet_plane_frame_tu15.py', Path(__file__),
             *(ROOT/name for name in JOURNAL))
    pins = {str(path.relative_to(ROOT)): sha(path) for path in paths}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch),
                  source_sha256=pins, retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong original TU1.5 image')
        image=Xbe(a.xbe)
        from oracle_native_campaign_screen_lifecycle_tu15 import _Code
        code=_Code(image,'.text')
        opcode=code.at(0x314a9a)
        if (opcode.mnemonic,opcode.op_str)!=('rsqrtss','xmm2, xmm0'):
            raise ValueError('Genuine facet plane estimate operand identity drift')
        body=actual_body();estimate=ESTIMATE.findall(SOURCE.read_text())
        if digest(body.encode())!=BODY_PIN or len(estimate)!=1 or digest(estimate[0].encode())!=ESTIMATE_PIN:
            raise ValueError('Complete authored facet plane or estimate helper drift')
        ordered_helper=''
        if a.ordered_reference:
            operation=code.at(0x314aef)
            if (operation.mnemonic,operation.op_str,bytes(operation.bytes).hex())!=('mulps','xmm0, xmm1','0f59c1'):
                raise ValueError('Original paired-NaN MULPS operand-order drift')
            anchor='if(s.services.instruction){s.services.instruction(0x314aefu);} s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[1]);'
            if body.count(anchor)!=1:
                raise ValueError('Complete authored final plane multiply drift')
            body=body.replace(anchor,anchor.replace('_mm_mul_ps','ordered_final_mul'),1)
            ordered_helper='static __m128 ordered_final_mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}\n'
            report['ordered_reference_site']=dict(pc='314aef',opcode=operation.mnemonic,operands=operation.op_str,
                                                   bytes=bytes(operation.bytes).hex(),
                                                   changed_body_sha256=digest(body.encode()))
        reference=('namespace plane_original {\nstatic constexpr std::array io_sites{'+
                   ','.join(hex(pc)+'u' for pc in IO)+'};\n'+
                   ordered_helper+
                   estimate[0].replace('source_estimate(WorldState','source_estimate(PlaneState',1)+'\n'+
                   body.replace('run_314a30(WorldState','retained(PlaneState',1)+'\n}\n')
        (scratch/'solver_hull_facet_plane_source.inc').write_text(reference)
        data=scratch/'inputs.json'
        data.write_text(json.dumps(inputs(),separators=(',',':')))
        command=[a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off',
                 '-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),
                 '-I'+str(scratch),str(ROOT/TEST),*(str(ROOT/name) for name in JOURNAL if name.endswith('.cpp')),
                 '-o',str(scratch/'facet-plane-source')]
        if a.sanitizers:
            command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report.update(command=command,input_sha256=sha(data),original_sha256=PIN,source_body_sha256=BODY_PIN,
                      estimate_helper_sha256=ESTIMATE_PIN,estimate_site_bytes=bytes(opcode.bytes).hex())
        compiled=subprocess.run(command,capture_output=True,text=True,timeout=900)
        if compiled.returncode:
            raise ValueError('Private facet-plane source compile: '+compiled.stderr[-8000:])
        with data.open('rb') as stream:
            result=subprocess.run([command[-1]],stdin=stream,capture_output=True,text=True,timeout=900)
        if result.returncode:
            raise ValueError(result.stderr[-8000:])
        if a.mutations:
            if not a.ordered_reference:
                raise ValueError('Compiled plane controls require the exact ordered final-multiply reference')
            defects=(
                ('wrong_estimate_operand',FRAME,'physics_step_detail::u(_mm_cvtss_f32(s_.xmm[0]))',
                 'physics_step_detail::u(_mm_cvtss_f32(s_.xmm[0]))^1u'),
                ('wrong_estimate_upper_lanes',FRAME,'s_.xmm[2]=_mm_move_ss(s_.xmm[2],','s_.xmm[2]=_mm_move_ss(s_.xmm[0],'),
                ('swapped_final_nan_operands','src/physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp',
                 'io_.load_vector(0x314aecu,1u,g[0]);x[0]=mul(x[0],x[1]);',
                 'io_.load_vector(0x314aecu,1u,g[0]);x[0]=mul(x[1],x[0]);'),
            )
            report['compiled_defects']={}
            for name,file,old,new in defects:
                text=(ROOT/file).read_text()
                if text.count(old)!=1:raise ValueError('Exact facet-plane defect source drift '+name)
                overlay=scratch/name;target=overlay/Path(file).relative_to('src');target.parent.mkdir(parents=True)
                target.write_text(text.replace(old,new,1))
                trial=command.copy();trial.insert(trial.index('-I'+str(ROOT/'include')),'-I'+str(overlay))
                trial[-1]=str(overlay/'facet-plane-source')
                built=subprocess.run(trial,capture_output=True,text=True,timeout=900)
                if built.returncode:raise ValueError('Facet-plane source defect did not compile '+name+': '+built.stderr[-6000:])
                with data.open('rb')as stream:
                    mutated=subprocess.run([trial[-1]],stdin=stream,capture_output=True,text=True,timeout=900)
                if not mutated.returncode or 'Complete retained facet plane differs' not in mutated.stderr:
                    raise ValueError('Facet-plane compiled defect missed its witness '+name+': '+mutated.stderr[-6000:])
                report['compiled_defects'][name]=mutated.stderr.strip()[-4000:]
        report.update(passed=True,comparisons=json.loads(result.stdout),
                      scope='Private retained source/estimate journal comparison; the optional phase changes one exact authored SIMD site.')
    except Exception as error:
        report['error']=repr(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    report['passed'] &= report['sources_unchanged']
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','error') if k in report}))
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
