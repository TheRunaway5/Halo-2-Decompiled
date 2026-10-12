#!/usr/bin/env python3
"""Execute unchanged direct damage owners on declared standalone banks.

The complete Lockout tag image supplies authored damage, bipd/hlmt and matg
bytes. Object/header/player/actor/options/RNG/engine records are explicitly
declared diagnostic inputs. Controlled metadata overlays are recorded.
No callback is patched or given a supplied return. Unknown callees stop at
their actual first instruction, retaining the complete pending frame/CPU.
These comparisons do not establish natural hits, death or effect attachment.
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
BASE=ROOT/'analysis/damage-shields-goal-20261011/direct-damage-runtime'
POOL,HEADERS,BODY,OPTIONS,RANDOM,PLAYERS,PLAYER_DATA,ENGINE,VTABLE,ACTORS,ACTOR_DATA,RELATION,REQUEST,OUTPUT,SP,STOP=(
    0x1000000+n for n in (0,0x1000,0x2000,0x5000,0x7000,0x8000,0x9000,0xa000,0xb000,0xc000,0xd000,0xf000,0x10000,0x11000,0x1f000,0x20000))
TAGS=0x1100000;UNIT=0x12340000;SOURCE=0x12350001;PARENT=0x12360002;PLAYER=0x23450000
GP=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def rows(tags):
    out=[]
    def add(entry=0xdb160,**kw):out.append(dict(entry=entry,**kw))
    # Every local authored jpt!, including response tags with zero damage.
    for tag,fraction in itertools.product(tags,(0.,.25,1.)):
        add(tag=tag['datum'],fraction=bits(fraction))
    varying=next(t for t in tags if t['name'].endswith('plasma_rifle_bolt'))['datum']
    for seed,cw,rounding in itertools.product((0,1,0x12345678,0xffffffff),(0x7f,0x27f,0x37f),(0,0x400,0x800,0xc00)):
        add(tag=varying,seed=seed,cw=cw|rounding,lower=bits(6.),upper=bits(10.))
    for lower,upper,fraction,scale in itertools.product(
            (0x80000000,bits(-2.),bits(2.),0x7fc12345),(bits(-1.),bits(2.),0x7f800000),(bits(0.),bits(1.),0x7fc12345),(bits(0.),bits(.5))):
        add(tag=varying,lower=lower,upper=upper,fraction=fraction,scale=scale)
    for actor,flag,multiplier,control in itertools.product((False,True),(0,8),(bits(0.),bits(.5),bits(2.),0x7fc00001),(False,True)):
        add(tag=varying,source=True,actor=actor,flags=flag,actor_multiplier=multiplier,controlling_unit=control)
    for source_team,victim_type,protected in itertools.product((0,1,5),(0,1,2),(False,True)):
        add(tag=varying,source=True,game_state=1,source_team=source_team,type=victim_type,protected=protected)
    for team,difficulty,relation in itertools.product((-32768,-1,0,1,15,16),(-1,0,1,3,4),(False,True)):
        add(tag=varying,team=team,game_state=1,difficulty=difficulty,relation=relation)
    for player,attacker,strength,engine,team in itertools.product((False,True),(-1,PLAYER,PLAYER+1),(-1,0,1,2,3,4),(False,True),(0,1)):
        add(tag=varying,player=player,attacker=attacker,strength=strength,engine=engine,player_team=team)
    for entry,engine,trait,flags,timer in itertools.product((0x1615f0,0x253f40,0x2c3d10),(0,2,4),(0,1,2,3,4),(0,0x3084),(-1,1)):
        add(entry,tag=varying,engine_index=engine,trait=trait,variant_flags=flags,king_timer=timer,king_traits=0x1c)
    for engine,left,right in itertools.product((0,2),(0xffff,0,1),(0xffff,0,1)):
        add(0x15f680,tag=varying,engine_index=engine,left=left,right=right)
    for tick in range(32):
        add(tag=varying,trajectory='rng_amounts',trajectory_tick=tick,continuation=tick>0,lower=bits(6.),upper=bits(10.))
    # Complete original dispatcher, including ancestry, exact indices and
    # real source accumulation before the first unimplemented owner.
    for tag,definition,flags in itertools.product(tags,(0,1),(0,4)):
        add(0xd9180,tag=tag['datum'],definition=definition,flags=flags)
    for flags,parents,node,model in itertools.product((0,1,0x200),(0,1,2),(-32768,-1,0,1),(False,True)):
        add(0xd9180,tag=varying,flags=flags,parents=parents,node=node,no_model=model)
    for tag in tags:
        add(0xd9180,tag=tag['datum'],target_none=True)
    for value in (-1,UNIT,SOURCE):
        add(0xd90e0,target=value,parents=1,type=2)
    return out

def execute(path,latest,paired,cache,declared):
    assert sha(path)==(NEW_PIN if latest else PIN)
    image=Xbe(path);code=_Code(image,'.text')
    selected={int(p,16):q for name in ('address_map','instruction_map','entries') for p,q in paired[name].items()}
    sites={int(p,16):(q if latest else int(p,16)) for p,q in paired['instruction_map'].items()};inverse={q:p for p,q in sites.items()}
    def address(p):return selected.get(p,p) if latest else p
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(POOL,0x210000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    registrations=ROOT/'analysis/damage-shields-goal-20261011/amount-boundary/engine-registrations.json'
    assert sha(registrations)=='b110be99112abefb67f26c02fc0759669673527b10571d82d8a623fbae250467'
    registration=json.loads(registrations.read_text())['versions'][int(latest)]
    assert registration['image_sha256']==sha(path)
    g.mem_write(SP,struct.pack('<I',STOP));g.reg_write(R.UC_X86_REG_ESP,SP)
    g.emu_start(int(registration['constructor'],16),STOP,count=64)
    assert bytes(g.mem_read(address(0x566084),40)).hex()==registration['after40_hex']
    page=cache.magic&~0xfff;g.mem_map(page,(len(cache.data)+cache.magic-page+0xfff)&~0xfff);g.mem_write(cache.magic,bytes(cache.data))
    for datum,tag in cache.tags.items():
        if tag['offset'] is not None:g.mem_write(TAGS+(datum&0xffff)*16+8,struct.pack('<I',cache.magic+tag['offset']))
    definitions=[next(t for t in cache.tags.values() if t['group']=='bipd' and t['name']==name) for name in (
        'objects\\characters\\masterchief\\masterchief_mp','objects\\characters\\elite\\elite_mp')]
    globals_tag=next(t for t in cache.tags.values() if t['group']=='matg')
    state={};rng=random.Random(0xd9180)
    def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
    def sh(p,v):g.mem_write(p,struct.pack('<H',v&0xffff))
    def on_code(_g,p,_n,_u):
        if p not in inverse:state['exit_entry']=p;g.emu_stop();return
        state['instructions'].append(inverse[p])
    def on_read(_g,_a,p,n,_x,_u):state['events'].append(dict(kind='R',instruction=inverse[g.reg_read(R.UC_X86_REG_EIP)],address=p,raw=bytes(g.mem_read(p,n)).hex()))
    def on_write(_g,_a,p,n,value,_u):state['events'].append(dict(kind='W',instruction=inverse[g.reg_read(R.UC_X86_REG_EIP)],address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    g.hook_add(UC_HOOK_CODE,on_code);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write)
    cases=[];prior_random=None
    for index,row in enumerate(declared):
        g.mem_write(POOL,rng.randbytes(0x21000));overlays=[]
        for p,v in ((0x4e78d0,POOL),(0x4eb114,TAGS),(0x4edf18,OPTIONS),(0x4ee9d8,RANDOM),(0x4f01f4,PLAYERS),
                    (0x4f10b8,ENGINE),(0x4fcdc8,ACTORS),(0x4fcdc4,RELATION),(0x4e791c,globals_tag['offset']+cache.magic)):
            w(address(p),v)
        w(RANDOM,row.get('seed',0x12345678));w(POOL+0x44,HEADERS);w(POOL+0x24,12);w(POOL+0x38,3)
        if row.get('continuation'):
            assert prior_random is not None;g.mem_write(RANDOM,prior_random)
        definition=definitions[row.get('definition',0)];d=definition['offset']
        g.mem_write(d+cache.magic,bytes(cache.data[d:d+definition['size']]))
        if row.get('no_model'):
            w(d+cache.magic+0x38,-1);overlays.append(dict(address=d+cache.magic+0x38,value=0xffffffff,field='bipd.model'))
        for n,handle in enumerate((UNIT,SOURCE,PARENT)):
            at=BODY+0x1000*n;raw=bytearray(rng.randbytes(0x600))
            struct.pack_into('<I',raw,0,int(definition['datum'],16));raw[0xaa]=row.get('type',0) if n==0 else 0
            for off,value in ((0x14,(SOURCE if n==0 else PARENT) if n<row.get('parents',0) else -1),(0x12c,0x34560000 if row.get('actor') else -1),
                (0x13c,PLAYER if row.get('player') else -1),(0x248,-1),(0x24c,PARENT if row.get('controlling_unit') and n==1 else -1),
                (0xe4,bits(1.)),(0xe8,bits(1.)),(0xec,bits(1.)),(0xf0,bits(1.)),(0x134,0x80000000 if row.get('protected') else 0)):
                struct.pack_into('<I',raw,off,value&0xffffffff)
            struct.pack_into('<H',raw,0x138,row.get('source_team',1) if n else 0);struct.pack_into('<H',raw,0x10a,0)
            g.mem_write(at,bytes(raw));g.mem_write(HEADERS+n*12,struct.pack('<HBBII',handle>>16,3,raw[0xaa],0,at))
        w(OPTIONS+8,row.get('game_state',0));g.mem_write(OPTIONS+12,b'\0');sh(OPTIONS+0x132,row.get('difficulty',1))
        w(OPTIONS+0x184,row.get('variant_flags',0));w(OPTIONS+0x22c,row.get('king_traits',0))
        w(ENGINE+0xc14,row.get('engine_index',2 if row.get('engine') else 0))
        w(PLAYERS+0x44,PLAYER_DATA)
        for n in range(2):
            g.mem_write(PLAYER_DATA+n*0x21c+0xc1,bytes((row.get('strength',0)&255,)))
            g.mem_write(PLAYER_DATA+n*0x21c+0xc0,bytes((row.get('player_team',0) if n else 0,)))
            w(PLAYER_DATA+n*0x21c+0x194,0)
            sh(PLAYER_DATA+n*0x21c+0x1b8,row.get('king_timer',0))
        w(ACTORS+0x44,ACTOR_DATA);w(ACTOR_DATA+0x7c0,row.get('actor_multiplier',bits(1.)))
        g.mem_write(RELATION+0xc4,bytes([255 if row.get('relation') else 0])*0x24)
        tag=cache.tags[int(row.get('tag',declared[0]['tag']),16)];tag_at=tag['offset']+cache.magic
        g.mem_write(tag_at,bytes(cache.data[tag['offset']:tag['offset']+tag['size']]))
        for off,key in ((0x20,'lower'),(0x24,'upper')):
            if key in row:w(tag_at+off,row[key]);overlays.append(dict(address=tag_at+off,field=key,value=row[key]))
        raw=bytearray(rng.randbytes(0x88))
        for off,value in ((0,int(tag['datum'],16)),(4,row.get('flags',0)),(8,row.get('attacker',-1)),(12,SOURCE if row.get('source') else -1),
                         (0x54,row.get('fraction',bits(1.))),(0x58,0),(0x5c,row.get('scale',bits(1.))),(0x68,0x55667701)):
            struct.pack_into('<I',raw,off,value&0xffffffff)
        struct.pack_into('<H',raw,0x10,row.get('team',-1)&0xffff);struct.pack_into('<H',raw,0x7e,0xffff)
        g.mem_write(REQUEST,bytes(raw));w(OUTPUT,0xaabbcc00);w(SP,STOP)
        args=[REQUEST,-1 if row.get('target_none') else UNIT,row.get('node',-1),0xabcdfffe,-1,OUTPUT] if row['entry']==0xd9180 else [OUTPUT] if row['entry']==0xdb160 else [PLAYER,row['trait']] if row['entry'] in (0x1615f0,0x253f40,0x2c3d10) else [0xabcd0000|row['left'],0x34560000|row['right']] if row['entry']==0x15f680 else []
        for n,value in enumerate(args):w(SP+4+4*n,value)
        for reg in GP:g.reg_write(reg,rng.getrandbits(32))
        g.reg_write(R.UC_X86_REG_ESP,SP)
        if row['entry']==0xdb160:
            g.reg_write(R.UC_X86_REG_EAX,REQUEST);g.reg_write(R.UC_X86_REG_ECX,tag_at+0x10);g.reg_write(R.UC_X86_REG_EBX,UNIT)
        elif row['entry']==0xd90e0:g.reg_write(R.UC_X86_REG_EAX,row['target']&0xffffffff)
        for n in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),rng.getrandbits(128))
        g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,row.get('cw',0x37f));g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.reg_write(R.UC_X86_REG_FPSW,3<<11);tagword=0xffff
        for n in range(3):
            physical=(3+n)&7;tagword&=~(3<<(physical*2));g.reg_write(getattr(R,f'UC_X86_REG_FP{physical}'),(0x8000000000000000,0x3fff+n))
        g.reg_write(R.UC_X86_REG_FPTAG,tagword)
        retained=[(BODY+n*0x1000,0x600) for n in range(3)]+[(REQUEST,0x88),(OUTPUT,4),(RANDOM,4)]
        before={p:bytes(g.mem_read(p,n)) for p,n in retained};state.update(events=[],instructions=[],exit_entry=None);entry_abi=capture(g)
        try:g.emu_start(address(row['entry']),0,count=30000)
        except Exception as error:raise RuntimeError(dict(case=index,input=row,eip=hex(g.reg_read(R.UC_X86_REG_EIP)))) from error
        assert state['exit_entry'] is not None,(index,row,state['instructions'][-12:])
        if row.get('trajectory'):
            assert state['exit_entry']==STOP;prior_random=bytes(g.mem_read(RANDOM,4))
        else:prior_random=None
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
            retained_regions=[dict(address=p,size=n) for p,n in ([(RANDOM,4)] if row.get('trajectory') else retained)],writes=[e for e in state['events'] if e['kind']=='W'],declared_input=declared_json(row),metadata_overlays=overlays))
    return cases

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    normalized=BASE/'generated/normalized-correspondence.json';paired=json.loads(normalized.read_text())
    assert paired['authority_sha256']=='705e957d4705d048b6ca23611cdd0fd12c5185e3eccd6e3e8b6fe06f8f7a651d'
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)==CACHE_PIN
    tags=[t for t in cache.tags.values() if t['group']=='jpt!' and t['offset'] is not None]
    declared=rows(tags);a.output.mkdir(parents=True,exist_ok=False);stats={}
    for name,latest,path in (('disc',False,ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),('u5',True,NEW_DEFAULT)):
        cases=execute(path,latest,paired,cache,declared);fixture=dict(schema=1,image='1.5' if latest else '1.1',cases=cases,
            tool_sha256=sha(Path(__file__)),correspondence_sha256=sha(normalized),cache_sha256=CACHE_PIN,instruction_patches=[],native_expected_data_used=False)
        p=a.output/f'{name}.json';p.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'),allow_nan=False)+'\n')
        stats[name]=dict(sha256=sha(p),cases=len(cases),complete=sum(c['complete'] for c in cases),instructions=sum(len(c['instructions']) for c in cases),
            writes=sum(len(c['writes']) for c in cases),covered_instructions=len({p for c in cases for p in c['instructions']}),stop_entries=sorted({c['exit_entry'] for c in cases if not c['complete']}))
    report=dict(tool_sha256=sha(Path(__file__)),correspondence_sha256=sha(normalized),cache_sha256=CACHE_PIN,fixtures=stats,boundary=__doc__,
        engine_registration_sha256='b110be99112abefb67f26c02fc0759669673527b10571d82d8a623fbae250467',
        distinct_quiet_nan_payload_selection_unresolved='Retained original-v4 case394 and nan_operand_probe: Unicorn and host literal ADDSS disagree; this corpus uses one quiet NaN payload',
        authored_local_damage_tags=[dict(datum=t['datum'],name=t['name']) for t in tags],instruction_patches=[],native_expected_data_used=False,naturally_admitted_damage=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(stats))
if __name__=='__main__':main()
