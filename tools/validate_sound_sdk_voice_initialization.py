#!/usr/bin/env python3
"""Validate voice initialization against original evidence and isolated faults.

Native expected outputs come only from captured original state and unchanged
original CPU instructions. Genuine cases additionally use real native storage,
the source allocator and real recursive synchronization. Fault controls use
declared services. No isolated wrong copy mutates production source.
"""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False);frozen=args.output/'source';pins={}
    inputs=['src/audio/sound_sdk_voice_initialization.cpp','src/audio/sound_capabilities.cpp','src/audio/native_sound_storage.cpp','src/platform/native_synchronization.cpp','tests/sound_sdk_voice_initialization_tests.cpp']
    pending=list(map(Path,inputs))
    while pending:
        p=pending.pop()
        if str(p)in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode()for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=Path('tests/fixtures/sound_sdk_voice_initialization.original.json');original=args.output/'original.json';shutil.copy2(ROOT/fixture,original)
    owner=frozen/inputs[0]
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math','-Wall','-Wextra','-pthread','-I'+str(frozen/'include'), '-I'+str(frozen/'src'),*[str(frozen/p)for p in inputs]]
    commands=[]
    def run(argv,label,success=True):
        argv=list(map(str,argv));r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode));assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'voice_init_{name}';run(common+flags+['-o',binary],name+'-build')
        print(run([binary,original],name+'-original').stdout.strip(),flush=True)
    source=owner.read_text()
    reds=[
        ('source_dirty','source,4,0x07ff0000u','source,4,0x00010000u','spatial'),
        ('format_width','output+2u,1,read(input+2u,1)','output+2u,2,read(input+2u,1)','nonspatial'),
        ('allocation_failure','if(!allocation)return 0x8007000eu;','if(!allocation)return 0;','initialize_flags00000010_failure0'),
        ('parameter_dirty','write(0x37ce8fu,identity+0xbu,1,read(identity+0xbu,1)|1u);','if(allocation)write(0x37ce8fu,identity+0xbu,1,read(identity+0xbu,1)|1u);','initialize_flags00000010_failure1'),
        ('format_guid','!=c.pcm_guid[i]','!=c.adpcm_guid[i]','format_tagfffe_channels1_mask00000000'),
        ('mask_limit','while(mask&&count<8u)','while(mask&&count<7u)','format_tagfffe_channels1_mask000000ff'),
        ('parent_clip','if(count+remaining>8u)write(0x37a549u,identity+0x24u,4,8u-remaining);','if(false)write(0x37a549u,identity+0x24u,4,8u-remaining);','parent_clip_count5_existing0'),
        ('source_reload','source=read(identity+0xb4u);\n        write(0x37bd33u','write(0x37bd33u','bins_source_reload'),
        ('buffer_cursor','write(0x37c1c8u,identity+0xc8u,4,bytes);','write(0x37c1c8u,identity+0xc8u,4,0);','initialize_flags00000000_failure-1'),
        ('frequency_scale','"m"(c.frequency_reciprocal_bits)','"m"(c.unsigned_correction_bits)','frequency_00000001_cw023f'),
        ('borrowed_release','if(read(identity+8u)&0x80000000u)write(0x37c8d3u','if(false)write(0x37c8d3u','buffer_flags80000000_size0_borrowed00000000')]
    for name,needle,replacement,case in reds:
        expected=2 if name in ('allocation_failure','frequency_scale')else 1
        assert source.count(needle)==expected,(name,source.count(needle));wrong=source.replace(needle,replacement)
        mutated=args.output/f'{name}-red.cpp';mutated.write_text(wrong);binary=args.output/f'voice_init_red_{name}'
        run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,original],f'red-{name}-original',False)
        diagnostic='Unexpected voice allocation' if name=='allocation_failure' else 'Original SDK'
        assert case in r.stderr and diagnostic in r.stderr,(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,'Concurrent source change: '+p
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256=sha(ROOT/fixture),commands=commands,normal_and_sanitizers_pass=True,isolated_errors_caught=len(reds),production_source_mutated=False,native_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
