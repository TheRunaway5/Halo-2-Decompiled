#!/usr/bin/env python3
"""Reproduce the complete checked-in shadow translation in owned test storage.

Run with the repository oracle Python through tools/run_test.py. The exact
lowered entry set replaces unavailable historical traces; every byte still
comes from the pinned original executable and unchanged generator guards.
"""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(data):return hashlib.sha256(data).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--translated-model',action='store_true',help='Compare the genuine136CE0 call variant')
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    reference=ROOT/'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'
    before=reference.read_bytes();text=before.decode()
    entries=sorted({int(x,16)for x in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &\);$',text,re.M)})
    output=scratch/'shadow-generated.cpp'
    command=[sys.executable,'-B',str(ROOT/'tools/generate_shadow_frame_tu15.py'),
        '--directory',str(scratch/'generation'),'--output',str(output)]
    expected=before
    if args.translated_model:
        command+=['--keep-translated-model-call']
        expected=before.replace(b's.required(0x136ce0u,0x2cbd0u,false);',b'run_2cbd0(s);')
    for entry in entries:command+=['--additional-entry',hex(entry)]
    result=subprocess.run(command,capture_output=True,text=True,timeout=180,cwd=ROOT)
    after=output.read_bytes()if output.is_file()else b''
    record=dict(passed=result.returncode==0 and expected==after,returncode=result.returncode,
        command=command,entries=len(entries),reference_sha256=sha(before),generated_sha256=sha(after),
        reference_unchanged=reference.read_bytes()==before,owned_scratch=str(scratch),
        translated_model=args.translated_model,expected_sha256=sha(expected),
        tail=(result.stdout+result.stderr)[-4000:],retained_large_artifacts=[])
    if result.returncode==0:
        fresh=after.decode()
        lines=list(difflib.unified_diff(text.splitlines(),fresh.splitlines(),n=0,lineterm=''))
        record['changed_lines']=sum(x.startswith(('+','-'))and not x.startswith(('+++','---'))for x in lines)
        record['diff_sample']=[x[:240]for x in lines[:60]]
        bodies=lambda s:{entry:body for body,entry in re.findall(r'(static void run_([0-9a-f]+)\(WorldState &s\) \{.*?\n\})',s,re.S)}
        old,new=bodies(text),bodies(fresh)
        record['changed_bodies']=[entry for entry in old if old[entry]!=new.get(entry)]
        selected=lambda s:set(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',
            next(line for line in s.splitlines()if line.startswith('std::uint32_t selected('))))
        old_cases,new_cases=selected(text),selected(fresh)
        record['address_cases_removed']=sorted(old_cases-new_cases)
        record['address_cases_added']=sorted(new_cases-old_cases)
        record['retired_camera_bodies_absent']=all(f'run_{entry:x}('not in fresh for entry in (0x2c470,0x144090))
        record['passed']&=record['retired_camera_bodies_absent']
    record['passed']&=record['reference_unchanged']
    record['input_sha256']={str(path.relative_to(ROOT)):sha(path.read_bytes())for path in (
        Path(__file__),reference,ROOT/'tools/generate_shadow_frame_tu15.py',
        ROOT/'tools/generate_physics_live_support_tu15.py',ROOT/'tools/native_normalize_vector_bridge_tu15.py')}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps({k:v for k,v in record.items()if k in ('passed','returncode','changed_lines','changed_bodies','address_cases_removed','address_cases_added')}))
    return int(not record['passed'])


if __name__=='__main__':raise SystemExit(main())
