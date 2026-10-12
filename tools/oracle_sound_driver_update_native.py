#!/usr/bin/env python3
"""Actual driver allocation prefixes composed with unchanged update owners.

No device, status, generation, count or availability field is assigned to
admit an update. First real required platform calls remain unavailable.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc as RealUc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
import oracle_sound_driver_startup_native as prior
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,_Code,sha,save,encoded
from oracle_sound_startup_native import AUDIO_ARENA,AUDIO_CAPACITY
BASE=ROOT/'analysis/multiplayer/integration/sound-driver-update-native'
PRIOR=ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/final-manifest.json'
PRIOR_PIN='e75f3b1fabc796cf5ddf7608fbcc7786ce6504f97ff180e93994d322349ab228'
STACK,STOP=0x2005000,0x3000000
class Boundary(Exception):pass
def execute(out):
    assert sha(PRIOR)==PRIOR_PIN and sha(prior.PRIOR)==prior.PRIOR_PIN
    pair=json.loads((BASE/'paired/report.json').read_text());packet=json.loads((BASE/'paired/providers.json').read_text());prep_pair=json.loads((prior.PAIR/'report.json').read_text());prep_packet=json.loads((prior.PAIR/'providers.json').read_text())
    cases=[];preparations=[];listings={};instances=[]
    def capture(*args):g=RealUc(*args);instances.append(g);return g
    for tag,path in(('old',OLD),('u5',NEW)):
        image=Xbe(path);code=_Code(image,'.text');executed=set();pcmap={}
        for row in pair['owners']:
            old=(BASE/'paired/instructions'/f'{row["old"]}-old.asm').read_text().splitlines();new=(BASE/'paired/instructions'/f'{row["old"]}-u5.asm').read_text().splitlines()
            pcmap.update({int(b.split()[0],16):int(a.split()[0],16)for a,b in zip(old,new)})
        for delta in range(4):
            original=prior.Uc;prior.Uc=capture
            try:prep=prior.run(tag,path,delta,prep_pair,prep_packet,out/'driver-prefix-reexecution')
            finally:prior.Uc=original
            g=instances[-1];preparations.append(prep);whole=[(lo,bytes(g.mem_read(lo,hi-lo+1)))for lo,hi,_ in g.mem_regions()]
            bindings={name:int(q[tag],16)for name,q in packet['bindings'].items()}
            driver=int(prep['after']['driver_address'],16)
            for operation,oldentry in(('update',0x220d10),('reload_effects',0x193020)):
                h=RealUc(UC_ARCH_X86,UC_MODE_32)
                for lo,raw in whole:h.mem_map(lo,len(raw));h.mem_write(lo,raw)
                def u(a,n=4):return int.from_bytes(h.mem_read(a,n),'little')
                regions=[(AUDIO_ARENA,AUDIO_CAPACITY)]
                sizes=dict(driver_pointer=4,effects_generation=4,effects_context_pointer=4,optional_sound_flag=1,quiesce_flag=1,quiesce_clear_word=2,section_name=9,quiesce_record=4)
                regions +=[(bindings[name],size)for name,size in sizes.items()]
                def snapshot():return [dict(address=f'{a:08x}',raw_hex=bytes(h.mem_read(a,n)).hex())for a,n in regions]
                events=[];stop=None
                entryrow=next(q for q in pair['owners']if q['old']==f'{oldentry:08x}');entry=oldentry if tag=='old'else int(entryrow['u5'],16)
                serviceold=0x381e84 if operation=='update'else 0x2dbda1
                servicerow=next(q for q in entryrow['calls']if q['old_target']==f'{serviceold:08x}');service=serviceold if tag=='old'else int(servicerow['new_target'],16)
                before=snapshot();h.mem_write(STACK,struct.pack('<I',STOP));h.reg_write(UC_X86_REG_ESP,STACK);h.reg_write(UC_X86_REG_FPCW,0x027f);h.reg_write(UC_X86_REG_MXCSR,0x1f80)
                status=bytes(h.mem_read(STACK-4,4)).hex()
                assert u(driver+4,2)==0 and u(driver+0x2ad4)==u(bindings['effects_generation'])==0
                def hook(_h,a,n,_):
                    nonlocal stop
                    if a==service:
                        args=[]if operation=='update'else[f'{u(h.reg_read(UC_X86_REG_ESP)+4):08x}']
                        if args:assert int(args[0],16)==bindings['section_name']
                        events.append(dict(service='required_DirectSoundDoWork'if operation=='update'else'required_XLoadSection',arguments=args))
                        stop=dict(entry=f'{a:08x}',caller_return=f'{u(h.reg_read(UC_X86_REG_ESP)):08x}',result_supplied=False);raise Boundary()
                    instruction=code.at(a)
                    assert instruction and instruction.bytes==image.read(a,n)==bytes(h.mem_read(a,n)),hex(a)
                    executed.add(a)
                def write(_h,kind,a,n,v,_):
                    if any(lo<=a and a+n<=lo+size for lo,size in regions):
                        actual=h.reg_read(UC_X86_REG_EIP);pc=actual if tag=='old'else pcmap[actual]
                        events.append(dict(store=dict(instruction=f'{pc:08x}',actual_instruction=f'{actual:08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(8*n))-1):0{n*2}x}')))
                h.hook_add(UC_HOOK_CODE,hook);h.hook_add(UC_HOOK_MEM_WRITE,write)
                try:h.emu_start(entry,STOP,count=1000000)
                except Boundary:pass
                assert stop and h.reg_read(UC_X86_REG_EIP)==service
                cases.append(dict(image=tag,image_sha256=sha(path),name=f'{tag}_cursor{delta}_{operation}',operation=operation,entry=f'{entry:08x}',canonical_entry=f'{oldentry:08x}',bindings={k:f'{v:08x}'for k,v in bindings.items()},driver=f'{driver:08x}',status_backing4_hex=status,initial_regions=before,output_regions=snapshot(),events=events,stop=stop,source_fields_assigned=[],whole_owner_returned=False,original_prefix_controls=prep['controls']))
        listing=out/'instructions'/f'{tag}-executed.asm';save(listing,''.join(f'{code.at(a).address:08x} {code.at(a).bytes.hex()} {code.at(a).mnemonic} {code.at(a).op_str}\n'for a in sorted(executed)).encode());listings[tag]=dict(path=str(listing.relative_to(out)),sha256=sha(listing),rows=len(executed))
    save(out/'driver-prefix-reexecution/report.json',encoded(dict(schema='sound-driver-prefix-independent-composition-v1',cases=preparations)))
    report=dict(schema='sound-driver-update-original-stops-v1',oracle_sha256=sha(__file__),prior_manifest_sha256=sha(PRIOR),paired_report_sha256=sha(BASE/'paired/report.json'),providers_sha256=sha(BASE/'paired/providers.json'),cases=cases,listings=listings,platform_results=[],instruction_patches=[],bounds=['Original2205C0 prefixes execute again to the genuine DirectSoundCreate stop; entire source memory is cloned, not selected driver fields. Three cursor residue controls are inherited declared raw ABI inputs, not voices or startup success.','Both original images execute their own instructions; u5 consumes the same explicitly retained original1.1 startup arena boundary as the existing prefix freeze. No fullu5 title initialization is claimed.','220D10 naturally reaches mandatoryDirectSoundDoWork with zero constructed stream count and equal original zero generation.193020 independently reaches mandatoryXLoadSection(DSPImage). No device/section/status result is supplied.','This establishes complete CPU caller contracts and exact original service stops, not a completed active driver update, section/DSP lifecycle or voiced completion.'])
    save(out/'report.json',encoded(report));return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=execute(a.output.resolve());print(json.dumps(dict(cases=len(r['cases']),report_sha256=sha(a.output/'report.json'))))
