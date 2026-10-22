#!/usr/bin/env python3
"""Validate construction CPU owners with original outputs and isolated errors.

Original expected values are independently generated from owned instructions
and genuine captured state. Platform providers in these tests are declared
models. Incorrect copies stay isolated; production source is never mutated.
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
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False);frozen=args.output/'source';pins={}
    pending=[Path(p)for p in ('src/audio/sound_sdk_construction.cpp','tests/sound_sdk_construction_tests.cpp')]
    while pending:
        p=pending.pop()
        if str(p)in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode()for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=Path('tests/fixtures/sound_sdk_construction.original.json');original=args.output/'original.json';shutil.copy2(ROOT/fixture,original)
    owner=frozen/'src/audio/sound_sdk_construction.cpp'
    common=['c++','-std=c++20','-Wall','-Wextra','-I'+str(frozen/'include'), '-I'+str(frozen/'src'),str(owner),str(frozen/'tests/sound_sdk_construction_tests.cpp')]
    commands=[]
    def run(argv,label,success=True):
        argv=[str(a)for a in argv];r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode))
        assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'construction_{name}';run(common+flags+['-o',binary],name+'-build')
        print(run([binary,original],name+'-original').stdout.strip(),flush=True)
    source=owner.read_text()
    reds=[
        ('voice_slot','node+0x10u,4,i,observer','node+0x10u,4,0,observer','voice_object'),
        ('voice_ref','retained=source_sound_sdk_retain(voice_data,data,services,observer);\n  r.write(0x382b41u','retained=voice_data;\n  r.write(0x382b41u','voice_object'),
        ('list_tail','r.write(0x37bea0u,tail,4,node,observer);','r.write(0x37bea0u,head,4,node,observer);','bind_nonempty_list'),
        ('override','(setting&0xffffu)>2u||(setting&0xfffc0000u)','(setting&0xffffu)>2u','override_00040000'),
        ('busy','if(r.read(data.busy_address))return 0x80004005u;','if(r.read(data.busy_address))return 0;','synchronized_reference_count00000000_busy80000000_access0'),
        ('release','~StateAccess(){if(acquired)services.release_state_access();}','~StateAccess(){services.release_state_access();}','buffer_object'),
        ('pair_snapshot','r.write(payload?0x37a11au:0x37a12cu','const auto cached=payload?r.read(payload+4u):0xffffffffu;\n  r.write(payload?0x37a11au:0x37a12cu','pair_01001004'),
        ('source_reload','s=source();r.write(0x37bedfu','r.write(0x37bedfu','bind_source_reload'),
        ('field_width','identity+0x10u,2,0xffff','identity+0x10u,4,0xffff','voice_object')]
    for name,needle,replacement,case in reds:
        assert source.count(needle)==1,(name,source.count(needle))
        wrong=source.replace(needle,replacement)
        if name=='pair_snapshot':wrong=wrong.replace('payload?r.read(payload+4u):0xffffffffu,observer','cached,observer')
        mutated=args.output/f'{name}-red.cpp';mutated.write_text(wrong);binary=args.output/f'construction_red_{name}'
        run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,original],f'red-{name}-original',False)
        assert case in r.stderr and ('Original SDK' in r.stderr),(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,'Concurrent source change: '+p
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,
        fixture_sha256=sha(ROOT/fixture),commands=commands,normal_and_sanitizers_pass=True,
        isolated_errors_caught=len(reds),production_source_mutated=False,native_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
