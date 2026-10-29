#!/usr/bin/env python3
"""Install one explicit, hashed PRE_TEST batch in an active owned final cohort.

Only the still-owned build's CTestCustom.cmake is changed. Engine source and
the production CMake graph are untouched. Install immediately before final
refresh3; run no unrelated CTests after installation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def hook_text(command):
    if any(any(c in value for c in ('\n','\r',';','"'))or']] 'in value or']]'in value for value in command):
        raise ValueError('Unsupported CTest command delimiter')
    quoted=' '.join('"'+value.replace('\\','\\\\')+'"'for value in command)
    return '\n# Explicit final New Game graphical acceptance.\nlist(APPEND CTEST_CUSTOM_PRE_TEST [['+quoted+']])\n'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cohort-report',type=Path,required=True)
    ap.add_argument('--mcc-root',type=Path,required=True)
    ap.add_argument('--frames',type=int,default=30000)
    ap.add_argument('--timeout',type=int,default=900)
    ap.add_argument('--receipt',type=Path,required=True)
    args=ap.parse_args()
    r=json.loads(args.cohort_report.read_text())
    if not r.get('owned_build_worker')or not r['phase'].startswith('waiting')or len(r['iterations'])!=3:
        raise ValueError('Require active owned cohort waiting for final control3')
    owner=Path(r['scratch']).resolve(strict=True)
    build=owner/'build';source=owner/'current-source'
    if not build.is_dir()or not source.is_dir():
        raise ValueError('Owned source/build is absent')
    batch=source/'tools/test_campaign_new_game_graphics_batch.py'
    expected=Path(__file__).with_name('test_campaign_new_game_graphics_batch.py')
    output=args.cohort_report.resolve().parent/'iteration-3/graphics-batch.json'
    command=[sys.executable,str(source/'tools/run_test.py'),'--scratch-root',str(owner/'final-gpu-scratch'),
             '--',sys.executable,'-B',str(batch),'--runtime',str(build/'halo2_native'),
             '--mcc-root',str(args.mcc_root.resolve(strict=True)),'--frames',str(args.frames),
             '--timeout',str(args.timeout),'--cold-data-check','--report-output',str(output)]
    custom=build/'CTestCustom.cmake'
    before=custom.read_bytes()if custom.exists()else b''
    marker=b'# Explicit final New Game graphical acceptance.'
    if marker in before:
        raise ValueError('Graphical hook is already installed')
    text=before.decode()+hook_text(command)
    custom.write_text(text)
    receipt=dict(scope=__doc__,installed=True,hook_path=str(custom),
        hook_sha256=sha(custom),prior_hook_sha256=hashlib.sha256(before).hexdigest(),
        command=command,acceptance_order=['graphics_startup','opening_handoff'],
        cold_data_ownership_check=True,
        expected_batch_helper_sha256=sha(expected),expected_batch_path=str(batch),
        source_refresh_required_before_invocation=True,report=str(output),
        frames=args.frames,timeout_seconds=args.timeout,passed=False,validation_not_run=True)
    args.receipt.parent.mkdir(parents=True,exist_ok=True)
    args.receipt.write_text(json.dumps(receipt,indent=2)+'\n')
    args.receipt.with_suffix('.cmake').write_text(text)
    print(json.dumps(receipt,indent=2))


if __name__=='__main__':
    main()
