#!/usr/bin/env python3
"""Uninterrupted original object birth, linked animation and first-owner stops.

Pool/heap initialization executes unchanged original owners. Authored cache,
an actual earlier original request and its observed clock cells are declared
inputs. No Unit body, component offset, readiness or successful lower result
is supplied during the compared calls. This is not full game startup.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID, UC_MEM_READ_UNMAPPED
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, Cache, Xbe, _Code, _trace
from generate_unit_object_runtime_1_1 import ENTRIES, ANIMATION_ENTRIES, WEAPON_ENTRIES

HEAP, HEADERS, SCRATCH = 0x02000000, 0x02200000, 0x02300000
POOL, TAGS, WORLD, STACK, STOP, MAIN = HEADERS-0x4c, 0x01080000, 0x02301000, 0x02408000, 0x03000000, 0x04000000
GP = [getattr(R, 'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
INPUT = ROOT/'analysis/multiplayer/integration/spawn-source/generic-object-constructor/measured-v3/report.json'
INPUT_PIN = '8ffdb075f939ac69eb3c2c95171c0809d0cceba7e4462e471660ae8dcdddc881'
CLOCK_INPUT = ROOT/'analysis/multiplayer/integration/spawn-source/unit-local-transition/measured-positive/owning/report.json'
CLOCK_PIN = '84b6cdf3b02e88f3c0428e6ee0a8d07bf5cfe2917a48964e0b6b3b501acb45e8'
GAME_INPUT = ROOT/'analysis/multiplayer/integration/spawn-source/unit-constructors/measured-complete/report.json'
GAME_PIN = 'f70eac9b61f3febe1b5f45627fa86c102413d8880588ec1f83995747ff5c190e'
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def nums(values): return ' '.join(f'{v:x}' for v in values)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);a=ap.parse_args()
    assert sha(INPUT)==INPUT_PIN
    request_source=json.loads(INPUT.read_text())
    assert sha(CLOCK_INPUT)==CLOCK_PIN
    clock_source=next(q['clock_after'] for q in json.loads(CLOCK_INPUT.read_text())['calls'] if 'clock_after' in q)
    assert sha(GAME_INPUT)==GAME_PIN
    game_reads=json.loads(GAME_INPUT.read_text())['reads']
    root=next(q for q in request_source['calls'] if not q['ancestors'] and q['id']==0)
    request=bytes.fromhex(root['before']['D'])
    path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==PIN
    image=Xbe(path);code=_Code(image,'.text')
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    owners=set(ENTRIES)|set(ANIMATION_ENTRIES)|set(WEAPON_ENTRIES)
    sites={p for e in owners for p in _trace(code,e,owners).code}
    all_executed=set();rows=[];lines=['# Uninterrupted original Unit construction/pose graph. Zero lower-call replay.']
    trials=[('unit',2048,0x100000),('body_exhaustion',2048,256),('component_exhaustion',2048,0x600),
        ('header_exhaustion',1,0x100000),('pressure_reclaim',2048,0x100000),('unit_seeded',2048,0x100000)]
    trials += [(f'layer_{layer}_{action:x}',2048,0x100000) for layer in (0,1,3) for action in (0x06000006,0x08000002)]
    trials += [('identity_smg',2048,0x100000),('identity_dual',2048,0x100000)]
    for title,capacity,heap_bytes in trials:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(HEAP,0x500000);g.mem_map(STOP,4096);g.mem_map(MAIN,0x107000)
        page=cache.magic&~4095;g.mem_map(page,(len(cache.data)+cache.magic-page+4095)&~4095)
        g.mem_write(cache.magic,bytes(cache.data));g.mem_map(TAGS,0x100000)
        def w(p,v,n=4):g.mem_write(p,v.to_bytes(n,'little'))
        def u(p,n=4):return int.from_bytes(g.mem_read(p,n),'little')
        w(0x4eb114,TAGS)
        for datum,t in cache.tags.items():
            if t['offset'] is not None:
                p=TAGS+(datum&65535)*16
                # Literal cache directory group, in the original tag-word order.
                g.mem_write(p,t['group'][::-1].encode())
                w(p+8,t['offset']+cache.magic)
        clock_pointer=next(bytes.fromhex(q['raw_hex']) for q in request_source['reads'] if q['address']=='00518444')
        clock=int.from_bytes(clock_pointer,'little');g.mem_map(clock&~4095,4096);g.mem_write(0x518444,clock_pointer)
        clock_cells={}
        assert int(clock_source['pointer'],16)==clock
        clock_bytes=bytes.fromhex(clock_source['raw0c_hex'])
        g.mem_write(clock,clock_bytes)
        clock_cells.update({clock+i:b for i,b in enumerate(clock_bytes)})
        for q in request_source['reads']:
            p=int(q['address'],16)
            if clock<=p and p+q['bytes']<=clock+0x100:
                raw=bytes.fromhex(q['raw_hex'])
                for i,b in enumerate(raw):
                    if p+i in clock_cells:assert clock_cells[p+i]==b
                    clock_cells[p+i]=b
                g.mem_write(p,raw)
        game_pointer=next(bytes.fromhex(q['raw_hex']) for q in game_reads if q['address']=='004edf18')
        game=int.from_bytes(game_pointer,'little');g.mem_map(game&~4095,4096);g.mem_write(0x4edf18,game_pointer)
        game_cells={}
        for q in game_reads:
            p=int(q['address'],16)
            if game<=p<game+0x100:
                raw=bytes.fromhex(q['raw_hex']);g.mem_write(p,raw)
                game_cells.update({p+i:b for i,b in enumerate(raw)})
        g.mem_write(HEAP,b'\xcd'*((heap_bytes+0x53)&~3))
        g.mem_write(SCRATCH+16,b'objects\0');g.mem_write(SCRATCH+48,b'object\0')
        w(0x4e78d0,POOL);w(0x4e58bc,HEAP);w(0x4e58c4,WORLD)
        active=[False];last=[0];before={};ranges=set();stores=[];stopped=[];missing=[];executed=set()
        def pages(p,n):
            ranges.add((p,p+n))
            for base in range(p&~4095,(p+n+4095)&~4095,4096):
                if base not in before:before[base]=bytes(g.mem_read(base,4096))
        def instruction(uc,p,n,_):
            if active[0] and p not in sites and p!=STOP:
                stopped.append(dict(site=last[0],target=p,gp=[uc.reg_read(r) for r in GP],
                    xmm=[uc.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')) for i in range(8)],
                    mxcsr=uc.reg_read(R.UC_X86_REG_MXCSR)))
                uc.emu_stop();return
            assert bytes(uc.mem_read(p,n))==image.read(p,n),hex(p)
            if active[0]:executed.add(p)
            last[0]=p
        def read(uc,access,p,n,value,_):
            if active[0]:
                if clock<=p<clock+0x100:assert all(p+i in clock_cells for i in range(n)),('unobserved clock cell',hex(p),n)
                if game<=p<game+0x100:assert all(p+i in game_cells for i in range(n)),('unobserved game cell',hex(p),n)
                pages(p,n)
        def write(uc,access,p,n,value,_):
            if active[0]:
                pages(p,n)
                stores.append((last[0],p,bytes(uc.mem_read(p,n)),(value&((1<<(8*n))-1)).to_bytes(n,'little')))
        def invalid(uc,access,p,n,value,_):
            assert active[0] and access==UC_MEM_READ_UNMAPPED,('unexpected original memory boundary',access,hex(p),n)
            missing.append(dict(instruction=last[0],address=p,bytes=n,gp=[uc.reg_read(r) for r in GP],
                xmm=[uc.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')) for i in range(8)]))
            return False
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        g.hook_add(UC_HOOK_MEM_INVALID,invalid)
        def run(entry,args=(),regs=(),until=STOP):
            for i,r in enumerate(GP):g.reg_write(r,0x11111111*(i+1)&0xffffffff)
            g.reg_write(R.UC_X86_REG_ESP,STACK);w(STACK,STOP)
            for i,v in enumerate(args):w(STACK+4+4*i,v)
            for r,v in regs:g.reg_write(r,v)
            g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
            g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,65535)
            g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
            initial=[g.reg_read(r) for r in GP]
            g.emu_start(entry,until,count=3000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==until or stopped
            return initial
        run(0x16d070,(POOL,SCRATCH+48,0),((R.UC_X86_REG_ECX,0),(R.UC_X86_REG_EDX,12),(R.UC_X86_REG_EBX,capacity)))
        run(0x16d210,regs=((R.UC_X86_REG_EAX,POOL),))
        run(0x18fc80,(SCRATCH+16,0),((R.UC_X86_REG_ESI,HEAP),(R.UC_X86_REG_EBX,heap_bytes)))
        # Exact B76B0 world-record clear; other subsystem initialization is
        # outside this explicitly bounded storage preparation experiment.
        run(0xb772e,regs=((R.UC_X86_REG_ESI,WORLD),),until=0xb7739)
        # The actual physical counter constructor prefix precedes admission.
        # Native arena address/cursor are explicit storage inputs; the owner
        # itself allocates, checksums, publishes and clears its four bytes.
        w(0x4ed650,SCRATCH+0x2000);w(0x4ed654,0)
        run(0x1c5f00,until=0x1c5f3b)
        # Retained anonymous backing is an explicit platform-storage input.
        # Actual148190/22E4E0 construct the main heap and pressure counters.
        w(0x480d90,MAIN+0x1000);run(0x148190)
        pressure_allocations=0
        if title=='pressure_reclaim':
            # Allocation pressure is produced by actual main-heap allocations,
            # never by editing its counters or supplying a classification.
            while True:
                run(0x148740)
                if g.reg_read(R.UC_X86_REG_EAX)>=2:break
                assert pressure_allocations<128
                run(0x22e950,(8192,),((R.UC_X86_REG_ECX,u(0x480da0)),))
                assert g.reg_read(R.UC_X86_REG_EAX)!=0
                pressure_allocations+=1
        if title=='header_exhaustion':run(0x16d2c0,regs=((R.UC_X86_REG_EDX,POOL),))
        g.mem_write(SCRATCH+0x100,request)
        if title=='unit_seeded':w(SCRATCH+0x100+0x74,1)
        before.clear();ranges.clear();stores.clear();stopped.clear();executed.clear()
        # ABI setup is a declared call input and precedes observation.
        for i,r in enumerate(GP):g.reg_write(r,0x11111111*(i+1)&0xffffffff)
        g.reg_write(R.UC_X86_REG_ESP,STACK)
        g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
        w(STACK,STOP);w(STACK+4,SCRATCH+0x100);initial=[g.reg_read(r) for r in GP]
        active[0]=True;pages(STACK,8)
        try:g.emu_start(0xb8a30,STOP,count=3000000)
        except UcError:
            assert len(missing)==1
        active[0]=False
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP or stopped or missing
        entry=0xb8a30
        prefix=None
        if title.startswith('layer_') or title.startswith('identity_'):
            # Separate declared queries on the genuinely source-allocated Unit
            # after the measured interrupted birth prefix. No weapon is
            # equipped or published by this preparation.
            assert missing and u(POOL+0x3c)==1
            prefix=dict(missing_instruction=missing[0]['instruction'],stores=len(stores))
            unit=u(HEADERS,2)<<16
            before.clear();ranges.clear();stores.clear();stopped.clear();missing.clear();executed.clear()
            if title.startswith('layer_'):
                _,layer,action=title.split('_');entry=0x115650
                args=(unit,int(action,16),0,0);regs=((R.UC_X86_REG_ECX,int(layer)),)
            else:
                entry=0x111500;args=(unit,0x03000d26)
                regs=((R.UC_X86_REG_EAX,0x0400054b if title=='identity_dual' else 0x05000720),(R.UC_X86_REG_ECX,1))
            # run() prepares the owning ABI before observation, so capture
            # that setup once, then execute from the same entry unchanged.
            for i,r in enumerate(GP):g.reg_write(r,0x11111111*(i+1)&0xffffffff)
            g.reg_write(R.UC_X86_REG_ESP,STACK);w(STACK,STOP)
            for i,v in enumerate(args):w(STACK+4+4*i,v)
            for r,v in regs:g.reg_write(r,v)
            g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
            initial=[g.reg_read(r) for r in GP];active[0]=True;pages(STACK,4+4*len(args))
            try:g.emu_start(entry,STOP,count=3000000)
            except UcError:assert len(missing)==1
            active[0]=False
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP or stopped or missing
        lines.append('C '+title+' '+nums((entry,g.reg_read(R.UC_X86_REG_EAX)))+' '+str(g.reg_read(R.UC_X86_REG_ESP)-STACK)+' '+nums(initial))
        # Compare complete request/world/header/bitmap banks and the complete
        # retained logical body, including bytes not touched by the query.
        # Every modified page was saved before its first original write.
        pages(POOL,0x4c+capacity*12+((capacity+31)//32)*4)
        pages(WORLD,0x84);pages(SCRATCH+0x100,0xc4);pages(HEAP,0x40)
        if u(HEADERS+8):pages(u(HEADERS+8),u(HEADERS+6,2))
        regions=[]
        for start,end in sorted(ranges):
            if regions and start<=regions[-1][1]:regions[-1][1]=max(end,regions[-1][1])
            else:regions.append([start,end])
        def old(p,n):return bytes(before[(p+i)&~4095][(p+i)&4095] for i in range(n))
        for start,end in regions:lines.append(f'M {start:x} '+old(start,end-start).hex())
        for pc,p,b,v in stores:lines.append('W '+nums((pc,p))+' '+b.hex()+' '+v.hex())
        if stopped:
            s,=stopped;lines.append('B '+nums((s['site'],s['target']))+' '+nums(s['gp']))
            lines.append('X '+nums([v>>(32*j)&0xffffffff for v in s['xmm'] for j in range(4)]))
        if missing:
            s,=missing;lines.append('D '+nums((s['instruction'],s['address'],s['bytes']))+' '+nums(s['gp']))
            lines.append('X '+nums([v>>(32*j)&0xffffffff for v in s['xmm'] for j in range(4)]))
        for start,end in regions:lines.append(f'A {start:x} '+bytes(g.mem_read(start,end-start)).hex())
        lines.append('R ffffffff')
        all_executed.update(executed)
        rows.append(dict(name=title,entry=entry,interrupted_unit_preparation=prefix,actual_main_heap_pressure_allocations=pressure_allocations,stores=len(stores),unique_instructions=len(executed),
            stopped=stopped[0]['target'] if stopped else None,
            missing_backing=missing[0] if missing else None,
            source_headers=bytes(g.mem_read(HEADERS,12)).hex(),source_pool_count=u(POOL+0x3c),
            source_body_extent=u(HEADERS+6,2),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR)))
    raw=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(raw)
    report=dict(schema=1,original_sha256=PIN,cache_sha256=CACHE_PIN,request_source_sha256=INPUT_PIN,clock_source_sha256=CLOCK_PIN,game_source_sha256=GAME_PIN,
        tool_sha256=sha(__file__),fixture_sha256=hashlib.sha256(raw).hexdigest(),cases=rows,
        unique_executed_instructions=len(all_executed),supplied_lower_results=0,replayed_lower_calls=0,
        scope=__doc__)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report))
if __name__=='__main__':main()
