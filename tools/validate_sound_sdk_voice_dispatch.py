#!/usr/bin/env python3
"""Validate original-backed voice format dispatch and isolated wrong copies.

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
    inputs=['src/sound_sdk_voice_dispatch.cpp','src/sound_sdk_hardware_voice.cpp','src/sound_sdk_device.cpp','src/sound_sdk_spatial.cpp','src/native_sound_storage.cpp','src/native_synchronization.cpp','tests/sound_sdk_voice_dispatch_tests.cpp']
    pending=list(map(Path,inputs))
    while pending:
        p=pending.pop()
        if str(p)in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode()for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=args.output/'original.json';immutable=args.output/'immutable.json'
    shutil.copy2(ROOT/'tests/fixtures/sound_sdk_voice_dispatch.original.json',fixture)
    shutil.copy2(ROOT/'tests/fixtures/sound_sdk_spatial_data.original.json',immutable)
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math','-Wall','-Wextra','-pthread','-I'+str(frozen/'include'),*[str(frozen/p)for p in inputs]]
    commands=[]
    def run(argv,label,success=True):
        argv=list(map(str,argv));r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode));assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'voice_dispatch_{name}';run(common+flags+['-o',binary],name+'-build')
        print(run([binary,fixture,immutable],name+'-original').stdout.strip(),flush=True)
    owner=frozen/inputs[0];source=owner.read_text()
    reds=[
        ('channels','(count-1u)>>1','(count-1u)>>2','format_tag0001_channels3','Original SDK'),
        ('active_error','result=0x88780032u;return;','result=0;return;','format_tag0001_channels0_bits0_active1_existing1','Original SDK'),
        ('pcm16','|0x50000000u);','|0xa0000000u);','nonspatial','Original SDK'),
        ('mono','format=mono?format&0xf7ffffffu:format|0x08000000u;','format=mono?format|0x08000000u:format&0xf7ffffffu;','nonspatial','Original SDK'),
        ('subvoice','index?current|0x400000u:current','current','format_tag0001_channels3_bits0_active1_existing2','Original SDK'),
        ('reconfigure_reload','const auto data=r.read(voice+0x80u);\n    if(channels','const auto data=r.read(voice+0x70u);\n    if(channels','reconfigure_data_reload','Original SDK'),
        ('attachment_result','result=need(s.attach_buffer','result=0,need(s.attach_buffer','prepare_flags00000000_buffer12345678_size32_status00000001','Original SDK'),
        ('initialize_skip','+0xau,1)&4u','+0xau,1)&2u','initialize_flags00040000_available0','Original SDK'),
        ('command_reload','need(s.device_write,"Original SDK voice format device owner unavailable")(\n          0xfe8202f8u,r.read(voice+0xcu+index*2u,2));\n      const auto current=r.read(voice+0x14u);','const auto current=r.read(voice+0x14u);\n      need(s.device_write,"Original SDK voice format device owner unavailable")(\n          0xfe8202f8u,r.read(voice+0xcu+index*2u,2));','format_reload_00380a31','Original SDK'),
        ('active_predicate','const bool active=(r.read(voice+0x12u,1)&1u)!=0;\n    r.write(0x380a0au,voice+0x14u,4,format);','r.write(0x380a0au,voice+0x14u,4,format);\n    const bool active=(r.read(voice+0x12u,1)&1u)!=0;','format_reload_00380a0a','Original SDK')]
    for name,needle,replacement,case,diagnostic in reds:
        assert source.count(needle)==1,(name,source.count(needle));mutated=args.output/f'{name}-red.cpp';mutated.write_text(source.replace(needle,replacement))
        binary=args.output/f'voice_dispatch_red_{name}';run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,fixture,immutable],f'red-{name}-original',False)
        assert case in r.stderr and diagnostic in r.stderr,(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,'Concurrent source change: '+p
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256=sha(fixture),immutable_fixture_sha256=sha(immutable),commands=commands,normal_and_sanitizers_pass=True,isolated_errors_caught=len(reds),production_source_mutated=False,native_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
