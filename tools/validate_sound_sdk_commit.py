#!/usr/bin/env python3
"""Validate combined original SDK commit contracts and isolated wrong copies.

All source numerical and command owners execute together. Expected state,
calls, device words and write order come from the original CPU/emulator.
Native test locks/FIFO remain declared models; no live DSP/audio claim.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    frozen=args.output/'source';pins={}
    pending=[Path(p) for p in ('src/sound_sdk_spatial.cpp','src/sound_sdk_device.cpp','src/sound_sdk_commit.cpp','tests/sound_sdk_commit_tests.cpp')]
    while pending:
        p=pending.pop()
        if str(p) in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode() for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixtures=[Path('tests/fixtures/sound_sdk_commit.original.json'),Path('tests/fixtures/sound_sdk_spatial_data.original.json')]
    arguments=[]
    for i,p in enumerate(fixtures):
        target=args.output/f'original-{i}.json';shutil.copyfile(ROOT/p,target);arguments.append(target)
    owner=frozen/'src/sound_sdk_commit.cpp'
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math',
        '-Wall','-Wextra','-I'+str(frozen/'include'),str(frozen/'src/sound_sdk_spatial.cpp'),
        str(frozen/'src/sound_sdk_device.cpp'),str(owner),str(frozen/'tests/sound_sdk_commit_tests.cpp')]
    commands=[]
    def run(argv,label,success=True):
        argv=[str(p) for p in argv];r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode))
        assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'commit_{name}';run(common+flags+['-o',binary],name+'-build')
        r=run([binary,*arguments],name+'-original');print(r.stdout.strip(),flush=True)
    original=owner.read_text()
    pitch='''  if(r.read(parameters(),1)&0x20u){
    if(!services.pitch)throw std::runtime_error("Original SDK pitch CPU owner unavailable");
    services.pitch(voice);
  }
'''
    hrtf='''  if(r.read(parameters(),1)&0x40u){
    if(!services.hrtf)throw std::runtime_error("Original SDK HRTF CPU owner unavailable");
    services.hrtf(voice,0,0);
  }
'''
    reds=[('gain_mask','parameters(),1)&0x1fu','parameters(),1)&0x0fu','sdk-live-commit-dirty-v1'),
        ('dispatch_order',pitch+hrtf,hrtf+pitch,'dispatch_flags_60'),
        ('dirty_width','for(auto &byte:raw)byte=std::byte(0);','raw[0]=std::byte(0);','sdk-live-commit-dirty-v1'),
        ('active_gate','&3u)!=3u','&3u)==3u','sdk-live-commit-v2'),
        ('source_reload','source=r.read(r.read(voice+0x70u)+0xb4u);\n    retained=',
            '/* isolated stale source */\n    retained=','update_reload_source'),
        ('parameter_reload','const auto parameters=[&]{return r.read(r.read(voice+0x70u)+0xb8u);};',
            'const auto initial_parameters=r.read(r.read(voice+0x70u)+0xb8u);\n  const auto parameters=[&]{return initial_parameters;};','dispatch_reload_identity'),
        ('environment_address','services.voice_data(retained+0x30u,source,retained+0x80u,source+0x7cu,parameters);',
            'services.voice_data(retained+0x30u,source,retained+0x80u,source+0x78u,parameters);','sdk-live-commit-dirty-v1')]
    for name,needle,replacement,case in reds:
        assert original.count(needle)==1,(name,original.count(needle))
        mutated=args.output/f'{name}-red.cpp';mutated.write_text(original.replace(needle,replacement))
        binary=args.output/f'commit_red_{name}'
        command=[str(mutated) if v==str(owner) else v for v in common]
        run(command+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,*arguments],f'red-{name}-original',False)
        assert case in r.stderr and 'Original combined SDK commit output differs' in r.stderr,(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,f'Concurrent source change: {p}'
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,
        fixture_sha256={str(p):sha(ROOT/p) for p in fixtures},commands=commands,
        normal_and_sanitizers_pass=True,all_seven_isolated_red_controls_caught=True,
        production_source_mutated=False,native_or_physical_DSP_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
