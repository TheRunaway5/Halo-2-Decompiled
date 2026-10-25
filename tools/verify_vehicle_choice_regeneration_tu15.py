#!/usr/bin/env python3
"""Verify the four qualified selector owners regenerate without promoting a module.

Run through run_test.py. Complete regenerated source/manifest are disposable;
retain only owner hashes and explicitly reported unrelated differences.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=('15cb60','15cc70','15cd30','15cdf0')


def bodies(text):
    # Preserve offsets while ignoring braces inside C++ strings/comments.
    tokens=r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/'
    masked=re.sub(tokens,lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),text,flags=re.S)
    result={}
    for match in re.finditer(r'static void run_([0-9a-f]+)\(WorldState &s\) \{\n',text):
        depth=1;end=match.end()
        while depth and end<len(masked):
            depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
        if depth:raise ValueError('Unterminated complete generated owner')
        result[match[1]]=text[match.start():end]
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=False)
    source=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';source_pin=sha(source)
    names=('tools/generate_physics_live_support_tu15.py','tools/native_vehicle_choice_kernel_tu15.py',
           'src/gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp')
    pins={name:sha(ROOT/name) for name in names}
    generated=owner/'regenerated.cpp';manifest=owner/'generated.json'
    command=[sys.executable,'-B',str(ROOT/names[0]),'--output',str(generated),'--manifest',str(manifest)]
    result=subprocess.run(command,capture_output=True,text=True)
    report=dict(passed=False,owner=str(owner),command=command,returncode=result.returncode,
                source_sha256=source_pin,dependency_hashes=pins,whole_module_promoted=False,
                full_vehicle_parity=False,generated_source_retained=False,tool_sha256=sha(Path(__file__)))
    if result.returncode==0:
        a=bodies(source.read_text());b=bodies(generated.read_text());rows=[]
        for entry in ENTRIES:
            if entry not in a or entry not in b:raise ValueError('A complete selector owner is missing')
            digest=lambda x:hashlib.sha256(x.encode()).hexdigest()
            rows.append(dict(entry=entry,current_sha256=digest(a[entry]),generated_sha256=digest(b[entry]),
                             equal=a[entry]==b[entry]))
        changed=sorted(e for e in a.keys()|b.keys() if a.get(e)!=b.get(e))
        stable=source_pin==sha(source) and all(sha(ROOT/name)==pin for name,pin in pins.items())
        report.update(passed=stable and all(r['equal'] for r in rows),source_unchanged=stable,
                      selectors=rows,unrelated_different_owners=changed,
                      current_owner_count=len(a),generated_owner_count=len(b),
                      whole_module_equal=source.read_bytes()==generated.read_bytes(),
                      generated_sha256=sha(generated),manifest_sha256=sha(manifest))
    else:report['failure_tail']=(result.stdout+result.stderr)[-5000:]
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else result.returncode or 1


if __name__=='__main__':raise SystemExit(main())
