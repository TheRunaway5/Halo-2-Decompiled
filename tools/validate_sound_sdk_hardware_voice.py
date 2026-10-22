#!/usr/bin/env python3
"""Validate original-backed hardware voice activation and isolated wrong copies.

Both genuine cases additionally use real OS storage and recursive locks. FIFO
and device sinks are declared CPU boundaries, never native DSP/output proof.
Expected results come only from original captures and unchanged original CPU.
"""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False);frozen=args.output/'source';pins={}
    inputs=['src/audio/sound_sdk_hardware_voice.cpp','src/audio/sound_sdk_device.cpp','src/audio/sound_sdk_spatial.cpp','src/audio/native_sound_storage.cpp','src/platform/native_synchronization.cpp','tests/sound_sdk_hardware_voice_tests.cpp']
    pending=list(map(Path,inputs))
    while pending:
        p=pending.pop()
        if str(p)in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode()for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=args.output/'original.json';immutable=args.output/'immutable.json'
    shutil.copy2(ROOT/'tests/fixtures/sound_sdk_hardware_voice.original.json',fixture)
    shutil.copy2(ROOT/'tests/fixtures/sound_sdk_spatial_data.original.json',immutable)
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math','-Wall','-Wextra','-pthread','-I'+str(frozen/'include'), '-I'+str(frozen/'src'),*[str(frozen/p)for p in inputs]]
    commands=[]
    def run(argv,label,success=True):
        argv=list(map(str,argv));r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode));assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'hardware_voice_{name}';run(common+flags+['-o',binary],name+'-build')
        print(run([binary,fixture,immutable],name+'-original').stdout.strip(),flush=True)
    owner=frozen/inputs[0];source=owner.read_text()
    reds=[
        ('slot_pool','device+(spatial?0x5cu:0x58u)','device+0x58u','spatial','Original SDK'),
        ('occupied_slot','if(!r.read(entry)){','if(true){','slots_spatial0_count1_occupied1','Original SDK'),
        ('mapping_formula','((first_slot*4u+2u)<<4u)','((first_slot+2u)<<4u)','nonspatial','Hardware voice extent differs'),
        ('mapping_count','voice+0x6cu,4,4);','voice+0x6cu,4,1);','nonspatial','Original SDK'),
        ('dirty_width','0x3808beu,voice+0x12u,2,','0x3808beu,voice+0x12u,1,','nonspatial','Original SDK'),
        ('bin_shift','bins[i]<<(i*5u+1u)','bins[i]<<(i*5u)','nonspatial','Original SDK'),
        ('subvoice_format','i?format|0x400000u:format','format','slots_spatial0_count2_available00000002','Original SDK'),
        ('command_reload','command(s.device,0xfe8202f8u,r.read(voice+0xcu+i*2u,2));\n      const auto format=r.read(voice+0x14u);','const auto format=r.read(voice+0x14u);\n      command(s.device,0xfe8202f8u,r.read(voice+0xcu+i*2u,2));','reload_00381b43','Original SDK'),
        ('hrtf_missing','s.apply_hrtf(voice);','if(false)s.apply_hrtf(voice);','spatial','Original SDK'),
        ('count_reload','while(index<r.read(voice+0x64u,1)){','while(index<needed){','reload_0037e798','Original SDK'),
        ('pitch_word','0xfe82037cu,pitch','0xfe82037cu,0u','varied_registers0','Original SDK')]
    for name,needle,replacement,case,diagnostic in reds:
        assert source.count(needle)==1,(name,source.count(needle));mutated=args.output/f'{name}-red.cpp';mutated.write_text(source.replace(needle,replacement))
        binary=args.output/f'hardware_voice_red_{name}';run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,fixture,immutable],f'red-{name}-original',False)
        assert case in r.stderr and diagnostic in r.stderr,(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,'Concurrent source change: '+p
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256=sha(fixture),immutable_fixture_sha256=sha(immutable),commands=commands,normal_and_sanitizers_pass=True,isolated_errors_caught=len(reds),production_source_mutated=False,native_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
