#!/usr/bin/env python3
"""Compare the exact Primary 1046C0 source, stopping at genuine child entries."""
import json
import os
import re
import subprocess
import sys

from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import (
    DISPATCH, GEN, JOIN, ROOT, STATE, STATE_PINS, SYMBOLS, UNIT, SourceProfile, digest, main, one)
from validate_native_object_ratio_caller_middle_frame_tu15 import whole_corpus
from validate_native_object_record_ratio_source_tu15 import (
    DISPATCH_PINS, PARENT, PARENT_PINS, RESOLVER, RESOLVER_PIN, SIGN_EXTEND, SIGN_EXTEND_PIN)

FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_source_frame.hpp'
DRIVER='tests/native_tu15_object_ratio_caller_source_tests.cpp'
HELPERS='src/gameplay/objects/native_tu15_object_ratio_caller_ordered.hpp'
CHILD_CALLS={0x104815:(0x107140,0x10481a),0x104b6c:(0x107140,0x104b71),
             0x1048ba:(0xfc510,0x1048bf),0x104957:(0xfc510,0x10495c),
             0x104a10:(0x165590,0x104a15),0x104a1e:(0x1655f0,0x104a23),
             0x104b35:(0x165590,0x104b3a),0x104b43:(0x1655f0,0x104b48)}


def prepare(scratch,require_binding=False):
    from native_object_ratio_caller_replacements import installed_entries, remove_object_ratio_caller
    fresh,manifest=scratch/'fresh.cpp',scratch/'generation.json'
    cmd=[sys.executable,'-B',str(ROOT/GEN),'--output',str(fresh),'--manifest',str(manifest)]
    p=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=600)
    if p.returncode:raise ValueError('Genuine Primary generation failed: '+p.stderr[-5000:])
    raw=[(ROOT/UNIT).read_text(),fresh.read_text()]
    if require_binding:
        for unit in raw:
            installed_entries(unit)
            installed_entries(specialize(unit)[0])
    raw=[remove_object_ratio_caller(text) for text in raw]
    variants=raw+[specialize(text)[0] for text in raw]
    bodies=[one(PARENT,text,'1046C0 complete owner') for text in variants]
    states=[one(STATE,text,'1046C0 complete LiveState') for text in variants]
    dispatchers=[one(DISPATCH,text,'1046C0 complete dispatcher') for text in variants]
    resolvers=[one(RESOLVER,text,'1046C0 resolver') for text in variants]
    helpers=[one(SIGN_EXTEND,text,'1046C0 sign extension') for text in variants]
    for rows,pins in ((bodies,(PARENT_PINS[0],)*2+(PARENT_PINS[1],)*2),(states,STATE_PINS*2),
                       (dispatchers,DISPATCH_PINS),(resolvers,(RESOLVER_PIN,)*4),(helpers,(SIGN_EXTEND_PIN,)*4)):
        if tuple(map(digest,rows))!=pins:raise ValueError('Complete correlated 1046C0 context drift')
    generated=[]
    original_sites=[]
    original_registry={}
    for body in bodies:
        sites={int(pc,16) for pc in re.findall(r'\.(?:instruction|instruction_literal)\((0x[0-9a-f]+)u\)',body)}
        if len(sites)!=406:raise ValueError('Complete 1046C0 callback site count changed')
        original_sites.append(sites)
        for pc,key in re.findall(r'source_object_function_compiled_symbol_tu15\(s\.memory,(0x[0-9a-f]+)u,(0x[0-9a-f]+)u\)',body):
            if original_registry.setdefault(int(pc,16),int(key,16))!=int(key,16):raise ValueError('Named operand drift')
        for site,(target,ret) in CHILD_CALLS.items():
            push=f's.push(0x{site:x}u,0x{ret:x}u);'
            expect=(push+f'run_{target:x}(s);') if target in (0x107140,0xfc510) else (
                f'const auto target=0x{target:x}u;'+push+f'dispatch(s,target,0x{site:x}u,false);')
            if body.count(expect)!=1:raise ValueError('Genuine child source call changed at '+hex(site))
        a='s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);'
        b='s.quotient(s.reg(1,4,0),true);'
        if body.count(a)!=3 or body.count(b)!=2:raise ValueError('Physical arithmetic correction anchors changed')
        ordered=body.replace(a,'native_object_ratio_caller_detail::shift_definition_index<true>(s);').replace(
            b,'native_object_ratio_caller_detail::divide_clock_digit<true>(s);')
        if ordered.replace('native_object_ratio_caller_detail::shift_definition_index<true>(s);',a).replace(
            'native_object_ratio_caller_detail::divide_clock_digit<true>(s);',b)!=body:
            raise ValueError('Scoped arithmetic correction is not exactly reversible')
        generated.append((body,ordered))
    adapters=sorted((ROOT/'src/gameplay/objects').glob('native_tu15_object_ratio_caller*source_frame.hpp'))
    native_sites={int(pc,16) for pc in re.findall(r'\b(0x104[0-9a-f]{3})u\b',''.join(p.read_text() for p in adapters))}
    if any(sites!=native_sites for sites in original_sites):raise ValueError('Authored 1046C0 callback set changed')
    basic=re.findall(r'\bSymbol\{0x[0-9a-f]+u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/SYMBOLS).read_text())
    object_rows=re.findall(r'ObjectFunctionSymbol\{(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/JOIN).read_text())
    names={}
    for key,name in basic+[(mcc_name,name) for _,_,_,mcc_name,name in object_rows]:
        if names.setdefault(int(key,16),name)!=name:raise ValueError('Conflicting original registry names')
    exact={int(pc,16):(int(original,16),int(mcc,16)) for pc,original,mcc,_,_ in object_rows
           if int(pc,16) in original_registry}
    if {pc:row[0] for pc,row in exact.items()}!=original_registry:raise ValueError('Exact retained registry drift')
    remap={}
    for old,new in exact.values():
        if remap.setdefault(old,new)!=new:raise ValueError('Conflicting source lookup for a named key')
    chunks=['static std::string fixture_name(std::uint32_t key){switch(key){'+''.join(
        f'case 0x{key:x}u:return {json.dumps(name)};' for key,name in sorted(names.items()))+
        'default:throw std::runtime_error("Missing exact 1046C0 registry name");}}',
        'static std::uint32_t fixture_key(std::uint32_t key){switch(key){'+''.join(
        f'case 0x{old:x}u:return 0x{new:x}u;' for old,new in sorted(remap.items()))+'default:return key;}}']
    for n,name in enumerate(('current','fresh')):
        chunks+=['namespace '+name+' {','using namespace halo2::physics_world_runtime_detail;',
                 states[n],helpers[n],
                 'static void run_107140(WorldState& s){call_child(s,0x107140u);}',
                 'static void run_fc510(WorldState& s){call_child(s,0xfc510u);}',
                 'static void dispatch(WorldState& s,std::uint32_t target,std::uint32_t site,bool tail){'
                 'if(tail)throw std::runtime_error("Unexpected 1046C0 tail");call_child(s,target,site);}']
        for index,label in ((n,'raw'),(n+2,'literal')):
            for body,version in zip(generated[index],('modeled','ordered'),strict=True):
                chunks.append(body.replace('run_1046c0(',label+'_'+version+'(',1))
        chunks+=['}',f'static void run_reference({name}::WorldState&s,unsigned v,bool ordered){{'
                  f'if(v&2u){{if(ordered){name}::literal_ordered(s);else {name}::literal_modeled(s);}}'
                  f'else if(ordered){name}::raw_ordered(s);else {name}::raw_modeled(s);}}']
    (scratch/'object_ratio_caller_reference.inc').write_text('\n'.join(chunks)+'\n')
    return dict(generation=cmd,body_pins=list(map(digest,bodies)),ordered_reference_pins=[digest(pair[1]) for pair in generated],
                callback_sites=406,child_sites={hex(pc):[hex(x) for x in call] for pc,call in CHILD_CALLS.items()},
                original_dispatcher_pins=DISPATCH_PINS,actual_children_return=bool(os.environ.get('HALO2_TEST_1046C0_RETURN_CHILDREN')),child_results_supplied=False,
                registry_keys=exact,source_window='complete paths with real native child adapters' if os.environ.get('HALO2_TEST_1046C0_RETURN_CHILDREN')
                else 'complete childless paths and genuine pre-child boundaries')


PROFILE=SourceProfile(UNIT,GEN,FRAME,DRIVER,prepare,whole_corpus,'Object ratio caller',(
    ('skip-childless-high','result=run_high_keys(high);','result=HighResult::finish;'),
    ('merge-unknown-return','high.finish_unknown();return;','exit.finish_result();return;'),
    ('use-wrong-signed-ratio','clamp_if(exit.sample_signed_ratio())','HighResult::finish'),
    ('skip-alternate-maxima','extended_result(extended.locate_initial_scan())','extended_result(ExtendedStage::scan_records)'),
),extra_pins=('tools/validate_native_object_ratio_caller_source_tu15.py',
              'tools/validate_native_object_ratio_caller_middle_frame_tu15.py',
              'tools/native_object_ratio_combat_fixtures.py','tools/native_object_ratio_high_fixtures.py',
              'tools/native_object_ratio_low_fixtures.py',HELPERS))

if __name__=='__main__':
    returning='--returning-children' in sys.argv
    if returning:
        sys.argv.remove('--returning-children')
        os.environ['HALO2_TEST_1046C0_RETURN_CHILDREN']='1'
    else:
        os.environ.pop('HALO2_TEST_1046C0_RETURN_CHILDREN',None)
    raise SystemExit(main(PROFILE))
