#!/usr/bin/env python3
"""Validate original-backed whole voice attachment ownership and isolated wrong copies.

All three genuine cases additionally use real OS storage and recursive locks. FIFO
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
    inputs=['src/audio/sound_sdk_voice_attachment.cpp','src/audio/sound_sdk_sample_pages.cpp','src/audio/sound_sdk_voice_initialization.cpp','src/audio/sound_sdk_voice_dispatch.cpp','src/audio/sound_sdk_hardware_voice.cpp','src/audio/sound_sdk_device.cpp','src/audio/sound_sdk_spatial.cpp','src/audio/native_sound_storage.cpp','src/platform/native_synchronization.cpp','tests/sound_sdk_voice_attachment_tests.cpp']
    pending=list(map(Path,inputs))
    while pending:
        p=pending.pop()
        if str(p)in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode()for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=args.output/'original.json'
    shutil.copy2(ROOT/'tests/fixtures/sound_sdk_voice_attachment.original.json',fixture)
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math','-Wall','-Wextra','-pthread','-I'+str(frozen/'include'), '-I'+str(frozen/'src'),*[str(frozen/p)for p in inputs]]
    commands=[]
    def run(argv,label,success=True):
        argv=list(map(str,argv));r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode));assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'voice_attachment_{name}';run(common+flags+['-o',binary],name+'-build')
        print(run([binary,fixture],name+'-original').stdout.strip(),flush=True)
    owner=frozen/inputs[0];source=owner.read_text()
    reds=[
        ('sample_format_owner','const auto data=r.read(voice+0x70u);','const auto data=r.read(voice+0x80u);','sample_tag0001_stride00000000_samples00000001','Original SDK'),
        ('adpcm_shift','samples>>=6u','samples>>=5u','sample_tag0069_stride00000001_samples00000020','Original SDK'),
        ('cursor_sentinel','page=0x7ffu','page=0x7feu','genuine_stream','Original SDK'),
        ('cursor_index_width','r.read(entry+8u,2)','r.read(entry+8u,4)','genuine_allocated','Original SDK'),
        ('dirty_store_width','0x3808beu,voice+0x12u,2,r.read(voice+0x12u,2)|0x100u);\n  const auto data=r.read(voice+0x80u),size=', '0x3808beu,voice+0x12u,4,r.read(voice+0x12u,2)|0x100u);\n  const auto data=r.read(voice+0x80u),size=','genuine_allocated','Original SDK'),
        ('cleanup_stop_status','need(s.stop,"Original SDK voice detach stop owner unavailable")(voice,0);','if(need(s.stop,"Original SDK voice detach stop owner unavailable")(voice,0))return;','buffer_failure_stop00000001','Original SDK'),
        ('stream_buffer_index','r.read(data+0xd4u)-0xbu','r.read(data+0xd4u)-0xau','genuine_stream','Original SDK'),
        ('branch_rereads_entry','if(!entry){source_sound_sdk_detach_voice_samples','if(!r.read(voice+0x84u)){source_sound_sdk_detach_voice_samples','buffer_failure_branch_keeps_returned_entry','Original SDK'),
        ('stream_oom_return','source_sound_sdk_voice_buffer(data,buffer,size,initialization,s.initialization,observer);','if(source_sound_sdk_voice_buffer(data,buffer,size,initialization,s.initialization,observer))return;','stream_allocate_00000000','Original SDK'),
        ('stream_final_reload','source_sound_sdk_voice_buffer(data,buffer,size,initialization,s.initialization,observer);\n  const auto current_data=r.read(voice+0x80u);','source_sound_sdk_voice_buffer(data,buffer,size,initialization,s.initialization,observer);\n  const auto current_data=data;','stream_reload_data_after_store','Original SDK')]
    for name,needle,replacement,case,diagnostic in reds:
        assert source.count(needle)==1,(name,source.count(needle));mutated=args.output/f'{name}-red.cpp';mutated.write_text(source.replace(needle,replacement))
        binary=args.output/f'voice_attachment_red_{name}';run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,fixture],f'red-{name}-original',False)
        assert case in r.stderr and diagnostic in r.stderr,(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,'Concurrent source change: '+p
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256=sha(fixture),commands=commands,normal_and_sanitizers_pass=True,isolated_errors_caught=len(reds),production_source_mutated=False,native_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
