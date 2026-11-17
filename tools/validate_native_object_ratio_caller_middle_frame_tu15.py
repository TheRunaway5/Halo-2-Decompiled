#!/usr/bin/env python3
"""Compare the actual 1046C0 middle record branch through its RET16 exits."""
import argparse
import copy
import json
import os
from pathlib import Path
import subprocess

import oracle_native_campaign_screen_lifecycle_tu15 as oracle
from oracle_smg_firing_1_1 import Xbe
from native_object_ratio_combat_fixtures import REGIONS as COMBAT_REGIONS, combat_corpus
from native_object_ratio_high_fixtures import KEYS as HIGH_KEYS, high_corpus
from native_object_ratio_low_fixtures import KEYS as LOW_KEYS, low_corpus
from validate_native_flight_property_query_frame_tu15 import sha
from validate_native_object_record_ratio_public_tu15 import REGIONS, parent_rows
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT=Path(__file__).resolve().parents[1]
ENTRY,CHILD=0x1046c0,0x107140
RETURNS={0x104d15,0x104d27}
FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_middle_frame.hpp'
DRIVER='tests/native_tu15_object_ratio_caller_middle_frame_tests.cpp'
EXTENDED_FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_extended_frame.hpp'
EXTENDED_DRIVER='tests/native_tu15_object_ratio_caller_extended_frame_tests.cpp'
MAXIMUM_FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_maximum_frame.hpp'
MAXIMUM_DRIVER='tests/native_tu15_object_ratio_caller_maximum_frame_tests.cpp'
HIGH_FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_high_frame.hpp'
HIGH_DRIVER='tests/native_tu15_object_ratio_caller_high_frame_tests.cpp'
LOW_FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_low_frame.hpp'
LOW_DRIVER='tests/native_tu15_object_ratio_caller_low_frame_tests.cpp'
COMBAT_FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_combat_frame.hpp'
COMBAT_DRIVER='tests/native_tu15_object_ratio_caller_combat_frame_tests.cpp'
WHOLE_FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_frame.hpp'
WHOLE_DRIVER='tests/native_tu15_object_ratio_caller_whole_frame_tests.cpp'
WHOLE_CONTROLS=(
    ('drop-combat-route','clamp_if(run_combat_ratio(combat,true))','HighResult::finish'),
    ('skip-alternate-maxima','extended_result(extended.locate_initial_scan())','extended_result(ExtendedStage::scan_records)'),
    ('omit-signed-ratio-clamp','clamp_if(low.sample_signed_ratio())','HighResult::finish'),
    ('merge-unknown-return','high.finish_unknown();return;','exit.finish_result();return;'),
)
COMBAT_EDGES={0x104a10:(0x165590,0x104a15),0x104a1e:(0x1655f0,0x104a23),
    0x104b35:(0x165590,0x104b3a),0x104b43:(0x1655f0,0x104b48),0x165602:(0x24b430,0x165607)}
COMBAT_CONTROLS=(
    ('wrong-definition-resume','middle?0x104b3au:0x104a15u','middle?0x104b39u:0x104a15u'),
    ('wrong-nested-resume','push(0x165602u,0x165607u);','push(0x165602u,0x165606u);'),
    ('wrong-definition-index','g[5]+0x17eu,2','g[5]+0x17cu,2'),
    ('wrong-mode-test','g[0]+0x180u),9u','g[0]+0x180u),8u'),
    ('wrong-lahf-mask','s_.flags.logic(ah&0x44u,1);','s_.flags.logic(ah&0x45u,1);'),
)
LOW_EDGES={0x1048ba:(0xfc510,0x1048bf),0x104957:(0xfc510,0x10495c),
           0x104815:(CHILD,0x10481a),0x104b6c:(CHILD,0x104b71)}
LOW_CONTROLS=(
    ('wrong-fc510-resume','parent_mode(0x1048bau,0x1048bfu)','parent_mode(0x1048bau,0x1048beu)'),
    ('wrong-late-mode-bit','s_.gp[5]+0x16cu,0,5u','s_.gp[5]+0x16cu,0,4u'),
    ('wrong-position-flags','if(!s_.flags.pf)return false;','if(s_.flags.pf)return false;'),
    ('wrong-ratio-stride','s_.flags.product(g[1],0x5cu)','s_.flags.product(g[1],0x58u)'),
    ('wrong-clock-cutoff','s_.flags.minus(g[2],1u,4);','s_.flags.minus(g[2],0u,4);'),
)
HIGH_CONTROLS=(
    ('wrong-clock-divisor','"c"(10u) : "cc");','"c"(8u) : "cc");'),
    ('missing-idiv-residue','s_.flags.load((dividend&0xd5u)|(std::uint32_t(overflow)<<11u));',''),
    ('wrong-secondary-column','(second_column?0x1b8u:0x1b4u)','(second_column?0x1b4u:0x1b8u)'),
    ('wrong-upper-flag','upper_middle?1u:2u','upper_middle?2u:1u'),
    ('wrong-record-stride','s_.flags.product(index,0x34u)','s_.flags.product(index,0x38u)'),
    ('discard-record-cmp-residue','return s_.flags.product(index,0x34u);',
     'return s_.flags.logic(s_.flags.product(index,0x34u),4);'),
)
CONTROLS=(
    ('reverse-record-bound','return s_.flags.sf!=s_.flags.of;','return s_.flags.sf==s_.flags.of;'),
    ('wrong-child-return','push(0x104b6cu,0x104b71u);','push(0x104b6cu,0x104b70u);'),
    ('wrong-output-byte','write_success(0x104d11u,g[1],std::byte{1});','write_success(0x104d11u,g[1],std::byte{0});'),
    ('skip-negative-clamp','{s_.xmm[2]=s_.xmm[3];return;}','{return;}'),
)
EXTENDED_CONTROLS=(
    ('wrong-child-return','push(0x104815u,0x10481au);','push(0x104815u,0x104819u);'),
    ('wrong-table-stride','s_.flags.plus(g[6],0x40u,4)','s_.flags.plus(g[6],0x3cu,4)'),
    ('skip-live-weight-reread','const auto weight=scalar(0x10483au,g[6]);','const auto weight=_mm_set_ss(1.0f);'),
    ('wrong-mode-byte','g[2]+0x82u,1','g[2]+0x81u,1'),
)
MAXIMUM_CONTROLS=(
    ('wrong-group-stride','s_.flags.plus(g[0],0xd0u,4)','s_.flags.plus(g[0],0xc0u,4)'),
    ('wrong-remainder-stride','s_.flags.plus(g[0],0x34u,4)','s_.flags.plus(g[0],0x30u,4)'),
    ('wrong-group-count','g[6]=g[1]*4u;','g[6]=g[1]*3u;'),
    ('cache-live-maximum-read','s_.flags.scalar_compare(s_.xmm[2],scalar(compare,address),false);\n'
     '        if(s_.flags.cf||s_.flags.zf)s_.xmm[2]=scalar(reload,address);',
     'const auto value=scalar(compare,address);s_.flags.scalar_compare(s_.xmm[2],value,false);\n'
     '        if(s_.flags.cf||s_.flags.zf)s_.xmm[2]=value;'),
)


def corpus(image):
    rows=parent_rows(image)[1::2]
    for n,row in enumerate(rows):
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}

        def locate(address):
            for base,data in banks.items():
                if base<=address<=base+len(data)-4:
                    return data,address-base
            raise ValueError('Middle record fixture escaped its declared banks')

        def get(address):
            data,offset=locate(address);return int.from_bytes(data[offset:offset+4],'little')

        def put(address,value):
            data,offset=locate(address);data[offset:offset+4]=value.to_bytes(4,'little')

        datum=row['matrix'];slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8);sp=row['gp'][4]
        if n%4==0:put(definition+0x2c8,0)
        if n%6==2:put(sp+16,0x61003e01)
        if n%6==4:put(sp+12,sp-4)
        if n%8==1:
            record=get(definition+0x2cc)+64
            put(record+0x1c,0x3c000000);put(0x61003804,0x40000000)
        row['banks']=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS]
    return rows


def extended_corpus(image,maximum=False):
    rows=parent_rows(image)[::2]
    for n,row in enumerate(rows):
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}

        def locate(address,width):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    return data,address-base
            raise ValueError('Extended record fixture escaped its declared banks')

        def get(address):
            data,offset=locate(address,4);return int.from_bytes(data[offset:offset+4],'little')

        def put(address,value,width=4):
            data,offset=locate(address,width)
            data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')

        datum=row['matrix'];slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8);sp=row['gp'][4]
        records=get(definition+0x2cc)
        scenario=n%8
        skip=scenario==0
        count=0 if scenario==2 else 1 if scenario in (1,7) else 2
        initial=((1,2,3,4,5,8)[n//8] if maximum else (2 if n//8%2 else 4)) if scenario==6 else 0
        if scenario==6 and not maximum:
            row.update(stop_before_entries=[0x10477c,0x1047c6],continuation=0x10477c if initial==4 else 0x1047c6)
        put(0x61003a82,int(skip),1)
        put(definition+0x2d0,initial);put(definition+0x2c8,count)
        expected=0
        for index in range(2):
            weight=(0,0xbf800000,0x40000000)[(n+index)%3]
            if scenario==4:weight=0x40000000
            put(records+index*64+0x1c,weight)
            put(records+index*64+0x28,(0x3f000000,0x3f400000)[index])
            put(body+0x20c+index*12,1 if (n+index)%2 else 2,1)
            put(body+0x20e+index*12,(3,-2,9)[(n+index)%3],2)
            if not skip and (maximum or not initial) and index<count and weight==0x40000000:expected+=1
        if maximum and initial:
            values=(0x3e000000,0x3e800000,0x3f000000,0x3f400000,0x7fc12345,0xbf800000,0,0x3f800000)
            for index in range(initial):put(body+0x1bc+index*0x34,values[index])
        put(body+0x180,(0,0x3f000000,0x3fc00000)[n%3])
        if n%6==2:put(sp+16,0x61003e01)
        if n%6==4:put(sp+12,sp-4)
        row.update(expected_child_returns=expected)
        row['banks']=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS]
    return rows


def whole_corpus(image):
    cases=[]
    for family,rows in (('high',high_corpus(image)),('low',low_corpus(image)),('combat',combat_corpus(image)),
                        ('maximum',extended_corpus(image,True)),('record',corpus(image))):
        for row in rows:
            banks={b['address']:b['before'] for b in row['banks']}
            row['banks']=[dict(address=base,before=banks.get(base,'00'*size)) for base,size in COMBAT_REGIONS]
            row['id']=len(cases);row['scope_family']=family
            cases.append(row)
    return cases


def fixture(image,scratch,report,with_observers,extended,maximum,high=False,low=False,combat=False,whole=False):
    code=oracle._Code(image,'.text');original=oracle.metadata
    callsite,resume=(0x104815,0x10481a) if extended else (0x104b6c,0x104b71)
    call=image.read(callsite,5)
    if not (high or low or combat or whole) and (call[0]!=0xe8 or resume+int.from_bytes(call[1:],'little',signed=True)!=CHILD):
        raise ValueError('Actual record branch child call changed')

    def metadata(image):
        result=original(image)
        for v in result['sites']:
            if code.at(v['instruction']).mnemonic in ('pop','ret'):
                v['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result

    all_edges=LOW_EDGES|COMBAT_EDGES
    selected_edges=all_edges if whole else LOW_EDGES if low else COMBAT_EDGES if combat else {}
    if selected_edges:
        for pc,(target,post) in selected_edges.items():
            instruction=image.read(pc,5)
            if instruction[0]!=0xe8 or post+int.from_bytes(instruction[1:],'little',signed=True)!=target:
                raise ValueError('Actual low key child call changed')
    entries=(ENTRY,CHILD,0xfc510,0x165590,0x1655f0,0x24b430) if whole else (ENTRY,) if high else (ENTRY,CHILD,0xfc510) if low else (
        ENTRY,0x165590,0x1655f0,0x24b430) if combat else (ENTRY,CHILD)
    regions=COMBAT_REGIONS if (combat or whole) else REGIONS
    returns=RETURNS|{0x1049d7} if (high or low or whole) else RETURNS
    oracle.ENTRIES=entries;oracle.metadata=metadata
    sites=metadata(image)['sites']
    if len(sites)!=(522 if whole else 406 if high else 457 if low else 471 if combat else 442):
        raise ValueError('Exact complete parent or leaf original site count changed')
    trace=[dict(entry=v['instruction'],arguments=0,gp=True,xmm=True) for v in sites
           if v['operands'] or code.at(v['instruction']).mnemonic in ('push','call')]
    rows=whole_corpus(image) if whole else high_corpus(image) if high else low_corpus(image) if low else combat_corpus(image) if combat else (
        extended_corpus(image,maximum) if extended else corpus(image))

    def observe(rows,folder):
        folder.mkdir()
        return physical_original(image,rows,folder,entries=entries,regions=regions,
                                 trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)

    normal,report['authority']=observe(rows,scratch/'normal')
    total=0;actual_rets=set();joins={};groups=remainders=0;low_calls=dict.fromkeys(LOW_EDGES,0);combat_calls=dict.fromkeys(COMBAT_EDGES,0);whole_calls=dict.fromkeys(all_edges,0)
    for n,(row,a) in enumerate(zip(rows,normal['cases'],strict=True)):
        pcs=a['executed_sites'];actual_rets.update(returns.intersection(pcs))
        if not a['completed']:
            raise ValueError('Actual record path did not return or reach the declared join')
        if 'continuation' in row:
            pc=row['continuation'];joins[hex(pc)]=joins.get(hex(pc),0)+1
            if a.get('stopped_at')!=pc or RETURNS.intersection(pcs):
                raise ValueError('Unexpected original initial maxima continuation')
        if whole:
            returned=0
            for pc,(target,post) in all_edges.items():
                times=pcs.count(pc)
                if pcs.count(post)!=times or pcs.count(target)<times:
                    raise ValueError('Actual whole-frame child did not return: '+row['name'])
                whole_calls[pc]+=times;returned+=times
            if returned!=sum(pcs.count(owner) for owner in entries[1:]):
                raise ValueError('Unexpected whole-frame child entry: '+row['name'])
        elif high:
            if row['expected_high_route'] not in pcs or any(
                code.at(pc).mnemonic=='call' for pc in pcs):
                raise ValueError('Unexpected high key original route: '+row['name'])
            returned=0
        elif low:
            if row['expected_low_route'] not in pcs:
                raise ValueError('Unexpected low key original route: '+row['name'])
            returned=0
            calls=[packet['entry'] for packet in a['calls']]
            for pc,(target,post) in LOW_EDGES.items():
                times=pcs.count(pc)
                if pcs.count(post)!=times or pcs.count(target)<times:
                    raise ValueError('Actual low key child did not return: '+row['name'])
                low_calls[pc]+=times;returned+=times
            if returned!=sum(calls.count(owner) for owner in entries if owner!=ENTRY):
                raise ValueError('Unexpected low key child call: '+row['name'])
        elif combat:
            if row['expected_combat_route'] not in pcs:
                raise ValueError('Unexpected original CombatAction route: '+row['name'])
            returned=0
            expected=[row['expected_combat_definition'],row['expected_combat_ratio'],row['expected_combat_ratio']]
            if [pcs.count(owner) for owner in entries[1:]]!=expected:
                raise ValueError('Unexpected original CombatAction child gates: '+row['name'])
            for pc,(target,post) in COMBAT_EDGES.items():
                times=pcs.count(pc)
                if pcs.count(post)!=times or pcs.count(target)<times:
                    raise ValueError('Actual CombatAction child did not return: '+row['name'])
                combat_calls[pc]+=times;returned+=times
            if returned!=sum(expected):
                raise ValueError('Unexpected CombatAction child count: '+row['name'])
        elif extended:
            calls=[packet['entry'] for packet in a['calls']]
            groups+=calls.count(0x104790);remainders+=calls.count(0x1047d4)
            returned=calls.count(CHILD)
            if (0x104747 not in pcs or calls.count(callsite)!=returned or calls.count(resume)!=returned or
                returned!=row['expected_child_returns']):
                raise ValueError('Actual extended branch child/return path changed: '+row['name'])
        else:
            returned=pcs.count(CHILD)
            if 0x104b50 not in pcs or returned!=pcs.count(resume) or returned!=int(n%4!=0):
                raise ValueError('Actual middle branch child/return path changed: '+row['name'])
        total+=returned
    if actual_rets!=returns:
        raise ValueError('Missing an original record branch return path')
    if maximum and (groups!=4 or remainders!=7 or joins):
        raise ValueError('Original initial maximum loop multiplicity changed')
    if low:
        if list(low_calls.values())[:2]!=[8,8] or not all(low_calls.values()):
            raise ValueError('Low key cases missed an original child route')
        report['actual_low_child_returns']={hex(pc):n for pc,n in low_calls.items()}
    if combat:
        if list(combat_calls.values())!=[30,24,10,8,32]:
            raise ValueError('CombatAction cases missed a real call/return edge')
        report['actual_combat_child_returns']={hex(pc):n for pc,n in combat_calls.items()}
    if whole:
        if not all(whole_calls.values()):
            raise ValueError('Whole frame cases missed an actual call')
        report['actual_whole_child_returns']={hex(pc):n for pc,n in whole_calls.items()}
    prefixes=[]
    for n,(row,a) in enumerate(zip(rows,normal['cases'],strict=True)):
        if whole:
            if row['id']%5:continue
        elif n>=(3*len(HIGH_KEYS) if high else 4*len(LOW_KEYS) if low else 24 if combat else 8) and not (maximum and n%8==6):continue
        for phase,field in (('before_read','reads'),('before_write','writes')):
            for ordinal in range(len(a[field])):
                candidate=copy.deepcopy(row)
                candidate.update(name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal))
                prefixes.append(candidate)
    stopped,report['prefix_authority']=observe(prefixes,scratch/'prefixes')
    retired=[];retired_cases=[]
    if with_observers:
        origins=[]
        for n,(row,a) in enumerate(zip(rows,normal['cases'],strict=True)):
            for ordinal,store in enumerate(a['writes']):
                candidate=copy.deepcopy(row)
                candidate.update(name=row['name']+f'-observer-{ordinal}',fault=dict(phase='after_write',ordinal=ordinal))
                retired.append(candidate);origins.append((n,store['instruction']))
        original_retired,report['retired_authority']=observe(retired,scratch/'retired')
        retired_cases=original_retired['cases']
        states=[[] for _ in rows]
        for row,a,(n,pc) in zip(retired,retired_cases,origins,strict=True):
            if a['completed'] or a['boundary']['instruction']!=pc:
                raise ValueError('Original middle-branch retired store changed')
            states[n].append(boundary_packet(a,row,pc))
            row.update(observer=True,observer_states=copy.deepcopy(states[n]))
        for row,packets in zip(rows,states,strict=True):row.update(observer=True,observer_states=packets)
    all_rows=rows+prefixes
    all_rows+=retired
    covered=set()
    for row,a in zip(all_rows,normal['cases']+stopped['cases']+retired_cases,strict=True):
        covered.update(a['executed_sites'])
        if 'fault' in row:pc=a['boundary']['instruction']
        elif 'continuation' in row:pc=row['continuation']
        else:pc=next(p for p in a['executed_sites'] if p in returns)
        if a['completed']==('fault' in row):
            raise ValueError('Actual middle branch stopped at the wrong boundary')
        for packet in a['calls']:
            if packet.pop('arguments')!=[]:
                raise ValueError('Unexpected original middle branch call arguments')
        row.update(expected=boundary_packet(a,row,pc),reads=a['reads'],writes=a['writes'],calls=a['calls'])
        for bank,expected in zip(row['banks'],a['banks'],strict=True):bank['after']=expected['after']
    if whole:
        oracle.ENTRIES=(ENTRY,)
        parent_sites={site['instruction'] for site in metadata(image)['sites']}
        report.update(parent_sites_reached=len(covered&parent_sites),missing_parent_sites=[hex(pc) for pc in sorted(parent_sites-covered)],
                      whole_returning_frame_exercised=True)
    report.update(original_returns=len(rows)-sum(joins.values()),child_returns=total,provider_prefixes=len(prefixes),
                  retired_store_prefixes=len(retired),covered_sites=len(covered),actual_ret_pcs=sorted(actual_rets),
                  actual_initial_maxima_joins=joins,actual_groups=groups,actual_remainders=remainders)
    return all_rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--observers',action='store_true');parser.add_argument('--semantic-mutants',action='store_true')
    parser.add_argument('--extended',action='store_true')
    parser.add_argument('--maximum',action='store_true')
    parser.add_argument('--high',action='store_true')
    parser.add_argument('--low',action='store_true')
    parser.add_argument('--combat',action='store_true')
    parser.add_argument('--whole',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    extended=args.extended or args.maximum
    if args.whole:frame,driver,controls=WHOLE_FRAME,WHOLE_DRIVER,WHOLE_CONTROLS
    elif args.combat:frame,driver,controls=COMBAT_FRAME,COMBAT_DRIVER,COMBAT_CONTROLS
    elif args.low:frame,driver,controls=LOW_FRAME,LOW_DRIVER,LOW_CONTROLS
    elif args.high:frame,driver,controls=HIGH_FRAME,HIGH_DRIVER,HIGH_CONTROLS
    elif args.maximum:frame,driver,controls=MAXIMUM_FRAME,MAXIMUM_DRIVER,MAXIMUM_CONTROLS
    elif extended:frame,driver,controls=EXTENDED_FRAME,EXTENDED_DRIVER,EXTENDED_CONTROLS
    else:frame,driver,controls=FRAME,DRIVER,CONTROLS
    paths=(frame,driver,'include/halo2/native_tu15_object_ratio_caller_middle_kernel.hpp',
           'src/gameplay/objects/native_tu15_object_ratio_caller_setup_frame.hpp',
           'src/gameplay/objects/native_tu15_object_ratio_caller_dispatch_frame.hpp',
           'src/gameplay/objects/native_tu15_object_record_ratio_frame.hpp',str(Path(__file__).relative_to(ROOT)))
    if extended:paths+=(FRAME,'include/halo2/native_tu15_object_ratio_caller_extended_kernel.hpp')
    if args.maximum:paths+=(EXTENDED_FRAME,'include/halo2/native_tu15_object_ratio_caller_maximum_kernel.hpp')
    if args.high:paths+=(FRAME,'include/halo2/native_tu15_object_ratio_caller_high_kernel.hpp',
                         'tools/native_object_ratio_high_fixtures.py')
    if args.low:paths+=(FRAME,EXTENDED_FRAME,MAXIMUM_FRAME,HIGH_FRAME,
        'include/halo2/native_tu15_object_ratio_caller_low_kernel.hpp',
        'include/halo2/native_tu15_object_parent_mode_kernel.hpp',
        'src/gameplay/weapons/native_tu15_object_parent_mode.hpp','tools/native_object_ratio_low_fixtures.py')
    if args.combat:paths+=(LOW_FRAME,FRAME,HIGH_FRAME,EXTENDED_FRAME,MAXIMUM_FRAME,
        'include/halo2/native_tu15_object_ratio_caller_combat_kernel.hpp','tools/native_object_ratio_combat_fixtures.py',
        'src/gameplay/objects/native_tu15_object_definition_mode_query_frame.hpp',
        'src/gameplay/objects/native_tu15_object_table_ratio_frame.hpp')
    if args.whole:paths+=(LOW_FRAME,FRAME,HIGH_FRAME,COMBAT_FRAME,EXTENDED_FRAME,MAXIMUM_FRAME,
        'tools/native_object_ratio_high_fixtures.py','tools/native_object_ratio_low_fixtures.py',
        'tools/native_object_ratio_combat_fixtures.py')
    before={name:sha(ROOT/name) for name in paths}
    report=dict(passed=False,production_bound=False,whole_parent_qualified=False,source_before=before,
                owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        if sha(args.xbe)!=PIN:
            raise ValueError('Wrong original TU1.5 image')
        rows=fixture(Xbe(args.xbe),scratch,report,args.observers,extended,args.maximum,args.high,args.low,args.combat,args.whole)
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name
            flags=['-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                   *('-I'+str(ROOT/tree) for tree in ('include','src','tests'))]
            if args.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']

            def compile_run(target,overlays=()):
                built=subprocess.run([compiler,*overlays,*flags,ROOT/driver,'-o',target],capture_output=True,text=True,timeout=600)
                if built.returncode:raise ValueError('Middle branch compile failed: '+built.stderr[-8000:])
                return subprocess.run([target,data],capture_output=True,text=True,timeout=1200)

            result=compile_run(binary)
            if result.returncode:raise ValueError('Middle branch comparison failed: '+(result.stdout+result.stderr)[-9000:])
            defects=[]
            if args.semantic_mutants:
                original=(ROOT/frame).read_text()
                for name,old,new in controls:
                    if original.count(old)!=1:raise ValueError('Missing middle branch defect seam: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);path=overlay/frame
                    path.parent.mkdir(parents=True);path.write_text(original.replace(old,new,1))
                    defect=compile_run(overlay/'bad',('-I'+str(overlay/'src'),))
                    diagnostic=(defect.stdout+defect.stderr)[-5000:]
                    if not defect.returncode or not any(text in diagnostic for text in ('differ','Ray-frame bank extent')):
                        raise ValueError('Middle branch defect survived: '+name+' '+diagnostic)
                    defects.append(dict(name=name,diagnostic=diagnostic))
            report['compilers'].append(dict(compiler=compiler,result=json.loads(result.stdout),compiled_defects=defects))
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        report['source_unchanged']=before=={name:sha(ROOT/name) for name in paths}
        report['passed']=report['passed'] and report['source_unchanged']
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
