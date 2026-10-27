#!/usr/bin/env python3
"""Execute complete unchanged TU1.5 1618B0 on declared component inputs.

Engine slot37 and15BE80 ranking outputs are explicit component services.
All selector and life-exhaustion instructions execute; this is not a gameplay
producer test. Authored status table/English locale are read without changes.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer_hud import HudCache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R

PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MAP_PIN='14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
IMAGE=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
ENGINE,VTABLE,POOL,GAME,PLAYER,CLOCK,STACK,STOP,SERVICE=[0x1000000+i*0x2000 for i in range(9)]

def original(image,spec):
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(ENGINE,0x20000)
    for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
    def w(p,v,n=4):g.mem_write(p,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
    def u(p):return int.from_bytes(g.mem_read(p,4),'little')
    w(0x4f63b8,ENGINE);w(0x56dfa0,ENGINE);w(ENGINE,VTABLE);w(VTABLE+0x94,SERVICE)
    w(ENGINE+0xc14,0);w(0x4f54f4,POOL);w(POOL+0x44,PLAYER);w(0x4f3218,GAME)
    w(GAME+0x1128,spec['special_mode'],1);w(GAME+0x1b8,spec['life_limit']);w(GAME+0x18c,spec['score_limit'])
    w(PLAYER+0x2c,spec['current_unit']);w(PLAYER+0xc0,spec['membership'],1)
    w(PLAYER+2,spec['player_flags'],2);w(PLAYER+0x164,spec['respawn_ticks'])
    w(PLAYER+0x1ac,spec['lives_remaining'],2);w(PLAYER+0x1a8,spec['alive_ticks'],1)
    w(0x51df54,CLOCK);w(CLOCK+2,spec['rate'],2);w(CLOCK+8,spec['tick'])
    w(ENGINE+0x70,spec['round_start_tick']);w(ENGINE+0xe0,spec['round_countdown'],2)
    def ret(n):
        sp=g.reg_read(R.UC_X86_REG_ESP);g.reg_write(R.UC_X86_REG_EIP,u(sp));g.reg_write(R.UC_X86_REG_ESP,sp+4+n)
    def hook(_g,pc,_size,_data):
        sp=g.reg_read(R.UC_X86_REG_ESP)
        if pc==SERVICE:
            w(u(sp+8),spec['engine_override'],1);g.reg_write(R.UC_X86_REG_EAX,spec['engine_status']&0xffffffff);ret(8)
        elif pc==0x15be80:
            w(u(sp+8),spec['leading'],1);w(u(sp+12),spec['tied'],1);ret(12)
    g.hook_add(UC_HOOK_CODE,hook)
    w(STACK,STOP);w(STACK+4,0x10000);g.reg_write(R.UC_X86_REG_ESP,STACK)
    g.emu_start(0x1618b0,STOP,count=10000)
    assert g.reg_read(R.UC_X86_REG_EIP)==STOP
    return struct.unpack('<i',struct.pack('<I',g.reg_read(R.UC_X86_REG_EAX)))[0]

def original_rectangle(image,outer,safe,line_height,view,players,species):
    # Declared25AE69 frame before25B53A. Earlier HUD message/killfeed traversal
    # is outside this geometry fragment; actual color/opacity owners execute.
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(ENGINE,0x20000)
    for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
    def w(p,v,n=4):g.mem_write(p,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
    users=SERVICE;frame=STACK+0x1000
    w(0x4f54f0,users);w(users+0xc+view*4,0x10000);w(0x4f54f4,POOL);w(POOL+0x44,PLAYER)
    w(PLAYER+0x88,species,1);w(0x4c66f8,view);w(0x4f3290+view*4,0x3f800000)
    w(frame+8,view);w(frame-0x50,5 if players>1 else 6)
    top=safe[2]-outer[0]-40;left=safe[1]-outer[1];right=safe[3]-outer[1]
    for p,v in ((frame-12,top),(frame-10,left),(frame-8,top+line_height*5),(frame-6,right)):w(p,v,2)
    g.reg_write(R.UC_X86_REG_EBP,frame);g.reg_write(R.UC_X86_REG_ESP,STACK+0x800)
    g.reg_write(R.UC_X86_REG_EBX,top)
    def hook(_g,pc,_size,_data):
        if pc==0x161ba0:g.emu_stop()
    g.hook_add(UC_HOOK_CODE,hook);g.emu_start(0x25b53a,STOP,count=10000)
    assert g.reg_read(R.UC_X86_REG_EIP)==0x161ba0
    return dict(outer=outer,safe=safe,line_height=line_height,view=view,players=players,species=species,
        rectangle=list(struct.unpack('<4h',g.mem_read(frame-12,8))),font=int.from_bytes(g.mem_read(frame-0x50,4),'little'),
        foreground_hex=bytes(g.mem_read(frame-0x60,16)).hex())

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--image',type=Path,default=IMAGE)
    ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    image=Xbe(args.image);assert hashlib.sha256(image.data).hexdigest()==PIN
    base=dict(engine_status=-1,engine_override=False,special_mode=False,leading=False,tied=False,
        current_unit=0xffffffff,membership=0,alive_ticks=100,player_flags=1,respawn_ticks=150,
        life_limit=0,score_limit=25,tick=200,round_start_tick=0,lives_remaining=-1,rate=30,round_countdown=0)
    specs=[]
    for status,override,member,flags,timer,lives in itertools.product((-1,0,12,16),(False,True),(-1,0),(1,0x4001),(-1,0,1,150),(-1,0,1)):
        specs.append(dict(base,engine_status=status,engine_override=override,membership=member,
            player_flags=flags,respawn_ticks=timer,life_limit=3,lives_remaining=lives))
    for special,leading,tied,life,alive,lives,tick,round_countdown,limit in itertools.product(
            (False,True),(False,True),(False,True),(0,3),(89,90),(1,2),(89,90),(0,3),(0,25)):
        specs.append(dict(base,current_unit=0x10001,special_mode=special,leading=leading,tied=tied,
            life_limit=life,alive_ticks=alive,lives_remaining=lives,tick=tick,
            round_countdown=round_countdown,score_limit=limit))
    cases=[]
    for spec in specs:
        result=original(image,spec)
        spec['exhausted']=spec['life_limit']>0 and spec['current_unit']==0xffffffff and spec['lives_remaining']==0
        cases.append(dict(input=spec,result=result))
    cache=HudCache(ROOT/'assets/disc/maps/lockout.map');assert hashlib.sha256(cache.data).hexdigest()==MAP_PIN
    gp=next(t['offset'] for t in cache.tags.values() if t['group']=='matg')
    mp=cache.tags[cache.u(gp+0x16c)]['offset'];runtime=cache.u(mp+12)-cache.magic
    rows=cache.u(gp+0x198);text=cache.u(gp+0x19c)
    def locale(datum):
        p=cache.tags[datum]['offset'];start=cache.h(p+16);length=cache.h(p+18)
        return {cache.u(rows+i*8):cache.text(text+cache.u(rows+i*8+4)) for i in range(start,start+length)}
    strings=locale(cache.u(runtime+0x8c));statuses=[]
    for p in cache.block(runtime+0x538,0x1c):
        a,b=cache.u(p+8),cache.u(p+12)
        statuses.append(dict(raw=bytes(cache.data[p:p+0x1c]).hex(),status=cache.h(p+4),
            string_id=a,team_string_id=b,text=strings.get(a,''),team_text=strings.get(b,'')))
    universal=cache.u(mp+4)-cache.magic;labels=locale(cache.u(universal+0x1c))
    out=dict(source_sha256=PIN,cache_sha256=MAP_PIN,entry=0x1618b0,
        instruction_sha256=hashlib.sha256(image.read(0x1618b0,556)).hexdigest(),
        boundaries=__doc__,cases=cases,statuses=statuses,
        lifecycle_labels=[labels.get(i,'') for i in (0xc000108,0xd000109,0x1900010a)],
        draw_rectangles=[original_rectangle(image,outer,safe,height,view,players,species)
            for outer,safe,view,players in [([0,0,480,640],[36,48,444,592],0,1),
                ([0,0,240,640],[36,48,236,592],0,2),([240,0,480,640],[244,48,444,592],1,2),
                ([0,0,240,320],[36,48,236,316],0,4)]
            for height in (19,20) for species in (0,1)],instruction_patches=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(out,separators=(',',':'))+'\n')
    print(f'{len(cases)} unchanged TU1.5 selector calls and {len(statuses)} authored status/locale records')

if __name__=='__main__':main()
