#!/usr/bin/env python3
"""Compare unchanged TU1.1/TU1.5 vehicle HUD ownership decisions.

Executes the original status producer's complete seat/child selection block
and original equipped-weapon and parent HUD tag helpers. The selected weapon's
numeric status consumer is a declared service boundary.
No selector instruction, seat flag or inventory result is replaced.
"""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
from weapon_reference_versions import images, unique_entry
from oracle_smg_firing_1_1 import _Code
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

BASE=0x1000000
def stores(row):
    out=[]
    def put(p,v,size=4):out.append([p,v&((1<<(size*8))-1),size])
    put(BASE+0x44,BASE+0x1000)
    objects=[{},dict(weapon=row['parent_weapon'])]+row['children']
    for n,o in enumerate(objects):
        p=BASE+0x2000+n*0x400;definition=BASE+0x5000+n*0x400
        put(BASE+0x1008+n*12,p);put(BASE+0x4008+n*16,definition)
        put(p,0x10000+n);put(p+0x14,0xffffffff);put(p+0xc,0xffffffff)
        put(p+0x10,0xffffffff);put(p+0x212,0 if o.get('weapon',False) else 0xff,1)
        put(p+0x218,0xb3210080+n if o.get('weapon',False) else 0xffffffff)
        put(p+4,o.get('flags',0));put(p+0xaa,o.get('type',0),1)
        put(definition+0xbc,o.get('unit_flags',0))
        if n>=2 and n+1<len(objects):put(p+0xc,0xa1230000+n+1)
    put(BASE+0x2014,0xa1230001 if row['parent'] else 0xffffffff)
    put(BASE+0x21fc,row['seat'],2)
    put(BASE+0x55cc,BASE+0x8000);put(BASE+0x8000,row['seat_flags'])
    put(BASE+0x807c,row['hud_count']);put(BASE+0x8080,BASE+0x8800)
    for n in range(row['hud_count']):put(BASE+0x8804+n*8,0xa3210900+n)
    if row['children']:put(BASE+0x2410,0xa1230002)
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    machines=[]
    for label,x,pin in images():
        if label not in ('TU1.1','TU1.5'):continue
        entry=unique_entry(x,bytes.fromhex('8d6c248c81ec20010000538bd8'),1);code=_Code(x,'.text')
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(BASE,0x10000)
        for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
        parent_query=int(code.at(entry+0x260).op_str,16);status=int(code.at(entry+0x350).op_str,16)
        pool=0x4ecbd0 if label=='TU1.5' else 0x4e78d0;tags=0x4f0414 if label=='TU1.5' else 0x4eb114
        game=struct.unpack('<I',x.read(entry+0x23c,4))[0]
        machines.append((label,pin,entry,g,parent_query,status,pool,tags,game))
    rows=[]
    for parent,seat,flags,parent_weapon,typ,object_flag,unit_flag in itertools.product(
            (False,True),(-1,0),(0,8,0x20,0x28),(False,True),(0,1,2,31,32),(0,0x4000000),(0,0x4000000)):
        rows.append(dict(parent=parent,seat=seat,seat_flags=flags,parent_weapon=parent_weapon,
            children=[dict(flags=object_flag,type=typ,unit_flags=unit_flag,weapon=True)]))
    for flags in (8,0x28):
        for n in range(4):
            children=[dict(flags=0x4000000,type=1,unit_flags=0x4000000,weapon=i==n) for i in range(4)]
            rows.append(dict(parent=True,seat=0,seat_flags=flags,parent_weapon=False,children=children))
    cases=[]
    for n,row in enumerate(rows):
        row['hud_count']=n%3;row['multiplayer']=bool(n&1)
        outputs=[]
        for label,pin,entry,g,parent_query,status,pool,tags,game in machines:
            g.mem_write(BASE,bytes(0x10000))
            def put(p,v,size=4):g.mem_write(p,int(v&((1<<(8*size))-1)).to_bytes(size,'little'))
            for at,v,size in stores(row):put(at,v,size)
            put(pool,BASE);put(tags,BASE+0x4000);put(game,BASE+0x9000);put(BASE+0x9008,2 if row['multiplayer'] else 1)
            frame=BASE+0xa000;output=BASE+0xb000
            put(frame+0x5c,BASE+0x2000);put(frame+0x6f,0,1)
            put(output+4,0xffffffff)
            for reg,v in ((UC_X86_REG_EAX,BASE+0x2000),(UC_X86_REG_EBX,output),(UC_X86_REG_EBP,frame),
                    (UC_X86_REG_ESP,BASE+0xc000),(UC_X86_REG_ESI,65535),(UC_X86_REG_EDI,BASE+0xd000)):
                g.reg_write(reg,v)
            chosen=[]
            def hook(uc,pc,size,unused):
                if pc in (entry+0x35f,entry+0x47f):uc.emu_stop();return
                if pc!=status:return
                sp=uc.reg_read(UC_X86_REG_ESP);ret=int.from_bytes(uc.mem_read(sp,4),'little')
                if pc==status:chosen.append(uc.reg_read(UC_X86_REG_ECX))
                uc.reg_write(UC_X86_REG_EAX,0xffffffff);uc.reg_write(UC_X86_REG_ESP,sp+8);uc.reg_write(UC_X86_REG_EIP,ret)
            handle=g.hook_add(UC_HOOK_CODE,hook);g.emu_start(entry+0x1db,0,count=10000);g.hook_del(handle)
            outputs.append(dict(seated=bool(g.mem_read(output+0x48,1)[0]),
                handheld_visible=g.reg_read(UC_X86_REG_EIP)==entry+0x35f,
                vehicle_weapon=chosen[0] if chosen else 0xffffffff,
                parent_hud=int.from_bytes(g.mem_read(output+4,4),'little')))
        assert outputs[0]==outputs[1],(row,outputs)
        cases.append(dict(input=row,expected=outputs[0]))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1,scope=__doc__,cases=cases,
        images=[dict(label=m[0],sha256=m[1],entry=hex(m[2])) for m in machines],
        instruction_patches=[],tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n')
    print(len(cases),'unchanged original vehicle HUD ownership cases agree across TU1.1/TU1.5')
if __name__=='__main__':main()
