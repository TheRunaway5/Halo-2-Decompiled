#!/usr/bin/env python3
"""Original action/drop routing until the first required external owner.

No lower-call replay or new supplied success is used by these comparisons.
The inherited equipped fixture is explicit, not live Unit construction.
"""
import argparse, hashlib, json, struct, random
from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_spartan_equipped_animation_1_1 import EquippedTrace
from oracle_mp_inventory_1_1 import InventoryTrace, SECOND, THIRD, W2, W3, SCRATCH
from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, Cache, UNIT, UI, WEAPON, WI, TAGS, HEADERS, STACK, STOP, PLAYER_DATA
from generate_unit_weapon_routing_1_1 import ENTRIES
REGS=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)
def nums(xs):return ' '.join(f'{x&0xffffffff:x}'for x in xs)
class Capture(EquippedTrace):
    def __init__(self):
        self.capturing=False;self.coverage=set();super().__init__();self.initialize_equipped()
        self.cache=Cache(ROOT/'assets/disc/maps/lockout.map')
        # Keep literal cache addresses for all target definitions.
        for datum,tag in self.cache.tags.items():
            if tag['offset']is not None:self.w(TAGS+(datum&65535)*16+8,tag['offset']+self.cache.magic)
        self.world_weapon=InventoryTrace.world_weapon.__get__(self)
        # world_weapon uses the inventory-report action helper; construction itself is original.
        self.action=lambda name,entry,args=(),registers=():self.run(entry,args,registers)
        self.world_weapon(W2,SECOND,'objects\\weapons\\rifle\\smg\\smg')
        self.world_weapon(W3,THIRD,'objects\\weapons\\rifle\\battle_rifle\\battle_rifle')
        self.native_sites={p for e in ENTRIES for p in __import__('oracle_smg_firing_1_1')._trace(self.code,e,set(ENTRIES)).code}
        self.g.hook_add(UC_HOOK_MEM_READ,self.read_capture);self.g.hook_add(UC_HOOK_MEM_WRITE,self.write_capture)
    def gp(self):return [self.g.reg_read(r)for r in REGS]
    def xmm(self):return [v for i in range(8)for v in struct.unpack('<4I',self.g.reg_read(UC_X86_REG_XMM0+i).to_bytes(16,'little'))]
    def pages(self,address,size):
        self.ranges.add((address,address+size))
        for page in range(address&~4095,((address+size+4095)&~4095),4096):
            if page not in self.before:self.before[page]=bytes(self.g.mem_read(page,4096))
    def read_capture(self,g,access,address,size,value,user):
        if self.capturing:self.pages(address,size)
    def write_capture(self,g,access,address,size,value,user):
        if not self.capturing:return
        self.pages(address,size);site=g.reg_read(UC_X86_REG_EIP)
        row=(site,address,bytes(g.mem_read(address,size)),(value&((1<<(size*8))-1)).to_bytes(size,'little'))
        (self.lower[-1]['stores']if self.lower else self.stores).append(row)
    def hook(self,g,address,size,user):
        if self.capturing:
            if address!=STOP and address not in self.native_sites:
                site=self.last_site
                assert site in self.native_sites,(hex(site),hex(address))
                self.stopped={'site':site,'target':address,'before':self.gp()}
                g.emu_stop();return
            self.last_site=address
            if address in self.native_sites:self.coverage.add(address)
            # Do not invoke the inherited oracle's supplied service when a
            # native owner is now present. Execute its untouched body instead.
            if address in self.services:return
        EquippedTrace.hook(self,g,address,size,user)
    def capture(self,name,entry,args=(),registers=()):
        self.before={};self.ranges=set();self.stores=[];self.leaves=[];self.lower=[];self.last_site=entry;self.stopped=None;self.request_capture=entry in (0xea660,0xce510)
        self.w(STACK,STOP)
        for i,v in enumerate(args):self.w(STACK+4+i*4,v)
        for r in REGS:self.g.reg_write(r,0)
        self.g.reg_write(UC_X86_REG_ESP,STACK);self.g.reg_write(UC_X86_REG_EFLAGS,2)
        self.g.reg_write(UC_X86_REG_FPCW,0x27f);self.g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        self.g.reg_write(UC_X86_REG_FPSW,0);self.g.reg_write(UC_X86_REG_FPTAG,65535)
        for i in range(8):self.g.reg_write(UC_X86_REG_XMM0+i,0)
        for r,v in registers:self.g.reg_write(r,v&0xffffffff)
        initial=self.gp();self.capturing=True
        self.pages(STACK,4+len(args)*4)
        if entry==0x152080:
            self.pages(self.u(STACK+8),4)
            for p,n in ((0x4e78d0,4),(0x4eb114,4),(0x4f01f4,4),(PLAYER_DATA,0x40),(UNIT,0x238),(HEADERS,64)):
                self.pages(p,n)
            for item in (WI,W2,W3):
                body=self.u(HEADERS+(item&65535)*12+8);datum=self.u(body);tag=self.u(TAGS+(datum&65535)*16+8)
                self.pages(body,4);self.pages(TAGS+(datum&65535)*16,16);self.pages(tag+0x12c,4)
        self.g.emu_start(entry,STOP,count=1000000);self.capturing=False
        assert (self.g.reg_read(UC_X86_REG_EIP)==STOP or self.stopped) and not self.lower
        lines=['C '+name+' '+nums((entry,self.g.reg_read(UC_X86_REG_EAX)))+' '+str(self.g.reg_read(UC_X86_REG_ESP)-STACK)+' '+nums(initial)]
        regions=[]
        for start,end in sorted(self.ranges):
            if regions and start<=regions[-1][1]:regions[-1][1]=max(end,regions[-1][1])
            else:regions.append([start,end])
        def initial_bytes(start,end):
            return b''.join(self.before[p&~4095][p&4095:(p&4095)+1]for p in range(start,end))
        for start,end in regions:lines.append(f'M {start:x} '+initial_bytes(start,end).hex())
        for row in self.stores:lines.append('W '+nums(row[:2])+' '+row[2].hex()+' '+row[3].hex())
        if self.stopped:lines.append('B '+nums((self.stopped['site'],self.stopped['target']))+' '+nums(self.stopped['before']))
        for row in self.leaves:
            lines.append('L '+nums((row['site'],row['target'],row['flags']))+' '+nums(row['before']+row['after']+row['xafter']))
            for store in row['stores']:lines.append('S '+nums(store[:2])+' '+store[2].hex()+' '+store[3].hex())
        for start,end in regions:lines.append(f'A {start:x} '+bytes(self.g.mem_read(start,end-start)).hex())
        return lines,{'name':name,'entry':f'{entry:08x}','pages':len(self.before),'stores':len(self.stores),
            'stopped_owner':None if not self.stopped else f'{self.stopped["target"]:08x}', 'external_calls':[{'site':f'{q["site"]:08x}','target':f'{q["target"]:08x}','stores':len(q['stores']),'origin':q['origin']}for q in self.leaves]}
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);a=ap.parse_args()
    t=Capture();lines=['# Original TU1.1 action/drop routing, first required owner only; no lower replay.'];rows=[]
    baseline={p:bytes(t.g.mem_read(p,n))for p,n in ((UNIT,0x5000),(WEAPON,0x800),(SECOND,0x800),(THIRD,0x800),(PLAYER_DATA,0x21c))}
    def reset():
        for p,b in baseline.items():t.g.mem_write(p,b)
    def case(name,entry,args=(),registers=(),mask=0):
        out,row=t.capture(name,entry,args,registers);lines.extend(out);lines.append(f'R {mask:x}');rows.append(row)
    for channel in (0,1,3):
        for action in (0x06000006,0x08000002,0x08000003,0x0d00053a,0x0d00053b,0x0d000021,0x1000006c,0x08000025,0x0a000026):
            for force in (0,1):
                reset();case(f'layer_{channel}_{action:08x}_{force}',0x115650,(UI,action,0x3e88b439,force),((UC_X86_REG_ECX,channel),),255)
    for flag in (0,1):
        for wc in (0x05000720,0x06000721,0x0400054b):
            for name in (0x03000d26,0x02000c90,0x07000001,0x07000101):
                reset();case(f'identity_{flag}_{wc:08x}_{name:08x}',0x111500,(UI,name),((UC_X86_REG_EAX,wc),(UC_X86_REG_ECX,flag)),255)
    for hand in (0,1):
        for magazine in (0,1):
            for empty in (False,True):
                reset()
                if empty:t.g.mem_write(UNIT+0x212,b'\xff\xff')
                case(f'reload_start_{hand}_{magazine}_{empty}',0xe8740,(hand,magazine),((UC_X86_REG_ESI,UI),),255)
    for action in (0,1,10,11):
        for dual in (False,True):
            reset()
            if dual:t.w(UNIT+0x21c,W2);t.g.mem_write(UNIT+0x213,b'\x01')
            t.w(SCRATCH+0x500,action)
            case(f'reload_request_{action}_{dual}',0xe88b0,(UI,SCRATCH+0x500),mask=255)
            case(f'reload_update_{action}_{dual}',0xe8950,(UI,action),mask=255)
    for hand in (0,1):
        for slot in (-1,0,1):
            for force in (0,1):
                reset();t.g.mem_write(UNIT+0x216+hand,bytes([slot&255]));t.w(UNIT+0x21c,W2)
                case(f'equip_{hand}_{slot}_{force}',0xe9d50,(UI,0x07000101,0,force),((UC_X86_REG_EAX,1-hand),))
    for owner in (0xffffffff,UI):
        for parent in (0xffffffff,UI):
            reset();t.w(WEAPON+0x14,parent)
            case(f'owner_{owner:08x}_{parent:08x}',0xfb120,(owner,),((UC_X86_REG_EAX,WI),))
            reset();t.w(WEAPON+0x14,parent)
            case(f'holster_{owner:08x}_{parent:08x}',0xfb3f0,registers=((UC_X86_REG_EAX,WI),))
    for slot in (0,1):
        for dropped in (0,1):
            reset();case(f'drop_{slot}_{dropped}',0xcf8c0,registers=((UC_X86_REG_EAX,dropped),(UC_X86_REG_ESI,WI),(UC_X86_REG_EBX,UI)))
    for mode in range(7):
        reset();case(f'pickup_dispatch_{mode}',0xce510,(UI,W2,mode),mask=255)
    for hand in (0,1):
        for notification in range(10):
            reset()
            if hand:t.w(UNIT+0x21c,W2);t.g.mem_write(UNIT+0x213,b'\x01')
            case(f'weapon_notification_{hand}_{notification}',0xcb060,(UI,W2 if hand else WI,notification))
    for action in (2,3,4,5,6,7,12,13,14,15,16,17):
        reset();t.w(SCRATCH+0x500,action)
        case(f'fire_layer_request_{action}',0xe8a70,(UI,SCRATCH+0x500),mask=255)
    raw=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(raw)
    report=dict(schema=1,original_sha256=PIN,cache_sha256=CACHE_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(raw).hexdigest(),cases=rows,
        tested_native_instruction_addresses=[f'{p:08x}'for p in sorted(t.coverage)],
        captured_lower_outputs_used=False,live_gameplay_proof=False,
        scope='Declared action, hand, selector, parent and owner controls around the explicit original equipped fixture. Untouched original instructions execute only through the first external owner. No external result, world physics, source clock or live Unit construction is supplied.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(rows),stopped=sum(r['stopped_owner']is not None for r in rows),stores=sum(r['stores']for r in rows),instructions=len(t.coverage),fixture_sha256=report['fixture_sha256'])))
if __name__=='__main__':main()
