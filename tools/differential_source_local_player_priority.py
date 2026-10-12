#!/usr/bin/env python3
"""Untouched3E9C0/155760/BAF80 over explicit actual/controlled CPU inputs.

No arithmetic, observer, owner-lookup or return hooks. This tool reads retained
files only; the separate read-only exporter records later3290A metadata.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,re,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
NONE=0xffffffff
BASE=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008'
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def word(raw,at=0):return struct.unpack_from('<I',raw,at)[0]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def virtual(ram,cr3,address,size):
    out=bytearray()
    for offset in range(size):
        va=address+offset;pde=word(ram,(cr3&0xfffff000)+(va>>22)*4);assert pde&1
        if pde&128:pa=(pde&0xffc00000)|(va&0x3fffff)
        else:
            pte=word(ram,(pde&0xfffff000)+((va>>12)&1023)*4);assert pte&1
            pa=(pte&0xfffff000)|(va&4095)
        assert pa<len(ram);out.append(ram[pa])
    return bytes(out)
def live(x):
    freshdir=BASE/'priority-metadata-3290A';olddir=BASE/'capture-opengl'
    fresh=json.loads((freshdir/'capture.json').read_text());old=json.loads((olddir/'capture.json').read_text())
    assert fresh['registers']['EIP']==0x3290a and fresh['initial']==fresh['final'] and not fresh['initial']['running']
    assert fresh['clock_tick']==12475 and fresh['frame']==111186
    ff={f['file']:f for f in fresh['files']}
    def load(name):
        raw=(freshdir/name).read_bytes();assert len(raw)==ff[name]['size'] and sha(raw)==ff[name]['sha256'];return raw
    for name in ff:load(name)
    assert load('clock-before.bin')==load('clock-after.bin')
    for item in fresh['code']:
        at=int(item['address'],16);raw=load(f'code-{at:08X}.bin');assert raw==x.read(at,len(raw))
    actual=old['priority_observations'];assert len(actual)==1
    event=actual[0];owner=int(event['owner'],16);assert owner==0xe2e60077 and event['priority']==1
    assert int(re.search(r'EAX=([0-9a-f]+)',event['exit_registers']).group(1),16)&255==1
    assert int(re.search(r'MXCSR=([0-9a-fA-F]+)',event['entry_registers']).group(1),16)==0x1fb0
    phase=next(p for p in old['phases']if p['name']=='33400-entry')
    assert fresh['clock_tick']==phase['clock_tick'] and fresh['view_index']==phase['view_index']
    files={f['file']:f for f in phase['files']}
    def prior(name):
        raw=(olddir/name).read_bytes();assert len(raw)==files[name]['size'] and sha(raw)==files[name]['sha256'];return raw
    before=prior(f'33400-entry-object-{owner:08X}.bin');current=load(f'object-{owner:08X}.bin')
    handles=prior('33400-entry-object-handles.bin');entry=(owner&65535)*12
    assert word(before,0x14)==word(current,0x14)==NONE and before[0x13c:0x140]==current[0x13c:0x140]
    assert handles[entry+3]==load('object-handles.bin')[entry+3]==0
    slot=struct.unpack('<i',load('local-slot.bin'))[0];assert slot==0
    assert word(prior('33400-entry-view-context.bin'),0x158)==slot
    player=word(current,0x13c);assert player==int(re.search(r'ECX=([0-9a-f]+)',event['exit_registers']).group(1),16)
    observer=load('observer-bank.bin');players=load('players-global.bin')
    assert word(observer,0xc)==0x23c110 and word(observer,4)==0 and word(players,0xc)==player
    consumed=[(0x4b9ed8,4,load('local-slot.bin')),(0x4e8c44+4,4,observer[4:8]),(0x4e8c44+0xc,4,observer[0xc:0x10]),
        (0x4e8c20,4,load('players-global-pointer.bin')),(int(fresh['players_global'],16)+0xc,4,players[0xc:0x10]),
        (int(fresh['owners'][0]['address'],16)+0x13c,4,current[0x13c:0x140])]
    baselines=[]
    for ramfile,manifestfile,key in [
        ('analysis/multiplayer-shading/registration-snapshot-restore-20261008/exports/physical-ram.bin','analysis/multiplayer-shading/registration-snapshot-restore-20261008/export.json','registers_before'),
        ('analysis/multiplayer-shading/shadow-live-phase-20261008/session3/restored-physical-ram.bin','analysis/multiplayer-shading/shadow-live-phase-20261008/session3/launch.json','registers')]:
        manifest=json.loads((ROOT/manifestfile).read_text());cr3=int(re.search(r'CR3=([0-9a-fA-F]+)',manifest[key]).group(1),16)
        ram=(ROOT/ramfile).read_bytes();assert len(ram)==0x4000000
        for at,size,raw in consumed:assert virtual(ram,cr3,at,size)==raw
        baselines.append(dict(file=ramfile,sha256=sha(ram),cr3=f'{cr3:08X}',matching_consumed_bytes=sum(size for _,size,_ in consumed)))
    row=[1,1,owner,slot,fresh['observer_special']]
    for i in range(4):row.extend([word(observer,i*0x140+0xc),word(observer,i*0x140+4)])
    row.extend(word(players,0xc+i*4)for i in range(4));row.extend([owner,NONE,0,player])
    report=dict(observed_priority=event['priority'],owner=f'{owner:08X}',root=f'{owner:08X}',actual_entry='0003E9C0',actual_return=event['return_address'],
        actual_AL='01',actual_EAX=re.search(r'EAX=([0-9a-f]+)',event['exit_registers']).group(1).upper(),local_slot=slot,
        local_player=f'{player:08X}',root_header_kind=0,observer_callback='0023C110',observer_scalar_bits='00000000',
        metadata_phase='0003290A',tick=fresh['clock_tick'],frame=fresh['frame'],source_backend='Original Xemu OpenGL/llvmpipe',
        consumed_earlier_body_and_view_fields_match=True,baseline_matches=baselines,
        metadata_manifest_sha256=sha((freshdir/'capture.json').read_bytes()),phase_manifest_sha256=sha((olddir/'capture.json').read_bytes()),
        boundary='3E9C0 entry/return registers and AL are actual call observations. Local slot/headerkind/parent/player fields agree with prior33400 exports; fresh observer/local-user fields agree with both earlier RAM banks. Callback/scalar are later3290A corroboration, while actual AL1 independently requires observerclass0. No simultaneous3E9C0 raw observer export claim. No guest IO in this tool.')
    return row,report
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);args=ap.parse_args()
    x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    MEMORY=0x2100000;u.mem_map(MEMORY,0x400000);STOP=0x3000000;u.mem_map(STOP,4096)
    HEADER=MEMORY;HANDLES=MEMORY+0x1000;OBJECTS=MEMORY+0x20000;PLAYERS=MEMORY+0x40000;STACK=MEMORY+0x3fff00
    u.mem_write(0x4e0300,pack([HEADER]));u.mem_write(HEADER+0x44,pack([HANDLES]));u.mem_write(0x4e8c20,pack([PLAYERS]))
    executed={};calls=collections.Counter();branches=collections.Counter()
    def observe(g,pc,size,_):
        if pc==STOP:g.emu_stop();return
        if pc not in executed:assert bytes(g.mem_read(pc,size))==x.read(pc,size);executed[pc]=size
        if pc in [0x3e9c0,0x155760,0xbaf80]:calls[f'{pc:08X}']+=1
        if pc in [0x3ea17,0x3ea1c,0x3ea21,0x15578d,0x155798,0x1557b7,0x1557bd]:branches[f'{pc:08X}']+=1
    u.hook_add(UC_HOOK_CODE,observe)
    def run(at,slot,owner):
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
        u.reg_write(UC_X86_REG_ESI,owner);u.reg_write(UC_X86_REG_EAX,slot);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.reg_write(UC_X86_REG_MXCSR,0x1fb0);u.mem_write(STACK,pack([STOP]));u.emu_start(at,STOP+1,count=10000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+4
        return u.reg_read(UC_X86_REG_EAX)&255
    def original(row):
        version,no,owner,slot,special=row[:5];assert version==1 and no<=64;at=5
        for i in range(4):
            callback,scalar=row[at:at+2];at+=2;u.mem_write(0x4e8c44+i*0x140+0xc,pack([callback]));u.mem_write(0x4e8c44+i*0x140+4,pack([scalar]))
        u.mem_write(PLAYERS+0xc,pack(row[at:at+4]));at+=4;u.mem_write(0x51ec10,bytes([special]));u.mem_write(0x4b9ed8,pack([slot]))
        for i in range(no):
            identity,parent,kind,player=row[at:at+4];at+=4;ptr=OBJECTS+i*0x200;entry=HANDLES+(identity&65535)*12
            u.mem_write(ptr,bytes(0x200));u.mem_write(ptr+0x14,pack([parent]));u.mem_write(ptr+0x13c,pack([player]));u.mem_write(entry+3,bytes([kind]));u.mem_write(entry+8,pack([ptr]))
        assert at==len(row)
        return [run(0x155760,i,owner)for i in range(4)]+[run(0x3e9c0,slot,owner)]
    rows=[];labels=[]
    def add(observers,players,slot,special,depth=0,kind=0,rootplayer=None,none=False,label='controlled source fields'):
        ids=[0xe2e60077+i*0x10001 for i in range(depth+1)]
        owners=[[id,ids[i+1]if i<depth else NONE,kind if i==depth else (i+1)*7,rootplayer if i==depth else NONE]for i,id in enumerate(ids)]
        row=[1,0 if none else len(owners),NONE if none else ids[0],slot&NONE,special,*[v for pair in observers for v in pair],*players]
        if not none:row.extend(v for o in owners for v in o)
        rows.append(row);labels.append(label)
    callbacks=[0,0x23c110,0x23cbb0,0x16c840,0x23d090,0x23de50,0x23c111,NONE]
    scalars=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc00000,0xffc00000,0x7f800001,0xff800001,0x7fffffff,NONE]
    players=[0xec700000,0xec710001,NONE,0]
    for callback in callbacks:
        for scalar in scalars:
            for special in [0,1,0x80,255]:
                for slot in range(4):
                    observers=[[0x23c110,0],[0x23cbb0,0],[0x16c840,0],[0x23d090,0]];observers[slot]=[callback,scalar]
                    for depth in [0,1,4]:add(observers,players,slot,special,depth=depth,rootplayer=players[slot],label='callback/scalar/special/slot/root product')
    for slot in range(4):
        for kind in [0,1,2,6,7,11,255]:
            for rootplayer in [players[slot],players[slot]^1,NONE,0]:
                for depth in [0,3,7]:add([[0x23c110,0]]*4,players,slot,0,depth=depth,kind=kind,rootplayer=rootplayer,label='literal root kind/player includingNONE')
    for callback in callbacks:
        add([[callback,0x7fc00000]]*4,players,-1,0,rootplayer=players[0],label='slotNONE early branch')
        add([[callback,0x7fc00000]]*4,players,0,255,none=True,label='ownerNONE early branch')
    rng=random.Random(0x3e9c0)
    for i in range(2048):
        observers=[[rng.choice(callbacks),rng.getrandbits(32)]for _ in range(4)];slot=rng.randrange(4)
        if i%2==0:observers[slot]=[0x23c110,rng.choice([0,0x80000000])]
        local=[rng.getrandbits(32)for _ in range(4)];root=local[slot]if i%3 else rng.getrandbits(32)
        add(observers,local,slot,rng.randrange(256),depth=rng.randrange(8),kind=rng.choice([0,0,1,2,7]),rootplayer=root,label='deterministic randomized literal states')
    liverow,live_report=live(x);rows.append(liverow);labels.append('corroborated actual original priority call')
    expected=[original(row)for row in rows];assert expected[-1]==[0,0,0,0,1]
    fixture=ROOT/'tests/fixtures/retail_source_local_player_priority.tsv'
    fixture.write_text('# Complete unchanged3E9C0/155760/BAF80. Explicit observer/local-user/owner fields; final row corroborates actual call with temporally labelled metadata.\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(rows,expected)))
    failures=[]
    if args.native:
        result=subprocess.run([str(args.native.resolve()),'--probe'],input=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in rows),text=True,capture_output=True,check=True)
        actual=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()];assert len(actual)==len(expected)
        failures=[dict(case=i,label=labels[i],source=b,native=a)for i,(a,b)in enumerate(zip(actual,expected))if a!=b]
    report=dict(cases=len(rows),output_bytes=len(rows)*5,failures=failures,native_compared=bool(args.native),
        native_binary_sha256=sha(args.native.read_bytes())if args.native else None,
        source_xbe_sha256=PIN,fixture_sha256=sha(fixture.read_bytes()),
        original_functions={f'{at:08X}':dict(size=n,sha256=sha(x.read(at,n)))for at,n in [(0x3e9c0,0x65),(0x155760,0x60),(0xbaf80,0x28)]},
        executed_instruction_bytes=sum(executed.values()),calls=dict(calls),branches=dict(branches),
        observer_classes=dict(collections.Counter(str(out[i])for out in expected for i in range(4))),priorities=dict(collections.Counter(str(out[4])for out in expected)),
        actual_live_call=live_report,boundary='Unchanged original routines run with literal CPU banks, masked SSE exceptions and source MXCSR1FB0/DAZclear. No function/lookup/math/return hooks. Controlled fields are not runtime producer proof. Recorded actual call is independently corroborated with later3290A metadata and earlier matched fields. No guest advancement or IO from this tool; native observer/local-user/registration producers remain caller inputs.')
    output=ROOT/'analysis/multiplayer-shading/local-player-priority-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ['cases','output_bytes','failures','calls','branches','priorities']},indent=2));assert not failures
if __name__=='__main__':main()
