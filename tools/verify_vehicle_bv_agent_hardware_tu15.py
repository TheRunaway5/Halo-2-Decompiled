#!/usr/bin/env python3
"""Resolve the BV witness FP state with untouched original i386 CPU execution.

The initial entry comes from untouched original scheduler execution over the
actual native input bank. Emulator output is only a comparison, never a callee
return. The derived expected fixture comes exclusively from original hardware.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def converted(row):
    value=copy.deepcopy(row)
    value['xmm']=[list(struct.unpack('<4I',int(word,16).to_bytes(16,'little'))) for word in row['xmm']]
    return value


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report-output',type=Path,required=True)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not args.output.resolve().is_relative_to(owner):parser.error('Hardware input/output must stay in owned scratch')
    original=json.loads(args.original.read_text());fixture=Path(original['fixture'])
    assert original['original_kernel_returned'] and sha(fixture)==original['fixture_sha256']
    data=json.loads(fixture.read_text());case=data['cases'][0]
    assert case['entry']==0x2f18d0 and not case['timestamps'] and not case['estimates']
    args.output.mkdir(parents=True,exist_ok=False);capture=args.output/'capture';capture.mkdir()
    regions=[];final={row['address']:row['hex'] for row in case['final_banks']}
    for bank in case['initial_banks']:
        address=bank['address'];before=capture/f'before-{address}.bin';after=capture/f'after-{address}.bin'
        raw=bytes.fromhex(bank['hex']);before.write_bytes(raw);after.write_bytes(bytes.fromhex(final[address]))
        regions.append(dict(address=address,size=len(raw),before_file=before.name,after_file=after.name))
    # Both before and the comparison-only after are original-oracle data.
    record=dict(entry=case['entry'],return_=0x315a53,before=converted(case['entry_abi']),
                after=converted(case['exit']),regions=regions,reference='English TU1.5 only')
    record['return']=record.pop('return_')
    (capture/'capture.json').write_text(json.dumps(record,indent=2)+'\n')
    hardware=args.output/'hardware';tool=root/'tools/replay_live_phantom_hardware_tu15.py'
    command=[sys.executable,'-B',str(tool),'--capture',str(capture),'--output',str(hardware)]
    with (args.output/'hardware.log').open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=120)
    report_file=hardware/'report.json'
    report=dict(original_reference_completed=report_file.exists(),command=command,returncode=result.returncode,
                input_source='Original2F18D0 entry reached by untouched14FEC0 over the actual native scheduler input',
                original_oracle_report_sha256=sha(args.original),emulated_fixture_sha256=sha(fixture),
                hardware_method_sha256=sha(tool),native_expected_output_used=False,
                physical_xbox_cpu_or_full_parity=False,owner=str(owner))
    if report_file.exists():
        measured=json.loads(report_file.read_text());trace=json.loads((hardware/'trace.json').read_text())
        report.update(hardware_reference_report_sha256=sha(report_file),
                      hardware_vs_emulated_differences=measured['differences'],
                      original_hardware_instructions=measured['original_instructions'],
                      host=measured['host'],hardware_code_authentication=measured['reference_code_authentication'])
        assert measured['entry']==case['entry'] and measured['original_instructions']==453
        assert trace['instructions']==case['instructions'],'Original CPU and emulator paths differ'
        new=copy.deepcopy(data);newcase=new['cases'][0];actual=measured['original_abi']
        exit_=copy.deepcopy(case['exit']);exit_.update(actual)
        exit_['xmm']=[''.join(f'{word:08x}' for word in reversed(row)) for row in actual['xmm']]
        newcase['exit']=exit_
        with (hardware/'output.bin').open('rb') as output:
            output.seek(276)
            for row in newcase['final_banks']:
                size=len(bytes.fromhex(row['hex']));raw=output.read(size);assert len(raw)==size;row['hex']=raw.hex()
            assert not output.read(1)
        newcase['declared_input']='Untouched original scheduler produces this entry from actual native input. Complete expected return, FP state and final banks are from original i386 CPU execution; no native expected output.'
        output=args.output/'original-hardware.json';output.write_text(json.dumps(new,separators=(',',':'))+'\n')
        report.update(original_kernel_returned=True,fixture=str(output),fixture_sha256=sha(output),
                      original_sha256=measured['original_sha256'],emulator_used_for_expected_output=False)
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report.get(key) for key in ('original_reference_completed','original_kernel_returned','hardware_vs_emulated_differences','fixture')}),flush=True)
    return 0 if report.get('original_kernel_returned') else 1


if __name__=='__main__':raise SystemExit(main())
