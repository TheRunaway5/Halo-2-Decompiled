#!/usr/bin/env python3
"""Preserve the existing public box-owner DATA and sixteen-byte event contract."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
REFERENCE_PIN='7221333bc8f9e8d4b7446c3e46976b5c98ba9067b46d412f1cfe3a6f342694d3'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
OLD_BOX='        const auto sign=m.vector(0x461960u),input=m.vector(direction),value=_mm_xor_ps(vertex,_mm_and_ps(input,sign));\n        const auto feature=std::uint32_t(_mm_movemask_ps(value))&7u;\n        m.vector(0x3067bfu,output,value);m.word(0x3067c2u,output+0xcu,feature|0x3f000000u);return;'
NEW_BOX='        const auto sign=m.vector(0x461960u),input=m.vector(direction);\n        const auto point=native_tu15_box_shape_support_point(std::bit_cast<std::array<std::uint32_t,4>>(vertex),std::bit_cast<std::array<std::uint32_t,4>>(sign),std::bit_cast<std::array<std::uint32_t,4>>(input));\n        m.vector(0x3067bfu,output,std::bit_cast<__m128>(point.point));m.word(0x3067c2u,output+0xcu,point.feature|0x3f000000u);return;'
INCLUDE='#include "halo2/native_tu15_box_shape_support_kernel.hpp"\n'
def public_overlay(text):
    if text.count(OLD_BOX)==1 and NEW_BOX not in text:
        if INCLUDE in text:raise ValueError('Unqualified box DATA header without expected owner')
        return INCLUDE+text.replace(OLD_BOX,NEW_BOX,1)
    if text.count(NEW_BOX)==1 and OLD_BOX not in text and text.count(INCLUDE)==1:return text
    raise ValueError('Complete public box DATA branch drift')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pending=['include/halo2/native_tu15_box_shape_support_kernel.hpp','src/physics/native_tu15_contact_shape.cpp','tests/native_tu15_box_shape_support_value_tests.cpp',str(Path(__file__).relative_to(ROOT))];seen=set(pending)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        if sha(ROOT/'tests/reference/native_tu15_contact_shape_support_before.inc')!=REFERENCE_PIN:raise ValueError('Complete frozen public support reference drift')
        current=(ROOT/'src/physics/native_tu15_contact_shape.cpp').read_text();candidate=public_overlay(current)
        if a.require_binding and candidate!=current:raise ValueError('Shared public box DATA reuse is not installed')
        source=scratch/'public-box-candidate.cpp';source.write_text(candidate);report['public_candidate_sha256']=sha(source);report['production_bound']=a.require_binding
        command=[a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_box_shape_support_value_tests.cpp'),str(source),'-o',str(scratch/'value')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Box public value compile failed: '+built.stderr[-8000:])
        run=subprocess.run([str(scratch/'value')],capture_output=True,text=True)
        if run.returncode:raise ValueError('Box public value differs: '+run.stderr[-7000:])
        report.update(passed=True,value=json.loads(run.stdout.strip().splitlines()[-1]),scope='Existing scalar box-owner acceptance/protocol, no new raw FP/MMX or production whole-parent closure.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Box public value source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
