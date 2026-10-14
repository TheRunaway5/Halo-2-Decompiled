#!/usr/bin/env python3
"""Untouched death-camera lifecycle with declared owner/API records.

Player ownership, occupied pools, seat flags, animation state, observer position,
frame time, local RNG and camera transition controls are explicit diagnostic
inputs. Every original nested owner executes, with no instruction patches or
supplied successful returns. These component experiments do not establish live
corpse poses, gameplay contacts or a rendered death-to-respawn sequence.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from export_mp_ragdoll_owners import spans

BASE,SIZE,STACK,SP,STOP=0x1000000,0x20000,0x2000000,0x200f000,0x3000000
POOL,HEADERS,UNIT,VEHICLE,PLAYERS,ROWS,TAGS,DEF,SEATS,USERS,GAME,CLOCK,RNG,GLOBALS,CONTROLS,MANAGER,ENGINE,OUT=(BASE+x for x in (0,0x1000,0x2000,0x4000,0x6000,0x7000,0x8000,0x9000,0xa000,0xb000,0xc000,0xd000,0xe000,0xf000,0x10000,0x11000,0x12000,0x13000))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--source',type=Path,default=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(a.source)
    assert hashlib.sha256(image.data).hexdigest()==PIN
    specs=[]
    for flags in (0,0x10,0x40,0x50):
        for state in (0,0x400004a,0x4000065):
            specs.append(dict(name=f'seat_{flags:x}_state_{state:x}',entry=0x156f60,flags=flags,state=state,parent=True))
    for identity in ('none','stale','type2','zero_salt','past_highwater'):
        specs.append(dict(name=f'query_{identity}',entry=0x156f60,identity=identity))
    for force in (0,1):
        for mode in (0x23e210,0x23ecb0,0x23ff50):
            for flags in (0,0x10,0x50):
                specs.append(dict(name=f'restore_{force}_{mode:x}_{flags:x}',entry=0x157070,force=force,mode=mode,flags=flags,parent=bool(flags)))
    for mode in (0x23e210,0x23ecb0,0x23ff50):
        for transition in (0,1):
            for duration in (0.,.125):
                specs.append(dict(name=f'set_{mode:x}_{transition}_{duration}',entry=0x157520,mode=mode,transition=transition,duration=duration))
    for view in range(4):
        for dead in (0,1):
            for mode in (0x23e210,0x23ff50):
                specs.append(dict(name=f'select_view{view}_dead{dead}_mode{mode:x}',entry=0x1573b0,view=view,dead=dead,mode=mode))
    for pause,dt,reset in ((1,1/30,0),(0,0.,0),(0,-1.,0),(0,1/30,1)):
        specs.append(dict(name=f'select_pause{pause}_dt{dt}_reset{reset}',entry=0x1573b0,pause=pause,dt=dt,reset=reset,dead=True))
    for round_mode in range(4):
        for binding in ('current','previous','participant','empty','no_local'):
            specs.append(dict(name=f'construct_{binding}_round{round_mode}',entry=0x23fb00,binding=binding,round_mode=round_mode))
    for team in (0,1):
        for watched in range(4):
            specs.append(dict(name=f'replace_team{team}_watched{watched}',entry=0x23fea0,team=team,watched=watched))
    specs.append(dict(name='construct_participant_team_only',entry=0x23fb00,binding='participant',team=1,constructor_team_probe=True))
    specs.append(dict(name='sequence_death_hold_respawn',entry=0x1573b0,sequence=True,dead=True))
    # Campaign uses the same complete lifecycle with a three-second watch.
    for occupied,teams in ((1,[0,1,1,1]),(3,[0,1,1,1]),(5,[0,1,0,1]),
                           (15,[-1,0,1,-1]),(15,[-1,0,1,2])):
        specs.append(dict(name=f'campaign_restrict_{occupied:x}_{"_".join(map(str,teams))}',
                          entry=0x23fca0,occupied=occupied,teams=teams))
    for team in (0,1):
        for binding in ('previous','participant','empty'):
            specs.append(dict(name=f'campaign_construct_{binding}_team{team}',entry=0x23fb00,
                              game_type=1,binding=binding,teams=[0,1,team,1]))
            specs.append(dict(name=f'campaign_replace_{binding}_team{team}',entry=0x23fea0,
                              game_type=1,binding=binding,teams=[0,1,team,1]))
    for flags in (0,8):
        for dead in (False,True):
            specs.append(dict(name=f'campaign_select_flags{flags}_dead{int(dead)}',entry=0x1573b0,
                              game_type=1,player_flags=flags,dead=dead))
    for kind in ('empty','first','last','attached','unlocated','retired','wrong_type'):
        specs.append(dict(name=f'campaign_fallback_{kind}',entry=0x23fe30,fallback=kind))
    rows=[];cases=[]
    for spec in specs:
        name=spec['name'];view=spec.get('view',0);base=0x4f0214+view*0x140;camera=base+0x10
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
        g.mem_map(BASE,SIZE);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000)
        g.mem_write(BASE,bytes(SIZE));g.mem_write(STACK,bytes(0x10000))
        w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
        h=lambda p,v:g.mem_write(p,struct.pack('<H',v&0xffff))
        b=lambda p,v:g.mem_write(p,bytes([v&0xff]))
        fv=lambda p,v:g.mem_write(p,struct.pack('<f',v))
        w(0x4e78d0,POOL);w(POOL+0x24,12);w(POOL+0x38,2);w(POOL+0x44,HEADERS)
        for index,address in enumerate((UNIT,VEHICLE)):
            h(HEADERS+index*12,1);b(HEADERS+index*12+3,index);w(HEADERS+index*12+8,address)
            b(address+0xaa,index);w(address,0x10000+index)
        w(UNIT+0x14,0x10001 if spec.get('parent') else 0xffffffff);h(UNIT+0x1fc,0);h(UNIT+0x12a,0x500)
        w(UNIT+0x500,0x10000);h(UNIT+0x506,0);w(UNIT+0x568,0x10000);w(UNIT+0x57c,spec.get('state',0))
        w(0x4eb114,TAGS);w(TAGS+24,DEF);w(DEF+0x1cc,SEATS);w(SEATS,spec.get('flags',0))
        w(0x4f01f4,PLAYERS);w(PLAYERS+0x24,0x21c);w(PLAYERS+0x38,4);w(PLAYERS+0x44,ROWS);w(PLAYERS+0x48,PLAYERS+0x100);w(PLAYERS+0x100,15)
        for index in range(4):
            row=ROWS+index*0x21c;h(row,index+1);w(row+0x2c,0x10000);w(row+0x30,0x10001);b(row+0xc0,index%2)
        if 'occupied' in spec:w(PLAYERS+0x100,spec['occupied'])
        for index,team in enumerate(spec.get('teams',[])):b(ROWS+index*0x21c+0xc0,team)
        if spec.get('constructor_team_probe'):
            # Own/first live player share a team. Later live players are enemies,
            # so an unfiltered constructor fallback picks the wrong participant.
            for index in range(4):b(ROWS+index*0x21c+0xc0,0 if index<2 else 1)
        own=view;w(ROWS+own*0x21c+0x2c,0xffffffff if spec.get('dead') else 0x10000)
        if 'player_flags' in spec:h(ROWS+own*0x21c+2,spec['player_flags'])
        w(0x4f01f0,USERS)
        for index in range(4):w(USERS+12+index*4,((index+1)<<16)|index)
        w(0x4edf18,GAME);w(GAME+8,spec.get('game_type',2));b(GAME+0x184,spec.get('team',0))
        w(0x4f10b8,ENGINE);w(ENGINE+0xc14,0);w(0x566084,1)
        w(0x518444,CLOCK);fv(CLOCK+12,spec.get('dt',1/30))
        w(0x4f0204,MANAGER);b(MANAGER,spec.get('pause',0))
        w(0x4ee9d8,RNG);w(RNG+4,0x12345678);h(0x46fb64,0)
        g.mem_write(0x4f125c+view*0x358,struct.pack('<3f',3.,-2.,4.));fv(0x5563a4,1.2217305)
        w(0x4e791c,GLOBALS);w(GLOBALS+0xec,CONTROLS);w(0x4e7920,1)
        for offset in (8,12,16):fv(CONTROLS+offset,spec.get('duration',.125))
        g.mem_write(base,bytes([0x73])*0x140);w(base+12,spec.get('mode',0x23e210));h(base+0x58,0)
        w(camera+0x2c,0x10000);w(camera+0x30,((spec.get('watched',0)+1)<<16)|spec.get('watched',0));w(camera+0x34,0x10001)
        binding=spec.get('binding')
        if binding and binding!='current':w(ROWS+0x2c,0xffffffff)
        if binding in ('participant','empty'):w(ROWS+0x30,0xffffffff)
        if binding=='empty':
            for index in range(1,4):w(ROWS+index*0x21c+0x2c,0xffffffff)
        if binding=='no_local':w(USERS+12,0xffffffff)
        fallback=spec.get('fallback')
        if fallback:
            # BBDA0's actual retained header/extent/type/parent/cluster bank.
            for index,address in enumerate((UNIT,VEHICLE)):
                b(HEADERS+index*12+3,6);w(address+0x14,0xffffffff);h(address+0x2c,0)
            if fallback=='empty':w(POOL+0x38,0)
            elif fallback=='first':h(HEADERS+12,0)
            elif fallback=='attached':w(VEHICLE+0x14,0x10000)
            elif fallback=='unlocated':h(VEHICLE+0x2c,-1)
            elif fallback=='retired':h(HEADERS+12,0)
            elif fallback=='wrong_type':b(HEADERS+12+3,0)
        identity=spec.get('identity');unit=0x10000
        if identity=='none':unit=0xffffffff
        if identity=='stale':unit=0x20000
        if identity=='type2':b(HEADERS+3,2)
        if identity=='zero_salt':h(HEADERS,0)
        if identity=='past_highwater':unit=0x10002
        entry=spec['entry'];force=spec.get('force',0);reset=spec.get('reset',0)
        fp=(0x27f+(spec.get('round_mode',0)<<10),0x1f80+(spec.get('round_mode',0)<<13))
        def call(args,eax=0,ecx=0,edx=0,ebx=0,esi=0,edi=0):
            w(SP,STOP)
            for i,v in enumerate(args):w(SP+4+i*4,v)
            for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_EAX,eax),(R.UC_X86_REG_ECX,ecx),
                (R.UC_X86_REG_EDX,edx),(R.UC_X86_REG_EBX,ebx),(R.UC_X86_REG_ESI,esi),(R.UC_X86_REG_EDI,edi),
                (R.UC_X86_REG_FPCW,fp[0]),(R.UC_X86_REG_MXCSR,fp[1]),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)]:g.reg_write(register,value)
        if entry==0x156f60:call([OUT],ebx=unit)
        elif entry==0x157070:call([force],edi=view)
        elif entry==0x157520:call([spec.get('transition',0)],eax=view,edx=spec['mode'])
        elif entry==0x1573b0:call([],eax=reset,ecx=view)
        elif entry==0x23fb00:call([0xffffffff],eax=camera,edx=view)
        elif entry==0x23fea0:call([],esi=camera)
        elif entry==0x23fca0:call([0x10000])
        elif entry==0x23fe30:call([])
        initial={};current={};writes=[];last=0
        def touch(p,n):
            for i,v in enumerate(bytes(g.mem_read(p,n))):
                if p+i not in current:initial[p+i]=v;current[p+i]=v
                else:assert current[p+i]==v,(name,f'{last:08x}',f'{p+i:08x}')
        def on_read(_g,_access,p,n,_x,_data):touch(p,n)
        def on_write(_g,_access,p,n,x,_data):
            touch(p,n);raw=(x&((1<<(n*8))-1)).to_bytes(n,'little')
            if not STACK<=p<STACK+0x10000 and p!=OUT:
                writes.append(dict(instruction=last,address=p,before=bytes(g.mem_read(p,n)).hex(),written=raw.hex()))
            current.update({p+i:v for i,v in enumerate(raw)})
        def on_code(_g,p,_size,_data):
            nonlocal last
            last=p
        g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
        phases=3 if spec.get('sequence') else 1
        mutations=[]
        for phase in range(phases):
            if phase:
                call([],eax=reset,ecx=view)
                # API call-frame setup is outside the measured owner interval.
                for p in range(SP,SP+4):current.pop(p,None)
            if phase==2:
                for p,raw in ((ROWS+0x2c,struct.pack('<I',0x20000)),(HEADERS,struct.pack('<H',2))):
                    g.mem_write(p,raw);mutations.append((phase,p,raw))
                    for i,v in enumerate(raw):
                        if p+i in current:current[p+i]=v
            g.emu_start(entry,STOP,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP,(name,hex(last))
        # Binding reads the complete declared occupied player bank, including
        # fields not needed after the original iterator's first match.
        for p,n in [(0x4f01f4,4),(PLAYERS+0x24,4),(PLAYERS+0x38,4),(PLAYERS+0x44,8),(PLAYERS+0x100,4)]+[(ROWS+i*0x21c+j,n) for i in range(4) for j,n in ((0,2),(0x2c,8),(0xc0,1))]:
            for k,v in enumerate(bytes(g.mem_read(p,n))):initial.setdefault(p+k,v)
        output={p+i:v for s in writes for p,raw in [(s['address'],bytes.fromhex(s['written']))] for i,v in enumerate(raw)}
        mode=int.from_bytes(g.mem_read(OUT,2),'little') if entry==0x156f60 else 0
        result=(g.reg_read(R.UC_X86_REG_EAX)&(0xff if entry==0x23fca0 else 0xffffffff)) if entry in (0x23fca0,0x23fe30) else (g.reg_read(R.UC_X86_REG_EAX)&0xffff if entry==0x156f60 else 0)
        rows.append(f'C {name} {entry:08x} {view&0xffffffff:08x} {camera:08x} {unit:08x} {force:x} {reset:x} {spec.get("mode",0):08x} {spec.get("transition",0):x} {fp[0]:04x} {fp[1]:08x} {mode:x} {result:x} {phases:x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
        for s in writes:rows.append(f"W {s['instruction']:08x} {s['address']:08x} {s['before']} {s['written']}")
        for phase,p,raw in mutations:rows.append(f'P {phase:x} {p:08x} {raw.hex()}')
        cases.append(dict(**spec,stores=len(writes),output_bytes=len(output),query_mode=mode,query_result=result,phases=phases))
        print(name,len(writes),flush=True)
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(tool_sha256=sha(__file__),source_sha256=PIN,fixture_sha256=sha(fixture),cases=cases,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(cases),stores=sum(c['stores'] for c in cases),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
