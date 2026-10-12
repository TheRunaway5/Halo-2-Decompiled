#!/usr/bin/env python3
"""Execute unchanged retail clan admission, search filtering and XP dispatch.

Synthetic fixtures supply session and result memory. Original 66330 supplies
network/rank configuration and 193C70 supplies launch fallback clan hoppers.
No instruction or called function is replaced. No Live service is emulated.
"""
import csv
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from oracle_game_variants import Source, VERSIONS
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from oracle_game_variants import STACK, STOP
from unicorn.x86_const import *

OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / '.local/clan-matchmaking-oracle'
OUT.mkdir(parents=True, exist_ok=True)
s = Source(VERSIONS['retail'])
g = s.g
g.mem_map(0x1100000, 0x80000)
SESSION, MATCH, BATCH = 0x1100000, 0x1120000, 0x1140000
SEARCH, ENTRY, OUTPUT = 0x1160000, 0x1161000, 0x1162000
HOPPER = 0x551ae8 + 0x614
CLAN_A = bytes.fromhex('111213141516171800000000')
CLAN_B = bytes.fromhex('212223242526272800000000')
PLAYER_A = bytes.fromhex('313233343536373800000000')
PLAYER_B = bytes.fromhex('414243444546474800000000')
checks = []

def put(at, fmt, *values):
    g.mem_write(at, struct.pack('<' + fmt, *values))

def read(at, fmt):
    return struct.unpack('<' + fmt, g.mem_read(at, struct.calcsize('<' + fmt)))

def check(name, actual, expected, **context):
    assert actual == expected, (name, actual, expected)
    checks.append(dict(name=name, actual=actual, expected=expected, **context))

def build_hopper(preset=4):
    g.mem_write(0x551ae8, bytes(16 * 0x614))
    put(0x47d8f8, 'IHH', 2, 2, 0)
    s.call(0x193c70, registers=[(UC_X86_REG_EAX, preset), (UC_X86_REG_EDX, HOPPER)])
    assert s.call(0x192e60, registers=[(UC_X86_REG_EAX, 1)]) == HOPPER

def session(count=3, identities=None, flags=None, map_state=1, capacity=16):
    g.mem_write(SESSION, bytes(0x8000))
    put(SESSION + 0x40, 'i', 0)
    put(SESSION + 0x54, 'i', 1)
    # One synthetic party machine. Its original capacity lookup uses these
    # measurements; these are not claimed to be captured network conditions.
    put(SESSION + 0x58 + 0x90, 'iiii', 0x100000, 0x100000 if capacity == 16 else 0, 0, 1)
    put(SESSION + 0x58 + 0xac + 4, 'i', map_state)
    put(SESSION + 0x1118, 'iI', count, (1 << count) - 1)
    for i in range(count):
        p = SESSION + 0x1120 + i * 0x13c
        g.mem_write(p, bytes([0x51 + i]) * 8)
        put(p + 8, 'I', (flags or {}).get(i, 0))
        g.mem_write(p + 0xa8 + 0x70, (identities or {}).get(i, CLAN_A))

def admit():
    return s.call(0x66050, registers=[(UC_X86_REG_EAX, SESSION), (UC_X86_REG_ECX, 1)])

s.call(0x66330)
defaults = []
for preset, expected in [(4, (5, 0, 25, 2, 3, 4, 0)), (5, (5, 0, 26, 2, 6, 8, 2))]:
    build_hopper(preset)
    values = (*read(HOPPER, 'iii'), *read(HOPPER + 0x5ec, 'iiii'))
    check('original built-in clan hopper ' + str(preset), values, expected)
    defaults.append(dict(preset=preset, hopper_type=values[0], statistics_key=values[2],
                         teams=values[3], minimum_party=values[4], maximum_party=values[5], imbalance=values[6],
                         blob_sha256=hashlib.sha256(bytes(g.mem_read(HOPPER, 0x614))).hexdigest()))
    for count in range(expected[4] - 1, expected[5] + 2):
        session(count)
        check(f'preset {preset} party {count}', admit(), 6 if count < expected[4] else 7 if count > expected[5] else 0)

build_hopper()
for name, identities, flags, state, expected in [
    ('same clan all active players', {}, {}, 1, 0),
    ('one active player without clan', {2: bytes(12)}, {}, 1, 8),
    ('one active player from different clan', {2: CLAN_B}, {}, 1, 9),
    ('all active players without clan', {i: bytes(12) for i in range(3)}, {}, 1, 8),
    ('guest member flag 1', {}, {2: 1}, 1, 12),
    ('guest member flag 2', {}, {2: 2}, 1, 12),
    ('offline identity sentinel', {}, {2: 0xbad00000}, 1, 13),
    ('map state unavailable', {}, {}, 0, 14),
    ('map state 2', {}, {}, 2, 11),
    ('map state 3', {}, {}, 3, 2),
]:
    session(3, identities, flags, state)
    check(name, admit(), expected)
build_hopper(5)
session(6, capacity=0)
check('insufficient selected host capacity', admit(), 10)
build_hopper()
session(3)
put(HOPPER + 0x5e4, 'i', 1)
check('configured eligibility property threshold', admit(), 15)
put(HOPPER + 0x5e4, 'i', 0)
session(3)
# Put an unlike clan in an inactive slot: no mask bit means it is ignored.
g.mem_write(SESSION + 0x1120 + 4 * 0x13c + 0xa8 + 0x70, CLAN_B)
check('inactive unlike clan ignored', admit(), 0)

# Different role metadata in controller social blocks has no effect on this
# gate; the source gate reads only each active player's 12-byte clan identity.
for role in range(4):
    session(3)
    for controller in range(4):
        put(0x54e8e0 + controller * 0xc70 + 0xb82 + 0x1c, 'i', role)
    check('social role does not restrict clan admission ' + str(role), admit(), 0)

# The generic type-5 validator permits >2 teams; the original launch presets
# above are the evidence for exactly two historical clan teams.
put(HOPPER + 0x5ec, 'iii', 3, 3, 4)
check('synthetic three-team type5 passes generic structural validator',
      s.call(0x1936a0, registers=[(UC_X86_REG_EAX, HOPPER)]) & 255, 1)
build_hopper()

def rating_summary(party_counts, identities=None, ranks=None):
    g.mem_write(SESSION, bytes(0x8000))
    put(SESSION + 0x741c, 'i', 5)
    put(SESSION + 0x49c8, 'i', 1)
    put(SESSION + 0x4994, 'i', read(HOPPER + 0x5ec, 'i')[0] * read(HOPPER + 0x5f4, 'i')[0])
    put(SESSION + 0x49fd, 'B', 1)
    summary = SESSION + 0x4a00
    put(summary, 'i', len(party_counts)); put(summary + 0x184, 'i', sum(party_counts))
    player = 0
    for i, count in enumerate(party_counts):
        put(summary + 4 + i*8, 'ii', i + 1, i + 1)
        g.mem_write(summary + 0x84 + i*12, (identities or {}).get(i, CLAN_A if i == 0 else CLAN_B))
        for _ in range(count):
            g.mem_write(summary + 0x188 + player*12, bytes([0x61+player])*8+bytes(4))
            put(summary + 0x248 + player*4, 'i', (ranks or {}).get(i, 15))
            put(summary + 0x288 + player*4, 'i', 5500)
            put(summary + 0x2c8 + player*4, 'i', i)
            player += 1
    g.mem_write(OUTPUT, bytes(0x364))
    valid = bool(s.call(0x7e210, (OUTPUT,), [(UC_X86_REG_EAX, SESSION)]) & 255)
    return dict(valid=valid, average=read(OUTPUT,'i')[0], group_count=read(OUTPUT+0x1d4,'i')[0],
                imbalance=read(OUTPUT+0x358,'i')[0], add_mask=read(OUTPUT+0x35c,'I')[0],
                preferred_group_mask=read(OUTPUT+0x360,'I')[0])

gather_controls = []
for preset, counts, expected in [(4,[3],True),(4,[3,3],True),(4,[4,4],True),
                                (4,[3,4],False),(4,[2,4],True),
                                (5,[6],True),(5,[6,8],True),(5,[7,8],True)]:
    build_hopper(preset)
    result = rating_summary(counts)
    check(f'original gathering group counts preset={preset} counts={counts}', result['valid'], expected)
    gather_controls.append(dict(preset=preset, counts=counts, **result))
build_hopper()
result=rating_summary([3,3], identities={1:CLAN_A})
check('rating summary alone does not reject duplicate clan identity', result['valid'], True)
result=rating_summary([3,3], identities={1:bytes(12)})
check('rating summary rejects empty group clan identity', result['valid'], False)

# Execute the original full host queue processor up to its real join evaluator
# or rejection-message call. Both are observation boundaries, not stubs: no
# original bytes/calls are replaced, and no final accepted reservation is claimed.
CLIENT, REQUEST = 0x1170000, 0x1171000
host_controls=[]
def host_stop(g, address, size, unused):
    if address in (STOP, 0x62fe0, 0x630f0):
        g.emu_stop()
hook=g.hook_add(UC_HOOK_CODE, host_stop)
for preset, own_count, request_count, identity, expected in [
    (4,3,3,CLAN_B,0), (4,3,3,CLAN_A,15), (4,3,3,bytes(12),14),
    (4,3,2,CLAN_B,4), (4,3,4,CLAN_B,4),
    (5,6,8,CLAN_B,0), (5,6,5,CLAN_B,4), (5,6,9,CLAN_B,3),
]:
    build_hopper(preset); rating_summary([own_count])
    put(SESSION+0x1118,'i',own_count)
    put(SESSION+0x148,'ii',1,1)
    g.mem_write(CLIENT,bytes(0x100)); g.mem_write(REQUEST,bytes(0x200))
    put(CLIENT+4,'Iiii',SESSION,2,REQUEST,1)
    put(REQUEST+0x164,'B',1); put(REQUEST+0x16c,'i',2)
    put(REQUEST+0x18,'i',request_count); g.mem_write(REQUEST+0x18c,identity)
    put(0x510548,'B',1); put(0x51054c,'i',0)
    put(STACK,'II',STOP,CLIENT); g.reg_write(UC_X86_REG_ESP,STACK)
    g.reg_write(UC_X86_REG_FPCW,0x027f); g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    g.emu_start(0x6d4c0,0,count=200000)
    boundary=g.reg_read(UC_X86_REG_EIP)
    assert boundary in (0x62fe0,0x630f0), hex(boundary)
    reason=read(g.reg_read(UC_X86_REG_ESP)+8,'i')[0] if boundary==0x630f0 else 0
    check(f'host clan gate preset={preset} host={own_count} request={request_count} empty={identity==bytes(12)} duplicate={identity==CLAN_A}',reason,expected)
    host_controls.append(dict(preset=preset,own_count=own_count,request_count=request_count,
                              empty_clan=identity==bytes(12),duplicate_clan=identity==CLAN_A,
                              rejection_reason=reason,boundary=hex(boundary)))
g.hook_del(hook)
build_hopper()

assignment_controls=[]
PARTIES, WEIGHTS, ASSIGNMENTS=0x1163000,0x1163100,0x1163200
for preset,counts in [(4,[3,3]),(4,[4,4]),(5,[6,8])]:
    build_hopper(preset)
    groups=[i for i,n in enumerate(counts) for _ in range(n)]
    weights=[127 if i%2 else 0 for i in range(len(groups))]
    put(PARTIES,str(len(groups))+'i',*groups);put(WEIGHTS,str(len(groups))+'i',*weights)
    g.mem_write(ASSIGNMENTS,bytes(64))
    valid=bool(s.call(0x91fc0,(2,read(HOPPER+0x5f4,'i')[0],True,
        read(HOPPER+0x5f0,'i')[0],read(HOPPER+0x5f4,'i')[0],True,len(groups),
        PARTIES,WEIGHTS,True,ASSIGNMENTS)) & 255)
    assignment=list(read(ASSIGNMENTS,str(len(groups))+'i'))
    first=set(assignment[:counts[0]]);second=set(assignment[counts[0]:])
    check(f'original optimized clan assignments preserve two whole parties preset={preset} counts={counts}',
          valid and len(first)==1 and len(second)==1 and first!=second,True)
    assignment_controls.append(dict(preset=preset,counts=counts,weights=weights,assignments=assignment))
build_hopper()

# Observe the complete unchanged team-snapshot loop within 195A40. Starting
# at 195BEC excludes that initializer's timer/platform/session setup; stopping
# at 195CCB excludes later transport bookkeeping. There are no calls inside
# this observed original block, and no executable bytes are patched.
snapshot_controls=[]
def snapshot_stop(g,address,size,unused):
    if address==0x195ccb:
        g.emu_stop()
hook=g.hook_add(UC_HOOK_CODE,snapshot_stop)
for clan_flag, empty_first in [(True,False),(False,False),(True,True)]:
    g.mem_write(0x511034,bytes(16*0xa4)); g.mem_write(0x511a74,bytes(16*24))
    put(0x510dda,'B',int(clan_flag))
    for i,(team,rank,xp,identity) in enumerate([(0,15,5500,bytes(12) if empty_first else CLAN_A),
                                              (0,27,16000,CLAN_A),(1,16,6000,CLAN_B)]):
        a=0x511034+i*0xa4
        put(a,'B',1); put(a+0x8c,'b',team); g.mem_write(a+0x80,identity)
        put(a+0x98,'hhi',60,rank,xp)
    g.reg_write(UC_X86_REG_ESP,STACK)
    g.emu_start(0x195bec,0,count=200000)
    assert g.reg_read(UC_X86_REG_EIP)==0x195ccb
    entries=[]
    for i in range(2):
        a=0x511a74+i*24
        entries.append(dict(active=bool(read(a,'B')[0]),standing=read(a+1,'b')[0],
                            previous_raw_rank=read(a+4,'h')[0],xp=read(a+8,'i')[0],
                            synthetic_clan_identity=bytes(g.mem_read(a+12,12)).hex()))
    expected_rank=15 if clan_flag and not empty_first else -1
    expected_xp=5500 if clan_flag and not empty_first else -1
    expected_clan=CLAN_A.hex() if clan_flag and not empty_first else bytes(12).hex()
    check(f'clan team snapshot raw rank clan={clan_flag} empty_first={empty_first}',
          (entries[0]['previous_raw_rank'],entries[0]['xp'],entries[0]['synthetic_clan_identity']),
          (expected_rank,expected_xp,expected_clan))
    snapshot_controls.append(dict(clan_flag=clan_flag,empty_first=empty_first,
                                  first_member_adjusted_skill=60,second_member_raw_rank=27,
                                  entries=entries,boundary='0x195BEC through 0x195CCB'))
g.hook_del(hook)

# Full original candidate exclusion function. QoS/details data are synthetic;
# do not generalize this outgoing candidate filter to the full join protocol.
put(0x4cf792, 'B', 0)
for details, clan_mode, advertised, expected in [
    (True, True, [CLAN_B], True),
    (True, True, [CLAN_A], False),
    (True, True, [CLAN_B, CLAN_A], False),
    (True, False, [CLAN_A], True),
    (False, True, [CLAN_A], True),
]:
    g.mem_write(SEARCH, bytes(0xa0)); g.mem_write(ENTRY, bytes(0x204))
    put(SEARCH + 0x15, 'B', int(clan_mode)); put(SEARCH + 0x38, 'i', 1)
    g.mem_write(SEARCH + 0x3d, CLAN_A)
    put(ENTRY + 0xa4, 'B', int(details))
    put(ENTRY + 0xac, 'iii', 4, 0x2651, 0x2651)
    put(ENTRY + 0x110, 'I', 1); put(ENTRY + 0x70, 'I', 1)
    put(ENTRY + 0x130, 'i', len(advertised))
    g.mem_write(ENTRY + 0x134, b''.join(advertised))
    actual = bool(s.call(0x90d90, (SEARCH, ENTRY)) & 255)
    check(f'candidate exclusion details={details} clan_mode={clan_mode} clans={len(advertised)} same={CLAN_A in advertised}', actual, expected)

def teams(entries):
    g.mem_write(MATCH, bytes(0x10000)); put(MATCH + 4, 'i', 1)
    for i, e in enumerate(entries):
        a = MATCH + 0xdc4 + i * 24
        put(a, 'Bbhhhi', int(e.get('active', True)), e['place'], 0, e['skill'], 0, e.get('xp', 5500))
        g.mem_write(a + 12, e.get('identity', CLAN_A if i == 0 else CLAN_B))

for name, entries, expected in [
    ('equal clan skill win/loss', [dict(skill=15, place=0), dict(skill=15, place=1)], [100, -100]),
    ('equal clan standing tie', [dict(skill=15, place=0), dict(skill=15, place=0)], [0, 0]),
    ('clan higher skill differential', [dict(skill=16, place=0), dict(skill=15, place=1)], [90, -90]),
    ('clan bottom XP clamp', [dict(skill=15, place=1, xp=3), dict(skill=15, place=0)], [-3, 100]),
    ('clan top XP clamp', [dict(skill=15, place=0, xp=0x3fffffff-3), dict(skill=15, place=1)], [3, -100]),
    ('unknown opposing clan skill ignored', [dict(skill=15, place=0), dict(skill=-1, place=1)], [0]),
    ('clan pair loop includes zero identity without admission', [dict(skill=15, place=0), dict(skill=15, place=1, identity=bytes(12))], [100]),
    ('clan pair loop averages extra active entries', [dict(skill=15, place=0), dict(skill=15, place=0), dict(skill=15, place=1)], [50, 50, -100]),
]:
    teams(entries)
    check(name, [s.call(0x74720, (MATCH, i)) for i in range(len(expected))], expected)

teams([dict(skill=15, place=0), dict(skill=15, place=1)])
put(0x4ce040 + 0xc78, 'ii', 7, -9)
check('clan configured delta limits', [s.call(0x74720, (MATCH, i)) for i in range(2)], [7, -9])
s.call(0x66330)

def batch():
    g.mem_write(0x509454, bytes(5 * (0x1c + 16 * 0x98)))
    g.mem_write(BATCH, bytes(0xc090))
    s.call(0x741f0, (MATCH, BATCH))
    attributes = []
    for i in range(read(BATCH, 'i')[0]):
        a = BATCH + 8 + i * 16
        identifier, datatype = read(a, 'HH')
        attributes.append(dict(id=identifier, datatype=datatype, value=read(a + 8, 'q' if datatype == 2 else 'i')[0]))
    operations = []
    for i in range(read(BATCH + 0xe08, 'i')[0]):
        a = BATCH + 0xe10 + i * 0x88
        operations.append(dict(type=read(a, 'H')[0], synthetic_subject=bytes(g.mem_read(a+8,12)).hex(), statistics_namespace=read(a+20,'i')[0]))
    return attributes, operations, hashlib.sha256(bytes(g.mem_read(BATCH, 0xc090))).hexdigest()

teams([dict(skill=15, place=0), dict(skill=15, place=1)])
attributes, operations, digest = batch()
check('clan result batch attribute count', len(attributes), 28)
check('clan result batch operation count', len(operations), 42)
check('clan result batch subjects', sorted(set(o['synthetic_subject'] for o in operations)), sorted([CLAN_A.hex(), CLAN_B.hex()]))
check('clan result batch statistics namespaces', sorted(set(o['statistics_namespace'] for o in operations)), [25, 57, 89])
check('clan result batch XP', [attributes[i]['value'] for i in [0, 14]], [100, -100])
producer = dict(attributes=attributes, operations=operations, batch_sha256=digest)
# Valid-looking personal result rows never become the statistic subjects in
# type5; the whole output batch is byte-identical.
for i, identity in enumerate([PLAYER_A, PLAYER_B]):
    a = MATCH + 0x384 + i * 0xa4
    put(a, 'B', 1); g.mem_write(a+2, identity)
    put(a+0x8c, 'b', i); put(a+0x94, 'ihhi', 1, 15, 15, 5500); put(a+0xa0, 'b', 1-i)
check('player result rows do not alter type5 clan batch', batch()[2], digest)
g.mem_write(MATCH + 0xdc4 + 12, bytes(12))
check('result producer skips selected empty clan identity', len(batch()[1]), 21)

decoder = Cs(CS_ARCH_X86, CS_MODE_32)
trace = '\n'.join(f'{a:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for a in sorted(s.executed)
                  for i in [next(decoder.disasm(s.image.read(a, 15), a))]) + '\n'
(OUT / 'retail-clan-executed.txt').write_text(trace)
wanted = {0x66050, 0x7ed20, 0x7e100, 0x193c70, 0x193c00, 0x1936e0, 0x1936a0, 0x7e210, 0x6d4c0, 0x63280, 0x195a40, 0x91fc0,
          0x90d90, 0x741f0, 0x74720, 0x746b0, 0x73e00, 0x74aa0, 0x18fd94}
spans = {}
with (ROOT / 'tools/reference/halo2-decompiled/config/functions.csv').open() as f:
    for row in csv.reader(f):
        try: a = int(row[0], 16)
        except (ValueError, IndexError): continue
        if a not in wanted: continue
        n = int(row[1]); data = s.image.read(a, n)
        listing = '\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in decoder.disasm(data, a)) + '\n'
        (OUT / f'retail-{a:06x}.txt').write_text(listing)
        spans[hex(a)] = dict(size=n, sha256=hashlib.sha256(data).hexdigest())
print(json.dumps(dict(scope=__doc__, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    xbe=VERSIONS['retail'], source_spans=spans, launch_fallback_defaults=defaults,
    controls=len(checks), checks=checks, gathering_controls=gather_controls, host_gate_controls=host_controls,
    clan_snapshot_controls=snapshot_controls, assignment_controls=assignment_controls, clan_result_producer=producer,
    instruction_trace_sha256=hashlib.sha256(trace.encode()).hexdigest(),
    limits='Unchanged retail instructions with synthetic fixtures. No Xbox Live membership service, TMS/downloaded configurations, authenticated report arbitration, full gathering/join path, title-update comparison, live match or GPU presentation is executed.'), indent=2))
