#!/usr/bin/env python3
"""Execute unchanged retail candidate scoring/sort/select bytes with fixture data.

No native implementation is imported and no instruction/callee is replaced.
Synthetic search/results/QoS data are explicitly not Live service validation.
"""
from pathlib import Path
import csv
import gzip
import hashlib
import json
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / '.local/session-candidates-oracle'
OUT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT / 'tools'))
from oracle_game_variants import Source, VERSIONS, STACK, STOP
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR)
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

SEARCH, RESULTS, SNAPSHOT, OUTPUT = 0x1100000, 0x1101000, 0x1110000, 0x1120000
STRIDE, COUNT, CONFIG = 0x204, 50, 0x4ce040
s = Source(VERSIONS['retail'])
g = s.g
g.mem_map(SEARCH, 0x40000)
controls = []
bank_fixtures, admission_fixtures, selection_fixtures, threshold_fixtures = [], [], [], []

def i32(at, value):
    g.mem_write(at, struct.pack('<i', value))

def read_i(at):
    return struct.unpack('<i', g.mem_read(at, 4))[0]

def b(at, value):
    g.mem_write(at, bytes([value]))

def reset(party=1, rank=10, language=0, own_quality=(0, 0)):
    s.call(0x66330)
    g.mem_write(SEARCH, bytes(0x40000))
    b(SEARCH, 1)
    i32(SEARCH + 0x38, party)
    i32(SEARCH + 0x4c, rank)
    i32(SEARCH + 0x58, own_quality[0])
    i32(SEARCH + 0x5c, own_quality[1])
    i32(SEARCH + 0x6c, language)
    i32(SEARCH + 0x88, STRIDE * COUNT)
    i32(SEARCH + 0x8c, RESULTS)
    i32(SEARCH + 0x90, SNAPSHOT)
    i32(SEARCH + 0x34, 100)
    b(0x4cf792, 0)  # Own-address cache unavailable; transport is unstarted.
    b(0x510548, 1)
    i32(0x51054c, 0)

def candidate(index=0, ident=1, rank=15, space=2, wait=0, latency=0,
              details=True, ready=False, quality=(0, 0), language=0,
              eligible=True, valid=True, mask=0xffff):
    at = RESULTS + index * STRIDE
    b(at, int(valid))
    b(at + 1, int(eligible))
    i32(at + 4, ident)
    b(at + 0x74, int(ready))
    i32(at + 0x90, latency)
    b(at + 0xa4, int(details))
    i32(at + 0xac, 4)
    i32(at + 0xb0, 0x2651)
    i32(at + 0xb4, 0x2651)
    i32(at + 0xb8, language)
    i32(at + 0xbc, ident)  # Synthetic session identifier for selection control.
    i32(at + 0xf8, quality[0])
    i32(at + 0xfc, quality[1])
    i32(at + 0x10c, space)
    i32(at + 0x110, mask)
    i32(at + 0x11c, wait)
    i32(at + 0x120, rank)
    i32(at + 0x124, rank)
    i32(at + 0x12c, rank)
    i32(at + 0x5c, rank)
    i32(at + 0x60, space)
    i32(at + 0x64, rank)
    i32(at + 0x68, rank)
    i32(at + 0x6c, wait)
    i32(at + 0x70, mask)
    return at

def packed_bank(at):
    rows = []
    for i in range(COUNT):
        data = bytes(g.mem_read(at+i*STRIDE, STRIDE))
        if any(data):
            rows.append([i,data.hex()])
    return rows

def search_values():
    return dict(player_count=read_i(SEARCH+0x38), average_rank=read_i(SEARCH+0x4c),
        language=read_i(SEARCH+0x6c), quality_first=read_i(SEARCH+0x58),
        quality_second=read_i(SEARCH+0x5c),
        minimum_rank=read_i(SEARCH+0x30) if g.mem_read(SEARCH+0x2c,1)[0] else None,
        maximum_rank=read_i(SEARCH+0x28) if g.mem_read(SEARCH+0x24,1)[0] else None,
        own_address=bytes(g.mem_read(0x4cf793,36)).hex() if g.mem_read(0x4cf792,1)[0] else None,
        clan=bytes(g.mem_read(SEARCH+0x3d,12)).hex() if g.mem_read(SEARCH+0x15,1)[0] else None)

def config_values():
    offsets = dict(rank_radius=0x1f0,rank_high=0x1f4,rank_low=0x1f8,
        exact_space_bonus=0x340,maximum_age_seconds=0x344,first_quality_threshold=0xdc,
        second_quality_threshold=0xd8,quality_adjustment=0x350,
        partial_qos_delay_ms=0x354,threshold_initial=0x1e8,threshold_final=0x1ec)
    result = {name:read_i(CONFIG+offset) for name,offset in offsets.items()}
    result.update(age_coefficient=struct.unpack('<d',g.mem_read(CONFIG+0x348,8))[0],
        latency_curve=list(struct.unpack('<10i',g.mem_read(CONFIG+0x1c0,40))),
        language=[list(struct.unpack('<9i',g.mem_read(CONFIG+0x1fc+36*i,36))) for i in range(9)])
    return result

def allowed_original(at):
    row = bytes(g.mem_read(at,STRIDE))
    result = s.call(0x90d90,(SEARCH,at)) & 0xff
    admission_fixtures.append(dict(search=search_values(),record=row.hex(),expected=bool(result)))
    return result

def select_original():
    inputs = dict(bank=packed_bank(RESULTS),minimum_score=read_i(SEARCH+0x34),
        elapsed_query_ms=((read_i(0x51054c)-read_i(SEARCH+4)+(1<<31))%(1<<32))-(1<<31),
        active=bool(g.mem_read(SEARCH,1)[0]),config=config_values())
    result=s.call(0x91400,(OUTPUT,),registers=((UC_X86_REG_EDX,SEARCH),)) & 0xff
    # The selected result contains kind/id/key/address/RTT, copied from its
    # complete source candidate. Preserve its identity and entire raw bank.
    inputs['expected_id'] = read_i(OUTPUT+4) if result else None
    selection_fixtures.append(inputs)
    return result

def scored():
    fixture=dict(search=search_values(),config=config_values(),bank=packed_bank(RESULTS),
                 previous=packed_bank(SNAPSHOT))
    s.call(0x91000, (SEARCH,))
    fixture['expected']=packed_bank(RESULTS)
    bank_fixtures.append(fixture)
    return [dict(id=read_i(RESULTS + i*STRIDE + 4),
                 score=read_i(RESULTS + i*STRIDE + 0x1f8),
                 rank_difference=read_i(RESULTS + i*STRIDE + 0x1fc),
                 bounded_wait=read_i(RESULTS + i*STRIDE + 0x200),
                 latency=read_i(RESULTS + i*STRIDE + 0x90),
                 space=read_i(RESULTS + i*STRIDE + 0x10c),
                 eligible=bool(g.mem_read(RESULTS + i*STRIDE + 1,1)[0]))
            for i in range(COUNT) if g.mem_read(RESULTS + i*STRIDE,1)[0]]

def check(name, actual, expected, **inputs):
    assert actual == expected, (name, actual, expected)
    controls.append(dict(name=name, inputs=inputs, actual=actual, expected=expected))

def one(name, expected, *, own=None, config=None, **values):
    reset(**(own or {}))
    if config:
        for offset, value in config.items():
            if isinstance(value, float):
                g.mem_write(CONFIG + offset, struct.pack('<d', value))
            else:
                i32(CONFIG + offset, value)
    candidate(**values)
    result = scored()[0]
    check(name, result['score'], expected, candidate=values, own=own, configuration=config)
    return result

# All isolated components use actual defaults, except explicitly labelled fixtures.
for difference, expected in enumerate([100,85,70,55,40,0,0]):
    one(f'rank_difference_{difference}', expected, rank=10+difference)
    one(f'rank_difference_negative_{difference}', expected, rank=10-difference)
one('exact_space_bonus', 50, space=1)
one('extra_space_has_no_bonus', 0, space=3)
for wait, expected in [(-20,0),(0,0),(1,2),(17,34),(179,358),(180,360),(181,360),(3600,360)]:
    r = one(f'wait_seconds_{wait}', expected, wait=wait)
    check(f'wait_clamp_{wait}', r['bounded_wait'], min(180,max(0,wait)))
for latency, expected in [(-1,100),(0,100),(1,99),(50,90),(99,80),(100,80),
                          (150,65),(200,50),(250,25),(300,0),(350,-50),
                          (400,-100),(450,-150),(499,-198),(500,-200),
                          (900,-200),(1000,-200)]:
    one(f'median_rtt_ms_{latency}', expected, latency=latency, ready=True)
one('rtt_without_full_qos_flag_excluded', 0, latency=50, ready=False)
one('brief_service_result_uses_same_rank_space_wait', 229,
    details=False, ready=True, rank=12,space=1,wait=5,latency=1)

Q=262144
for own, other, expected in [((Q,Q),(Q,Q),0),((Q,Q),(Q-1,Q),100),
                              ((Q,Q),(Q,Q-1),100),((Q-1,Q),(0,0),0),
                              ((Q,Q-1),(0,0),0),((Q,Q),(0,0),100)]:
    one(f'host_quality_own_{own}_candidate_{other}', expected,
        own={'own_quality':own},quality=other)
one('host_quality_unavailable_in_brief_result',0,details=False,
    own={'own_quality':(Q,Q)},quality=(0,0))

special={1,6,7}
for mine in range(9):
    for theirs in range(9):
        expected=100 if mine==theirs and mine in special else (
            0 if mine==theirs else (-100 if mine in special or theirs in special else -50))
        one(f'language_{mine}_{theirs}',expected,own={'language':mine},language=theirs)
one('language_unavailable_in_brief_result',0,details=False,
    own={'language':1},language=1)

one('custom_nonintegral_wait_truncates_positive',0,wait=3,config={0x348:0.25})
one('custom_nonintegral_wait_truncates_negative_toward_zero',-49,
    wait=1,rank=10,config={0x348:0.25,0x1f4:-50,0x1f8:-50})
one('custom_rank_step_divides_before_multiplying',82,rank=12,
    config={0x1f0:4,0x1f4:100,0x1f8:52}) # 100-trunc(48/5)*2.
one('combined_retail_components',423,rank=12,space=1,wait=23,ready=True,
    latency=175,language=1,quality=(Q-1,Q),
    own={'language':1,'own_quality':(Q,Q)}) #70+50+46+100+100+57.

# Full sort includes all fifty records, not a replacement sorting implementation.
reset()
for index,wait in enumerate([0,10,20]):
    candidate(index,ident=index+1,wait=wait,ready=True)
check('sort_composite_descending', [r['id'] for r in scored()], [3,2,1])
reset()
for index, latency in enumerate([600,800,500]):
    candidate(index,ident=index+1,ready=True,latency=latency,wait=150)
check('sort_equal_score_lower_median_rtt_first',[r['id'] for r in scored()],[3,1,2])
reset()
i32(CONFIG+0x1f0,0)
for index,rank in enumerate([18,15,16]):
    candidate(index,ident=index+1,ready=True,rank=rank)
check('sort_equal_score_lower_rank_difference_first',[r['id'] for r in scored()],[2,3,1])
reset()
for index,space in enumerate([5,2,3]):
    candidate(index,ident=index+1,ready=True,space=space)
check('sort_equal_score_rtt_rank_smaller_space_first',[r['id'] for r in scored()],[2,3,1])

# Identical prior bytes bypass recomputation; fixture changes to data/global config
# alone do not prove the caller requests a fresh result snapshot.
reset()
candidate(ready=True)
first=scored()
i32(CONFIG+0x1f4,9999)
before=set(s.executed)
second=scored()
check('identical_snapshot_bypasses_rescore',second,first)
check('snapshot_copied_after_sort',hashlib.sha256(g.mem_read(RESULTS,STRIDE*COUNT)).hexdigest(),
      hashlib.sha256(g.mem_read(SNAPSHOT,STRIDE*COUNT)).hexdigest())

def selected(count,ready,score=100,threshold=100,elapsed=0,eligible_count=None):
    reset()
    i32(SEARCH+0x34,threshold)
    i32(0x51054c,elapsed)
    for index in range(count):
        at=candidate(index,ident=index+1,ready=index<ready,
            eligible=eligible_count is None or index<eligible_count)
        i32(at+0x1f8,score)
    result=select_original()
    return bool(result),read_i(OUTPUT+4)

for name,args,expected in [
    ('all_ready_selects_immediately',dict(count=8,ready=8),True),
    ('score_equal_threshold_is_accepted',dict(count=1,ready=1),True),
    ('score_below_threshold_rejected',dict(count=1,ready=1,score=99),False),
    ('no_full_qos_candidate_rejected',dict(count=1,ready=0,elapsed=6000),False),
    ('partial_ready_exact_5000ms_waits',dict(count=8,ready=2,elapsed=5000),False),
    ('partial_ready_5001ms_selects',dict(count=8,ready=2,elapsed=5001),True),
    ('below_floor_quarter_waits_even_after_delay',dict(count=8,ready=1,elapsed=6000),False),
    ('floor_quarter_not_ceil_for_seven',dict(count=7,ready=1,elapsed=6000),True),
    ('floor_quarter_zero_for_three_but_one_candidate_required',dict(count=3,ready=1,elapsed=6000),True),
    ('ineligible_results_do_not_count_in_readiness_denominator',dict(count=8,ready=1,eligible_count=1),True),
]:
    result, ident=selected(**args)
    check(name,result,expected,**args)
    if result: check(name+'_first_qualifying_id',ident,1)

reset()
candidate(0,ident=11,ready=True,wait=10)
candidate(1,ident=22,ready=True,wait=40)
candidate(2,ident=33,ready=True,wait=20)
scored()
i32(SEARCH+0x34,100)
for iteration in range(12):
    result=select_original()
    check(f'repeated_selection_without_rng_{iteration}',(result,read_i(OUTPUT+4)),(1,22))

reset()
candidate(0,ident=11,ready=False,wait=100)
candidate(1,ident=22,ready=True,wait=10)
candidate(2,ident=33,ready=True,wait=20)
scored()
i32(0x51054c,5001)
result=select_original()
check('higher_scoring_unready_host_skipped',(result,read_i(OUTPUT+4)),(1,33))

# The comparator's QoS tie branch is deliberately asymmetric before all entries
# are ready. Do not silently replace it with a global strict tuple ordering.
reset()
a=candidate(0,ident=1,ready=True,latency=100,rank=20)
c=candidate(1,ident=2,ready=False,latency=0,rank=30)
for at,delta in [(a,10),(c,20)]:
    i32(at+0x1f8,100);i32(at+0x1fc,delta)
check('mixed_readiness_compare_a_b',s.call(0x90f10,(a,c,0)) & 0xff,1)
check('mixed_readiness_compare_b_a',s.call(0x90f10,(c,a,0)) & 0xff,1)

# Admission independently precedes scoring. Synthetic fixture changes affect data,
# not the original instruction bytes or target routines.
for name,offset,value,expected in [
    ('required_space_mask_present',0x110,1,True),
    ('required_space_mask_missing',0x110,2,False),
    ('description_protocol_mismatch',0xac,3,False),
    ('description_lower_version_above_supported',0xb0,0x2652,True),
    ('description_lower_version_below_supported',0xb0,0x2650,False),
    ('description_upper_version_above_supported',0xb4,0x2652,False),
    ('description_upper_version_below_supported',0xb4,0x2650,True),
]:
    reset();at=candidate();i32(at+offset,value)
    allowed=allowed_original(at)
    check(name,bool(allowed),expected,offset=hex(offset),value=value)

for details in (False,True):
    for party,mask,expected in [(4,8,True),(4,4,False),(8,128,True)]:
        reset(party=party);at=candidate(details=details,mask=mask)
        check(f'mask_party_{party}_mask_{mask}_details_{details}',
              bool(allowed_original(at)),expected)
    for use_lower, use_upper, low,high,expected in [
        (True,False,11,100,False),(True,False,10,100,True),
        (False,True,0,9,False),(False,True,0,10,True),
        (False,False,100,0,True)]:
        reset();at=candidate(details=details,rank=10)
        b(SEARCH+0x2c,int(use_lower));i32(SEARCH+0x30,low)
        b(SEARCH+0x24,int(use_upper));i32(SEARCH+0x28,high)
        check(f'rank_gate_details_{details}_lower_{use_lower}_{low}_upper_{use_upper}_{high}',
              bool(allowed_original(at)),expected)

reset();at=candidate(details=False);i32(at+0xac,123)
check('brief_result_does_not_yet_check_description_version',
    bool(allowed_original(at)),True)
reset();at=candidate();b(0x4cf792,1)
own_address=b'\x5a'*36
g.mem_write(0x4cf793,own_address);g.mem_write(at+0x24,own_address)
check('cached_own_advertised_address_rejected',bool(allowed_original(at)),False)
for same,expected in [(True,False),(False,True)]:
    reset();at=candidate();b(SEARCH+0x15,1)
    g.mem_write(SEARCH+0x3d,b'A'*12)
    i32(at+0x130,1);g.mem_write(at+0x134,(b'A' if same else b'B')*12)
    check(f'clan_same_{same}_gate',bool(allowed_original(at)),expected)
reset();at=candidate(mask=0)
r=scored()[0]
check('failed_admission_clears_eligible_and_score',(r['eligible'],r['score']),(False,0))

# The original QoS extraction proves the measured latency's exact source field.
# Original pool, record, XNQOS and XNQOSINFO layouts are supplied as fixture data.
reset()
pool,record,qos,payload=SEARCH+0x22000,SEARCH+0x22100,SEARCH+0x22200,SEARCH+0x22300
b(0x4cf8d4,1);i32(0x4cf8d8,pool)
i32(pool+0x38,1);i32(pool+0x24,8);i32(pool+0x44,record)
g.mem_write(record,struct.pack('<hhI',1,1,qos))
g.mem_write(qos+8,struct.pack('<BBHHHIHHII',0x0b,0,8,7,3,payload,41,73,123456,654321))
g.mem_write(payload,b'abc')
result=s.call(0x7b7b0,(0,),registers=((UC_X86_REG_EAX,0x10000),(UC_X86_REG_EDI,OUTPUT))) & 0xff
check('qos_extracts_probe_rtt_min_median_up_down_data',
    (result,*struct.unpack('<8i',g.mem_read(OUTPUT,32))),
    (1,8,7,41,73,123456,654321,3,payload))
i32(0x4cf968,345678);i32(0x4cf96c,8)
s.call(0x64870,registers=((UC_X86_REG_EDX,OUTPUT),(UC_X86_REG_EAX,2)))
check('interface_caches_qos_downstream_at_quality_first_source',read_i(0x4cd868+0x78),654321)
check('interface_caches_separate_bandwidth_estimate',read_i(0x4cd868+0x88),345678)

# Original downloadable-config validation. A synthetic CRC-valid file is not a
# recovered historical title-storage blob or evidence of any Live-era override.
reset()
definition=0x467140
check('config_definition_format_version',read_i(definition+8),0x23)
check('config_destination_is_live_configuration',read_i(0x477058+0x24),CONFIG)
check('config_buffer_exact_size',read_i(0x477058+0x28),0x1730)
for version,size,corrupt,expected in [(0x23,0x1730,False,1),
    (0x22,0x1730,False,11),(0x24,0x1730,False,12),
    (0x23,0x172c,False,9),(0x23,0x1730,True,8)]:
    g.mem_write(OUTPUT,bytes(0x1730))
    i32(OUTPUT+4,version);i32(OUTPUT+8,size)
    checksum=s.call(0x812a0,registers=((UC_X86_REG_EAX,OUTPUT),(UC_X86_REG_ECX,size)))
    i32(OUTPUT,checksum)
    if corrupt:b(OUTPUT+0x340,99)
    result=s.call(0x81230,registers=((UC_X86_REG_EDX,0x477058),
        (UC_X86_REG_EDI,OUTPUT),(UC_X86_REG_ECX,size)))
    check(f'config_validation_version_{version}_size_{size}_corrupt_{corrupt}',result,expected)

# Original score-threshold interpolation, stopping before the genuine unavailable
# Live query entry. There is no replacement query result, stub or instruction edit.
def query_boundary(g,address,size,unused):
    g.emu_stop()
hook=g.hook_add(UC_HOOK_CODE,query_boundary,begin=0x8fa80,end=0x8fa80)
for radius,maximum,expected in [(0,6,200),(1,6,183),(2,6,166),(3,6,150),
    (4,6,133),(5,6,116),(6,6,100),(-1,6,100),(0,0,100)]:
    reset()
    i32(SEARCH+0x1c,radius);i32(SEARCH+0x20,maximum)
    i32(SEARCH+0x50,20);i32(SEARCH+0x60,1);i32(SEARCH+0x64,50);i32(SEARCH+0x68,10)
    i32(STACK,STOP)
    g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_ESI,SEARCH)
    g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    g.emu_start(0x90b30,STOP,count=200000)
    check(f'query_boundary_radius_{radius}_maximum_{maximum}',g.reg_read(UC_X86_REG_EIP),0x8fa80)
    check(f'score_threshold_radius_{radius}_maximum_{maximum}',read_i(SEARCH+0x34),expected)
    threshold_fixtures.append(dict(radius=radius,maximum_radius=maximum,expected=read_i(SEARCH+0x34)))
# All integer expansion radii through representative allowed maxima.
for maximum in (1,4,7,25,49):
    for radius in range(maximum+1):
        reset();i32(SEARCH+0x1c,radius);i32(SEARCH+0x20,maximum)
        i32(SEARCH+0x50,20);i32(SEARCH+0x60,1);i32(SEARCH+0x64,50);i32(SEARCH+0x68,10)
        i32(STACK,STOP);g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_ESI,SEARCH)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(0x90b30,STOP,count=200000)
        assert g.reg_read(UC_X86_REG_EIP)==0x8fa80
        threshold_fixtures.append(dict(radius=radius,maximum_radius=maximum,expected=read_i(SEARCH+0x34)))
g.hook_del(hook)
s.executed.discard(0x8fa80) # Boundary hook observes entry without executing it.

# Differential banks exercise the complete fixed 50-slot sort, including
# invalid records, negative scores, mixed readiness, opaque bytes and ties.
# The values are synthetic controls, not a recovered Live candidate sample.
rng=random.Random(0x9100013da70)
for index in range(96):
    reset(party=rng.randrange(1,9),rank=rng.randrange(0,51),language=rng.randrange(9),
          own_quality=rng.choice([(0,-1),(262144,262144),(100000,900000)]))
    if index%4==0:
        i32(CONFIG+0x1f0,0);i32(CONFIG+0x340,0);i32(CONFIG+0x350,0)
        g.mem_write(CONFIG+0x348,struct.pack('<d',0.0))
        g.mem_write(CONFIG+0x1c0,bytes(40));g.mem_write(CONFIG+0x1fc,bytes(81*4))
    elif index%4==1:
        g.mem_write(CONFIG+0x348,struct.pack('<d',rng.choice([0.25,-0.25,1.5])))
        i32(CONFIG+0x1f4,rng.choice([-50,80,100]));i32(CONFIG+0x1f8,52)
    count=50 if index<48 else rng.randrange(1,50)
    for i in range(count):
        at=candidate(i,ident=i+1,rank=rng.randrange(0,51),space=rng.randrange(0,17),
            wait=rng.randrange(-20,201),latency=rng.randrange(-1,1501),
            details=bool(rng.getrandbits(1)),ready=bool(rng.getrandbits(1)),
            quality=rng.choice([(0,-1),(262143,262144),(262144,262144)]),
            language=rng.randrange(9),eligible=rng.randrange(6)!=0,valid=rng.randrange(8)!=0,
            mask=rng.choice([0xffff,1,2,4,8,16,32,64,128]))
        g.mem_write(at+0x1d0,rng.randbytes(16))
    result=scored()
    check(f'complete_bank_{index}_retains_valid_records',len(result),
          sum(bool(g.mem_read(RESULTS+i*STRIDE,1)[0]) for i in range(COUNT)))
    i32(SEARCH+0x34,rng.choice([-100,0,100,200]))
    i32(0x51054c,rng.choice([0,5000,5001,6000]))
    select_original()

# Signed elapsed-query subtraction is deliberately distinct from session
# wall-clock age, including a DWORD tick wrap and an inactive search.
for start,end,active,expected in [(0,5000,True,False),(0,5001,True,True),
        (-20,4981,True,True),(0,-1,True,False),(0,6000,False,False)]:
    reset();b(SEARCH,int(active));i32(SEARCH+4,start);i32(0x51054c,end)
    for i in range(2):
        at=candidate(i,ident=i+1,ready=i==0);i32(at+0x1f8,100)
    result=select_original()
    check(f'select_elapsed_start_{start}_end_{end}_active_{active}',bool(result),expected)

# Every integer RTT across all interpolation intervals gets independent
# executed score output. Banks of 50 also check how the original sort handles
# the resulting score plateaus and ties.
for first in range(0,1001,50):
    reset()
    for i,ms in enumerate(range(first,min(first+50,1001))):
        candidate(i,ident=ms+1,rank=15,latency=ms,ready=True)
    scored()

# A changed opaque byte invalidates the exact byte cache, even though all
# interpreted score inputs are unchanged. Configuration alone does not.
reset();candidate(ready=True);scored()
i32(CONFIG+0x1f4,9999)
b(RESULTS+0x1d0,33)
scored()

# Provenance captures are regenerated from the same pinned executable.
s.call(0x66330)
defaults={hex(o):read_i(CONFIG+o) for o in [0xd8,0xdc,*range(0x1c0,0x1fc,4),
    0x340,0x344,0x350,0x354,0x358,0x35c]}
defaults['0x348_double']=struct.unpack('<d',g.mem_read(CONFIG+0x348,8))[0]
defaults['language_9x9']=[list(struct.unpack('<9i',g.mem_read(CONFIG+0x1fc+36*i,36))) for i in range(9)]
(OUT/'retail-defaults.json').write_text(json.dumps(defaults,indent=2)+'\n')
rows={int(r[0],16):r for r in csv.reader(open(ROOT/'tools/reference/halo2-decompiled/config/functions.csv'))
    if r and all(c in '0123456789abcdefABCDEF' for c in r[0])}
addresses=[0x66330,0x662f0,0x902b0,0x90840,0x91000,0x90d90,0x90f10,0x91400,
    0x13da70,0x13dcd0,0x7b7b0,0x73530,0x7e100,0x7e210,0x70ea0,0x90b30,
    0x90880,0x8fb30,0xb4a40,0x7db10,0x7e080,0x64870,0x658c0,0x19adf6,
    0x2bb0e9,0x7f410,0x6bff0,0x80f70,0x81230,0x812a0,0x81050,
    0x8e240,0xb4450,0xb4120,0x163ba0,0x163c00]
cs=Cs(CS_ARCH_X86,CS_MODE_32);manifest=[]
for a in addresses:
    row=rows[a];size=int(row[1]);raw=s.image.read(a,size)
    (OUT/f'original-{a:08x}.asm').write_text('\n'.join(
        f'{i.address:08x} {i.bytes.hex():22s} {i.mnemonic:8s} {i.op_str}'
        for i in cs.disasm(raw,a))+'\n')
    manifest.append(dict(address=hex(a),size=size,sha256=hashlib.sha256(raw).hexdigest(),
        source=row[-2],status=row[-1]))
(OUT/'original-function-spans.json').write_text(json.dumps(manifest,indent=2)+'\n')

trace=[]
cs=Cs(CS_ARCH_X86,CS_MODE_32)
for address in sorted(s.executed):
    instruction=next(cs.disasm(bytes(g.mem_read(address,15)),address))
    original=s.image.read(address,instruction.size)
    assert original == instruction.bytes,hex(address)
    trace.append(f'{address:08x} {instruction.bytes.hex():22s} {instruction.mnemonic:8s} {instruction.op_str}')
trace_text='\n'.join(trace)+'\n'
(OUT/'executed-original-instructions.txt').write_text(trace_text)
report=dict(xbe_sha256=VERSIONS['retail']['pin'],controls=controls,
    control_count=len(controls),unique_original_instructions=len(trace),
    executed_instruction_sha256=hashlib.sha256(trace_text.encode()).hexdigest(),
    oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope='Unchanged retail scoring, comparator, full sort, admission, selection, QoS extraction and config validation with synthetic data; threshold prefix stops before original Live query 0x8fa80. No Live callbacks or native implementation replaced.')
report.update(bank_controls=len(bank_fixtures),admission_controls=len(admission_fixtures),
    selection_controls=len(selection_fixtures),threshold_controls=len(threshold_fixtures))
fixture=dict(xbe_sha256=report['xbe_sha256'],oracle_sha256=report['oracle_sha256'],
    executed_instruction_sha256=report['executed_instruction_sha256'],
    banks=bank_fixtures,admission=admission_fixtures,selection=selection_fixtures,
    thresholds=threshold_fixtures)
(OUT/'retail-session-candidates.json').write_text(json.dumps(fixture,separators=(',',':'))+'\n')
(OUT/'retail-session-candidates.json.gz').write_bytes(gzip.compress(
    (json.dumps(fixture,separators=(',',':'))+'\n').encode(),mtime=0))
(OUT/'oracle-candidate.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='controls'},indent=2))
