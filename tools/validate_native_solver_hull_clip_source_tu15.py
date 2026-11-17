#!/usr/bin/env python3
"""Compare the two genuine 3147D0 source spill/C1 profiles, without binding."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import struct
import subprocess
import sys

from generated_routine_ownership import function_bodies
from research_mp_sandbox_update_functions import Xbe
from validate_native_solver_hull_clip_frame_tu15 import BODY_PIN, IO_DEPTH, inputs
from validate_native_tu15_prt_helpers import PIN

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
GENERATOR = 'tools/generate_physics_live_support_tu15.py'
FRAME = 'src/physics/runtime/native_tu15_solver_hull_clip_source_frame.hpp'
TEST = 'tests/native_tu15_solver_hull_clip_source_tests.cpp'
FRESH_PIN = '047f77092e14911acabb44eb57e7f83024cba069793758d05d4d2811833bd071'
SPILL_PIN = 'e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
SPILL = re.compile(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}', re.S)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def sha(path):
    return digest(path.read_bytes())


def body(path, expected):
    result = [body for sig, body in function_bodies(path.read_text()) if sig.symbol == '3147d0']
    if len(result) != 1 or digest(result[0].encode()) != expected:
        raise ValueError('Complete genuine 3147D0 source profile drift')
    return result[0]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--mutations', action='store_true')
    p.add_argument('--fresh-primary', type=Path)
    p.add_argument('--fresh-sha256')
    a = p.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        p.error('Run through tools/run_test.py')
    if bool(a.fresh_primary) != bool(a.fresh_sha256):
        p.error('Borrowed genuine generation must supply its whole source hash')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686') or not a.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    names = (PRIMARY, GENERATOR, FRAME, TEST, 'include/halo2/native_tu15_solver_hull_clip_kernel.hpp',
             'src/physics/runtime/physics_world_runtime_internal.hpp', 'src/physics/runtime/physics_contact_runtime_internal.hpp',
             'src/physics/runtime/physics_step_internal.hpp', 'tools/validate_native_solver_hull_clip_frame_tu15.py',
             'tools/validate_native_solver_hull_facet_clip_tu15.py', 'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
             str(Path(__file__).relative_to(ROOT)))
    pins = {name: sha(ROOT / name) for name in names}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch),
                  source_sha256=pins, retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong original TU1.5 image')
        fresh = scratch / 'fresh.cpp'
        if a.fresh_primary:
            shutil.copyfile(a.fresh_primary, fresh)
            if sha(fresh) != a.fresh_sha256:
                raise ValueError('Borrowed genuine full source drift')
            report['borrowed_generated_source'] = dict(path=str(a.fresh_primary),sha256=a.fresh_sha256,
                                                        private_copy_before_compiler=True)
        else:
            command = [sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--manifest',str(scratch/'fresh-manifest.json')]
            generate = subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=900)
            if generate.returncode:
                raise ValueError('Genuine Primary generation failed: '+generate.stderr[-8000:])
            report['generation_command'] = command
        current = body(ROOT / PRIMARY, BODY_PIN)
        fresh_body = body(fresh, FRESH_PIN)
        helpers = SPILL.findall(fresh.read_text())
        if len(helpers) != 1 or digest(helpers[0].encode()) != SPILL_PIN:
            raise ValueError('Complete generated native-single-spill helper drift')
        io = 'static constexpr std::array io_sites{' + ','.join(hex(pc)+'u' for pc in sorted(IO_DEPTH)) + '};'
        reference = ('namespace clip_source {\n'+io+'\n'+helpers[0]+'\n'+
                     current.replace('run_3147d0(WorldState','current(ClipState',1)+'\n'+
                     fresh_body.replace('run_3147d0(WorldState','fresh(ClipState',1)+'\n}\n')
        (scratch/'solver_hull_clip_source.inc').write_text(reference)
        data = scratch/'inputs.json'
        rows=inputs(Xbe(a.xbe))
        directed=copy.deepcopy(next(row for row in rows if row['name']=='clip_lower_update_0_0'))
        directed.update(id=len(rows),name='fresh-directed-rounding-spill')
        directed['before_fp'][0]=0xb7f
        bank=bytearray.fromhex(directed['banks'][0]['before'])
        struct.pack_into('<4I',bank,0x100,0x3f800000,0,0,0)
        struct.pack_into('<4I',bank,0x200,0,0,0,0)
        struct.pack_into('<4I',bank,0x300,0x3f800000,0,0,0)
        struct.pack_into('<2I',bank,0x400,0x3f800000,0x40000000)
        directed['banks'][0]['before']=bank.hex()
        rows.append(directed)
        data.write_text(json.dumps(rows,separators=(',',':')))
        command = [a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off',
                   '-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),
                   '-I'+str(scratch),str(ROOT/TEST),'-o',str(scratch/'clip-source')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report.update(command=command, current_body_sha256=BODY_PIN, fresh_body_sha256=FRESH_PIN,
                      fresh_spill_sha256=SPILL_PIN, input_sha256=sha(data))
        compile_result = subprocess.run(command,capture_output=True,text=True,timeout=900)
        if compile_result.returncode:
            raise ValueError('Private retained clip compilation failed: '+compile_result.stderr[-8000:])
        with data.open('rb') as stream:
            result = subprocess.run([command[-1]],stdin=stream,capture_output=True,text=True,timeout=900)
        if result.returncode:
            raise ValueError(result.stderr[-8000:])
        if a.mutations:
            text=(ROOT/FRAME).read_text()
            relative=Path(FRAME).relative_to('src')
            faults=(
                ('premature_source_pop','w(s_.memory,s_.observe,0x314849u,s_.gp[1],4u,spill_(s_));s_.fp.pop();',
                 'const auto bits=spill_(s_);s_.fp.pop();w(s_.memory,s_.observe,0x314849u,s_.gp[1],4u,bits);'),
                ('omitted_fresh_spill_c1','w(s_.memory,s_.observe,0x314849u,s_.gp[1],4u,spill_(s_));',
                 'w(s_.memory,s_.observe,0x314849u,s_.gp[1],4u,u(spill(s_.fp.st(0))));'),
                ('stale_projection_lane','load(0x3147ebu,1u,g[5]+12u);\n        x[0]=_mm_mul_ps(x[0],x[1]);',
                 'const auto stale=x[1];load(0x3147ebu,1u,g[5]+12u);\n        x[0]=_mm_mul_ps(x[0],stale);'),
            )
            report['compiled_defects']={}
            for name,old,new in faults:
                if text.count(old)!=1:raise ValueError('Exact retained clip defect source drift '+name)
                overlay=scratch/name
                header=overlay/relative;header.parent.mkdir(parents=True)
                header.write_text(text.replace(old,new,1))
                trial=command.copy();trial.insert(trial.index('-I'+str(ROOT/'include')),'-I'+str(overlay))
                trial[-1]=str(overlay/'clip-source')
                compiled=subprocess.run(trial,capture_output=True,text=True,timeout=900)
                if compiled.returncode:raise ValueError('Clip source defect compile failed '+name+': '+compiled.stderr[-5000:])
                with data.open('rb')as stream:
                    mutant=subprocess.run([trial[-1]],stdin=stream,capture_output=True,text=True,timeout=900)
                if not mutant.returncode or 'Complete retained hull clip differs' not in mutant.stderr:
                    raise ValueError('Retained clip defect missed its witness '+name+': '+mutant.stderr[-5000:])
                report['compiled_defects'][name]=mutant.stderr.strip()[-4000:]
        report.update(passed=True, comparisons=json.loads(result.stdout),
                      scope='Private retained source FP, ABI and stopped publication; no hardware fault or default claim.')
    except Exception as error:
        report['error'] = repr(error)
    report['sources_unchanged'] = all(sha(ROOT/name)==value for name,value in pins.items())
    report['passed'] &= report['sources_unchanged']
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','error') if k in report}))
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
