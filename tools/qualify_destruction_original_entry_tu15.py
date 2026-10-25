#!/usr/bin/env python3
"""Compare an actual owned entry capture with original i386 and retire its banks."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture-name',required=True)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py or its finite session')
    if Path(args.capture_name).name!=args.capture_name:parser.error('Capture must be a direct owned directory name')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    capture=scratch/args.capture_name
    if capture.is_symlink() or capture.resolve(strict=True).parent!=scratch:raise ValueError('Capture escaped its finite owner')
    metadata=json.loads((capture/'capture.json').read_text())
    if metadata['reference']!='English TU1.5 only':raise ValueError('Different original authority')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    oracle=scratch/(args.capture_name+'-original')
    if oracle.exists():raise ValueError('Original comparison output already exists')
    log=scratch/(args.capture_name+'-original.log')
    command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),
        '--capture',str(capture),'--output',str(oracle)]
    execution=dict(passed=False,command=command,entry=metadata['entry'],tool_sha256=sha(Path(__file__)),capture_metadata_sha256=sha(capture/'capture.json'),
        required_temporary_reference_input=str(capture),temporary_oracle_output=str(oracle),
        scope='One actual original entry/return and all captured accessible banks; no all-branch or full-goal acceptance')
    def save():
        (args.output/'execution.json').write_text(json.dumps(execution,indent=2)+'\n')
    save()
    with log.open('w') as stream:
        code=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT).returncode
    execution['returncode']=code
    result=oracle/'report.json'
    if result.exists():
        if result.stat().st_size>1024*1024:raise ValueError('Original comparison report exceeded the bounded evidence budget')
        shutil.copyfile(result,args.output/'report.json')
    original_result=json.loads(result.read_text()) if result.exists() else {}
    passed=code==0 and original_result.get('passed') is True
    execution['passed']=passed;save()
    if not passed:
        with log.open('rb') as stream:
            stream.seek(max(0,log.stat().st_size-16384))
            (args.output/'failure.log').write_bytes(stream.read(16384))
        trace=oracle/'trace.json'
        if trace.exists() and trace.stat().st_size<=1024*1024:
            shutil.copyfile(trace,args.output/'trace.json')
        print(json.dumps(dict(passed=False,report=str(args.output/'execution.json'),reference_input_still_owned=True)),flush=True)
        return code or 1
    if (capture/'capture.json').stat().st_size>1024*1024:raise ValueError('Capture metadata exceeded bounded evidence budget')
    shutil.copyfile(capture/'capture.json',args.output/'capture-metadata.json')
    rows=[]
    for region in metadata['regions']:
        files=[]
        for key in ('before_file','after_file'):
            value=region.get(key,region['before_file']);path=capture/value
            if path.is_symlink() or path.resolve(strict=True).parent!=capture:raise ValueError('Capture bank escaped its owner')
            files.append(path)
        rows.append(dict(address=region['address'],size=region['size'],before_sha256=sha(files[0]),after_sha256=sha(files[1])))
    hashes=json.dumps(rows,separators=(',',':'))+'\n'
    if len(hashes.encode())>1024*1024:raise ValueError('Capture hash index exceeded bounded evidence budget')
    (args.output/'memory-hashes.json').write_text(hashes)
    removed=[]
    for path in (capture,oracle):
        if path.is_symlink() or path.parent!=scratch:raise ValueError('Cleanup escaped its finite owner')
        files=[p for p in path.rglob('*') if p.is_file()]
        removed.append(dict(path=str(path),files=len(files),bytes=sum(p.stat().st_size for p in files)))
        shutil.rmtree(path)
    log.unlink()
    cleanup=dict(passed=all(not Path(row['path']).exists() for row in removed),removed=removed,
        total_removed_bytes=sum(row['bytes'] for row in removed),retained_large_artifacts=[])
    (args.output/'cleanup.json').write_text(json.dumps(cleanup,indent=2)+'\n')
    execution.update(required_reference_banks_removed=cleanup['passed'],retained_large_artifacts=[]);save()
    print(json.dumps(dict(passed=passed and cleanup['passed'],report=str(args.output/'report.json'),
        original_instructions=original_result['original_instructions'],removed_bytes=cleanup['total_removed_bytes'])),flush=True)
    return 0 if cleanup['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
