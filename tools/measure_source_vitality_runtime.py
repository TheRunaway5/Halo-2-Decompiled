#!/usr/bin/env python3
"""Execute unchanged complete vitality owners on declared standalone banks.

Actual Lockout bipd/hlmt/matg metadata is bound without gameplay substitutions.
Explicit object/player/clock/region inputs and metadata field overlays are
reported per case. No external owner is hooked or given a supplied return.
Execution stops before its first instruction with the full real call frame.
This is original instruction evidence, not naturally admitted pickups or ticks.
"""
import argparse,itertools,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN,CACHE_PIN
from inspect_multiplayer import Cache
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_world_bootstrap_runtime_20261011 import sha
from oracle_vehicle_update_20261011 import capture,declared_json
from export_mp_physics_contact_lifecycle import segments
BASE=ROOT/'analysis/damage-shields-goal-20261011/vitality-runtime'
POOL,HEADERS,BODY,OPTIONS,CLOCK,PLAYERS,PLAYER_DATA,HUD,JUGGERNAUT,OUTPUT,SP,STOP=(
    0x1000000+n for n in (0,0x1000,0x2000,0x4000,0x5000,0x6000,0x7000,0x9000,0xa000,0xb000,0x1f000,0x20000))
TAGS=0x1100000;UNIT=0x12340000;PLAYER=0x23450000
GP=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def rows():
    out=[]
    def add(entry=0xd73e0,**kw):out.append(dict(entry=entry,**kw))
    values=[bits(v) for v in (-2.,-0.,0.,.01,.5,1.,2.,2.99,3.,3.1)]+[0x3f7fffff,0x3f800001,0x7f800000,0xff800000,0x7fc12345]
    for value,dead,entity in itertools.product(values,(0,4),(-1,0x34560000)):
        add(0xd7d90,body=value,flags=dead,entity=entity)
    for value,flags,stun in itertools.product(values,(0,4,0xffef),(-32768,-1,0,32767)):
        add(0xd80f0,shield=value,flags=flags,shield_stun=stun)
    for definition,shield,rate in itertools.product((0,1),values,(30,60,120)):
        add(definition=definition,shield=shield,flags=16,tick_rate=rate,maximum_body=0.)
    for shield,rate,player,user in itertools.product((1.00001,1.5,2.99),(30,60),(False,True),(-1,0)):
        add(shield=bits(shield),tick_rate=rate,game_state=2,player=player,user=user,maximum_body=0.)
    for stun,paused,charging,depleted in itertools.product((-32768,-1,0,1,32767),(False,True),(False,True),(False,True)):
        add(shield=bits(.25),shield_stun=stun,mode=4 if paused else 0,
            flags=(512 if charging else 0)|(8 if depleted else 0),type=6,maximum_body=0.)
    for value,stun,paused in itertools.product(values,(-32768,-1,0,1,32767),(False,True)):
        add(body=value,body_stun=stun,mode=4 if paused else 0,maximum_shield=0.,type=6,body_rate=1.,body_max=1.)
    for pending,damage,ticks,decay in itertools.product((0,8,16,0xffff),(0,1,255),(0,1,-1),(0.,.25)):
        add(regions=2,pending=pending,region_damage=damage,region_ticks=ticks,region_decay=decay,
            maximum_body=1.,maximum_shield=0.,body=bits(1.),type=6)
    for flags,mode in itertools.product((4,32,64,1024,8192,2048),(0,4)):
        add(flags=flags,mode=mode,body=bits(.5),shield=bits(.5),type=6)
    for visible,accumulated,timer,flags in itertools.product((0.,.5),(0.,.75),(-128,-1,0,60,127),(0,4,8,2048)):
        add(recent=bits(visible),accumulated=bits(accumulated),recent_ticks=timer,flags=flags,
            maximum_body=0.,maximum_shield=0.)
    for strength,trait,juggernaut in itertools.product((-1,0,1,2,3,4),(0,1,2),(False,True)):
        add(game_state=2,player=True,strength=strength,trait=trait,juggernaut=juggernaut,
            maximum_body=0.,shield=bits(3.),flags=16)
    for body,shield,stun in itertools.product((.5,1.),(.25,1.),(0,1,-1)):
        add(body=bits(body),shield=bits(shield),body_stun=stun,shield_stun=stun,
            game_state=2,flags=512,body_rate=1.)
    for difficulty,selector,present in itertools.product((-32768,-1,0,1,3,4),(0,3),(False,True)):
        add(0x1ed030,difficulty=difficulty,selector=selector,globals_present=present)
    add(no_model=True);add(no_info=True);add(game_state=1,spawn_shield_hold=True,player=True)
    # One source-produced activation/charge/drain history. Only the first row
    # declares its unit state; subsequent rows retain the entire body/HUD bank.
    add(0xd80f0,trajectory='charge_drain',trajectory_tick=0,shield=bits(0.),game_state=2,player=True,user=0)
    for tick in range(1,2801):
        add(trajectory='charge_drain',trajectory_tick=tick,continuation=True,game_state=2,player=True,user=0)
    return out

def execute(path,latest,paired,cache,declared):
    assert sha(path)==(NEW_PIN if latest else PIN)
    image=Xbe(path);code=_Code(image,'.text');selected={int(p,16):q for name in ('address_map','instruction_map','entries') for p,q in paired[name].items()}
    sites={int(p,16):(q if latest else int(p,16)) for p,q in paired['instruction_map'].items()};inverse={q:p for p,q in sites.items()}
    def address(p):return selected.get(p,p) if latest else p
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(POOL,0x210000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    page=cache.magic&~0xfff;g.mem_map(page,(len(cache.data)+cache.magic-page+0xfff)&~0xfff);g.mem_write(cache.magic,bytes(cache.data))
    for datum,tag in cache.tags.items():
        if tag['offset'] is not None:g.mem_write(TAGS+(datum&0xffff)*16+8,struct.pack('<I',cache.magic+tag['offset']))
    definitions=[next(t for t in cache.tags.values() if t['group']=='bipd' and t['name']==name) for name in (
        'objects\\characters\\masterchief\\masterchief_mp','objects\\characters\\elite\\elite_mp')]
    model_tags=[cache.tags[cache.u(t['offset']+0x38)] for t in definitions]
    infos=[cache.block(t['offset']+0x60,0x100)[0] for t in model_tags]
    globals_tag=next(t for t in cache.tags.values() if t['group']=='matg')
    state={};rng=random.Random(0xd73e0)
    def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
    def sh(p,v):g.mem_write(p,struct.pack('<H',v&0xffff))
    def on_code(_g,p,_n,_u):
        if p not in inverse:state['exit_entry']=p;g.emu_stop();return
        state['instructions'].append(inverse[p])
    def on_read(_g,_a,p,n,_x,_u):state['events'].append(dict(kind='R',instruction=inverse[g.reg_read(R.UC_X86_REG_EIP)],address=p,raw=bytes(g.mem_read(p,n)).hex()))
    def on_write(_g,_a,p,n,value,_u):state['events'].append(dict(kind='W',instruction=inverse[g.reg_read(R.UC_X86_REG_EIP)],address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    g.hook_add(UC_HOOK_CODE,on_code);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write)
    cases=[];prior_retained=None
    for index,row in enumerate(declared):
        if row.get('continuation'):
            assert prior_retained is not None
        g.mem_write(POOL,rng.randbytes(0x21000))
        for p,v in ((0x4e78d0,POOL),(0x4eb114,TAGS),(0x4edf18,OPTIONS),(0x518444,CLOCK),
                    (0x4f01f4,PLAYERS),(0x51843c,HUD),(0x51848c,JUGGERNAUT),
                    (0x4e791c,globals_tag['offset']+cache.magic if row.get('globals_present',True) else 0)):
            w(address(p),v)
        g.mem_write(address(0x4fcdbc),bytes((row.get('spawn_shield_hold',False),)))
        w(POOL+0x44,HEADERS);w(POOL+0x24,12);w(POOL+0x38,1)
        g.mem_write(HEADERS,struct.pack('<HBBII',UNIT>>16,3,row.get('type',0),0,BODY))
        for t,info in zip(model_tags,infos):
            g.mem_write(t['offset']+cache.magic,bytes(cache.data[t['offset']:t['offset']+0x68]))
            g.mem_write(info+cache.magic,bytes(cache.data[info:info+0x100]))
            regions=cache.block(info+0xbc,0x38)
            for region in regions:g.mem_write(region+cache.magic,bytes(cache.data[region:region+0x38]))
        definition=definitions[row.get('definition',0)];info=infos[row.get('definition',0)]+cache.magic
        if row.get('no_model'):w(TAGS+(int(definition['datum'],16)&0xffff)*16+8,OUTPUT);w(OUTPUT+0x38,-1)
        else:w(TAGS+(int(definition['datum'],16)&0xffff)*16+8,cache.magic+definition['offset'])
        if row.get('no_info'):w(model_tags[row.get('definition',0)]['offset']+cache.magic+0x60,0)
        overlays=[]
        for field,key in ((0x38,'body_max'),(0xd0,'shield_rate'),(0xd4,'body_rate')):
            if key in row:w(info+field,bits(row[key]));overlays.append(dict(address=info+field,key=key,value=bits(row[key])))
        if 'region_decay' in row:
            for region in cache.block(infos[row.get('definition',0)]+0xbc,0x38):
                w(region+cache.magic+0x2c,bits(row['region_decay']));overlays.append(dict(address=region+cache.magic+0x2c,key='region_decay',value=bits(row['region_decay'])))
        raw=bytearray(rng.randbytes(0x600));struct.pack_into('<I',raw,0,int(definition['datum'],16))
        for off,v in ((0x14,-1),(0xd4,row.get('entity',-1)),(0x12c,-1),(0x13c,PLAYER if row.get('player') else -1),
                (0x248,-1),(0xc4,-1),(0xc8,-1)):
            struct.pack_into('<I',raw,off,v&0xffffffff)
        raw[0xaa]=row.get('type',0);struct.pack_into('<H',raw,0xc2,0xffff);struct.pack_into('<H',raw,0x138,0)
        for off,key,default in ((0xe4,'maximum_body',1.),(0xe8,'maximum_shield',1.)):
            struct.pack_into('<f',raw,off,row.get(key,default))
        for off,key,default in ((0xec,'body',bits(1.)),(0xf0,'shield',bits(1.)),(0xf4,'recent',0),(0xf8,'recent',0),(0xfc,'accumulated',0),(0x100,'accumulated',0)):
            struct.pack_into('<I',raw,off,row.get(key,default))
        struct.pack_into('<hh',raw,0x104,row.get('shield_stun',0),row.get('body_stun',0))
        struct.pack_into('<H',raw,0x10a,row.get('flags',0))
        raw[0x108]=raw[0x109]=row.get('recent_ticks',-1)&255
        struct.pack_into('<hh',raw,0x120,row.get('regions',0)*8,0x500)
        for n in range(row.get('regions',0)):
            struct.pack_into('<BBHHh',raw,0x500+8*n,0,0,row.get('region_damage',0),row.get('pending',0),row.get('region_ticks',0))
        g.mem_write(BODY,bytes(raw));w(OPTIONS+8,row.get('game_state',0));g.mem_write(OPTIONS+12,bytes((row.get('mode',0),)))
        w(OPTIONS+0x180,7 if row.get('juggernaut') else 0);w(OPTIONS+0x1c4,row.get('trait',0));w(OPTIONS+0x22c,0);sh(JUGGERNAUT,1)
        sh(CLOCK+2,row.get('tick_rate',30));w(CLOCK+4,bits(1./row.get('tick_rate',30)))
        w(PLAYERS+0x44,PLAYER_DATA);sh(PLAYER_DATA+0x28,row.get('user',-1));g.mem_write(PLAYER_DATA+0xc1,bytes((row.get('strength',0)&255,)))
        w(HUD,bits(3.))
        if row.get('continuation'):
            for at,raw in prior_retained.items():g.mem_write(at,raw)
        w(SP,STOP);args=[UNIT] if row['entry']==0xd73e0 else [row['selector']] if row['entry']==0x1ed030 else []
        for n,value in enumerate(args):w(SP+4+4*n,value)
        for reg in GP:g.reg_write(reg,rng.getrandbits(32))
        g.reg_write(R.UC_X86_REG_ESP,SP)
        if row['entry'] not in (0xd73e0,0x1ed030):g.reg_write(R.UC_X86_REG_EAX,UNIT)
        if row['entry']==0x1ed030:g.reg_write(R.UC_X86_REG_ECX,row['difficulty']&0xffffffff)
        for n in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),rng.getrandbits(128))
        g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,0x37f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.reg_write(R.UC_X86_REG_FPSW,3<<11);tag=0xffff
        for n in range(3):
            physical=(3+n)&7;tag&=~(3<<(physical*2));g.reg_write(getattr(R,f'UC_X86_REG_FP{physical}'),(0x8000000000000000,0x3fff+n))
        g.reg_write(R.UC_X86_REG_FPTAG,tag)
        retained=[(BODY,0x600),(HUD,0x6c)];before={p:bytes(g.mem_read(p,n)) for p,n in retained}
        state.update(events=[],instructions=[],exit_entry=None);entry_abi=capture(g)
        try:g.emu_start(address(row['entry']),0,count=30000)
        except Exception as error:raise RuntimeError(dict(case=index,input=row,eip=hex(g.reg_read(R.UC_X86_REG_EIP)))) from error
        assert state['exit_entry'] is not None,(index,row,state['instructions'][-12:])
        first={};current={}
        for event in state['events']:
            p=event['address'];raw=bytes.fromhex(event['raw'] if event['kind']=='R' else event['before'])
            for n,value in enumerate(raw):
                if p+n not in current:first[p+n]=current[p+n]=value
                assert current[p+n]==value
            if event['kind']=='W':
                for n,value in enumerate(bytes.fromhex(event['written'])):current[p+n]=value
        for p,raw in before.items():
            after=bytes(g.mem_read(p,len(raw)))
            for n,value in enumerate(raw):
                first.setdefault(p+n,value);assert first[p+n]==value
                current.setdefault(p+n,after[n]);assert current[p+n]==after[n]
        if row.get('trajectory'):
            assert state['exit_entry']==STOP,(index,row,state['exit_entry'])
            prior_retained={p:bytes(g.mem_read(p,n)) for p,n in retained}
        else:prior_retained=None
        complete=state['exit_entry']==STOP;pending=None
        if not complete:
            last=state['instructions'][-1];instruction=code.at(sites[last]);assert instruction.mnemonic in ('call','jmp')
            pending=dict(instruction=last,target=state['exit_entry'],tail=instruction.mnemonic=='jmp',frame_esp=g.reg_read(R.UC_X86_REG_ESP),stack32=bytes(g.mem_read(g.reg_read(R.UC_X86_REG_ESP),32)).hex())
            for n,value in enumerate(bytes.fromhex(pending['stack32'])):
                p=pending['frame_esp']+n
                if p not in current:first[p]=current[p]=value
                assert current[p]==value
        cases.append(dict(id=index,entry=row['entry'],entry_abi=entry_abi,exit=capture(g),exit_entry=state['exit_entry'],return_address=STOP,
            complete=complete,pending=pending,arguments=args,initial_banks=[dict(address=p,hex=b.hex()) for p,b in segments(first)],
            final_banks=[dict(address=p,hex=b.hex()) for p,b in segments(current)],instructions=state['instructions'],
            retained_regions=[dict(address=p,size=n) for p,n in retained],
            writes=[e for e in state['events'] if e['kind']=='W'],declared_input=declared_json(row),metadata_overlays=overlays))
    return cases

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    normalized=BASE/'generated/normalized-correspondence.json';paired=json.loads(normalized.read_text())
    assert paired['authority_sha256']=='e0f135ff479f39a091589fa7393d3a72654b8d91ffe8aa334a4a874c6a17c3d1'
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)==CACHE_PIN
    a.output.mkdir(parents=True,exist_ok=False);stats={}
    for name,latest,path in (('disc',False,ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),('u5',True,NEW_DEFAULT)):
        cases=execute(path,latest,paired,cache,rows());fixture=dict(schema=1,image='1.5' if latest else '1.1',cases=cases,
            tool_sha256=sha(Path(__file__)),correspondence_sha256=sha(normalized),cache_sha256=CACHE_PIN,instruction_patches=[],native_expected_data_used=False)
        p=a.output/f'{name}.json';p.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'),allow_nan=False)+'\n')
        stats[name]=dict(sha256=sha(p),cases=len(cases),complete=sum(c['complete'] for c in cases),instructions=sum(len(c['instructions']) for c in cases),
            writes=sum(len(c['writes']) for c in cases),covered_instructions=len({p for c in cases for p in c['instructions']}),
            stop_entries=sorted({c['exit_entry'] for c in cases if not c['complete']}))
    report=dict(tool_sha256=sha(Path(__file__)),correspondence_sha256=sha(normalized),cache_sha256=CACHE_PIN,fixtures=stats,boundary=__doc__,
        capture_helper_sha256=sha(ROOT/'tools/oracle_vehicle_update_20261011.py'),segments_helper_sha256=sha(ROOT/'tools/export_mp_physics_contact_lifecycle.py'),
        instruction_patches=[],native_expected_data_used=False,naturally_admitted_pickup_or_world_update=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
