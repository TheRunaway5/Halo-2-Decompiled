#!/usr/bin/env python3
"""Validate original-backed sample-page ownership and isolated wrong copies.

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
    inputs=['src/audio/sound_sdk_sample_pages.cpp','src/audio/native_sound_storage.cpp','src/platform/native_synchronization.cpp','tests/sound_sdk_sample_pages_tests.cpp']
    pending=list(map(Path,inputs))
    while pending:
        p=pending.pop()
        if str(p)in pins:continue
        raw=(ROOT/p).read_bytes();pins[str(p)]=hashlib.sha256(raw).hexdigest()
        target=frozen/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        pending.extend(Path('include')/p.decode()for p in re.findall(rb'#include\s+"([^"]+)"',raw))
    fixture=args.output/'original.json'
    shutil.copy2(ROOT/'tests/fixtures/sound_sdk_sample_pages.original.json',fixture)
    common=['c++','-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-fno-associative-math','-Wall','-Wextra','-pthread','-I'+str(frozen/'include'), '-I'+str(frozen/'src'),*[str(frozen/p)for p in inputs]]
    commands=[]
    def run(argv,label,success=True):
        argv=list(map(str,argv));r=subprocess.run(argv,capture_output=True,text=True)
        (args.output/f'{label}.stdout').write_text(r.stdout);(args.output/f'{label}.stderr').write_text(r.stderr)
        commands.append(dict(label=label,argv=argv,exit_code=r.returncode));assert (r.returncode==0)==success,(label,r.stdout,r.stderr);return r
    for name,flags in [('normal',['-O2']),('sanitizers',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        binary=args.output/f'sample_pages_{name}';run(common+flags+['-o',binary],name+'-build')
        print(run([binary,fixture],name+'-original').stdout.strip(),flush=True)
    owner=frozen/inputs[0];source=owner.read_text()
    reds=[
        ('cached_reference','r.read(current+0xcu)+1u','r.read(current+0xcu)','genuine_cached','Original SDK'),
        ('best_fit','count<r.read(selected+0xau,2)','count>r.read(selected+0xau,2)','bestfit_counts(1, 2, 5)_refs(0, 0, 0)_pages1','Original SDK'),
        ('split_width','0x3803a5,selected+0xau,2,needed','0x3803a5,selected+0xau,4,needed','genuine_allocated','Original SDK'),
        ('mapping_index','r.read(result+8u,2),base,pages&0xffffu,s','0u,base,pages&0xffffu,s','bestfit_counts(1, 2, 5)_refs(0, 0, 0)_pages2','Original SDK'),
        ('unlock_extent','buffer,pages<<12u,true','buffer,pages<<11u,true','release_counts(8,)_index0_pattern0','Original SDK'),
        ('merge_count','r.read(first+0xau,2)+count','r.read(first+0xau,2)+1u','release_counts(1, 2, 5)_index0_pattern0','Original SDK'),
        ('zero_page_pin','need(s.lock_pages,"Original SDK sample-page pin owner unavailable")(buffer,pages<<12u,false);','if(pages)need(s.lock_pages,"Original SDK sample-page pin owner unavailable")(buffer,pages<<12u,false);','acquire_offset0_bytes00000000','Original SDK'),
        ('allocation_failure','if(!pool)return 0x8007000eu;','if(!pool)return 0;','initialize_count0_success0','Original SDK'),
        ('available_count','r.read(available)-count','r.read(available)+count','genuine_allocated','Original SDK')]
    for name,needle,replacement,case,diagnostic in reds:
        assert source.count(needle)==1,(name,source.count(needle));mutated=args.output/f'{name}-red.cpp';mutated.write_text(source.replace(needle,replacement))
        binary=args.output/f'sample_pages_red_{name}';run([str(mutated)if a==str(owner)else a for a in common]+['-O2','-o',binary],f'red-{name}-build')
        r=run([binary,fixture],f'red-{name}-original',False)
        assert case in r.stderr and diagnostic in r.stderr,(name,r.stderr)
        print(f'RED {name} caught: {r.stderr.splitlines()[0]}',flush=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,'Concurrent source change: '+p
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),source_sha256=pins,fixture_sha256=sha(fixture),commands=commands,normal_and_sanitizers_pass=True,isolated_errors_caught=len(reds),production_source_mutated=False,native_or_physical_DSP_claimed=False),indent=2)+'\n')
if __name__=='__main__':main()
