#!/usr/bin/env python3
"""Complete unchanged retail party-rank collection and gathered rank controls."""
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
OUT=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/".local/party-search-ranks-oracle"
OUT.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(ROOT/'tools'))
from oracle_game_variants import Source,VERSIONS
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_ESI
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

s=Source(VERSIONS['retail']);g=s.g;g.mem_map(0x1100000,0x80000)
SESSION,STATE,OWNER,OUTPUT=0x1100000,0x1120000,0x1130000,0x1140000
HOPPER=0x551ae8+0x614
controls=[]
def w(at,value):g.mem_write(at,struct.pack('<i',value))
def r(at):return struct.unpack('<i',g.mem_read(at,4))[0]
def check(name,actual,expected,**inputs):
    assert actual==expected,(name,actual,expected)
    controls.append(dict(name=name,actual=actual,expected=expected,inputs=inputs))

def setup(ranks,*,flags=None,values=None,properties=None,clan=False,map_state=1):
    s.call(0x66330)
    g.mem_write(SESSION,bytes(0x80000))
    g.mem_write(0x551ae8,bytes(16*0x614));w(0x47d8f4,2)
    g.mem_write(0x47d8f8,struct.pack('<IHH',2,2,0))
    # Original built-in Minor Clan Match or Team Skirmish; 72540 itself does
    # not inspect the hopper type and uses its published selected-hopper ranks.
    s.call(0x193c70,registers=((UC_X86_REG_EAX,4 if clan else 3),(UC_X86_REG_EDX,HOPPER)))
    w(STATE+8,OWNER);w(OWNER+0x30,SESSION);w(STATE+0x978,3);w(STATE+0x18,1)
    w(STATE+0x7b4,len(ranks));w(SESSION+0x1118,len(ranks));w(SESSION+0x111c,(1<<len(ranks))-1)
    w(SESSION+0x54,1);w(SESSION+0x104+4,map_state)
    for at in (STATE+0x958,STATE+0x95c,STATE+0x960):w(at,404)
    for i,rank in enumerate(ranks):
        user=struct.pack('<III',i+1,0,(flags or {}).get(i,0))
        p=SESSION+0x1120+i*0x13c
        g.mem_write(p,user);g.mem_write(STATE+0x7b8+i*12,user)
        w(STATE+0x878+i*4,rank);w(STATE+0x8b8+i*4,(values or {}).get(i,1000))
        selected,raw,xp,adjusted=(properties or {}).get(i,(1,-1,-1,60))
        w(p+0x12c,selected)
        g.mem_write(p+0x130,struct.pack('<hhi',adjusted,raw,xp))
        g.mem_write(p+0x118,struct.pack('<III',100,100,0) if clan else bytes(12))

def collect(ranks,**kw):
    setup(ranks,**kw)
    s.call(0x72540,registers=((UC_X86_REG_ESI,STATE),))
    return dict(mode=r(STATE+0x978),ready=bool(g.mem_read(STATE+0x954,1)[0]),
        maximum=r(STATE+0x958),minimum=r(STATE+0x95c),average=r(STATE+0x960),
        ranks=tuple(r(STATE+0x878+i*4) for i in range(len(ranks))))

for ranks,mean in [((0,1),0),((1,2),1),((2,3,8),4),((1,2,2),1),((49,48),48),((80,100),90),((127,126),126),(tuple(range(112,128)),119)]:
    v=collect(ranks)
    check(f'integer_mean_{ranks}',(v['mode'],v['ready'],v['minimum'],v['maximum'],v['average']),
        (3,True,min(ranks),max(ranks),mean),ranks=ranks)
for flag in (1,2):
    v=collect((10,49,20),flags={1:flag})
    check(f'guest_flag_{flag}_excluded',(v['mode'],v['ready'],v['average'],v['ranks']),
        (3,True,15,(10,-1,20)))
for name,kw in [('missing_raw_rank',{}),('stale_selected_hopper',{'properties':{1:(2,7,1234,60)}}),
    ('missing_property_xp',{'properties':{1:(1,7,-1,60)}})]:
    v=collect((10,-1,20),**kw)
    check(name,(v['mode'],v['ready'],v['average']),(3,False,404))
v=collect((10,7,20),values={1:-1})
check('missing_cached_xp',(v['mode'],v['ready'],v['average']),(3,False,404))
v=collect((10,-1,20),properties={1:(1,7,1234,60)})
check('recover_raw_property_and_ignore_adjusted',(v['mode'],v['ready'],v['average'],v['ranks']),
    (3,True,12,(10,7,20)))
v=collect((-1,-1),flags={0:1,1:2})
check('all_guests_no_rated_players',(v['mode'],v['ready'],v['average']),(1,True,404))
v=collect((1,2),map_state=0)
check('unavailable_map_blocks_average',(v['mode'],v['average']),(6,404))
v=collect((-1,-1,-1),clan=True,properties={i:(1,15,5500,60) for i in range(3)})
check('clan_published_raw_rank_repeated',(r(HOPPER),v['mode'],v['ready'],v['average'],v['ranks']),
    (5,3,True,15,(15,15,15)))
v=collect((15,16,16),clan=True)
check('clan_generic_helper_still_integer_divides',v['average'],15,
    boundary='Intentionally inconsistent synthetic cached clan ranks; no service conflict policy inferred.')

def gathered(ranks,*,synthetic_mapping=False):
    setup(ranks,clan=True)
    if synthetic_mapping:
        g.mem_write(HOPPER+0x51c,bytes(min(i+1,127) for i in range(128)))
    w(SESSION+0x741c,5);w(SESSION+0x49c8,1);w(SESSION+0x4994,8)
    g.mem_write(SESSION+0x49fd,b'\x01')
    summary=SESSION+0x4a00;w(summary,1);w(summary+4,1);w(summary+8,1)
    g.mem_write(summary+0x84,struct.pack('<III',100,100,0));w(summary+0x184,len(ranks))
    for i,rank in enumerate(ranks):
        g.mem_write(summary+0x188+i*12,struct.pack('<III',i+1,0,0))
        w(summary+0x248+i*4,rank);w(summary+0x288+i*4,1000);w(summary+0x2c8+i*4,0)
    valid=bool(s.call(0x7e210,(OUTPUT,),registers=((UC_X86_REG_EAX,SESSION),))&0xff)
    return valid,r(OUTPUT),tuple(r(OUTPUT+0xd4+i*4) for i in range(len(ranks))),\
        tuple(r(OUTPUT+0x154+i*4) for i in range(len(ranks)))

v=gathered((1,2,8),synthetic_mapping=True)
check('gathered_raw_not_adjusted_average',v,(True,3,(1,2,8),(7,7,8)),
    boundary='Synthetic monotone adjustment table, not a historical coefficient claim.')
v=gathered((10,-1,20))
check('gathered_incomplete_nonguest_summary_literal',(v[0],v[1]),(True,9),
    boundary='Intentionally incomplete synthetic summary; 72540 separately prevents searching readiness.')

rows={int(row[0],16):row for row in csv.reader(open(ROOT/'tools/reference/halo2-decompiled/config/functions.csv'))
    if row and all(c in '0123456789abcdefABCDEF' for c in row[0])}
cs=Cs(CS_ARCH_X86,CS_MODE_32)
spans=[]
for address in (0x72540,0x63190,0x7e210,0x71ff0,0x71730,0x65530,0x1931a0):
    row=rows[address];raw=s.image.read(address,int(row[1]))
    (OUT/f'original-{address:08x}.asm').write_text('\n'.join(
        f'{i.address:08x} {i.bytes.hex():22s} {i.mnemonic:8s} {i.op_str}' for i in cs.disasm(raw,address))+'\n')
    spans.append(dict(address=hex(address),size=len(raw),sha256=hashlib.sha256(raw).hexdigest(),source=row[-2],status=row[-1]))
(OUT/'original-function-spans.json').write_text(json.dumps(spans,indent=2)+'\n')
lines=[]
for address in sorted(s.executed):
    i=next(cs.disasm(bytes(g.mem_read(address,15)),address))
    assert i.bytes==s.image.read(address,i.size),hex(address)
    lines.append(f'{address:08x} {i.bytes.hex():22s} {i.mnemonic:8s} {i.op_str}')
trace='\n'.join(lines)+'\n';(OUT/'executed-original-instructions.txt').write_text(trace)
report=dict(xbe_sha256=VERSIONS['retail']['pin'],control_count=len(controls),controls=controls,
    unique_original_instructions=len(lines),trace_sha256=hashlib.sha256(trace.encode()).hexdigest(),
    oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope='Complete original 72540 and 7E210 with synthetic party/cache/summary records; no instructions or callees replaced.')
(OUT/'oracle-party-average.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='controls'},indent=2))
