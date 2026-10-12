#!/usr/bin/env python3
"""New isolated original FP gates/marker/audibility controls; no audio success.

Reuses the frozen source-allocated equipped/registered context unchanged.
Control events and helper ABI inputs are explicit stimuli, not authored event
production. Does not run or replace the audio/device/bootstrap constructors.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from oracle_smg_fp_sound_1_1 import SoundTrace, ROOT, PIN, CACHE_PIN
from oracle_smg_firing_1_1 import (UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI)

BACKING=0x7d00000

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,value):
    raw=(json.dumps(value,indent=2,sort_keys=True)+'\n').encode()
    p.parent.mkdir(parents=True,exist_ok=True)
    if p.exists():assert p.read_bytes()==raw,'Existing evidence differs'
    else:p.write_bytes(raw)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',required=True,type=Path);args=ap.parse_args()
    t=SoundTrace();t.g.mem_map(BACKING,0x1000)
    p=t.u(0x4f1198);initial=bytes(t.g.mem_read(p,4*0x20cc))
    context={'original_fp_views_hex':initial.hex(),'local_users':[], 'objects':[], 'camera_modes':[],
             'current_bsp_word':f'{t.u(0x46fb64,2):04x}'}
    for view in range(4):
        player=t.u(t.u(0x4f01f0)+12+view*4)
        unit=0xffffffff if player==0xffffffff else t.u(t.u(t.u(0x4f01f4)+0x44)+(player&0xffff)*0x21c+0x2c)
        t.fp_action('read_original_camera_mode',0x156eb0,regs=((UC_X86_REG_EAX,view),))
        mode=t.g.reg_read(UC_X86_REG_EAX)
        context['local_users'].append({'view':view,'player':f'{player:08x}','unit':f'{unit:08x}'})
        context['camera_modes'].append(mode)
    header=t.u(t.u(0x4e78d0)+0x44)
    for datum in (0x10001,t.initial_datum):
        h=header+(datum&0xffff)*12;body=t.u(h+8)
        context['objects'].append({'datum':f'{datum:08x}','header12_hex':bytes(t.g.mem_read(h,12)).hex(),
            'body_address':f'{body:08x}','parent':f'{t.u(body+0x14):08x}',
            'flags_12c':t.u(body+0x12c,1),'associated_154':f'{t.u(body+0x154):08x}'})
    rows=[]
    event=bytes.fromhex('010001003704ace5ffffffff0000000000000000')
    for name,view,kind,sound,reference in (
        ('type_zero',0,0,0xe5ac0437,0),('type_two',0,2,0xe5ac0437,0),
        ('sound_none',0,1,0xffffffff,0),('unbound_view_reference_flag',1,1,0xe5ac0437,8),
        ('unbound_view_real_owner_reject',1,1,0xe5ac0437,0)):
        stimulus=bytearray(event);struct.pack_into('<H',stimulus,0,kind)
        struct.pack_into('<I',stimulus,4,sound);struct.pack_into('<I',stimulus,12,reference)
        t.g.mem_write(BACKING,bytes(stimulus));before=bytes(t.g.mem_read(p+view*0x20cc,0x20cc))
        first=len(t.audio_calls)
        t.fp_action(name,0x1687c2,(view,BACKING),((UC_X86_REG_EAX,0),))
        after=bytes(t.g.mem_read(p+view*0x20cc,0x20cc))
        rows.append({'kind':'event','name':name,'view':view,'event_hex':stimulus.hex(),
            'hand_weapon':f'{int.from_bytes(before[0x10:0x14],"little"):08x}',
            'hand_activity':f'{int.from_bytes(before[0x90:0x94],"little"):08x}',
            'state_before':before[0x1014:0x101a].hex(),'state_after':after[0x1014:0x101a].hex(),
            'full_view_before_hex':before.hex(),'full_view_after_hex':after.hex(),
            'calls':[{'entry':r['entry'],'return_eax':r.get('return_eax')} for r in t.audio_calls[first:]],
            'request_completed':True,'scheduler_success_supplied':False})
    marker_backing=bytes.fromhex('aabbccdd')*10
    for sound,owner in ((0xe5ac0437,t.initial_datum),(0xe4e1036c,t.initial_datum),
                         (0xe4e1036c,0x10001),(0xe4e1036c,0xffffffff)):
        t.g.mem_write(BACKING+0x100,marker_backing)
        t.fp_action('literal_marker_none_input',0x18acd0,(sound,owner),
            ((UC_X86_REG_EAX,t.u(0x46fc48)),(UC_X86_REG_ECX,t.u(0x46fc28)),
             (UC_X86_REG_EDX,0xffffffff),(UC_X86_REG_ESI,BACKING+0x100)))
        t.fp_action('literal_callback_classification_input',0x18aa40,
            regs=((UC_X86_REG_EDX,sound),(UC_X86_REG_ECX,owner)))
        rows.append({'kind':'marker','sound':f'{sound:08x}','owner':f'{owner:08x}',
            'marker_before_hex':marker_backing.hex(),
            'marker_after_hex':bytes(t.g.mem_read(BACKING+0x100,40)).hex(),
            'weapon_callbacks':bool(t.g.reg_read(UC_X86_REG_EAX)&255)})
    for owner in (t.initial_datum,0x10001,0xffffffff):
        for requested in (0,1,2,255):
            t.g.mem_write(BACKING+0x200,bytes([requested]));t.w(BACKING+0x204,0xaabbccdd)
            t.fp_action('literal_audibility_input',0x18ef20,(owner,BACKING+0x200,BACKING+0x204))
            rows.append({'kind':'audibility','owner':f'{owner:08x}','requested':requested,
                'result':t.u(BACKING+0x200,1),'resolved_view':f'{t.u(BACKING+0x204):08x}'})
    c=t.cache;defs=[]
    class_tag=next(tag for tag in c.tags.values() if tag['group']=='sncl')
    classes=c.block(class_tag['offset'],92)
    for datum in (0xe5ac0437,0xe5ad0438,0xe4e1036c):
        tag=c.tags[datum];raw=bytes(c.data[tag['offset']:tag['offset']+20]);index=struct.unpack_from('<b',raw,2)[0]
        defs.append({'datum':f'{datum:08x}','definition20_hex':raw.hex(),'class_index':index,
                     'class92_hex':bytes(c.data[classes[index]:classes[index]+92]).hex()})
    report={'schema':'original-fp-sound-request-controls-v1','xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
        'tool_sha256':sha(__file__),'dependency_sha256':sha(ROOT/'tools/oracle_smg_fp_sound_1_1.py'),
        'context':context,'definitions':defs,'cases':rows,
        'original_calls':t.audio_calls,'original_rng_pointer':f'{t.u(0x4ee9d8):08x}',
        'original_rng8_hex':bytes(t.g.mem_read(t.u(0x4ee9d8),8)).hex(),
        'scope':'Declared event/helper inputs on unchanged old equipped source fixture. Camera modes, actual owners, parents and association fields are source context outputs. No audio init/driver/voice/clock/admission success supplied; unbound owner actually rejects. Prior fixture boundaries retained.',
        'inherited_services':{f'{a:08x}':v for a,v in t.services.items()}}
    save(args.output/'report.json',report)
    print(json.dumps({'cases':len(rows),'report_sha256':sha(args.output/'report.json')}))

if __name__=='__main__':main()
