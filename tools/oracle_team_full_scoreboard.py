#!/usr/bin/env python3
"""Execute the unchanged original Team/FFA roster builder and ranking routines.

Pool, signed stat shorts, masks, options, view rectangles and fade/scroll are
declared API stimuli. They do not establish allocation, score production,
round completion or a full match. Serialized locale readiness stays false;
the only returned service is the existing raw host XGetLanguage code1. Team
labels are separately resolved by original ordinal/index routines against the
actual map's team_names unic, without assigning a locale-loaded flag.
"""
import argparse, csv, json, random, struct
from pathlib import Path
from oracle_mp_full_scoreboard_1_5 import (Trace, HudCache, ROOT, NEW, NEW_PIN,
    PIN, CACHE_PIN, Xbe, _Code, literal, complete_tokens, table_pair, references,
    original_metadata, sha, F, G, M, P, R, B, U, O, L,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESI)
from unicorn.x86_const import UC_X86_REG_XMM0

OUT = ROOT / 'analysis/multiplayer/title-update-1.5/team-full-scoreboard'
FIXTURE = ROOT / 'tests/fixtures/team_full_scoreboard_original.json'
STAT_OFFSETS = (4, 6, 8, 10, 16, 18)
TEAM_OFFSETS = (0x1c4, 0x1c6, 0x1c8, 0x1ca, 0x1d0, 0x1d2)
PAIRS = ((0x161790,0x162b70),(0x161a90,0x162e70),(0x161f80,0x163360),
    (0x241500,0x2486f0),(0x241880,0x248a70),(0x241aa0,0x248c90),
    (0x241c50,0x248e40),(0x241c90,0x248e80),(0x241d30,0x248f20),
    (0x241e00,0x248ff0),(0x13f6f0,0x140a60),(0x15f1e0,0x1605c0),
    (0x1634b0,0x164890),(0x162ac0,0x163ea0))

class TeamTrace(Trace):
    def hook(self,g,a,n,user):
        super().hook(g,a,n,user)
        if a==0x162b70:
            row=self.calls[-1]
            row['color12_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_EAX),12)).hex()
            row['coordinate4_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDI),4)).hex()

def bits(f): return struct.unpack('<I',struct.pack('<f',f))[0]
def string(t, p):
    result=bytearray()
    while t.u(p,1): result.append(t.u(p,1));p+=1
    return result.decode('utf8')
def clean_widget(w, text=True):
    if not w['valid']: return dict(valid=0)
    result={k:w[k] for k in ('valid','rectangle','text_rectangle','right_justified',
        'text_utf16_hex','background_bits','text_color_bits')}
    if not text:result.pop('text_utf16_hex') # emblem payload is an icon short, not a string
    return result
def output(t, rank, call):
    raw=t.output(O);n=raw['team_count'];assert n<=8
    teams=[]
    for i in range(n):
        p=O+0x2d4+i*0x13c
        teams.append(dict(team=t.u(rank+0x40+i*2,2),encoded_rank=t.u(rank+0x60+i,1),
            widgets={name:clean_widget(t.widget(p+offset,cap,valid)) for name,offset,cap,valid
                in (('place',0,6,0x44),('name',0x48,20,0x60),('wins',0xac,6,0x44),('score',0xf4,6,0x44))}))
    players=[]
    deliveries=[r for r in call['calls'] if r['entry']=='00162e70']
    assert len(deliveries)==raw['player_count']
    for i,row in enumerate(raw['players']):
        # MembershipNONE discards the emblem; its embedded datum/icon payload
        # then remains undefined. Read the executed row API's actual datum
        # delivery instead of treating discarded bytes as output identities.
        datum=struct.unpack('<I',bytes.fromhex(deliveries[i]['stack64_hex'])[8:12])[0]
        assert datum==t.u(rank+i*4)
        players.append(dict(full_datum=f'{datum:08x}',encoded_rank=t.u(rank+0x50+i,1),
            current_player=row['current_player'],icon_kind=t.u(O+0xcb8+i*0x1c8+0x80,2) if row['widgets']['icon']['valid'] else None,
            widgets={k:clean_widget(v,k!='icon') for k,v in row['widgets'].items()}))
    return dict(columns=raw['columns'],team_count=n,player_count=raw['player_count'],
        scroll_bits=f'{t.u(0x4f63c8+t.u(R+0x28,2)*4):08x}',title=clean_widget(raw['title']),
        headers={k:clean_widget(v) for k,v in raw['headers'].items()},teams=teams,players=players)

def base(n=4):
    return dict(team_mode=True,game_over=False,extended=False,display_mode=2,
        score_limit=25,round_limit=0,life_limit=0,retained=3,playing=3,
        outer=[0,0,480,640],inner=[24,32,456,608],scroll=0.,alpha=1.,view=0,
        players=[dict(slot=i,membership=i%2,flags=1,current_unit=0xe2700000+i,
            lives=0,stats=[8 if i<2 else 3,0,0,0,0,0]) for i in range(n)],
        teams=[[5,0,0,0,0,0],[7,0,0,0,0,0]]+[[0]*6 for _ in range(6)])
def cases():
    result=[]
    def add(name,x): result.append(dict(name=name,input=x))
    for mode in (2,3,4,8):
        for over in (False,True):
            for extended in (False,True):
                for rounds in (0,3):
                    for limit in (1,25):
                        x=base();x.update(display_mode=mode,game_over=over,extended=extended,
                            round_limit=rounds,score_limit=limit)
                        add(f'columns-{mode}-{over}-{extended}-{rounds}-{limit}',x)
    for n in (1,4,8,9,16):
        for team in (False,True):
            x=base(n);x.update(team_mode=team,retained=255,playing=255)
            for p in x['players']: p.update(membership=p['slot']%8,stats=[0]*6)
            x['teams']=[[0]*6 for _ in range(8)]
            add(f'all-tied-{n}-{team}',x)
    for mask in (0,1,2,3,0x55,0x80,0xff):
        for playing in (0,3,0xff):
            x=base(8);x.update(retained=mask,playing=playing)
            for p in x['players']:p['membership']=p['slot']
            add(f'masks-{mask}-{playing}',x)
    for score in (-32768,-30000,-62,-1,0,1,59,60,32767):
        for over in (False,True):
            x=base();x.update(display_mode=3,game_over=over)
            for p in x['players']:p['stats']=[score]*6
            x['teams']=[[score]*6 for _ in range(8)]
            add(f'signed-time-{score}-{over}',x)
    # Distinct extended statistics with all earlier criteria equal.
    for field in (0,1,2,3,4,5):
        for over in (False,True):
            x=base(16);x.update(game_over=over,extended=True,retained=255,playing=255)
            for p in x['players']:
                p.update(membership=p['slot']%8,stats=[0]*6);p['stats'][field]=p['slot']-8
            x['teams']=[[0]*6 for _ in range(8)]
            for i in range(8):x['teams'][i][field]=i-4
            add(f'criterion-{field}-{over}',x)
    for view in range(4):
        for scroll in (-10.,0.,33.25,10000.):
            x=base(16);x.update(view=view,outer=[(view//2)*240,(view%2)*320,(view//2+1)*240,(view%2+1)*320],
                inner=[(view//2)*240+18,(view%2)*320+18,(view//2+1)*240-18,(view%2+1)*320-18],
                retained=255,playing=255,scroll=scroll,alpha=.5)
            add(f'split-scroll-{view}-{scroll}',x)
    rng=random.Random(0x2486f0)
    for i in range(192):
        n=rng.choice((1,4,8,9,12,16));x=base(n)
        x.update(team_mode=i%4!=0,game_over=bool(rng.randrange(2)),extended=bool(rng.randrange(2)),
            display_mode=rng.choice((2,3,4,8)),score_limit=rng.choice((0,1,25)),
            round_limit=rng.choice((0,1,3)),life_limit=rng.choice((0,1,5)),
            retained=rng.randrange(256),playing=rng.randrange(256),
            alpha=rng.choice((0.,.0625,.5,1.,1.25)),scroll=rng.choice((-3.,0.,7.25,10000.)))
        # Sparse source bitmap enumeration, not contiguous native input order.
        slots=[0]+sorted(rng.sample(range(1,16),n-1))
        for p,slot in zip(x['players'],slots):
            p.update(slot=slot,membership=rng.choice((-1,0,1,2,3,4,5,6,7)),flags=rng.choice((0,1,2,3)),
                current_unit=0xffffffff if rng.randrange(3)==0 else 0xe2700000+slot,
                lives=rng.choice((-1,0,1)),stats=[rng.choice((-32768,-8,-1,0,1,8,32767)) for _ in range(6)])
        x['teams']=[[rng.choice((-32768,-8,-1,0,1,8,32767)) for _ in range(6)] for _ in range(8)]
        add(f'sparse-mixed-{i}',x)
    return result

def configure(t,x):
    for p,n in ((G,0x1200),(M,0xc18),(R,16*0x21c)):t.g.mem_write(p,b'\0'*n)
    t.fixture();t.g.mem_write(R,b'\0'*(16*0x21c))
    t.w(P+0x38,max(p['slot'] for p in x['players'])+1);t.w(B,sum(1<<p['slot'] for p in x['players']))
    t.w(G+0x184,int(x['team_mode'])|int(x['extended'])*16)
    t.g.mem_write(G+0x1128,bytes([x['game_over']]))
    for offset,key in ((0x180,'display_mode'),(0x188,'round_limit'),(0x18c,'score_limit'),(0x1b8,'life_limit')):t.w(G+offset,x[key])
    t.h(M+0xe,x['retained']);t.h(M+0xc,x['playing'])
    t.g.mem_write(0x4c65f0,struct.pack('<4h',*x['outer']));t.g.mem_write(0x4c65f8,struct.pack('<4h',*x['inner']))
    for i in range(4):t.f(0x4f63c8+i*4,x['scroll'])
    for p in x['players']:
        slot=p['slot'];a=R+slot*0x21c;t.h(a,0x8000+slot);p['datum']=f'{((0x8000+slot)<<16)|slot:08x}'
        t.h(a+2,p['flags']);t.w(a+0x24,-1);t.h(a+0x28,x['view'] if slot==0 else -1)
        t.w(a+0x2c,p['current_unit']);t.h(a+0x1ac,p['lives']);t.w(a+0x1b0,-1)
        t.g.mem_write(a+0xc0,bytes([p['membership']&255]));t.g.mem_write(a+0x84,bytes([10,0,0,10]))
        p['name']=f'Player{slot:02}';t.g.mem_write(a+0x44,(p['name']+'\0').encode('utf-16le'))
        for offset,value in zip(STAT_OFFSETS,p['stats']):t.h(M+0x304+slot*0x1c+offset,value)
        r=t.run('original_player_color',0x830c0,(p['membership'] if x['team_mode'] else -1,int(p['membership']==-1)),
            ((UC_X86_REG_EDI,a+0x84),(UC_X86_REG_ESI,F+0x5a000)))
        assert r['completion']=='complete_original_return'
        p['color_bits']=[f'{t.u(F+0x5a000+j*4):08x}' for j in range(3)]
    for i,stats in enumerate(x['teams']):
        for offset,value in zip(TEAM_OFFSETS,stats):t.h(M+0x304+i*0x12+offset,value)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=FIXTURE);args=ap.parse_args()
    assert sha(NEW)==NEW_PIN;assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
    assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
    c=HudCache(ROOT/'assets/disc/maps/lockout.map');t=TeamTrace(c);t.fixture();ctor=t.run('original_engine_constructor',0x3825d0);assert ctor['completion']=='complete_original_return'
    metadata=original_metadata(c);un=metadata['universal_offset'];team_tag=c.tags[c.u(un+12)];start,count=struct.unpack_from('<HH',c.data,team_tag['offset']+16)
    store=metadata['english_file_storage'];t.w(L,c.magic+store['indices']);t.w(L+4,c.magic+store['strings']);t.w(L+8,store['count'])
    label_rows=[];colors=[]
    for i in range(9):
        ordinal=t.run('original_team_label_ordinal',0x1a9910,regs=((UC_X86_REG_ECX,i),));sid=t.g.reg_read(UC_X86_REG_EAX)
        lookup=t.run('original_team_label_index',0x1a9ee0,regs=((UC_X86_REG_EAX,start),(UC_X86_REG_ECX,count),(UC_X86_REG_EDX,L),(UC_X86_REG_EDI,sid)))
        assert ordinal['completion']==lookup['completion']=='complete_original_return'
        label_rows.append(dict(team=i,string_id=f'{sid:08x}',text=string(t,t.g.reg_read(UC_X86_REG_EAX)),ordinal_call=ordinal,lookup_call=lookup))
        color=t.run('original_team_palette',0x83050,(i,),((UC_X86_REG_EAX,F+0x5a000),));assert color['completion']=='complete_original_return'
        colors.append([f'{t.u(F+0x5a000+j*4):08x}' for j in range(3)])
    results=[];rank=F+0x58000
    for case in cases():
        t.calls.clear();t.services.clear();configure(t,case['input'])
        r=t.run('original_rank_list',0x2486f0,(int(case['input']['game_over']),1),((UC_X86_REG_ESI,rank),));assert r['completion']=='complete_original_return',case['name']
        call=t.run('original_complete_roster_builder',0x163360,(0x80000000,O,bits(case['input']['alpha'])));assert call['completion']=='complete_original_return',(case['name'],call['stop'])
        result=output(t,rank,call);assert result['player_count']==len(case['input']['players'])
        case.update(output=result,completion=call['completion'],original_rank_buffer108_hex=bytes(t.g.mem_read(rank,108)).hex())
        results.append(case)
        if len(results)%64==0:print('completed',len(results),'original layouts',flush=True)
    # Original row API receives independently resolved actual tag labels. This
    # does not change the full builder's serialized locale readiness branch.
    named_input=base();named_input.update(retained=255,playing=3,alpha=.5)
    configure(t,named_input);t.calls.clear()
    ranking=t.run('named_row_rank_inputs',0x2486f0,(0,1),((UC_X86_REG_ESI,rank),))
    assert ranking['completion']=='complete_original_return'
    call=t.run('named_row_caller_inputs',0x163360,(0x80000000,O,bits(named_input['alpha'])))
    assert call['completion']=='complete_original_return'
    named_rows=[]
    for i,delivery in enumerate(r for r in call['calls'] if r['entry']=='00162b70'):
        values=list(struct.unpack('<7I',bytes.fromhex(delivery['stack64_hex'])[:28]))[1:]
        team=t.u(rank+0x40+i*2,2);assert team<8
        name=label_rows[team]['text'];t.g.mem_write(F+0x5c000,(name+'\0').encode('utf-16le'))
        t.g.mem_write(F+0x5d000,bytes.fromhex(delivery['color12_hex']))
        t.g.mem_write(F+0x5d020,bytes.fromhex(delivery['coordinate4_hex']))
        values[0]=F+0x5c000;values[5]=F+0x5e000
        row_call=t.run('original_named_team_row',0x162b70,values,
            ((UC_X86_REG_EAX,F+0x5d000),(UC_X86_REG_ECX,O),(UC_X86_REG_EDI,F+0x5d020),
             (UC_X86_REG_XMM0,int(delivery['registers']['xmm0'],16))))
        assert row_call['completion']=='complete_original_return'
        named_rows.append(dict(team=team,widgets={name:clean_widget(t.widget(F+0x5e000+offset,cap,valid))
            for name,offset,cap,valid in (('place',0,6,0x44),('name',0x48,20,0x60),('wins',0xac,6,0x44),('score',0xf4,6,0x44))}))
    old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');oc=_Code(old,'.text');os={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    directory=args.output.parent/'instructions';directory.mkdir(parents=True,exist_ok=True);pairs=[];listings=[]
    for a,b in PAIRS:
        of,oi=literal(oc,a,os);nf,ni=literal(t.code,b,t.starts);assert complete_tokens(oi,old)==complete_tokens(ni,t.image),(hex(a),hex(b))
        tables=table_pair(of,nf,oi,ni,old,t.image);assert tables is not None and all(r['topology_equal'] for r in tables)
        for side,entry,ins in (('tu11',a,oi),('u5',b,ni)):
            p=directory/f'{side}-{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
        constraints=[];mapping=dict(PAIRS)
        for x,y in zip(oi,ni):
            if x.mnemonic=='call' and x.op_str.startswith('0x') and int(x.op_str,16) in mapping:
                assert int(y.op_str,16)==mapping[int(x.op_str,16)]
                constraints.append(dict(old_site=f'{x.address:08x}',later_site=f'{y.address:08x}',old_target=x.op_str,later_target=y.op_str))
        pairs.append(dict(old_entry=f'{a:08x}',later_entry=f'{b:08x}',instructions=len(oi),complete_shape_equal=True,switch_tables=tables,relocated_operands=references(oi,ni,old,t.image),mapped_direct_call_constraints=constraints))
    words={f'{p:08x}':dict(width=n,bytes_hex=t.image.read(p,n).hex()) for p,n in ((0x46a130,4),(0x46a120,4),(0x46a11c,4),(0x46a0cc,4),(0x46a0d0,4),(0x46a1d8,4),(0x456918,4),(0x46a22c,4),(0x46a240,4),(0x445b20,4),(0x46a81c,4),(0x46a378,8),(0x466164,10),(0x466170,16),(0x466180,8))}
    fixture=dict(schema='original-team-full-scoreboard-v1',source_xbe_sha256=NEW_PIN,comparison_xbe_sha256=PIN,lockout_sha256=CACHE_PIN,oracle_sha256=sha(__file__),instruction_patches=[],supplied_game_or_graphics_returns=[],source_data_words=words,team_palette_bits=colors,cases=results,literal_named_rows=dict(input=named_input,rows=named_rows))
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
    report=dict(schema=fixture['schema'],boundaries=__doc__,source_xbe_sha256=NEW_PIN,comparison_xbe_sha256=PIN,lockout_sha256=CACHE_PIN,oracle_sha256=sha(__file__),fixture_sha256=sha(args.fixture),fixture=str(args.fixture.relative_to(ROOT)),original_constructor=ctor,team_names_tag=team_tag,team_names_english_slice=[start,count],original_team_label_lookups=label_rows,team_palette_bits=colors,source_data_words=words,complete_source_pairs=pairs,instruction_listings=listings,original_entry_counts={f'{p:08x}':n for p,n in sorted(t.entry_counts.items())},instruction_patches=[],supplied_game_or_graphics_returns=[],summary=dict(complete_original_layouts=len(results),team_layouts=sum(r['input']['team_mode'] for r in results),ffa_layouts=sum(not r['input']['team_mode'] for r in results),original_team_names=len(label_rows),complete_source_pairs=len(pairs)))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report['summary']))
if __name__=='__main__':main()
