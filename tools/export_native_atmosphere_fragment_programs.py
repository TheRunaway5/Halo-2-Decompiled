#!/usr/bin/env python3
"""Source-maintenance export of all finite atmosphere fragment controls."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
FILES=('tools/export_native_atmosphere_fragment_programs.cpp',
       'src/rendering/fog_composite.cpp','src/rendering/patchy_fog.cpp',
       'src/rendering/world/bsp_patchy_fog.cpp','src/core/transform.cpp',
       'src/ui/menu_bloom.cpp','src/ui/popup_blur.cpp','src/campaign/cinematic_crossfade.cpp',
       'src/rendering/effects/light_volume.cpp')
SITES={'fog':'Original 25E50/26520/3A5C0 mode1 complete native producer branches',
       'bloom':'Original 33A00/33C30/33D00 threshold, blur and composite; 235756/211A0/20F30 popup blur',
       'patchy-noise':'Original 26CA0 noise complete source producer',
       'patchy-composite':'Original 26E50 cached and direct source producers',
       'bsp-patchy-noise':'Original 26CA0 finite flag0/flag1 source branches',
       'bsp-patchy-composite':'Original 26E50/3A5C0 mode2 finite low-byte, reciprocal and cached/direct branches',
       'cinematic-crossfade':'Original 142D0 full source crossfade program',
       'light-volume':'Original fixed light-volume fragment program'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    binary=scratch/'export-native-atmosphere-fragments'
    command=['c++','-std=c++20','-O1','-fno-fast-math','-ffp-contract=off',
             '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include'),
             *[str(ROOT/name) for name in FILES],'-Wl,--gc-sections','-o',str(binary)]
    build=subprocess.run(command,capture_output=True,text=True)
    if build.returncode:raise RuntimeError(build.stderr[-6000:])
    run=subprocess.run([str(binary)],capture_output=True,text=True)
    if run.returncode:raise RuntimeError(run.stderr[-3000:])
    rows=json.loads(run.stdout)
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in FILES}
    entries={}
    for row in rows:
        words=row['words']
        if len(words)!=60 or any(type(w)!=int or not 0<=w<=0xffffffff for w in words):
            raise ValueError('Invalid complete native atmosphere program')
        controls=[0 if 10<=i<=25 or i in (43,44) else word for i,word in enumerate(words)]
        key=(row['family'],tuple(controls))
        if key not in entries:
            entries[key]={'family':row['family'],'controls':controls,
                          'provenance':{'authority':SITES[row['family']],
                                        'source_files':pins,'producer_arguments':[]}}
        entries[key]['provenance']['producer_arguments'].append(row['arguments'])
    registry={'schema':'HALO2_NATIVE_FRAGMENT_PROGRAMS_V1','entries':list(entries.values())}
    encoded=(json.dumps(registry,indent=2)+'\n').encode()
    if len(encoded)>1024*1024 or args.output.is_symlink() or args.report.is_symlink():
        raise ValueError('Atmosphere source export exceeds its bounded owner')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(encoded)
    receipt={'passed':True,'producer_cases':len(rows),'unique_programs':len(entries),
             'counts':{family:sum(row['family']==family for row in entries.values()) for family in SITES},
             'source_files':pins,'registry':str(args.output),
             'registry_sha256':hashlib.sha256(encoded).hexdigest(),
             'scratch_owner':str(scratch),'compiler_products_lifecycle':'Removed by enclosing tools/run_test.py'}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({key:receipt[key] for key in ('passed','producer_cases','unique_programs','counts')}))


if __name__=='__main__':main()
