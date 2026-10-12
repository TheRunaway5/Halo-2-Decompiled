#!/usr/bin/env python3
"""Validate original SDK device CPU contracts and isolated incorrect copies.

Normal and ASan/UBSan executables compare original-backed outputs. Test lock,
FIFO and device providers are explicit models; no live DSP/audio claim. All
incorrect copies are isolated from production source and must fail their
specific reference controls.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    frozen=args.output/'source';pins={}
    pending=[Path('src/sound_sdk_device.cpp'),Path('tests/sound_sdk_device_tests.cpp')]
    while pending:
        path=pending.pop()
        if str(path) in pins:continue
        raw=(ROOT/path).read_bytes();pins[str(path)]=hashlib.sha256(raw).hexdigest()
        target=frozen/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/name.decode() for name in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=Path('tests/fixtures/sound_sdk_device.original.json')
    argument=args.output/'original.json';shutil.copyfile(ROOT/fixture,argument)
    owner=frozen/'src/sound_sdk_device.cpp'
    common=['c++','-std=c++20','-Wall','-Wextra','-I'+str(frozen/'include'),str(owner),
            str(frozen/'tests/sound_sdk_device_tests.cpp')]
    commands=[]

    def run(argv,label,success=True):
        argv=[str(value) for value in argv];result=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(result.stdout)
        (args.output/f'{label}.stderr').write_text(result.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=result.returncode))
        assert (result.returncode==0)==success,(label,result.stdout,result.stderr)
        return result

    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined',
                                                     '-fno-omit-frame-pointer'])]:
        binary=args.output/f'device_{name}'
        run(common+flags+['-o',binary],name+'-build')
        result=run([binary,argument],name+'-original');print(result.stdout.strip(),flush=True)
    original=owner.read_text()
    reds=[('gain_units','(attenuation<<6)/100u','(attenuation<<6)/101u','actual_gain_commands'),
          ('gain_routing','selected==2u?3u','selected==2u?0u','gain_route_'),
          ('gain_pack','(gain[7]&0xfu)|(gain[1]<<4)','(gain[7]&0xf0u)|(gain[1]<<4)','gain_'),
          ('pitch_parent','if(extra)value+=r.read(extra+0x24u);','if(extra)(void)r.read(extra+0x24u);','pitch_word_parent_parameters'),
          ('pitch_limit','std::int32_t(-32767)','std::int32_t(-32768)','pitch_word_'),
          ('filter_mask','0xffe0ffffu','0xffc0ffffu','filter_word_voice'),
          ('filter_store','r.store(0x3817c9u,voice+0x3cu,low,observer);','/* isolated omitted intermediate store */','filter_state_'),
          ('reload_count','if(++i>=next_count)break;','if(++i>=count)break;','reload_count_gain_0')]
    for name,needle,replacement,case in reds:
        assert original.count(needle)==(3 if name=='reload_count' else 1),(name,original.count(needle))
        mutated=args.output/f'{name}-red.cpp';mutated.write_text(original.replace(needle,replacement,1))
        binary=args.output/f'device_red_{name}'
        command=[str(mutated) if value==str(owner) else value for value in common]
        run(command+['-O2','-o',binary],f'red-{name}-build')
        result=run([binary,argument],f'red-{name}-original',False)
        assert case in result.stderr and 'Original SDK device CPU output differs' in result.stderr,(name,result.stderr)
        print(f'RED {name} caught: {result.stderr.splitlines()[0]}',flush=True)
    for path,pin in pins.items():assert sha(ROOT/path)==pin,f'Concurrent source change: {path}'
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,
        fixture_sha256={str(fixture):sha(ROOT/fixture)},commands=commands,
        normal_and_sanitizers_pass=True,all_eight_isolated_red_controls_caught=True,
        production_source_mutated=False,native_or_physical_DSP_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
