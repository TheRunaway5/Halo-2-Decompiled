#!/usr/bin/env python3
"""Validate SDK setters and driver operation bridge against original evidence.

Expected state/store/device values come from genuine calls and original CPU
fragments. Native synchronization/FIFO/publication services are explicit test
models. Isolated incorrect copies never mutate the production implementation.
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
    pending=[Path(p)for p in ('src/sound_sdk_setters.cpp','src/sound_sdk_device.cpp','tests/sound_sdk_setters_tests.cpp')]
    while pending:
        p=pending.pop()
        if str(p)in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode()for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=Path('tests/fixtures/sound_sdk_setters.original.json');original=args.output/'original.json';shutil.copy2(ROOT/fixture,original)
    owner=frozen/'src/sound_sdk_setters.cpp'
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math',
        '-Wall','-Wextra','-I'+str(frozen/'include'),str(owner),str(frozen/'src/sound_sdk_device.cpp'),str(frozen/'tests/sound_sdk_setters_tests.cpp')]
    commands=[]
    def run(argv,label,success=True):
        argv=[str(a)for a in argv];r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode))
        assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'setters_{name}';run(common+flags+['-o',binary],name+'-build')
        r=run([binary,original],name+'-original');print(r.stdout.strip(),flush=True)
    text=owner.read_text()
    cached='''  const auto cached_source=r.read(r.read(owner+0x10u)+0xb4u);
  for(unsigned i=0;i<3;++i){
    const auto source=cached_source;'''
    sequential='''  for(unsigned i=0;i<3;++i){
    const auto source=r.read(r.read(owner+0x10u)+0xb4u);'''
    snapshot='''  std::array<std::uint32_t,9> payload_snapshot{};
  for(unsigned i=0;i<9;++i)payload_snapshot[i]=r.read(payload+i*4u);
  auto source=r.read(r.read(owner+0x10u)+0xb4u);'''
    reds=[('dirty_width','source+2u,1,r.read(source+2u,1)|1u','source+2u,4,r.read(source+2u,1)|1u','sdk-live-setter-position-v1'),
        ('apply_bit','if(!(apply&1u))commit(owner,r,services);','if(apply&1u)commit(owner,r,services);','sdk-live-setter-position-v1'),
        ('source_reload',sequential,cached,'position_source_reload'),
        ('payload_snapshot','  auto source=r.read(r.read(owner+0x10u)+0xb4u);',snapshot,'environment_alias_-8'),
        ('argument_spill','position[i]=value;','(void)value;','position_axis0_7f801234'),
        ('busy_result','if(r.read(bindings.busy_address))return 0x80004005u;','if(r.read(bindings.busy_address))return 0u;','position_busy_00000001_access0'),
        ('conditional_release','~StateAccess(){if(acquired)services.release_state_access();}','~StateAccess(){services.release_state_access();}','position_busy_00000001_access0'),
        ('driver_axes','{request.words[0],request.words[1],request.words[2]}','{request.words[0],request.words[2],request.words[1]}','position_axis1_80000000')]
    for name,needle,replacement,case in reds:
        expected_count=2 if name=='apply_bit' else 3 if name=='busy_result' else 1
        assert text.count(needle)==expected_count,(name,text.count(needle))
        wrong=text.replace(needle,replacement)
        if name=='payload_snapshot':wrong=wrong.replace('const auto value=r.read(payload+i*4u);','const auto value=payload_snapshot[i];')
        mutated=args.output/f'{name}-red.cpp';mutated.write_text(wrong)
        binary=args.output/f'setters_red_{name}'
        run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,original],f'red-{name}-original',False)
        assert case in r.stderr and 'Original SDK setter output differs' in r.stderr,(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,'Concurrent source change: '+p
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256={str(fixture):sha(ROOT/fixture)},
        commands=commands,normal_and_sanitizers_pass=True,all_eight_isolated_red_controls_caught=True,
        production_source_mutated=False,native_or_physical_DSP_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
