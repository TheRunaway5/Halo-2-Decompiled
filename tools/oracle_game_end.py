#!/usr/bin/env python3
"""Execute original round-end, game-over, final-score and scoreboard owners.

Player pools, stat banks, options, local bindings and clock records are declared
API stimuli. Persistent profile recording is disabled and replicated entities
are NONE. Original event functions execute against the image's cold globals;
no game, graphics, allocator, session or platform return is supplied. This is
not an original world tick, live Xbox session, audio fade or lobby transition.
"""
import argparse, csv, json, random, struct
from pathlib import Path
from oracle_mp_full_scoreboard_1_5 import (Trace, HudCache, ROOT, NEW, NEW_PIN,
    PIN, CACHE_PIN, Xbe, _Code, literal, complete_tokens, table_pair, references,
    sha, G, M, P, R, B, C, UC_X86_REG_EBX, UC_X86_REG_ESI)
from unicorn.x86_const import UC_X86_REG_ESP

OUT = ROOT/'analysis/multiplayer/game-variants/game-end'
FIXTURE = ROOT/'tests/fixtures/game_end_original.json'
PAIRS = ((0x13a130,0x13b4a0),(0x15c9e0,0x15ddc0),
    (0x15e070,0x15f450),(0x15dbc0,0x15efa0),(0x15f370,0x160750),
    (0x1ed140,0x1f1a60),(0x1ed6a0,0x1f1fc0),(0x1ed7b0,0x1f20d0),
    (0x15cfc0,0x15e3a0),(0x15c930,0x15dd10),
    (0x15d160,0x15e540),(0x1642f0,0x1656d0),(0x15f190,0x160570))

class EndTrace(Trace):
    def hook(self,g,a,n,user):
        super().hook(g,a,n,user)
        if a in {b for _,b in PAIRS}-{0x15e540,0x15dd10,0x1656d0,0x160570}:
            sp=g.reg_read(UC_X86_REG_ESP)
            self.calls.append(dict(phase=self.phase,entry=f'{a:08x}',registers=self.reg(),
                stack64_hex=bytes(g.mem_read(sp,64)).hex()))

def bank(t): return bytes(t.g.mem_read(M+0x304,0x254)).hex()
def record(t):
    return dict(game_over=t.u(G+0x1128,1),fade_started=t.u(G+0x1129,1),
        countdown=struct.unpack('<i',t.g.mem_read(G+0x112c,4))[0],
        status=t.u(M+0xc04),lifecycle=t.u(M+0x6c,2),round_index=t.u(M+0x6e,2),
        finalized=t.u(M+0xc08,1),flags=t.u(M),round_countdown=t.u(M+0xc00))
def fixture(t,stats=None,players=None,team=False,mode=0,round_kind=0,index=0):
    t.fixture();t.g.mem_write(G+0x1128,bytes(8));t.g.mem_write(M,bytes(0xc18))
    t.w(M+0xc14,2);t.w(M+0xc04,1);t.h(M+0x6c,1);t.h(M+0x6e,index)
    t.w(M+0x24,-1);t.w(M+0x28,-1);t.w(G+8,2);t.g.mem_write(G+0xc,bytes([mode]))
    t.w(G+0x184,int(team));t.w(G+0x188,round_kind);t.w(G+0x1b8,0)
    t.g.mem_write(0x51dfa0,bytes(0x12));t.g.mem_write(R,bytes(16*0x21c))
    players=players if players is not None else [dict(slot=i,datum=0x80000000+i,membership=i//2,flags=1)for i in range(4)]
    mask=0
    for p in players:
        slot=p['slot'];at=R+slot*0x21c;mask|=1<<slot
        t.h(at,p['datum']>>16);t.h(at+2,p['flags']);t.g.mem_write(at+0xc0,bytes([p['membership']&255]))
        t.h(at+0x28,-1);t.w(at+0x24,-1);t.w(at+0x2c,0xe2700000+slot);t.w(at+0x1b0,-1)
    # Pool+38 is the allocated index high-water mark, not bitmap population.
    t.w(B,mask);t.w(P+0x38,max((p['slot']+1 for p in players),default=0));t.w(C+8,101)
    t.g.mem_write(M+0x304,bytes(stats) if stats is not None else bytes(0x254))
    return players
def run(t,name,e,args=(),regs=(),**kw):
    out=t.run(name,e,args,regs,**kw)
    assert out['completion']=='complete_original_return',out
    assert not out['services'],out
    return out
def random_stats(rng):
    b=bytearray(rng.randbytes(0x254))
    # Header is independent of the stat shorts; arbitrary literal data stays.
    return b
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');a=ap.parse_args()
    a.output.parent.mkdir(parents=True,exist_ok=True)
    assert sha(NEW)==NEW_PIN and sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
    assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
    t=EndTrace(HudCache(ROOT/'assets/disc/maps/lockout.map'));t.fixture()
    ctor=run(t,'original_engine_registration',0x3825d0)
    t.watch=[(M,0xc18),(G+0x1128,8),(0x4f63c0,24)]
    rng=random.Random(0x15ddc0);ends=[];carries=[];held=[];traces=[]
    for team in (False,True):
        winners=[0xffffffff,0,7] if team else [0xffffffff,0x80000000,0x8a31000f,0x11110003]
        players=[dict(slot=i,datum=(0x80000000+i)if i!=15 else 0x8a31000f,
            membership=(7 if i in(0,3,15)else 0),flags=(0x4002 if i==3 else 1))for i in(0,3,15)]
        for winner in winners:
            for win in (-32768,-30001,-1,0,1,2,3,29999,30000,32767):
                stats=random_stats(rng)
                for i in range(16):struct.pack_into('<h',stats,0x12+i*0x1c,win)
                for i in range(8):struct.pack_into('<h',stats,0x1d2+i*0x12,win)
                fixture(t,stats,players,team);before=bank(t)
                name=f'end-{team}-{winner:08x}-{win}'
                end=run(t,name,0x15ddc0,(0,),((UC_X86_REG_EBX,winner),))
                after_end=bank(t);end_record=record(t)
                final=run(t,name+'-finalize',0x15f450);after_final=bank(t);final_record=record(t)
                repeat=run(t,name+'-repeat-finalize',0x15f450)
                assert bank(t)==after_final and record(t)==final_record
                row=dict(name=name,team=team,winner=f'{winner:08x}',players=players,
                    before_bank=before,after_award_bank=after_end,after_final_bank=after_final,
                    end_record=end_record,final_record=final_record)
                ends.append(row);traces.append(dict(name=name,end=end,finalize=final,repeat=repeat))
    # Full bank carry includes disconnected slots and unretained teams.
    for over in (False,True):
        for n in range(64):
            stats=random_stats(rng)
            if n<12:
                values=(-32768,-30000,-1,0,1,30000,32767)
                for i in range(16):
                    struct.pack_into('<hh',stats,4+i*28,values[(i+n)%7],values[(i+2*n)%7])
                for i in range(8):struct.pack_into('<hh',stats,0x1c4+i*18,values[(i+n)%7],values[(i+2*n)%7])
            fixture(t,stats);t.g.mem_write(G+0x1128,bytes([int(over)]))
            before=bank(t);call=run(t,f'carry-{over}-{n}',0x1f1a60,(M+0x304,))
            carries.append(dict(game_over=over,before_bank=before,after_bank=bank(t)))
            traces.append(call)
    # The whole end owner also establishes exact round-selection/status branches.
    rounds=[]
    for kind in range(8):
        for index in (-1,0,1,3,5,30,31):
            for force in (False,True):
                for wins in (0,1,2,3):
                    for winner in (0x80000000,0xffffffff):
                        stats=bytearray(0x254);struct.pack_into('<h',stats,0x12,wins)
                        fixture(t,stats,round_kind=kind,index=index)
                        call=run(t,f'round-{kind}-{index}-{force}-{wins}-{winner:08x}',0x15ddc0,(int(force),),((UC_X86_REG_EBX,winner),))
                        rounds.append(dict(kind=kind,index=index,force=force,winner=f'{winner:08x}',
                            winner_wins_after=t.u(M+0x316,2),record=record(t)))
                        traces.append(call)
    for mode in (0,1,2,3,4,5):
        fixture(t,mode=mode);before=bank(t)
        call=run(t,f'mode-{mode}',0x15ddc0,(0,),((UC_X86_REG_EBX,0x80000000),))
        final=t.run(f'mode-{mode}-finalize',0x15f450)
        if mode==5:
            assert final['completion']=='stopped_original_instruction' and final['stop']['eip']=='000b8061'
        else:assert final['completion']=='complete_original_return' and not final['services']
        rounds.append(dict(mode=mode,kind=0,index=0,force=False,winner='80000000',
            before_bank=before,after_bank=bank(t),record=record(t),finalization_completion=final['completion']))
        traces.extend((call,final))
    # Automatic display follows actual original end+finalizer, without held input.
    for views in range(1,5):
        fixture(t);run(t,'held-end',0x15ddc0,(0,),((UC_X86_REG_EBX,0x80000000),));run(t,'held-finalize',0x15f450)
        t.g.mem_write(0x4f63c0,bytes(24));t.g.mem_write(0x4f2a89,b'\0')
        for i in range(views):
            at=R+i*0x21c;t.h(at+0x28,i);t.w(at+0x24,i);t.w(0x4f2a9c+i*4,1)
            t.g.mem_write(0x4f2aac+i*0x48,bytes(0x48))
        sequence=[]
        for tick in range(40):
            t.w(C+8,101+tick)
            for i in range(views):
                call=run(t,f'auto-{views}-{tick}-{i}',0x15e6bc,(0x80000000+i,),
                    ((UC_X86_REG_ESI,R+i*0x21c),(UC_X86_REG_EBX,0xffffffff)),tail=True)
                sequence.append(dict(tick=101+tick,view=i,counters=list(t.g.mem_read(0x4f63c0,4)),inactive=t.u(0x4f63c4,1)))
                traces.append(call)
        held.append(dict(views=views,sequence=sequence))
    old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');oc=_Code(old,'.text')
    os={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    listingdir=a.output.parent/'instructions';listingdir.mkdir(exist_ok=True);listings=[];pairs=[];mapping=dict(PAIRS)
    def save(code,image,e,starts,side):
        f,ins=literal(code,e,starts);p=listingdir/f'{side}-{e:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
        listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
        return f,ins
    for oe,ne in PAIRS:
        of,oi=save(oc,old,oe,os,'tu11');nf,ni=save(t.code,t.image,ne,t.starts,'u5')
        ot,nt=complete_tokens(oi,old),complete_tokens(ni,t.image)
        differences=[dict(index=i,old=oi[i].op_str,later=ni[i].op_str)for i in range(len(ot))if ot[i]!=nt[i]]
        if oe==0x15c9e0:
            assert differences==[dict(index=82,old='esi, 0x5193f0',later='esi, 0x51ef00')]
        else:assert ot==nt,(hex(oe),hex(ne),differences)
        tables=table_pair(of,nf,oi,ni,old,t.image);assert tables is not None and all(r['topology_equal']for r in tables)
        for x,y in zip(oi,ni):
            if x.mnemonic=='call' and x.op_str.startswith('0x') and int(x.op_str,16)in mapping:
                assert int(y.op_str,16)==mapping[int(x.op_str,16)]
        pairs.append(dict(old=f'{oe:08x}',later=f'{ne:08x}',instructions=len(oi),
            complete_shape_equal_except_declared_profile_base=not differences or oe==0x15c9e0,
            explicit_literal_differences=differences,switch_tables=tables,
            relocated_operands=references(oi,ni,old,t.image)))
    for e in sorted(t.entries-{b for _,b in PAIRS}):save(t.code,t.image,e,t.starts,'u5')
    data=dict(schema='original-game-end-v1',source_xbe_sha256=NEW_PIN,
        comparison_xbe_sha256=PIN,lockout_sha256=CACHE_PIN,oracle_sha256=sha(__file__),
        boundaries=__doc__,instruction_patches=[],supplied_game_or_platform_returns=[],
        endings=ends,carries=carries,rounds=rounds,automatic_scoreboard=held)
    FIXTURE.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
    report=dict(**data,fixture_sha256=sha(FIXTURE),constructor=ctor,traces=traces,
        source_xbe_path=str(NEW),comparison_xbe_path=str(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
        dependency_sha256={p:sha(ROOT/p)for p in('tools/oracle_mp_full_scoreboard_1_5.py',
            'tools/oracle_smg_firing_1_1.py','tools/inspect_multiplayer_hud.py',
            'tools/research_mp_lobby_later_update.py','tools/reference/halo2-decompiled/tools/functions.py',
            'tools/reference/halo2-decompiled/tools/xbe.py')},
        complete_source_pairs=pairs,instruction_listings=listings,
        source_end_seconds_word=t.image.read(0x46a2b8,4).hex(),
        source_audio_fade_threshold_word=t.image.read(0x46a11c,4).hex(),
        summary=dict(endings=len(ends),full_bank_carries=len(carries),round_cases=len(rounds),
            automatic_scoreboard_calls=sum(len(x['sequence'])for x in held),
            complete_source_pairs=len(pairs),supplied_returns=0,instruction_patches=0))
    a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report['summary']))
if __name__=='__main__':main()
