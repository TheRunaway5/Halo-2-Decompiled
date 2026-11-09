#!/usr/bin/env python3
"""Preserve the authenticated polygon owner through both complete generators."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_polygon_support_replacements as binding
from specialize_native_operands import specialize

ROOT=Path(__file__).resolve().parents[1]
CORRESPONDENCE='analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011/correspondence-v2/complete-report.json'
CORRESPONDENCE_PIN='257cf7d69e56957067898e7bf2df455f9bc54e93b8851dafc2e8fac74edbb17b'
FILES=(
    'src/physics/runtime/physics_world_runtime_generated.cpp',
    'src/physics/runtime/physics_contact_runtime_generated.cpp',
    'tools/fix_physics_world_observation_tu15.py',
    'tools/generate_physics_world_contact_runtime_20261011.py',
    'tools/research_physics_world_contact_runtime_20261011.py',
    'tools/specialize_native_operands.py',
    'tools/native_polygon_support_replacements.py',
    'tools/validate_native_polygon_support_generation_tu15.py',
    'src/physics/runtime/native_tu15_polygon_support_frame.hpp',
    'include/halo2/native_tu15_polygon_support_kernel.hpp',
    'tools/native_mopp_aabb_replacements.py',
    'tools/native_mopp_aabb_canonical_replacements.py',
)
PREPARED_TARGETS={
    'world':'207092075ad2a3e1a837d4943583b21412cd336c9867b3cf13fa8023b74de551',
    'contact':'171098f13a4096d7ed80aeb6e861853f574e44dcf1c9bc4d6afa50a5660983d1',
}


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def hashes():return {name:sha(ROOT/name)for name in FILES}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='clang++')
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=hashes(),units=[])
    try:
        paired=ROOT/CORRESPONDENCE
        if sha(paired)!=CORRESPONDENCE_PIN:raise ValueError('Complete82 correspondence drift')
        source={label:(ROOT/name).read_text()for label,name in zip(('world','contact'),FILES[:2],strict=True)}
        frozen=scratch/'world-frozen.cpp';frozen.write_text(source['world'])
        commands=(('world',[sys.executable,'-B',str(ROOT/'tools/fix_physics_world_observation_tu15.py'),
            '--source',str(frozen),'--output',str(scratch/'world.cpp'),'--manifest',str(scratch/'world.json')]),
            ('contact',[sys.executable,'-B',str(ROOT/'tools/generate_physics_world_contact_runtime_20261011.py'),
            '--correspondence',str(paired),'--output',str(scratch/'contact.cpp'),'--manifest',str(scratch/'contact.json')]))
        for label,command in commands:
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Complete '+label+' generator failed '+(done.stdout+done.stderr)[-5000:])
            generated=(scratch/(label+'.cpp')).read_text()
            if generated!=source[label]:
                delta=''.join(difflib.unified_diff(source[label].splitlines(True),generated.splitlines(True),fromfile='frozen-current',tofile='generated'))
                if len(delta.encode())<100000:report[label+'_bounded_diff']=delta
                raise ValueError('Complete '+label+' regeneration changed current source bytes')
            match,_=binding.authenticate(generated)
            if binding.PREFIX not in match[0]or binding.replace_polygon_support(generated)!=generated:
                raise ValueError('Complete '+label+' polygon default missing')
            prepared,receipt=specialize(generated)
            body=binding.BODY.search(prepared)[0]
            if binding.digest(body)!=PREPARED_TARGETS[label]or receipt['native_polygon_support_entries']!=['1f6610']:
                raise ValueError('Qualified derived '+label+' polygon target drift')
            (scratch/(label+'-prepared.cpp')).write_text(prepared)
            for name in(label+'.cpp',label+'-prepared.cpp'):
                compile_command=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(scratch/name)]
                done=subprocess.run(compile_command,capture_output=True,text=True)
                if done.returncode:raise ValueError('Complete '+name+' syntax failed '+done.stderr[-5000:])
            metadata=json.loads((scratch/(label+'.json')).read_text())
            # The pinned quaternion repair places a helper after the original
            # static token; its routine remains in the anonymous namespace.
            owners=len(re.findall(r'(?:static )?void run_[0-9a-f]+\((?:ObservedWorldState|State) &s\)\s*\{',generated))
            if owners!={'world':477,'contact':82}[label]:raise ValueError('Complete generated owner count changed')
            report['units'].append(dict(owner=label,owners=owners,whole_source_byte_equal=True,raw_and_prepared_syntax=True,
                raw_sha256=binding.digest(generated),prepared_sha256=binding.digest(prepared),
                raw_target_sha256=binding.digest(match[0]),prepared_target_sha256=binding.digest(body),
                native_polygon_support_entries=receipt['native_polygon_support_entries'],
                old_baseline_sha256=metadata.get('canonical_baseline_sha256'),
                preserved_native_overlays=metadata.get('preserved_native_overlays'),
                correspondence_sha256=metadata.get('correspondence_sha256')))
        report.update(passed=True,original_complete_correspondence_sha256=CORRESPONDENCE_PIN,
            whole_runtime_or_other_image_qualification=False)
    except BaseException as error:
        report.update(passed=False,error=repr(error));raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS complete477/82 generation, old baseline, exact polygon targets and raw/prepared syntax')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
