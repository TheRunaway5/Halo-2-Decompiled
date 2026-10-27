#!/usr/bin/env python3
"""Check the native void-frame comparison contract against untouched TU1.5.

This is a replay-harness regression, not live gameplay qualification. A declared
null world exercises the genuine original return without any dependency result
replacement. All raw banks and original binaries stay in run_test's scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=False);cases=[]
    for name,kind in (('clean','clean'),('dead-stack','dead-stack'),('bad-heap','heap'),('bad-fp','fp')):
        capture=scratch/name;capture.mkdir()
        banks={0x52b000:bytearray(4096),0x1000000:bytearray(4096),0x600e0000:bytearray(0x20000),0x60200000:bytearray(4096)}
        struct.pack_into('<I',banks[0x1000000],0,0x12345678)
        struct.pack_into('<I',banks[0x600e0000],0x10000,0x60300000)
        before=dict(gp=[0,0,0,0,0x600f0000,0,0,0],xmm=[[0]*4 for _ in range(8)],eflags=2,fs_base=0x60200000,
                    x87_values=[],x87_status=0,x87_control_word=0x27f,mxcsr=0x1f80)
        after={key:before[key] for key in ('x87_values','x87_status','x87_control_word','mxcsr')}
        record=dict(entry=0x1cc8d0,return_=0x60300000,before=before,after=after,regions=[],
                    reference='English TU1.5 only',comparison_contract='native_tu15_void_world_frame_v1',
                    private_returned_stack=dict(first=0x600e0000,end=0x600f0000),
                    boundary='Declared null-world replay-contract regression, not an actual gameplay capture')
        record['return']=record.pop('return_')
        for address,bank in banks.items():
            file=f'before-{address}.bin';(capture/file).write_bytes(bank)
            region=dict(address=address,size=len(bank),before_file=file)
            if (kind=='heap' and address==0x1000000) or (kind=='dead-stack' and address==0x600e0000):
                changed=bytearray(bank);changed[0]^=0x80
                file=f'after-{address}.bin';(capture/file).write_bytes(changed);region['after_file']=file
            record['regions'].append(region)
        if kind=='fp':after['mxcsr']|=1
        (capture/'capture.json').write_text(json.dumps(record))
        output=scratch/(name+'-original')
        command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),
                 '--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(output)]
        run=subprocess.run(command,text=True,capture_output=True,timeout=90)
        if not (output/'report.json').exists():
            (args.output/(name+'-failure.log')).write_text((run.stdout+run.stderr)[-32768:])
            raise RuntimeError(f'{name}: original replay did not produce its real result')
        report=json.loads((output/'report.json').read_text())
        fields=[row['field']for row in report['differences']]
        accepted=run.returncode==0 and report['passed'] is True
        expected=kind in ('clean','dead-stack')
        passed=accepted==expected and (expected or ('memory' if kind=='heap' else 'mxcsr') in fields)
        row=dict(name=name,passed=passed,replay_accepted=accepted,expected_acceptance=expected,
                 differences=report['differences'],compared_memory_bytes=report['compared_memory_bytes'],
                 returned_private_stack_bytes=report['returned_private_stack_bytes'],command=command,
                 original_sha256=report['original_sha256'],replay_report_sha256=sha(output/'report.json'))
        cases.append(row)
        if not passed:raise RuntimeError(f'{name}: comparison contract failed its unchanged-original control')
    report=dict(passed=all(row['passed']for row in cases),cases=cases,
                scope='Independent original null-world harness controls only; composed multiplayer/native-only physics remains separate',
                source_hashes={str(path.relative_to(ROOT)):sha(path)for path in
                    (Path(__file__),ROOT/'tools/replay_live_phantom_hardware_tu15.py')},retained_large_artifacts=[])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],controls=len(cases),retained_large_artifacts=[])))
    return 0


if __name__=='__main__':raise SystemExit(main())
