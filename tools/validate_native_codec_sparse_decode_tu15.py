#!/usr/bin/env python3
"""Compare private 284D30 sparse decoding with authentic row-8 or row-1 callees."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import re
import shlex
import struct
import subprocess
import sys
from pathlib import Path

from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_codec_dispatch_tu15 import source_owners as selector_owners
from validate_native_codec_inverse_transform_source_tu15 import exact
from validate_native_codec_model_initialize_tu15 import (
    BYPASS, ENTRY as PARENT, SELECTOR, TABLE_REGION, first_selector_rows, owners as parent_owners, rows_for,
)
from validate_native_codec_transform_chain_tu15 import ROOT, STOP, original_owners as transform_owners, sha

ENTRY = 0x284d30
RETURN = 0x284edc
TARGETS = (0x2963d0,0x296410,0x2cec60)
PACKED_TARGETS = (0x296180,0x2961f0,0x296220)
COPY_RETURNS = (0x29640b,0x29644a,0x2cec86)
PACKED_RETURNS = (0x2961e7,0x29621e,0x29623c)
PACKED_ESTIMATE = 0x2961db
SELECTORS = (0x283570,0x2840d0)
CALLS = (0x284da7,0x284e27,0x284ea7)
CONTINUATIONS = (0x284dad,0x284e2d,0x284ead)
REGIONS = ((0x46a000,4096),TABLE_REGION,(0x510000,4096),(0x56e000,4096),(0x61000000,4096),(0x62000000,8192))
FRAME = 'src/physics/runtime/native_tu15_codec_sparse_decode_frame.hpp'
KERNEL = 'include/halo2/native_tu15_codec_sparse_decode_kernel.hpp'
SECONDARY_FRAME = 'src/physics/runtime/native_tu15_codec_model_secondary_data_frame.hpp'
TRANSFORM_FRAME = 'src/physics/runtime/native_tu15_codec_model_secondary_transform_frame.hpp'
PARENT_EPILOGUE = (0x283947,0x28394a,0x28394d,0x283a3c,0x283a3d,0x283a3e,0x283a3f)
PARENT_TRANSFORM_CALL, TRANSFORM_ENTRY, PARENT_TRANSFORM_JOIN, SECOND_SELECTOR = 0x28396c,0x283e10,0x283971,0x283a37
TEST = 'tests/native_tu15_codec_transform_composition_tests.cpp'
SOURCE = 'src/physics/physics_live_support_tu15_generated.cpp'
INLINE = 'src/campaign/source_campaign_birth_query_variants_tu15_generated.cpp'
SOURCE_SHA = '2ecd97dc19b1a8b704cbacc7f16ee540ca33d4c7b5f2108594d244fc31a63a06'
INLINE_SHA = 'cc71375dbe7d33ea9ff80c33b1ad1c509a4a13a7e2318470b80d45365637d78d'
SUFFIX_SHA = '413af934d422f3a73cbb55fe7e8065c0ff31e3d1fc43b8efec2b6a39ee2db1fe'
DEFECTS = (
    ('wrong-return-word','Base::store(call,stack_slot,returned,[&]{gp[4]=stack_slot;},true);',
     'Base::store(call,stack_slot,call,[&]{gp[4]=stack_slot;},true);'),
    ('early-stack-publication','Base::store(call,stack_slot,returned,[&]{gp[4]=stack_slot;},true);',
     'gp[4]=stack_slot;Base::store(call,stack_slot,returned,[]{},true);'),
    ('cached-child-output','gp[0]=load(site(lane,0x4cu),0x510fa0u);',
     'gp[0]=gp[0];'),
    ('wrong-bitmap-output-stride','gp[0]=s_.flags.plus(gp[0],256u,4u);',
     'gp[0]=s_.flags.plus(gp[0],32u,4u);'),
    ('wrong-post-child-counter','store(site(lane,0x47u),0x510f94u+lane*4u,gp[0]);',
     'store(site(lane,0x47u),0x510f94u,gp[0]);'),
)
SECONDARY_DEFECTS = (
    ('drop-live-descriptor-reload','gp[0]=word(gp[6]+4u);','gp[0]=gp[6];'),
    ('skip-secondary-length-load','const auto length=word(gp[0]+12u);','const auto length=0u;'),
    ('omit-saved-ebp-pop','gp[5]=Base::pop(0x283a3cu);','gp[5]=gp[5];'),
    ('wrong-ret28-cleanup','gp[4]+=40u;','gp[4]+=36u;'),
)
TRANSFORM_DEFECTS = (
    ('wrong-transform-return','Base::push(0x28396cu,0x283971u);','Base::push(0x28396cu,0x28397fu);'),
    ('wrong-second-table-base','gp[1]=0x48c33cu+gp[0]+gp[1]*4u;','gp[1]=0x48c33cu+gp[0]+gp[1]*4u+4u;'),
    ('wrong-third-stream','store(0x283a2bu,0x510f78u,gp[1]);','store(0x283a2bu,0x510f78u,gp[1]+4u);'),
    ('wrong-second-return','Base::push(0x283a37u,0x283a3cu);','Base::push(0x283a37u,0x283947u);'),
)


def original_owners(code):
    standalone=exact((ROOT/SOURCE).read_text(),r'static void run_284d30\(WorldState &s\) \{.*?\n\}')
    caller=exact((ROOT/INLINE).read_text(),r'static void run_2849f0\(WorldState &s\) \{.*?\n\}')
    start='if(s.services.instruction){s.services.instruction(0x284d30u);}'
    suffix=standalone[standalone.index(start):]
    if (hashlib.sha256(standalone.encode()).hexdigest()!=SOURCE_SHA or
        hashlib.sha256(caller.encode()).hexdigest()!=INLINE_SHA or
        hashlib.sha256(suffix.encode()).hexdigest()!=SUFFIX_SHA or suffix!=caller[caller.index(start):]):
        raise ValueError('Complete sparse decoder source or byte-identical embedded suffix changed')
    starts={ENTRY,0x2849f0,*TARGETS}
    graph=_trace(code,ENTRY,starts)
    inline=_trace(code,0x2849f0,starts)
    expected={int(pc,16)for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',suffix)}
    calls={pc:code.at(pc).op_str for pc in graph.code if code.at(pc).mnemonic=='call'}
    if (len(graph.code)!=141 or set(graph.code)!=expected or set(inline.code)&expected!=expected or
        len(inline.code)!=339 or calls!={pc:f'dword ptr [0x{0x510f64+4*i:x}]'for i,pc in enumerate(CALLS)} or
        code.at(RETURN).mnemonic!='ret'):
        raise ValueError('Exact sparse decoder physical owner or call sites changed')
    return graph


def patch(row,address,value,width=4):
    for bank in row['banks']:
        offset=address-bank['address'];data=bytearray.fromhex(bank['before'])
        if 0<=offset<=len(data)-width:
            data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
            bank['before']=data.hex();return
    raise ValueError(f'Sparse decoder fixture escaped backing at {address:#x}')


def fixtures(image,smoke,packed,parent_cold,parent_secondary):
    targets=PACKED_TARGETS if packed else TARGETS
    table=image.read(0x48c364 if packed else 0x48c47c,40)
    if struct.unpack('<3I',table[:12])!=targets:
        raise ValueError('Exact original callback table changed')
    original_table=image.read(*TABLE_REGION)
    seeds=rows_for(image,smoke)
    rng=random.Random(ENTRY)
    rows=[]
    data=0x61000180
    bitmaps=(0x61000600,0x61000640,0x61000680)
    output=0x61000a00
    limits=(0,-3,1,8,9,15,16,17)
    masks=(0x00,0x01,0x81,0x55,0xaa,0xff,0x80,0x7e)
    def fill_packed(row,profile,track_rows=21,clip=data):
        row.update(live_x87=[],packed_rows=True)
        row['before_fp'][1]&=~0x3800
        patch(row,clip+12,0x110);patch(row,clip+16,0x240)
        for n in range(track_rows):
            for lane,value in enumerate((2+n,0x8000+profile,0xfff0+n,0x7fff-n)):
                patch(row,clip+32+n*8+lane*2,value,2)
            for word in range(3):patch(row,clip+0x110+n*12+word*4,rng.getrandbits(32))
            patch(row,clip+0x240+n*4,rng.getrandbits(32))
    def fill_copy_parent(row,clip):
        for offset,value in ((12,0x110),(16,0x240),(20,0x20),(24,0x14),(28,4)):
            patch(row,clip+offset,value)
        for n in range(4):
            for word in range(4):patch(row,clip+32+n*0x20+word*4,rng.getrandbits(32))
            for word in range(3):patch(row,clip+0x110+n*0x14+word*4,rng.getrandbits(32))
            patch(row,clip+0x240+n*4,rng.getrandbits(32))
    for profile in range(0 if parent_cold or parent_secondary else 3 if smoke else 8):
        for mode,limit in enumerate(limits):
            row=copy.deepcopy(seeds[(profile*16+mode)%len(seeds)])
            row.update(id=len(rows),name=f'sparse-decode-direct-{profile}-{mode}',entry=ENTRY,source=0,matrix=0,
                       max_steps=3800,stop_before_entries=[],via_parent=False)
            row['banks'].insert(1,dict(address=TABLE_REGION[0],before=original_table.hex()))
            sp=row['gp'][4]
            for n,value in enumerate((STOP,0,0)):patch(row,sp+4*n,value)
            frame=(0,1,2,3)[profile%4]
            for index,value in enumerate((limit,0,frame)):patch(row,0x510f3c+index*4,value)
            patch(row,0x510f60,data);patch(row,0x510f7c,output+(0 if packed else (profile&3)*4))
            for lane,target in enumerate(targets):
                patch(row,0x510f64+lane*4,target)
                patch(row,0x510f70+lane*4,bitmaps[lane])
                for n in range(3):
                    patch(row,bitmaps[lane]+n,masks[(mode+profile+lane+n)%len(masks)],1)
            # Each selected track has real row data. The sparse decoder
            # moves the output cursor independently of the decoded count.
            if packed:fill_packed(row,profile)
            else:
                for offset,value in ((12,0x260),(16,0x380),(20,0x20),(24,0x14),(28,4)):
                    patch(row,data+offset,value)
                for n in range(21):
                    for word in range(4):patch(row,data+(frame+2)*16+n*0x20+word*4,rng.getrandbits(32))
                    for word in range(3):patch(row,data+0x260+frame*12+n*0x14+word*4,rng.getrandbits(32))
                    patch(row,data+0x380+(frame+n)*4,rng.getrandbits(32))
            rows.append(row)

    _,tails=selector_owners()
    parent_seed=next(row for row in first_selector_rows(image,True,tails) if row.get('expected_tail_target')==ENTRY)
    profiles=2 if smoke else 8
    if parent_cold and not packed:profiles+=1
    for profile in range(profiles):
        row=copy.deepcopy(parent_seed)
        row.update(id=len(rows),name=f'sparse-decode-parent-{profile}',via_parent=True,stop_before_entries=[BYPASS],max_steps=3800)
        sp=row['gp'][4]
        description=0x61000240
        if parent_cold or parent_secondary:
            stack=next(bank for bank in row['banks']if bank['address']==0x62000000)
            seed=bytes.fromhex(stack['before']);offset=sp-stack['address']
            moved=sp+profile
            for at in range(0,44,4):patch(row,moved+at,int.from_bytes(seed[offset+at:offset+at+4],'little'))
            row['gp'][4]=sp=moved
            row.update(parent_cold=parent_cold,parent_secondary=parent_secondary,stop_before_entries=[])
        patch(row,sp+12,8);patch(row,sp+24,output)
        patch(row,data,1 if packed else 8,1)
        patch(row,description+6,0x500 if parent_secondary else 0x340 if parent_cold else 0x300,2)
        patch(row,description+12,0x180 if parent_secondary else 0 if parent_cold else 0x40)
        patch(row,row['gp'][6]+8,16,1)
        stream=data+(0x680 if parent_secondary else 0x340)
        for lane,mask in enumerate((0x81,0x15,0x42)):
            patch(row,stream+lane*4,mask,1)
        if packed:fill_packed(row,profile,4)
        else:
            fill_copy_parent(row,data)
            if parent_cold and profile==profiles-1:
                # The first translation word becomes the descriptor's live
                # secondary length. Rotation and scale use later tracks.
                row['live_secondary_alias']=description+12
                patch(row,sp+24,description-4)
                patch(row,description+6,0x300,2);patch(row,description+12,0x40)
                for lane,mask in enumerate((0x02,0x01,0x04)):patch(row,stream+lane*4,mask,1)
                patch(row,data+0x110,0)
        if parent_secondary:
            record_count=profile%2
            row['transform_record_count']=record_count
            patch(row,row['source']+100,record_count)
            patch(row,row['source']+104,0x610009c0);patch(row,0x610009c0,-1,2)
            patch(row,sp+16,0)
            patch(row,sp+28,1 if packed and profile%2 else 0,1)
            if packed and profile%2:row['xmm'][1][0]=0x3fc00000
            bias=(-32,32)[profile%2]
            patch(row,description,bias,1)
            second=data+0x500
            patch(row,second,1 if packed else 8,1)
            if packed:fill_packed(row,profile+8,4,second)
            else:fill_copy_parent(row,second)
            for lane,mask in enumerate((0x24,0x81,0x09)):patch(row,stream+bias+lane*4,mask,1)
        rows.append(row)
    return rows


def comparison_source(scratch):
    text=(ROOT/TEST).read_text()
    for old,new in (
        ('#include "physics/runtime/native_tu15_codec_transform_composition_frame.hpp"',
         '#include "physics/runtime/native_tu15_codec_model_secondary_transform_frame.hpp"'),
        ('return pc==0x12058eu||pc==0x1205a3u;', 'return pc==0x284da7u||pc==0x284e27u||pc==0x284ea7u;'),
        ('if(repeat(pc)){packets.erase(packets.size()-1);after_repeat_read=false;}', '''if(repeat(pc)) {
            // The direct hardware trace has one packet per instruction.
            // Both provider calls must see that same pre-retirement ABI.
            check(packets.size()>=2&&packets.back()==packets[packets.size()-2],
                  "Indirect CALL provider pre-write ABI differs");
            packets.erase(packets.size()-1);
            after_repeat_read=false;
        }'''),
        ('if(memory.pc==0x1205b2u)++returned_product;',
         'if(memory.pc==0x284dadu||memory.pc==0x284e2du||memory.pc==0x284eadu)++returned_product;'),
        ('            State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};', '''
            unsigned estimates{};
            if(row.value("packed_rows",false))services.reciprocal_sqrt=[&](std::uint32_t bits) {
                if(estimates%4u==0u)memory.packets.push_back(memory.capture());
                else check(memory.capture()==memory.packets.back(),"Sparse decoder estimate lanes changed ABI");
                ++estimates;
                auto value=std::bit_cast<float>(bits);
                asm volatile("rsqrtss %1,%0":"=x"(value):"x"(value):"memory");
                return std::bit_cast<std::uint32_t>(value);
            };
            State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};'''),
        ('''namespace compose=halo2::native_codec_transform_composition_detail;
                if(use_default)compose::run_private(state);
                else compose::run_private(state,[&](auto&,auto pc){
                    check(memory.pc==pc,"Composition cursor differs");if(noisy)hostile();
                });''', '''namespace decoder=halo2::native_codec_sparse_decode_detail;
                const auto execute=[&](auto before) {
                    if(row.at("via_parent").get<bool>()) {
                        const auto boundary=halo2::native_codec_model_first_selector_detail::run_private_prefix(state,before);
                        check(boundary.instruction==0x284d30u,name+": sparse decoder parent tail differs");
                    }
                    decoder::run_private(state,before);
                    if(row.value("parent_cold",false)) {
                        namespace secondary=halo2::native_codec_model_secondary_data_detail;
                        const auto boundary=secondary::run_private(state,before);
                        check(boundary==secondary::Boundary::returned,name+": sparse decoder parent did not retire RET28");
                    }
                    if(row.value("parent_secondary",false)) {
                        namespace secondary=halo2::native_codec_model_secondary_transform_detail;
                        const auto stop=secondary::run_private(state,before);
                        check(stop.kind==secondary::Boundary::returned&&stop.instruction==0x283a3fu,
                              name+": secondary parent did not retire RET28");
                    }
                };
                const bool transform_stop=row.value("before_transform_continuation",false);
                if(use_default&&!transform_stop)execute(decoder::Noop{});
                else execute([&](auto&,auto pc){
                    check(memory.pc==pc,"Sparse decoder cursor differs");if(noisy)hostile();
                    if(transform_stop&&pc==0x283971u)throw Fault{};
                });'''),
        ('            check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": composition ordered I/O differs");', '''
            if(row.value("packed_rows",false)) {
                const auto& calls=row.at("calls");
                const auto hardware_estimates=std::count_if(calls.begin(),calls.end(),[](const auto& call) {
                    return call.at("entry")==0x2961dbu;
                });
                check(estimates==unsigned(hardware_estimates)*4u,name+": sparse decoder estimate count differs");
                J stores=J::array();
                for(unsigned n=0;n<memory.writes.size();++n) {
                    auto current=memory.writes[n];
                    if(current.at("instruction")==0x2961e1u) {
                        check(n+1<memory.writes.size(),name+": sparse decoder missing retained second vector half");
                        const auto& next=memory.writes[++n];
                        check(next.at("instruction")==current.at("instruction")&&
                              next.at("address").get<std::uint32_t>()==current.at("address").get<std::uint32_t>()+8u,
                              name+": sparse decoder retained vector half order differs");
                        current["before"]=current.at("before").get<std::string>()+next.at("before").get<std::string>();
                        current["written"]=current.at("written").get<std::string>()+next.at("written").get<std::string>();
                    }
                    stores.push_back(std::move(current));
                }
                memory.writes=std::move(stores);
                J packets=J::array();
                for(unsigned n=0;n<memory.packets.size();++n) {
                    const auto& current=memory.packets[n];
                    if(current.at("entry")==0x2961e1u) {
                        check(n+1<memory.packets.size()&&current==memory.packets[n+1],
                              name+": sparse decoder retained vector halves changed ABI");
                        ++n;
                    }
                    packets.push_back(current);
                }
                memory.packets=std::move(packets);
            }
            if(memory.reads!=row.at("reads")||memory.writes!=row.at("writes")) {
                for(const auto* key:{"reads","writes"}) {
                    const auto& actual=std::string_view(key)=="reads"?memory.reads:memory.writes;
                    const auto& expected=row.at(key);
                    if(actual==expected)continue;
                    std::cerr<<key<<" counts "<<expected.size()<<"/"<<actual.size()<<'\\n';
                    for(unsigned n=0;n<std::min(expected.size(),actual.size());++n)if(expected[n]!=actual[n]) {
                        std::cerr<<"expected "<<expected[n].dump()<<" actual "<<actual[n].dump()<<'\\n';break;
                    }
                }
                throw std::runtime_error(name+": sparse decoder ordered I/O differs");
            }'''),
        ('name+": composition', 'name+": sparse decoder'),
    ):
        if old not in text:raise ValueError('Private sparse decoder comparator seam changed')
        text=text.replace(old,new)
    path=scratch/'sparse_decode_compare.cpp';path.write_text(text);return path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    for name in ('smoke','prefixes','mmx','sanitizers','semantic-mutants','row1','parent-cold','parent-secondary'):
        parser.add_argument('--'+name,action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:parser.error('Use tools/run_test.py')
    if args.row1 and args.mmx:parser.error('Row 1 uses the dense empty input domain of the packed child composition')
    if args.parent_cold and args.parent_secondary:parser.error('Choose one parent continuation')
    if platform.system()!='Linux'or platform.machine().lower()not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','src','include','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/FRAME,ROOT/KERNEL,ROOT/TEST,ROOT/SECONDARY_FRAME,ROOT/TRANSFORM_FRAME,
          Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    targets=PACKED_TARGETS if args.row1 else TARGETS
    child_ret_sites=PACKED_RETURNS if args.row1 else COPY_RETURNS
    mutant_subject=TRANSFORM_FRAME if args.parent_secondary else SECONDARY_FRAME if args.parent_cold else FRAME
    defects=TRANSFORM_DEFECTS if args.parent_secondary else SECONDARY_DEFECTS if args.parent_cold else DEFECTS
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                original_codec_table_row=1 if args.row1 else 8,actual_indirect_children=[hex(t)for t in targets],
                original_source_owner=dict(body=SOURCE_SHA,campaign_2849f0=INLINE_SHA,shared_suffix=SUFFIX_SHA),
                after_target_read_before_call_write_qualified=False,packed_vector_half_failure_qualified=False,
                parent_returns_after_zero_secondary_length=args.parent_cold,
                parent_returns_after_real_secondary_transform=args.parent_secondary)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');decoder=original_owners(code)
        parent=parent_owners()
        selector_source,tails=selector_owners()
        entries=(PARENT,*SELECTORS,ENTRY,*targets)
        if args.parent_secondary:entries+=(TRANSFORM_ENTRY,)
        starts=set(entries)|{p for routes in tails.values()for p in routes.values()}|{TRANSFORM_ENTRY,0x120570,0x120740}
        graphs={entry:_trace(code,entry,starts)for entry in entries};graphs[ENTRY]=decoder
        if args.parent_secondary:
            transform_sources=transform_owners()
            if set(graphs[TRANSFORM_ENTRY].code)!=set(transform_sources[SOURCE]):
                raise ValueError('Original secondary transform source and physical owner differ')
            used.update(ROOT/name for name in transform_sources)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=entries;common.complete_body=lambda _,entry:graphs[entry]
        original_metadata=common.metadata
        def metadata(ref):
            result=original_metadata(ref)
            for site in result['sites']:
                pc=site['instruction'];ins=code.at(pc)
                if ins.mnemonic in ('pop','ret'):
                    if site['operands']:raise ValueError('Unexpected explicit decoder POP/RET operand')
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if pc==0x28382c:
                    if bytes(ins.bytes)!=bytes.fromhex('84481c') or len(site['operands'])!=1:
                        raise ValueError('Exact parent override TEST changed')
                    site['operands'][0].update(read=True,write=False)
            return result
        common.metadata=metadata
        trace,stores=[],[]
        for site in metadata(image)['sites']:
            pc=site['instruction'];pushed=code.at(pc).mnemonic in ('push','call')
            if site['operands']or pushed or (args.row1 and pc==PACKED_ESTIMATE):trace.append(dict(entry=pc,gp=True,xmm=True))
            if pc not in CALLS and (pushed or any(op['write']for op in site['operands'])):stores.append(pc)

        rows=fixtures(image,args.smoke,args.row1,args.parent_cold,args.parent_secondary);normal=len(rows);proofs=[]
        def measure(batch,label):
            folder=scratch/label;folder.mkdir()
            opaque='active_mmx'in batch[0];runner=physical_original_mmx if opaque else physical_original_dense
            observed,proof=runner(image,batch,folder,entries=entries,
                regions=REGIONS+(((0x64000000,4096),)if opaque else()),trace_calls=trace,trace_extended=True)
            proofs.append(proof);return observed['cases']
        original=measure(rows,'normal')
        if args.mmx:
            rng=random.Random(ENTRY^0xff);opaque=[]
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(id=len(rows)+len(opaque),name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                opaque.append(row)
            rows+=opaque;original+=measure(opaque,'opaque')
        if args.prefixes:
            seen,pending=set(),[]
            for row,observed in zip(rows,original,strict=True):
                opaque='active_mmx'in row
                if args.parent_secondary:
                    signature=(opaque,'transform_return',row['transform_record_count'])
                    if signature not in seen:
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(name=row['name']+'-before-transform-return-continuation',
                                      phase='before_transform_continuation',before_transform_continuation=True,
                                      stop_before_entries=[PARENT_TRANSFORM_JOIN])
                        pending.append(prefix)
                for phase,stream in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    if args.row1 and phase!='before_read':continue
                    second_pass=False
                    for ordinal,event in enumerate(observed[stream]):
                        pc=event['instruction']
                        if pc in (0x2839ad,SECOND_SELECTOR):second_pass=True
                        # The physical single-step prefix ends before the
                        # whole indirect CALL. It cannot represent a failed
                        # stack write after the separate target read.
                        if phase=='before_write'and pc in CALLS:continue
                        parent_stop=row['via_parent'] and phase=='before_read' and pc in CONTINUATIONS
                        signature=(opaque,phase,pc,parent_stop,second_pass and parent_stop)
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(name=row['name']+f'-{phase}-{pc:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        if args.parent_secondary and parent_stop:prefix['second_decoder_pass']=second_pass
                        pending.append(prefix)
            for opaque in (False,True):
                batch=[row for row in pending if ('active_mmx'in row)==opaque]
                if not batch:continue
                for n,row in enumerate(batch,len(rows)):row['id']=n
                rows+=batch;original+=measure(batch,'prefix-'+str(opaque))

        coverage=set();parent_coverage=set();complete_direct=0;complete_parent=0
        child_returns=[0,0,0];continuation_stops=[];call_stops=[];secondary_aliases=0
        transform_stops=[];transform_returns_by_record_count=[0,0]
        secondary_sites=set(range(PARENT_TRANSFORM_JOIN,SECOND_SELECTOR+1))&set(graphs[PARENT].code)
        secondary_sites.update(pc for pc in graphs[PARENT].code if PARENT_EPILOGUE[0]<=pc<PARENT_TRANSFORM_JOIN)
        secondary_sites.update(PARENT_EPILOGUE)
        def verify_call(observed,pc,returned):
            expected=returned.to_bytes(4,'little').hex()
            if [w['written'] for w in observed['writes'] if w['instruction']==pc]!=[expected]:
                raise ValueError(f'Original CALL {pc:x} does not own return address {returned:x}')
        for row,observed in zip(rows,original,strict=True):
            sites=set(observed['executed_sites'])
            if not row['via_parent']:coverage.update(sites)
            else:parent_coverage.update(sites)
            transform_stop=row.get('before_transform_continuation',False)
            if observed['completed']!=('phase'not in row or transform_stop):raise ValueError('Original sparse decoder completion differs')
            if transform_stop:
                pc=observed['stopped_at']
                transform_ret=0x284076 if row['transform_record_count'] else 0x284081
                if (pc!=PARENT_TRANSFORM_JOIN or not {TRANSFORM_ENTRY,transform_ret}<=sites or SECOND_SELECTOR in sites or
                    observed['boundary_gp'][0]&255):
                    raise ValueError('Original transform-return stop does not carry genuine AL=0')
                verify_call(observed,PARENT_TRANSFORM_CALL,PARENT_TRANSFORM_JOIN)
                transform_stops.append(dict(pc=hex(pc),record_count=row['transform_record_count'],
                    opaque='active_mmx'in row,actual_gp=observed['boundary_gp']))
            elif 'phase'in row:
                pc=observed['boundary']['instruction']
                if pc in CALLS:
                    lane=CALLS.index(pc)
                    after=row['phase']=='after_write'
                    prior=next(packet for packet in reversed(observed['calls']) if packet['entry']==pc)
                    if observed['boundary_gp'][4]!=prior['gp'][4]-(4 if after else 0):
                        raise ValueError('Original decoder indirect CALL stack retirement changed')
                    if after:
                        write=observed['writes'][-1]
                        if (observed['stopped_at']!=targets[lane] or write['instruction']!=pc or
                            write['written']!=CONTINUATIONS[lane].to_bytes(4,'little').hex()):
                            raise ValueError('Retired original decoder CALL did not reach the genuine child')
                    call_stops.append(dict(pc=hex(pc),phase=row['phase'],target=hex(targets[lane]),
                        opaque='active_mmx'in row,stack_pointer=observed['boundary_gp'][4]))
                if row['phase']=='before_read' and pc in CONTINUATIONS:
                    lane=CONTINUATIONS.index(pc)
                    if not {targets[lane],child_ret_sites[lane]}<=sites or not any(w['instruction']==CALLS[lane]for w in observed['writes']):
                        raise ValueError('Sparse decoder continuation lacks its genuine child')
                    continuation_stops.append(dict(pc=hex(pc),target=hex(targets[lane]),opaque='active_mmx'in row,
                                                   via_parent=row['via_parent'],actual_gp=observed['boundary_gp'],
                                                   second_decoder_pass=row.get('second_decoder_pass',False)))
            else:
                cold=row.get('parent_cold',False)
                secondary=row.get('parent_secondary',False)
                pc=PARENT_EPILOGUE[-1] if cold or secondary else BYPASS if row['via_parent']else RETURN
                if observed['stopped_at']!=(BYPASS if row['via_parent'] and not (cold or secondary) else STOP) or RETURN not in sites:
                    raise ValueError(f'Original sparse decoder did not execute its RET: {row["name"]} {observed["stopped_at"]:#x}')
                if row['via_parent']:
                    complete_parent+=1
                    if not {SELECTOR,*SELECTORS,*targets}<=sites:
                        raise ValueError('Parent decoder route did not execute all three genuine children')
                    if cold and (not set(PARENT_EPILOGUE)<=sites or 0x283953 in sites or
                                 observed['boundary_gp'][4]!=row['gp'][4]+44):
                        raise ValueError('Original parent did not complete its zero-secondary RET28')
                    if secondary:
                        record_count=row['transform_record_count']
                        transform_ret=0x284076 if record_count else 0x284081
                        if (not {TRANSFORM_ENTRY,transform_ret,PARENT_TRANSFORM_JOIN,SECOND_SELECTOR,*PARENT_EPILOGUE}<=sites or
                            0x283975 in sites or observed['boundary_gp'][4]!=row['gp'][4]+44):
                            raise ValueError('Original parent did not complete its genuine AL=0 secondary RET28')
                        for call,returned in ((SELECTOR,0x283947),(PARENT_TRANSFORM_CALL,PARENT_TRANSFORM_JOIN),
                                              (SECOND_SELECTOR,0x283a3c)):
                            verify_call(observed,call,returned)
                        sites_written=[w['instruction'] for w in observed['writes']]
                        second_call=sites_written.index(SECOND_SELECTOR)
                        if not (set(CALLS)<=set(sites_written[:second_call]) and set(CALLS)<=set(sites_written[second_call+1:])):
                            raise ValueError('Original second selector did not exercise all three genuine indirect children')
                        transform_returns_by_record_count[record_count]+=1
                    if 'live_secondary_alias'in row:
                        address=row['live_secondary_alias']
                        after=next(w for w in observed['writes']if w['instruction']==0x29643c and w['address']==address)
                        read=next(r for r in observed['reads']if r['instruction']==0x28394a)
                        if after['before']!='40000000' or after['written']!='00000000' or read['address']!=address:
                            raise ValueError('Actual decoder write did not clear the parent secondary length')
                        secondary_aliases+=1
                else:complete_direct+=1
                for lane,returned in enumerate(CONTINUATIONS):
                    child_returns[lane]+=sum(packet['entry']==returned for packet in observed['calls'])
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],
                fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            if 'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for packet in row['calls']:
                if packet.pop('arguments'):raise ValueError('Unexpected sparse decoder trace arguments')
            for bank,after in zip(row['banks'],observed['banks'],strict=True):
                if bank['address']!=after['address']:raise ValueError('Sparse decoder fixture bank order changed')
                bank['after']=after['after']
        required=set(decoder.code)|{pc for target in targets for pc in graphs[target].code}
        if ((not (args.parent_cold or args.parent_secondary) and coverage!=required) or min(child_returns)==0 or not complete_parent):
            raise ValueError('Direct original sparse decoder site/child coverage differs: '+str(sorted(required-coverage)))
        if args.parent_cold:
            if not set(PARENT_EPILOGUE)<=parent_coverage or (not args.row1 and not secondary_aliases):
                raise ValueError('Missing original parent epilogue and backed secondary-length alias')
        if args.parent_secondary:
            if (len(secondary_sites)!=78 or secondary_sites-parent_coverage!={0x283975} or
                min(transform_returns_by_record_count)==0):
                raise ValueError(f'Original AL=0 parent secondary coverage differs: {sorted(hex(pc) for pc in secondary_sites-parent_coverage)}')
        if args.prefixes:
            if {(int(row['pc'],16),row['via_parent'])for row in continuation_stops}!={
                (pc,via_parent) for pc in CONTINUATIONS for via_parent in ((True,) if args.parent_cold or args.parent_secondary else (False,True))}:
                raise ValueError('Missing independent original post-child continuation stops')
            if args.parent_secondary and (
                {int(row['pc'],16) for row in continuation_stops if row['second_decoder_pass']}!=set(CONTINUATIONS) or
                {(row['record_count'],row['opaque']) for row in transform_stops}!={
                    (count,opaque) for count in (0,1) for opaque in ((False,True) if args.mmx else (False,))}):
                raise ValueError('Missing independent original transform/second-decoder continuation stops')
        cpp=comparison_source(scratch)
        fixture=scratch/'fixture.json';negative=scratch/'mutants.json'
        fixture.write_text(json.dumps(dict(cases=rows,write_sites=stores),separators=(',',':')))
        negative.write_text(json.dumps(dict(cases=rows[:min(normal,32)]+[row for row in rows[32:normal]if row['via_parent']],
                                            write_sites=stores),separators=(',',':')))
        report.update(physical_paths=len(rows),direct_original_sites=len(coverage&set(decoder.code)),
                      original_child_sites=len(required& (coverage|parent_coverage))-len(set(decoder.code)&(coverage|parent_coverage)),
                      normal_paths=normal,direct_returns=complete_direct,parent_returns=complete_parent,
                      fault_paths=sum('phase'in row for row in rows),opaque_paths=sum('active_mmx'in row for row in rows),
                      genuine_child_returns_by_lane=child_returns,post_child_continuations=continuation_stops,
                      indirect_call_interruptions=call_stops,original=proofs,
                      parent_epilogue_sites=(len(set(PARENT_EPILOGUE)&parent_coverage) if args.parent_cold else 0),
                      parent_secondary_sites=(len(secondary_sites&parent_coverage) if args.parent_secondary else 0),
                      original_transform_returns_by_record_count=transform_returns_by_record_count,
                      original_transform_return_continuations=transform_stops,
                      original_backed_secondary_length_changes=secondary_aliases)
        if args.row1:
            report['hardware_packed_estimates']=sum(call['entry']==PACKED_ESTIMATE for row in rows for call in row['calls'])
            report['hardware_packed_vector_writes']=sum(write['instruction']==0x2961e1 for row in rows for write in row['writes'])
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(cpp),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            def run(build,probe,input_path):
                done=subprocess.run(build,capture_output=True,text=True,timeout=480)
                if done.returncode:raise ValueError('Sparse decoder compile failed: '+done.stderr[-12000:])
                return subprocess.run([str(probe),str(input_path)],capture_output=True,text=True,timeout=480)
            done=run(command,binary,fixture)
            if done.returncode:raise ValueError('Sparse decoder original comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[]);report['compilers'].append(result)
            print(f'{compiler} complete sparse decoder comparison passed',flush=True)
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            if args.semantic_mutants:
                for name,old,new in defects:
                    text=(ROOT/mutant_subject).read_text()
                    if text.count(old)!=1:raise ValueError('Sparse decoder defect seam changed: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);header=overlay/mutant_subject
                    header.parent.mkdir(parents=True);header.write_text(text.replace(old,new,1));probe=overlay/'probe'
                    build=[command[0],'-I'+str(overlay/'src'),*command[1:]]
                    build[build.index('-o')+1]=str(probe);build[build.index('-MF')+1]=str(overlay/'probe.d')
                    done=run(build,probe,negative);diagnostic=done.stdout+done.stderr
                    recognized='sparse-decode-'in diagnostic or 'Indirect CALL provider pre-write ABI differs'in diagnostic
                    if name=='wrong-second-table-base':
                        recognized|=diagnostic.strip()=='Unqualified TU1.5 sparse decoder child target'
                    if not done.returncode or not recognized:
                        raise ValueError('Sparse decoder defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected sparse decoder defect {name}',flush=True)
        if (original_owners(code).code!=decoder.code or parent_owners()!=parent or selector_owners()!=(selector_source,tails)):
            raise ValueError('Exact sparse decoder and parent source owners drifted')
        if args.parent_secondary and transform_owners()!=transform_sources:
            raise ValueError('Exact secondary transform source owners drifted')
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
