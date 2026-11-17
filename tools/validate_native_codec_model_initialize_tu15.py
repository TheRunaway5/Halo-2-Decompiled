#!/usr/bin/env python3
"""Qualify private 2835B0 initialization against the unchanged TU1.5 owner."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import re
import shlex
import subprocess
import sys
from pathlib import Path

from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_codec_dispatch_tu15 import (
    ENTRIES as SELECTOR_ENTRIES, RETURNS as SELECTOR_RETURNS, source_owners as selector_owners,
)
from validate_native_codec_inverse_transform_source_tu15 import exact
from validate_native_codec_transform_chain_tu15 import OWNERS, ROOT, STOP, harness, sha

ENTRY, SECONDARY, NO_SECONDARY, JOIN, RET = 0x2835b0,0x28371e,0x2837c0,0x283815,0x283a3f
SELECTOR, BYPASS = 0x283942,0x283947
ENTRIES = (ENTRY,0x283570,0x283e10)
REGIONS = ((0x46a000,4096),(0x510000,4096),(0x56e000,4096),(0x61000000,4096),(0x62000000,8192))
TABLE_REGION = (0x48c000,4096)
FRAME = 'src/physics/runtime/native_tu15_codec_model_initialize_frame.hpp'
KERNEL = 'include/halo2/native_tu15_codec_model_initialize_kernel.hpp'
MODE_FRAME = 'src/physics/runtime/native_tu15_codec_model_mode_frame.hpp'
MODE_KERNEL = 'include/halo2/native_tu15_codec_model_mode_kernel.hpp'
SETUP_FRAME = 'src/physics/runtime/native_tu15_codec_model_selector_setup_frame.hpp'
SETUP_KERNEL = 'include/halo2/native_tu15_codec_model_selector_setup_kernel.hpp'
FIRST_SELECTOR_FRAME = 'src/physics/runtime/native_tu15_codec_model_first_selector_frame.hpp'
BODY = ('9df18d41ddcf5f384ff3b09ccaccf1d5f9280436ce670b9aca667eb7ab27a21c',
        '901776a499ec2595c2f6fe5d7ce63f80ea7914774a089896d86921a2c7b30f56')
RESOLVER = ('1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
            '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657')
DEFECTS = (
    ('accept-equal-weight','return !s_.flags.cf&&!s_.flags.zf;','return !s_.flags.cf;'),
    ('stale-count-before-spill', '''Base::push(0x2835cfu,g[5]);store(0x2835d0u,g[4]+44u,scalar_bits(1u));
        s_.xmm[0]=scalar(0x2835d6u,g[4]+44u);
        asm volatile("cvttss2si %1,%0":"=r"(g[0]):"x"(s_.xmm[0]));
        const auto count=std::uint16_t(load(0x2835e0u,g[7]+20u,2u));''',
     '''Base::push(0x2835cfu,g[5]);const auto count=std::uint16_t(load(0x2835e0u,g[7]+20u,2u));
        store(0x2835d0u,g[4]+44u,scalar_bits(1u));s_.xmm[0]=scalar(0x2835d6u,g[4]+44u);
        asm volatile("cvttss2si %1,%0":"=r"(g[0]):"x"(s_.xmm[0]));'''),
    ('missing-plane-clear','n<22u','n<21u'),
    ('wide-mode-formal','load(0x2835bbu,g[4]+32u,1u)','load(0x2835bbu,g[4]+32u,4u)'),
)
MODE_DEFECTS = (
    ('skip-late-mode-read','s_.flags.minus(load(0x2837c0u,g[4]+40u,1u),g[3]&255u,1u);',
     's_.flags.minus(g[2]&255u,g[3]&255u,1u);'),
    ('lost-first-secondary-ceiling','s_.flags.minus(g[0],g[1],4u);store(0x283759u,0x510f88u,g[1]);',
     's_.flags.minus(g[0],g[1],4u);'),
    ('reversed-secondary-subtract','subtract(s_.xmm[2],s_.xmm[0]);',
     'auto reversed=s_.xmm[0];subtract(reversed,s_.xmm[2]);s_.xmm[2]=reversed;'),
    ('wrong-primary-snap-index','store(0x283803u,0x510f44u,s_.gp[2]);',
     'store(0x283803u,0x510f44u,s_.gp[0]);'),
)
SETUP_DEFECTS = (
    ('wrong-default-duplicate-limit','if(s_.flags.zf||s_.flags.sf!=s_.flags.of)store(0x28385cu,0x510f3cu,g[2]);',
     'if(s_.flags.zf||s_.flags.sf!=s_.flags.of)store(0x28385cu,0x510f3cu,g[0]);'),
    ('omit-late-model-marker','s_.flags.minus(load(0x28389cu,g[7]+16u,1u),g[3]&255u,1u);',
     's_.flags.minus(0u,g[3]&255u,1u);'),
    ('unsigned-descriptor-offset','std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(load(pc,address,2u)))))',
     'load(pc,address,2u)'),
    ('zero-late-track-count','g[2]=s_.flags.plus(g[2],31u,4u);','g[2]=s_.flags.plus(0u,31u,4u);'),
)
FIRST_SELECTOR_DEFECTS = (
    ('wrong-call-return', 'Base::push(0x283942u,0x283947u);', 'Base::push(0x283942u,0x283942u);'),
    ('early-call-stack-publication', 'Base::push(0x283942u,0x283947u);',
     's_.gp[4]-=4u;Base::store(0x283942u,s_.gp[4],0x283947u,[]{});'),
    ('unretired-call-stack', 'Base::push(0x283942u,0x283947u);',
     'Base::store(0x283942u,s_.gp[4]-4u,0x283947u,[]{});'),
    ('field-entry-instead-of-tail', 'return {Boundary::tail_entry,tail.target};',
     'return {Boundary::tail_entry,field.target};'),
    ('call-on-bypass', 'if(setup::prepare_selector(selector)==setup::Boundary::bypassed)return {Boundary::bypassed,0x283947u};',
     'setup::prepare_selector(selector);'),
)


def owners():
    out={}
    for index,name in enumerate(OWNERS):
        text=(ROOT/name).read_text()
        body=exact(text,r'static void run_2835b0\(WorldState &s\) \{.*?\n\}')
        resolver=exact(text,r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}')
        pins=tuple(hashlib.sha256(x.encode()).hexdigest()for x in (body,resolver))
        if pins!=(BODY[index],RESOLVER[index]):raise ValueError('Exact 2835B0 owner tuple changed')
        out[name]=dict(body=pins[0],resolver=pins[1],sites=sorted({int(pc,16)for pc in
                re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)}))
    return out


def rows_for(image,smoke):
    rng=random.Random(ENTRY);rows=[]
    live=[(0x8000000000000000|rng.getrandbits(61)).to_bytes(8,'little').hex()+'ff3f'for _ in range(8)]
    constants=image.read(0x46a000,4096)
    threshold=int.from_bytes(image.read(0x46a0ec,4),'little')
    if threshold!=0x38d1b717:raise ValueError('Original 2835B0 compare constant changed')
    weights=(0,0x80000000,threshold-1,threshold,threshold+1,0x3f800000,0x7f800000,
             0xff800000,0x7fc12345,0x7f812345,0x80000001,1)
    inputs=(0xc0600000,0,0x3fc00000,0x41200000,0x7fc12345,0x7f812345,0x7f800000,0x4f000000,
            0xcf000000,0x3f800007,0x3f808005,0x80000000,0x80000001,1,0x40000000,0x40a00000)
    for profile in range(4 if smoke else 16):
        for mode in range(16):
            n=len(rows);sp=0x62000400+profile
            if mode==12:sp=0x56e020+profile
            if mode==13:sp=0x510f2c+profile
            root=sp+12 if mode in (14,15) else 0x61000100
            banks={base:bytearray(constants if base==0x46a000 else rng.randbytes(size)) for base,size in REGIONS}
            def put(address,value,width=4):
                for base,data in banks.items():
                    if base<=address<=base+len(data)-width:
                        data[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
                        return
                raise ValueError('Model initial formal left declared backing')
            for lane,value in enumerate((STOP,0x61411101,root,0x61333303,0x61555505,0x61666606,0x61777707,0x61888808,
                                         0 if profile%2 else 0x81,0x61999909,0x61aaaa0a)):
                put(sp+lane*4,value)
            put(root+20,(7,0,1,-4,32767,-32768,5,2)[profile%8],2)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            xmm[4][0]=weights[mode] if mode<len(weights) else 0x3f800000
            xmm[1][0]=inputs[profile] if mode not in (14,15) else (0x3f800007,0x3f808005)[mode-14]
            cw=(0x7f,0x27f,0x37f)[mode%3]|((profile%4)<<10)
            sw=((n%8)<<11)|((n<<7)&0x4700)|(n&0x3f)
            mx=0x1f80|((profile%4)<<13)|(n&0x3f)|(0,0x40,0x8000,0x8040)[mode%4]
            rows.append(dict(id=n,name=f'model-initialize-{mode}-{profile}',entry=ENTRY,gp=gp,xmm=xmm,
                matrix=0x61411101,source=root,
                eflags=0x202|(rng.getrandbits(12)&0xcd5),before_fp=[cw,sw,mx],live_x87=live[:(0,1,4,8)[mode%4]],
                max_steps=100,stop_before_entries=[SECONDARY,NO_SECONDARY],alias=mode if mode>=12 else 0,
                banks=[dict(address=base,before=data.hex())for base,data in banks.items()]))
    return rows


def mode_rows(image,smoke):
    rows=rows_for(image,smoke)
    for row in rows:row.update(stop_before_entries=[JOIN],max_steps=180)
    seed=rows[4]
    profiles=(
        ('late-mode-disabled',0,0x3fc00000,0,7,0),
        ('exact-floor',0,0x3f800000,0,7,1),
        ('near-floor',0,0x3f800001,0,7,1),
        ('near-ceiling',0,0x3f7fffe0,0,7,1),
        ('middle',0,0x3fc00000,0,7,1),
        ('last',0,0x40c00000,0,7,1),
        ('negative-count',0,0xc0600000,0,-4,1),
        ('primary-unordered',0,0x7fc12345,0,7,1),
        ('secondary-negative',1,0x3fc00000,0xc0600000,7,0),
        ('secondary-unordered',1,0x3fc00000,0xffc23456,7,0),
        ('secondary-above-last',1,0x3fc00000,0x41300000,7,0),
        ('secondary-valid',1,0x3fc00000,0x40000000,7,0),
        ('secondary-negative-count',1,0x40c00000,0x40000000,-4,0),
    )
    for profile in range(1 if smoke else 4):
        for name,mode,primary,secondary,count,late in profiles:
            row=copy.deepcopy(seed);row.update(id=len(rows),name=f'model-initialize-mode-{name}-{profile}',alias=0)
            row['xmm'][1][0]=primary;row['before_fp'][2]=0x1f80|(profile<<13)
            sp=row['gp'][4]
            def patch(address,value,width=4):
                for bank in row['banks']:
                    offset=address-bank['address'];data=bytearray.fromhex(bank['before'])
                    if 0<=offset<=len(data)-width:
                        data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
                        bank['before']=data.hex();return
                raise ValueError('Mode fixture escaped backing')
            patch(sp+28,late,1);patch(sp+32,mode,1);patch(sp+36,secondary);patch(sp+40,0x3f200000)
            patch(row['source']+20,count,2)
            rows.append(row)
    return rows


def selector_rows(image,smoke):
    table=image.read(*TABLE_REGION)
    rows=[row for row in mode_rows(image,smoke) if not row['alias']]
    seed=copy.deepcopy(rows[4])
    variants=(
        # Weight, type, override, original limit, signed descriptor offset,
        # track count, table row and backed feedback route.
        (0x3f000000,0,0,3,23,63,1,''),
        (0x3f800000,0,0,15,-23,96,2,''),
        (0x3f000000,1,0,3,23,255,3,''),
        (0x3f800000,1,1,15,23,1,4,''),
        (0x3f000000,2,2,3,23,15,5,''),
        (0x3f800000,2,1,15,23,31,6,''),
        (0x3f000000,0,2,3,0,47,7,''),
        (0x3f800000,0,1,15,23,32,8,''),
        (0x3f800000,0,2,3,-23,0,2,'late-tracks'),
        (0x3f000001,0,0,3,23,16,1,'late-marker'),
    )
    def bind(row,variant,directed=False):
        weight,kind,override,limit,offset,tracks,method,alias=variant
        row.update(stop_before_entries=[SELECTOR,BYPASS],max_steps=280,setup_alias=alias)
        row['banks'].insert(1,dict(address=TABLE_REGION[0],before=table.hex()))
        if directed:row['xmm'][4][0]=weight
        sp=row['gp'][4]
        stream,data,description=0x61000300,0x61000180,0x61000240
        if alias=='late-marker':row['source']=0x510f40
        if alias=='late-tracks':stream=0x510f58
        row['gp'][6]=stream
        def patch(address,value,width=4):
            for bank in row['banks']:
                at=address-bank['address'];data=bytearray.fromhex(bank['before'])
                if 0<=at<=len(data)-width:
                    data[at:at+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
                    bank['before']=data.hex();return
            raise ValueError('Selector fixture escaped backing')
        patch(sp+8,row['source']);patch(sp+12,limit);patch(sp+16,data if override else 0)
        patch(sp+20,description);patch(row['source']+16,kind,1);patch(row['source']+19,8,1)
        patch(data,method,1);patch(data+28,int(override==2),1)
        patch(description+6,offset,2);patch(description+12,32)
        if not alias or alias=='late-marker':
            patch(stream,data);patch(stream+4,description);patch(stream+8,tracks,1)
        row['setup_expected_offsets']=[data+offset+32+(16*n)for n in range(3)]if alias=='late-tracks'else []
    for i,row in enumerate(rows):
        bind(row,variants[i%8])
    for profile in range(1 if smoke else 4):
        for n,variant in enumerate(variants):
            row=copy.deepcopy(seed);row.update(id=len(rows),name=f'model-initialize-selector-{n}-{profile}')
            bind(row,variant,True)
            row['before_fp'][2]=0x1f80|(profile<<13)
            rows.append(row)
    for i,row in enumerate(rows):row['id']=i
    return rows


def first_selector_rows(image,smoke,tails):
    rows=selector_rows(image,smoke)
    seed=next(row for row in rows if row['name']=='model-initialize-selector-0-0')
    stop_entries=sorted({BYPASS,*(target for field in SELECTOR_ENTRIES[1:] for target in tails[field].values())})
    for profile in range(1 if smoke else 4):
        for format_index,field in enumerate(SELECTOR_ENTRIES[1:3]):
            for variant,target in enumerate(tails[field].values()):
                row=copy.deepcopy(seed)
                row.update(id=len(rows),name=f'model-initialize-first-call-{format_index}-{variant}-{profile}',
                           expected_tail_target=target)
                row['xmm'][4][0]=(0x3f800000,0x3f000000)[format_index]
                row['before_fp'][2]=0x1f80|(profile<<13)
                sp=row['gp'][4]

                def put(address,value,width=4):
                    for bank in row['banks']:
                        offset=address-bank['address'];data=bytearray.fromhex(bank['before'])
                        if 0<=offset<=len(data)-width:
                            data[offset:offset+width]=value.to_bytes(width,'little');bank['before']=data.hex();return
                    raise ValueError('First selector fixture escaped backing')

                # Fields 0 and 1 each have four wide/byte combinations.
                # Field 2 selects a further tail selector.
                field_mode=variant//4
                put(sp+16,0x61000180 if field_mode else 0)
                put(0x61000180+28,int(field_mode==2),1)
                put(sp+20,0x80340000 if variant%4<2 else 0)
                put(sp+32,0x80 if variant%2==0 else 0,1)
                rows.append(row)
    for row in rows:
        row.update(stop_before_entries=stop_entries,max_steps=360)
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    for option in ('smoke','prefixes','mmx','sanitizers','semantic-mutants'):parser.add_argument('--'+option,action='store_true')
    phase=parser.add_mutually_exclusive_group()
    phase.add_argument('--mode-join',action='store_true')
    phase.add_argument('--selector-setup',action='store_true')
    phase.add_argument('--first-selector-call',action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:parser.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','src','include','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    setup=args.selector_setup or args.first_selector_call
    mode_or_setup=args.mode_join or setup
    frame=FIRST_SELECTOR_FRAME if args.first_selector_call else SETUP_FRAME if setup else MODE_FRAME if args.mode_join else FRAME
    defects=FIRST_SELECTOR_DEFECTS if args.first_selector_call else SETUP_DEFECTS if setup else MODE_DEFECTS if args.mode_join else DEFECTS
    regions=tuple(sorted((*REGIONS,TABLE_REGION))) if setup else REGIONS
    used={ROOT/FRAME,ROOT/KERNEL,ROOT/frame,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
          ROOT/'tests/native_tu15_codec_transform_composition_tests.cpp'}
    if mode_or_setup:used.add(ROOT/MODE_KERNEL)
    if setup:used.add(ROOT/SETUP_KERNEL)
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                readonly_gp_simd_before_provider_domain=True,selector_or_other_child_called=args.first_selector_call,
                mode_join_283815=args.mode_join,selector_setup_before_call=args.selector_setup,
                first_selector_call=args.first_selector_call,unqualified_tail_owner_called=False)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');source=owners()
        parent_graph=_trace(code,ENTRY,set(ENTRIES))
        parent=set(parent_graph.code)
        initial={pc for pc in parent if pc<SECONDARY or pc>=0x283a3d}
        ceiling=SELECTOR+1 if args.first_selector_call else SELECTOR if setup else JOIN if args.mode_join else SECONDARY
        scope={pc for pc in parent if pc<ceiling or pc>=0x283a3d}
        # After the original initial clamp and unequal-index branch, the
        # primary index is nonnegative and at most signed16 maximum.
        impossible={0x2837d1,0x2837d7} if mode_or_setup else set()
        if len(parent)!=301 or len(initial)!=77 or not impossible<=scope or any(set(x['sites'])!=parent for x in source.values()):
            raise ValueError('Exact original and source initialization scope changed')
        if [(pc,code.at(pc).op_str)for pc in sorted(parent)if code.at(pc).mnemonic=='call']!=[
                (0x283942,'0x283570'),(0x28396c,'0x283e10'),(0x283a37,'0x283570')]:
            raise ValueError('Exact authentic downstream call frontier changed')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        observed_entries=(ENTRY,)
        selector_scope,expected_tails=set(),set()
        if args.first_selector_call:
            selector_source,tails=selector_owners()
            starts=set(SELECTOR_ENTRIES)|{target for transfers in tails.values() for target in transfers.values()}
            graphs={entry:_trace(code,entry,starts) for entry in SELECTOR_ENTRIES}
            for (_,entry),(_,sites) in selector_source.items():
                if sorted(graphs[entry].code)!=sites:
                    raise ValueError('Exact original first selector source sites differ')
            for entry,graph in graphs.items():
                actual={pc:int(code.at(pc).op_str,16) for pc in graph.code if code.at(pc).mnemonic=='jmp'}
                if actual!=tails[entry] or code.at(SELECTOR_RETURNS[entry]).mnemonic!='ret':
                    raise ValueError('Exact original selector tail or return changed')
            observed_entries+=(SELECTOR_ENTRIES[0],*SELECTOR_ENTRIES[1:3])
            selector_scope={pc for pc in graphs[SELECTOR_ENTRIES[0]].code if pc<=0x283583}
            for field in SELECTOR_ENTRIES[1:3]:
                selector_scope.update(set(graphs[field].code)-{SELECTOR_RETURNS[field]})
                expected_tails.update(tails[field].values())
            graphs[ENTRY]=parent_graph
            common.complete_body=lambda _,entry:graphs[entry]
            report['selector_source_pins']={f'{name}:{entry:x}':pin for (name,entry),(pin,_) in selector_source.items()}
        common.ENTRIES=observed_entries
        old=common.metadata
        def metadata(ref):
            result=old(ref)
            for site in result['sites']:
                pc=site['instruction']
                if pc in scope|selector_scope and code.at(pc).mnemonic in ('pop','ret'):
                    if site['operands']:raise ValueError('Unexpected explicit POP/RET operand')
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if setup and pc==0x28382c:
                    # Capstone leaves access unset on this TEST. The exact
                    # memory operand is read before the published flags.
                    ins=code.at(pc)
                    if bytes(ins.bytes)!=bytes.fromhex('84481c') or ins.op_str!='byte ptr [eax + 0x1c], cl' or len(site['operands'])!=1:
                        raise ValueError('Exact selector override TEST changed')
                    site['operands'][0].update(read=True,write=False)
            return result
        common.metadata=metadata
        trace,stores=[],[]
        for site in metadata(image)['sites']:
            pc=site['instruction']
            if pc not in scope|selector_scope:continue
            pushed=code.at(pc).mnemonic in ('push','call')
            if site['operands']or pushed:trace.append(dict(entry=pc,gp=True,xmm=True))
            if pushed or any(op['write']for op in site['operands']):stores.append(pc)
        rows=(first_selector_rows(image,args.smoke,tails) if args.first_selector_call else
              (selector_rows if setup else mode_rows if args.mode_join else rows_for)(image,args.smoke))
        normal=len(rows);proofs=[]
        def measure(batch,label):
            folder=scratch/label;folder.mkdir()
            opaque='active_mmx'in batch[0];runner=physical_original_mmx if opaque else physical_original_dense
            observed,proof=runner(image,batch,folder,entries=observed_entries,
                regions=regions+(((0x64000000,4096),)if opaque else()),trace_calls=trace,trace_extended=True)
            proofs.append(proof);return observed['cases']
        original=measure(rows,'normal')
        if args.mmx:
            rng=random.Random(ENTRY^0xff);opaque=[]
            opaque_seeds=rows
            if args.first_selector_call:
                first_call=next(i for i,case in enumerate(original) if SELECTOR in case['executed_sites'])
                opaque_seeds=[rows[first_call],*rows[:first_call],*rows[first_call+1:]]
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(opaque_seeds[mask%normal]);row.update(id=len(rows)+len(opaque),name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                opaque.append(row)
            rows+=opaque;original+=measure(opaque,'opaque')
        if args.prefixes:
            seen,pending=set(),[]
            for row,observed in zip(rows,original,strict=True):
                for phase,stream in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(observed[stream]):
                        pc=event['instruction'];signature=('active_mmx'in row,phase,pc)
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(name=row['name']+f'-{phase}-{pc:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
            for opaque in (False,True):
                batch=[row for row in pending if ('active_mmx'in row)==opaque]
                if not batch:continue
                for n,row in enumerate(batch,len(rows)):row['id']=n
                rows+=batch;original+=measure(batch,'prefix-'+str(opaque))
        covered,counts,aliases=set(),[0,0,0],{}
        snaps=dict(first=0,near=0,late_clear_alias=0)
        setup_aliases=dict(late_tracks=0,late_marker=0)
        tail_counts={}
        call_stops=[]
        for row,observed in zip(rows,original,strict=True):
            executed=set(observed['executed_sites'])
            covered.update(executed)
            if observed['completed']!=('phase'not in row):raise ValueError('Original codec model completion differs')
            if 'phase'in row:pc=observed['boundary']['instruction']
            else:
                pc=observed['stopped_at']
                if args.first_selector_call:
                    kind=0 if pc==STOP else 1 if pc==BYPASS else 2
                    if kind==2:
                        if pc not in expected_tails or SELECTOR not in executed or SELECTOR_ENTRIES[0] not in executed:
                            raise ValueError('Actual first selector did not reach a declared field tail')
                        if row.get('expected_tail_target',pc)!=pc:
                            raise ValueError('Actual first selector did not match the directed fixture route')
                        tail_counts[hex(pc)]=tail_counts.get(hex(pc),0)+1
                    elif SELECTOR in executed:
                        raise ValueError('The bypass cohort reached the original CALL')
                elif setup:
                    kind={STOP:0,BYPASS:1,SELECTOR:2}[pc]
                elif args.mode_join and pc==JOIN:
                    kind=2 if SECONDARY in observed['executed_sites']else 1
                    if kind==1 and NO_SECONDARY not in observed['executed_sites']:
                        raise ValueError('Missing actual no-secondary mode')
                else:kind={STOP:0,NO_SECONDARY:1,SECONDARY:2}[pc]
                row['boundary_kind']=kind;counts[kind]+=1
                if pc==STOP:pc=RET
                if row['alias'] and not kind:raise ValueError('Backed alias never entered the real initialization path')
                init_gp=observed['boundary_gp'] if not args.mode_join or not row['alias']else next(
                    call['gp'] for call in observed['calls'] if call['entry']==0x283712)
                if row['alias'] in (14,15) and init_gp[1]!=((6,0xffff8004)[row['alias']-14]):
                    raise ValueError('Actual count was not read from the backed spill')
                if row['alias']==12 and init_gp[5]!=0:
                    raise ValueError('Actual plane clear did not overwrite the late stack formal')
                if row['alias']:aliases[row['alias']]=aliases.get(row['alias'],0)+1
                if args.mode_join and kind==1:
                    snaps['first']+=0x283809 in executed and 0x283803 not in executed
                    snaps['near']+=0x283803 in executed
                    snaps['late_clear_alias']+=row['alias']==12 and 0x2837c0 in executed and 0x2837c6 not in executed
                if setup and row['setup_alias']:
                    if row['setup_alias']=='late-tracks':
                        bank=bytes.fromhex(next(b['after']for b in observed['banks']if b['address']==0x510000))
                        offsets=[int.from_bytes(bank[at:at+4],'little') for at in (0xf70,0xf74,0xf78)]
                        if kind!=2 or offsets!=row['setup_expected_offsets'] or not {0x2838f1,0x283916}<=executed:
                            raise ValueError('Actual late track alias did not feed both offsets')
                        setup_aliases['late_tracks']+=1
                    else:
                        if kind!=1 or not {0x283877,0x283894,0x28389c}<=executed or 0x2838a5 in executed:
                            raise ValueError('Actual late marker alias did not bypass the selector')
                        setup_aliases['late_marker']+=1
            if args.first_selector_call:
                writes=[write for write in observed['writes'] if write['instruction']==SELECTOR]
                stack_slot=row['gp'][4]-16
                if writes and (len(writes)!=1 or writes[0]['address']!=stack_slot or
                               writes[0]['written']!=BYPASS.to_bytes(4,'little').hex()):
                    raise ValueError('Actual first CALL did not push its genuine return word once')
                if 'phase'not in row and kind==2:
                    if len(writes)!=1 or observed['boundary_gp'][4]!=stack_slot:
                        raise ValueError('Actual selector tail did not retain the CALL stack word')
                if 'phase'in row and pc==SELECTOR:
                    after=row['phase']=='after_write'
                    if bool(writes)!=after or observed['boundary_gp'][4]!=stack_slot+(0 if after else 4):
                        raise ValueError('Original CALL interruption stack retirement changed')
                    call_stops.append(dict(phase=row['phase'],opaque='active_mmx'in row,
                        stack_pointer=observed['boundary_gp'][4],stack_slot=stack_slot,committed_return=after))
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],
                fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            if 'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for packet in row['calls']:
                if packet.pop('arguments'):raise ValueError('Unexpected codec model trace arguments')
            for bank,after in zip(row['banks'],observed['banks'],strict=True):
                if bank['address']!=after['address']:raise ValueError('Codec model fixture bank order changed')
                bank['after']=after['after']
        required_aliases=set() if setup else {12,13,14,15}
        expected_coverage=(scope-impossible)|selector_scope
        if covered!=expected_coverage or min(counts)==0 or set(aliases)!=required_aliases:
            raise ValueError('Original codec model path coverage incomplete: '+str(sorted(expected_coverage-covered))+
                             '; unexpected: '+str(sorted(covered-expected_coverage)))
        if args.mode_join and min(snaps.values())==0:raise ValueError('Actual mode witnesses did not reach both snaps and late alias')
        if setup and min(setup_aliases.values())==0:raise ValueError('Missing actual late selector aliases')
        if args.first_selector_call:
            if {int(target,16) for target in tail_counts}!=expected_tails:
                raise ValueError('Missing original first selector tail routes')
            if args.prefixes:
                expected_stops={(phase,opaque) for phase in ('before_write','after_write') for opaque in ((False,True) if args.mmx else (False,))}
                if {(row['phase'],row['opaque']) for row in call_stops}!=expected_stops:
                    raise ValueError('Missing actual first CALL pre-write and retired prefixes')
        cpp=harness(scratch)
        text=cpp.read_text().replace('native_tu15_codec_transform_chain_frame.hpp',Path(frame).name)
        namespace='first_selector' if args.first_selector_call else 'selector_setup' if setup else 'mode' if args.mode_join else 'initialize'
        text=text.replace('native_codec_transform_chain_detail','native_codec_model_'+namespace+'_detail')
        text=text.replace('return pc==0x283eb0u||pc==0x12058eu||pc==0x1205a3u;','(void)pc;return false;')
        if args.first_selector_call:
            text=text.replace('unsigned stopped{};', 'unsigned stopped{},stop_instruction{};')
            line='namespace compose=halo2::native_codec_model_first_selector_detail;'
            text=text.replace(line,line+'''\n                const auto record_stop=[&](compose::Stop stop) {
                    stopped=unsigned(stop.kind);stop_instruction=stop.instruction;
                };''')
            text=text.replace('stopped=unsigned(compose::run_private_prefix(', 'record_stop(compose::run_private_prefix(')
            line='const auto actual=packet(state,row.at("expected").at("entry"));'
            text=text.replace(line,'''if(!interrupted)check(stop_instruction==row.at("expected").at("entry"),
                name+": incorrect first selector boundary address");\n            '''+line)
        cpp.write_text(text)
        fixture=scratch/'fixture.json';mutants=scratch/'mutant-fixture.json'
        fixture.write_text(json.dumps(dict(cases=rows,write_sites=stores),separators=(',',':')))
        negative=rows[:16]+[row for row in rows if ('mode-'in row['name']or 'selector-'in row['name'])and row['name'].endswith('-0')]
        if args.first_selector_call:
            negative.extend(row for row in rows if 'first-call-'in row['name'] and row['name'].endswith('-0'))
            negative.extend(row for row in rows if row['name'].endswith('-write-283942'))
        mutants.write_text(json.dumps(dict(cases=negative,write_sites=stores),separators=(',',':')))
        report.update(physical_paths=len(rows),original_parent_sites=len(covered&parent),normal_paths=normal,
            fault_paths=sum('phase'in row for row in rows),opaque_paths=sum('active_mmx'in row for row in rows),
            boundaries=(dict(returned=counts[0],bypassed=counts[1],tail_entry=counts[2]) if args.first_selector_call else
                dict(returned=counts[0],bypassed=counts[1],before_selector=counts[2]) if setup else
                dict(returned=counts[0],without_secondary=counts[1],with_secondary=counts[2])),
            actual_backed_aliases=aliases,source_owners=source,original=proofs,generated_harness_sha256=sha(cpp),
            mode_observations=snaps,setup_alias_observations=setup_aliases,unreachable_after_original_clamp=sorted(impossible))
        if args.first_selector_call:
            report.update(selector_sites=len(covered&selector_scope),selector_tail_counts=tail_counts,call_interruption_witnesses=call_stops)
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(cpp),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:cmd.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            def run(build,probe,input_path):
                done=subprocess.run(build,capture_output=True,text=True,timeout=480)
                if done.returncode:raise ValueError('Codec model compile failed: '+done.stderr[-12000:])
                return subprocess.run([str(probe),str(input_path)],capture_output=True,text=True,timeout=480)
            done=run(cmd,binary,fixture)
            if done.returncode:raise ValueError('Codec model original comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[]);report['compilers'].append(result)
            print(f'{compiler} full codec model initialization comparison passed',flush=True)
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            if args.semantic_mutants:
                for name,old,new in defects:
                    text=(ROOT/frame).read_text()
                    if text.count(old)!=1:raise ValueError('Codec model defect seam changed: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);header=overlay/frame
                    header.parent.mkdir(parents=True);header.write_text(text.replace(old,new,1));probe=overlay/'probe'
                    build=[cmd[0],'-I'+str(overlay/'src'),*cmd[1:]]
                    build[build.index('-o')+1]=str(probe);build[build.index('-MF')+1]=str(overlay/'probe.d')
                    done=run(build,probe,mutants);diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'model-initialize-'not in diagnostic:
                        raise ValueError('Codec model defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected codec model defect {name}',flush=True)
        if owners()!=source:raise ValueError('Exact original codec model source tuple drift')
        if args.first_selector_call and selector_owners()!=(selector_source,tails):
            raise ValueError('Exact original selector source tuples drifted')
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        used.update(Path(module.__file__).resolve()for module in tuple(sys.modules.values())
            if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):before[path]for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name)for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
