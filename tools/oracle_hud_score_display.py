#!/usr/bin/env python3
"""Unchanged retail/TU1.1 score display and owning HUD conversion instructions.

Actual Lockout matg/mulg color blocks and executable engine registration are
used. Salted player/stat/local-view/clock records are explicit API stimuli.
Current units are NONE and Territories has zero zones: charge, objective-zone
producers, real joining/scoring, graphics and Xbox runtime are not claimed.
The owning13992A score branch stops before162FD0, and the13A050 engine mask
branch stops before its caller-owned output pointers. No result is supplied.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ESI
from oracle_mp_spawn import ROOT, Oracle, VERSIONS, CACHE_PIN, MEM, TAGS, PLAYER_POOL, PLAYERS, BITMAP, ENGINE_GLOBALS, OPTIONS
from inspect_multiplayer_hud import HudCache
from oracle_smg_firing_1_1 import _Code
from functions import _trace
from research_mp_lobby_later_update import complete_tokens, references, table_pair

LOCAL, FRAME, STATUS = MEM + 0x90000, MEM + 0x94000, MEM + 0x95000
SLOTS = (0, 3, 15)
ENTRY = {'base': dict(register=0x378500, query=0x15f3a0, local=0x4e8c20,
                     hud_begin=0x139e57, hud_end=0x13a008, mask_begin=0x13a2b0, mask_end=0x13a39c),
         'tu11': dict(register=0x37eb40, query=0x160a40, local=0x4f01f0,
                     hud_begin=0x13b5c7, hud_end=0x13b778, mask_begin=0x13ba20, mask_end=0x13bb0c)}

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

class ScoreOracle(Oracle):
    def hook(self, g, address, size, user):
        # Observation only: no original instruction or return is replaced.
        self.visited.add(address)

    def __init__(self, version, cache):
        self.visited = set()
        super().__init__(version)
        page = cache.magic & ~4095
        self.g.mem_map(page, ((len(cache.data) + cache.magic - page + 4095) // 4096) * 4096)
        self.g.mem_write(cache.magic, bytes(cache.data))
        for datum, tag in cache.tags.items():
            if tag['offset'] is not None:
                self.w(TAGS + (datum & 65535) * 16 + 8, cache.magic + tag['offset'])
        matg = next(t for t in cache.tags.values() if t['group'] == 'matg')
        self.w(self.a['globals'], cache.magic + matg['offset'])
        self.w(self.a['scenario'], cache.magic + cache.scenario)
        self.w(self.a['tags'], TAGS)
        mulg = cache.tags[cache.u(matg['offset'] + 0x16c)]['offset']
        self.runtime = cache.magic + cache.block(mulg + 8, 0x568)[0]
        self.call(ENTRY[version]['register'])
        self.g.mem_write(OPTIONS + 0x140, 'Source score\0'.encode('utf-16le'))
        for key, target in [('players', PLAYER_POOL), ('options', OPTIONS), ('engine_globals', ENGINE_GLOBALS)]:
            self.w(self.a[key], target)
        self.w(ENTRY[version]['local'], LOCAL)
        self.w(PLAYER_POOL + 0x20, 16); self.w(PLAYER_POOL + 0x24, 0x21c)
        self.w(PLAYER_POOL + 0x38, 16); self.w(PLAYER_POOL + 0x44, PLAYERS); self.w(PLAYER_POOL + 0x48, BITMAP)

    def run_case(self, row):
        self.g.mem_write(PLAYERS, bytes(16 * 0x21c))
        self.g.mem_write(ENGINE_GLOBALS, bytes(0xc18))
        self.g.mem_write(STATUS, bytes(0x274))
        self.g.mem_write(FRAME - 0x100, bytes(0x200))
        self.w(OPTIONS + 8, 2)
        self.w(OPTIONS + 0x180, row['type'])
        self.w(OPTIONS + 0x184, row['teams'])
        self.w(OPTIONS + 0x188, row['round_mode'])
        self.w(OPTIONS + 0x18c, row['limit'])
        self.w(OPTIONS + 0x190, row['time_limit'])
        self.w(ENGINE_GLOBALS + 0xc14, row['type'] if row['engine'] else 0)
        self.sh(ENGINE_GLOBALS + 0xc, row['playing'])
        self.sh(ENGINE_GLOBALS + 0xe0, row['clock'])
        self.w(BITMAP, row['bitmap'])
        self.w(self.a['scenario'], self.scenario if row['scenario'] else 0)
        for slot, member, flags, color, score, wins in row['players']:
            p = PLAYERS + slot * 0x21c
            self.sh(p, 0x7000 + slot); self.sh(p + 2, flags); self.w(p + 0x2c, -1)
            self.g.mem_write(p + 0xc0, bytes([member & 255]))
            self.g.mem_write(p + 0x84, bytes([color & 255, 0, 0, 0]))
            self.sh(ENGINE_GLOBALS + 0x308 + slot * 0x1c, score)
            self.sh(ENGINE_GLOBALS + 0x316 + slot * 0x1c, wins)
        for team, (score, wins) in enumerate(row['team_stats']):
            self.sh(ENGINE_GLOBALS + 0x4c8 + team * 0x12, score)
            self.sh(ENGINE_GLOBALS + 0x4d6 + team * 0x12, wins)
        datum = 0xffffffff if row['local'] == -1 else ((0x7000 + row['local']) << 16) | row['local']
        self.w(LOCAL + 0xc, datum); self.w(FRAME + 0x7c, 0)
        self.call(ENTRY[self.version]['hud_begin'], registers=((UC_X86_REG_EBP, FRAME), (UC_X86_REG_EBX, STATUS), (UC_X86_REG_ESI, self.runtime)), end=ENTRY[self.version]['hud_end'])
        display = bytes(self.g.mem_read(FRAME - 0xac, 0xfc))
        # The query's defined AL is tested by its untouched owning caller.
        ready = bool(self.g.mem_read(STATUS + 0x130, 1)[0])
        status = bytes(self.g.mem_read(STATUS, 0x274))
        self.call(ENTRY[self.version]['mask_begin'], registers=((UC_X86_REG_ESI, STATUS),), end=ENTRY[self.version]['mask_end'])
        mask = self.g.reg_read(UC_X86_REG_EAX) & 65535
        return dict(ready=ready, display_fc_hex=display.hex(), status_274_hex=status.hex(), engine_mask=mask,
                    score_fraction_bits=f'{struct.unpack_from("<I", status, 0x1cc)[0]:08x}',
                    rival_fraction_bits=f'{struct.unpack_from("<I", status, 0x218)[0]:08x}')

def cases():
    rows = []
    patterns = [(0,0,0), (8,8,3), (8,3,8), (-2,-1,-3), (-32768,-32768,-32768), (32767,-32768,0)]
    def add(type_id=2, teams=0, limit=25, scores=(8,8,3), local=0, flags=(1,1,1), members=(0,1,7), bitmap=0x8009, playing=0x83, round_mode=0, engine=1, scenario=1, time_limit=480, clock=91):
        players = [[s,m,f,c,score,w] for s,m,f,c,score,w in zip(SLOTS,members,flags,(10,-1,99),scores,(2,4,-1))]
        ts = [[-9,1] for _ in range(8)]
        for m,score,w in zip((0,1,7),scores,(3,5,-2)):ts[m]=[score,w]
        rows.append(dict(type=type_id, engine=engine, teams=teams, limit=limit, round_mode=round_mode, local=local,
                         bitmap=bitmap, playing=playing, scenario=scenario, time_limit=time_limit, clock=clock,
                         players=players, team_stats=ts))
    for type_id in (1,2,3,4,7,8,9):
        for teams in (0,1):
            for limit in (0,25,32767):
                for scores in patterns:
                    for local in SLOTS:add(type_id,teams,limit,scores,local)
            for round_mode in range(7):
                for local in SLOTS:add(type_id,teams,1,local=local,round_mode=round_mode)
    for teams in (0,1):
        for local in SLOTS:
            for members in ((0,0,1), (0,-1,7), (-1,1,7), (7,1,0)):
                for flags in ((1,1,1), (1,3,1), (3,3,3)):
                    for bitmap in (0x8009,0x8001):add(teams=teams,local=local,members=members,flags=flags,bitmap=bitmap)
            for playing in (0,1,2,3,0x80,0xff):add(teams=teams,local=local,playing=playing,limit=0)
            for scenario in (0,1):
                for time_limit in (0,480):
                    for clock in (-32768,-1,0,32767):add(teams=teams,local=local,scenario=scenario,time_limit=time_limit,clock=clock)
        add(teams=teams,local=-1);add(teams=teams,engine=0)
    return rows

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/game-variants/hud-score-original.json')
    ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/hud_score_original.tsv')
    args=ap.parse_args();cache=HudCache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)==CACHE_PIN
    matg=next(t for t in cache.tags.values() if t['group']=='matg')['offset']
    mulg=cache.tags[cache.u(matg+0x16c)]['offset'];universal=cache.block(mulg,0x20)[0]
    palettes={name:[list(struct.unpack_from('<3I',cache.data,p)) for p in cache.block(at,12)]
              for name,at in [('appearance',matg+0x160),('team',universal+0x10)]}
    inputs=cases();versions={};oracles={};listings={};pairs=[]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    for version in ENTRY:
        o=ScoreOracle(version,cache);o.scenario=cache.magic+cache.scenario;oracles[version]=o
        result=[o.run_case(r) for r in inputs];versions[version]=result
        md=Cs(CS_ARCH_X86,CS_MODE_32)
        path=args.output.with_suffix('.'+version+'.executed.asm')
        path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for a in sorted(o.visited) for i in [next(md.disasm(o.x.read(a,15),a))]))
        listings[str(path.relative_to(ROOT))]=sha(path)
    assert versions['base']==versions['tu11'],'retail/TU HUD result differs'
    # Record full function correspondence, including the query's switch table.
    old,new=[oracles[v] for v in ENTRY];codes=[_Code(o.x,'.text')for o in (old,new)]
    starts=[{int(r['address'],16) for r in csv.DictReader((ROOT/'analysis'/directory/'functions.tsv').open(),delimiter='\t')} for directory in ('retail','map-pack-update')]
    for oe,ne in ((0x15f3a0,0x160a40),(0x13992a,0x13b09a),(0x13a050,0x13b7c0)):
        fs=[_trace(c,e,s|{e})for c,e,s in zip(codes,(oe,ne),starts)]
        ins=[[c.at(a)for a in sorted(f.code)]for c,f in zip(codes,fs)]
        assert complete_tokens(ins[0],old.x)==complete_tokens(ins[1],new.x),(hex(oe),hex(ne))
        correspondence={i.address:j.address for i,j in zip(*ins)}
        for begin,end in [('hud_begin','hud_end'),('mask_begin','mask_end')]:
            for boundary in (begin,end):
                if ENTRY['base'][boundary] in correspondence:
                    assert correspondence[ENTRY['base'][boundary]]==ENTRY['tu11'][boundary]
        tables=table_pair(*fs,*ins,old.x,new.x);assert tables is not None and all(t['topology_equal']for t in tables)
        pairs.append(dict(retail_entry=f'{oe:08x}',tu11_entry=f'{ne:08x}',instructions=len(ins[0]),switch_tables=tables,relocated_operands=references(*ins,old.x,new.x)))
    fallback_data={}
    for name,address in [('appearance',0x468710),('team',0x468734)]:
        pointer=struct.unpack('<I',old.x.read(address,4))[0]
        fallback_data[name]=dict(pointer_address=f'{address:08x}',rgb_address=f'{pointer:08x}',
                                 rgb_bits=list(struct.unpack('<3I',old.x.read(pointer,12))))
    report=dict(scope=__doc__,source_versions=VERSIONS,lockout_sha256=CACHE_PIN,tool_sha256=sha(__file__),
                dependencies={f'tools/{n}.py':sha(ROOT/'tools'/f'{n}.py') for n in ('oracle_mp_spawn','inspect_multiplayer_hud','oracle_smg_firing_1_1','research_mp_lobby_later_update')},
                entries=ENTRY,palettes=palettes,retail_color_fallback_data=fallback_data,listings=listings,complete_pairs=pairs,cases=[dict(input=r,result=v)for r,v in zip(inputs,versions['base'])],
                instruction_patches=[],supplied_returns=[],summary=dict(cases=len(inputs),versions=2,original_returns_supplied=0,ready=sum(r['ready']for r in versions['base'])))
    lines=['# Unchanged retail/TU query and owning score-to-status conversion. Scores/wins are signed shorts.']
    for label,name in [('A','appearance'),('T','team')]:lines.append(' '.join(map(str,[label,len(palettes[name]),*[v for rgb in palettes[name] for v in rgb]])))
    for row,value in zip(inputs,versions['base']):
        fields=[row[k] for k in ('type','engine','teams','limit','round_mode','local','bitmap','playing','scenario','time_limit','clock')]
        fields += [x for p in row['players'] for x in p] + [x for p in row['team_stats'] for x in p]
        fields += [int(value['ready']),value['display_fc_hex'],value['engine_mask'],int(value['score_fraction_bits'],16),int(value['rival_fraction_bits'],16)]
        lines.append(' '.join(map(str,['S',*fields])))
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    report['fixture_sha256']=sha(args.fixture);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['summary']))

if __name__=='__main__':main()
