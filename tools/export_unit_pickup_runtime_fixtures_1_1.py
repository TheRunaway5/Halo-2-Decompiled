#!/usr/bin/env python3
"""Independent original pickup captures with explicit complete lower-call boundaries.

Only the native inventory owners are compared here. Graph lookup, world/physics,
notifications and action callbacks execute in the original oracle and remain
recorded, named differential boundaries, never production implementations.
"""
import argparse, hashlib, json, struct, random
from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_spartan_equipped_animation_1_1 import EquippedTrace
from oracle_mp_inventory_1_1 import InventoryTrace, SECOND, THIRD, W2, W3, SCRATCH
from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, Cache, UNIT, UI, WEAPON, WI, TAGS, HEADERS, STACK, STOP, PLAYER_DATA
from generate_unit_pickup_runtime_1_1 import ENTRIES
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
            if self.lower and address==self.lower[-1]['ret']:
                row=self.lower.pop();row['after']=self.gp();row['xafter']=self.xmm();row['flags']=g.reg_read(UC_X86_REG_EFLAGS);self.leaves.append(row)
            if not self.lower and address!=STOP and address not in self.native_sites:
                sp=g.reg_read(UC_X86_REG_ESP);ret=self.u(sp)
                site=self.last_site
                assert site in self.native_sites,(hex(site),hex(address))
                row={'site':site,'target':address,'ret':ret,'before':self.gp(),'stores':[],
                    'origin':self.services[address][0]if address in self.services else ('explicit exact cache-byte delivery'if address==0x124f30 else'original instruction execution')}
                if address in (0xba980,0xbdf10,0xcf8c0,0xfb120,0xfb3f0) or (self.request_capture and address not in (0x1cfde0,0x1e14e0,0x1d1b70)):
                    self.stopped=row;g.emu_stop();return
                self.lower.append(row)
            if not self.lower:
                self.last_site=address
                if address in self.native_sites:self.coverage.add(address)
        super().hook(g,address,size,user)
        # Supplied oracle services return inside the entry hook.
        if self.capturing and self.lower and g.reg_read(UC_X86_REG_EIP)==self.lower[-1]['ret']:
            row=self.lower.pop();row['after']=self.gp();row['xafter']=self.xmm();row['flags']=g.reg_read(UC_X86_REG_EFLAGS);self.leaves.append(row)
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
    t=Capture();lines=['# literal original TU1.1 pickup runtime v1'];rows=[]
    baseline={p:bytes(t.g.mem_read(p,n))for p,n in ((UNIT,0x5000),(WEAPON,0x800),(SECOND,0x800),(THIRD,0x800),(PLAYER_DATA,0x21c))}
    def reset():
        for p,b in baseline.items():t.g.mem_write(p,b)
    def case(name,entry,args=(),registers=()):
        out,row=t.capture(name,entry,args,registers);lines.extend(out);rows.append(row)
    rng=random.Random(0xcec00)
    # Full player restriction path over real full-datum slots and both hand selectors.
    defs=[tag for tag in t.cache.tags.values()if tag['group']=='weap']
    for index in range(160):
        reset();target=rng.choice(defs);t.w(SECOND,int(target['datum'],16));t.w(WEAPON,int(rng.choice(defs)['datum'],16));t.w(THIRD,int(rng.choice(defs)['datum'],16));slots=[rng.choice((0xffffffff,WI,W3))for _ in range(4)]
        for i,slot in enumerate(slots):t.w(UNIT+0x218+i*4,slot)
        valid=[-1]+[i for i,s in enumerate(slots)if s!=0xffffffff]
        hands=[rng.choice(valid)for _ in range(4)];t.g.mem_write(UNIT+0x212,struct.pack('<bb',*hands[:2]));t.g.mem_write(UNIT+0x216,struct.pack('<bb',*hands[2:]))
        t.w(UNIT+0x14,0xffffffff if index%5 else W3);flags=bytes(rng.randrange(2)for _ in range(4));t.g.mem_write(SCRATCH+0x400,flags)
        case(f'player_limits_{index}',0x152080,(W2,SCRATCH+0x400),((UC_X86_REG_EAX,0),))
    # Complete CEC00 paths through real manager capability and player limits.
    for index in range(24):
        reset();target=rng.choice(defs);t.w(SECOND,int(target['datum'],16))
        if index%3:t.w(UNIT+0x21c,W3)
        if index%4==0:t.g.mem_write(UNIT+0x212,b'\xff\xff')
        if index%4==1:t.g.mem_write(UNIT+0x213,b'\x01');t.g.mem_write(UNIT+0x217,b'\x01')
        t.sh(WEAPON+0x22a,(0,120,240)[index%3]);t.sh(SECOND+0x22a,index*7);t.sh(SECOND+0x22c,0 if index%2 else 60)
        t.g.mem_write(SCRATCH+0x400,b'\xa5\xa5\xa5\xa5')
        case(f'admission_{index}',0xcec00,(UI,W2,SCRATCH+0x400))
    # Empty/incompatible/owned target and all pickup mode gates, including signed invalid modes.
    for scenario in ('same','other','empty_primary','dual'):
        for mode in (-1,0,1,2,3,4,5,6,7):
            reset()
            if scenario=='other':t.w(SECOND,t.u(THIRD))
            if scenario=='empty_primary':t.g.mem_write(UNIT+0x212,b'\xff\xff')
            if scenario=='dual':
                t.w(THIRD,t.u(WEAPON));t.w(UNIT+0x21c,W3);t.g.mem_write(UNIT+0x213,b'\x01');t.g.mem_write(UNIT+0x217,b'\x01');t.f(THIRD+0x184,.75)
            t.g.mem_write(SCRATCH+0x500,struct.pack('<IIh',20,W2,mode))
            case(f'request_{scenario}_mode_{mode}',0xea660,(UI,SCRATCH+0x500))
    for scenario in ('same','other','empty_primary','dual'):
        for mode in range(7):
            reset()
            if scenario=='other':t.w(SECOND,t.u(THIRD))
            if scenario=='empty_primary':t.g.mem_write(UNIT+0x212,b'\xff\xff')
            if scenario=='dual':
                t.w(THIRD,t.u(WEAPON));t.w(UNIT+0x21c,W3);t.g.mem_write(UNIT+0x213,b'\x01');t.g.mem_write(UNIT+0x217,b'\x01');t.f(THIRD+0x184,.75)
            case(f'dispatch_{scenario}_mode_{mode}',0xce510,(UI,W2,mode))
    reset();t.w(SECOND+0x154,UI);case('request_owned_target',0xea660,(UI,SCRATCH+0x500))
    # All reserve/loaded donor ordering and full-capacity rejections.
    for index in range(20):
        reset();t.sh(WEAPON+0x22a,(0,120,239,240)[index%4]);t.sh(SECOND+0x22a,(0,50)[index%2]);t.sh(SECOND+0x22c,(0,1,60)[index%3]);t.f(WEAPON+0x184,(0.,.75)[index%2]);t.f(SECOND+0x184,(0.,.25)[index%3==0])
        case(f'ammo_{index}',0x1058f0,registers=((UC_X86_REG_EAX,WI),(UC_X86_REG_ECX,W2)))
    raw=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(raw)
    report={'schema':1,'original_sha256':PIN,'cache_sha256':CACHE_PIN,'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'fixture_sha256':hashlib.sha256(raw).hexdigest(),'cases':rows,'live_gameplay_proof':False,
        'tested_native_instruction_addresses':[f'{p:08x}'for p in sorted(t.coverage)],
        'input_controls':'Declared inventory, hand, parent, authored weapon metadata, magazine and heat perturbations around an original equipped fixture; not 269 natural gameplay scenarios.',
        'scope':'Complete native pickup owners versus unchanged original instructions. Explicit preallocated equipped unit/player and item banks; graph, action, world/physics and notification owners execute originally and are recorded differential-only boundaries.'}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'cases':len(rows),'stores':sum(q['stores']for q in rows),'external_calls':sum(len(q['external_calls'])for q in rows),'fixture_bytes':len(raw),'fixture_sha256':report['fixture_sha256']}))
if __name__=='__main__':main()
