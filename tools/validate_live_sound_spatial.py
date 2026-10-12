#!/usr/bin/env python3
"""Freeze and independently validate the spatial CPU owner, including a red control.

The red control removes the inverse transform in an isolated source copy. The
real game fixture must catch the resulting wrong coordinates. Production files
are never reverted or mutated. A pass remains CPU-boundary evidence only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--actual',type=Path,required=True)
    parser.add_argument('--declared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    source=args.output/'source';source.mkdir()
    pending=[Path('src/sound_spatial.cpp'),Path('tests/live_sound_spatial_tests.cpp')];seen={}
    while pending:
        path=pending.pop()
        if str(path)in seen:continue
        raw=(ROOT/path).read_bytes();seen[str(path)]=hashlib.sha256(raw).hexdigest()
        dest=source/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
        for include in re.findall(rb'#include\s+"([^"]+)"',raw):pending.append(Path('include')/include.decode())
    fixtures=[]
    for label,path in [('actual',args.actual),('declared',args.declared)]:
        dest=args.output/f'{label}.tsv';shutil.copyfile(path,dest);fixtures.append(dict(label=label,path=str(path),sha256=sha(dest)))
    commands=[]
    def run(command,label,expect=0):
        result=subprocess.run([str(v)for v in command],capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(result.stdout);(args.output/f'{label}.stderr').write_text(result.stderr)
        commands.append(dict(label=label,argv=[str(v)for v in command],exit_code=result.returncode))
        assert (result.returncode==0)==(expect==0),(label,result.stdout,result.stderr)
        return result
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math',
            '-Wall','-Wextra','-I'+str(source/'include'),str(source/'src/sound_spatial.cpp'),str(source/'tests/live_sound_spatial_tests.cpp')]
    for kind,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'live_sound_spatial_{kind}';run(common+flags+['-o',binary],f'{kind}-build')
        for label in ('actual','declared'):
            result=run([binary,args.output/f'{label}.tsv'],f'{kind}-{label}')
            print(result.stdout.strip())
    mutation=args.output/'inverse-transform-removed.cpp'
    original=(source/'src/sound_spatial.cpp').read_text();needle='inverse_transform(listener+8,point);'
    assert original.count(needle)==1
    mutation.write_text(original.replace(needle,'/* Deliberate isolated red control: no inverse transform. */'))
    binary=args.output/'spatial_red_control'
    command=[str(mutation)if v==str(source/'src/sound_spatial.cpp')else v for v in common]
    run(command+['-O2','-o',binary],'red-build')
    result=run([binary,args.output/'actual.tsv'],'red-actual',expect=1)
    assert 'Original spatial driver request differs'in result.stderr,result.stderr
    print('RED control caught: '+result.stderr.strip().splitlines()[-1])
    for path,pin in seen.items():assert sha(ROOT/path)==pin
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=seen,fixtures=fixtures,commands=commands,
        normal_and_sanitizers_pass=True,removed_transform_real_game_regression_caught=True,
        production_source_mutated=False,gameplay_or_DSP_parity_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
