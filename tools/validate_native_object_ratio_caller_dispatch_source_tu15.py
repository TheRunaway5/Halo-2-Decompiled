#!/usr/bin/env python3
"""Compare the exact four Primary 1046C0 initial selector source contexts."""
import json
import re
import subprocess
import sys

from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import (
    DISPATCH, GEN, JOIN, ROOT, STATE, STATE_PINS, SYMBOLS, UNIT, SourceProfile, digest, main, one)
from validate_native_object_ratio_caller_setup_tu15 import DISPATCH_CASES, DISPATCH_KEYS
from validate_native_object_record_ratio_public_tu15 import parent_rows
from validate_native_object_record_ratio_source_tu15 import DISPATCH_PINS, PARENT, PARENT_PINS, RESOLVER, RESOLVER_PIN

FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_dispatch_source_frame.hpp'
DRIVER='tests/native_tu15_object_ratio_caller_dispatch_source_tests.cpp'
LABELS={'10487a':'low','104adf':'middle','104bfe':'high','104cc5':'exact','1049d0':'unknown'}


def inputs(image):
    rows=parent_rows(image)[:48]
    for n,row in enumerate(rows):
        key,resume=DISPATCH_CASES[n%6]
        if n%6==0 and n//6%2:key=0xffffffff
        row.update(source=key,continuation=resume)
        bank=next(b for b in row['banks'] if b['address']==0x62000000)
        data=bytearray.fromhex(bank['before']);offset=row['gp'][4]+8-0x62000000
        data[offset:offset+4]=key.to_bytes(4,'little');bank['before']=data.hex()
    return rows


def prepare(scratch,require_binding=False):
    if require_binding:
        raise ValueError('1046C0 initial dispatch has no installed default')
    fresh,manifest=scratch/'fresh.cpp',scratch/'generation.json'
    command=[sys.executable,'-B',str(ROOT/GEN),'--output',str(fresh),'--manifest',str(manifest)]
    generated=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=600)
    if generated.returncode:
        raise ValueError('Genuine Primary generation failed: '+generated.stderr[-8000:])
    raw=[(ROOT/UNIT).read_text(),fresh.read_text()]
    from native_object_ratio_caller_replacements import remove_object_ratio_caller
    raw=[remove_object_ratio_caller(text) for text in raw]
    variants=raw+[specialize(text)[0] for text in raw]
    parents=[one(PARENT,text,'1046C0 complete body') for text in variants]
    states=[one(STATE,text,'1046C0 Primary state') for text in variants]
    dispatch=[one(DISPATCH,text,'1046C0 Primary dispatcher') for text in variants]
    resolvers=[one(RESOLVER,text,'1046C0 Primary resolver') for text in variants]
    for actual,pins in ((parents,(PARENT_PINS[0],)*2+(PARENT_PINS[1],)*2),(states,STATE_PINS*2),
                        (dispatch,DISPATCH_PINS),(resolvers,(RESOLVER_PIN,)*4)):
        if tuple(map(digest,actual))!=pins:
            raise ValueError('Complete correlated Primary 1046C0 context changed')
    prefixes=[];sites=[]
    for body in parents:
        boundary=re.search(r'^.*(?:services\.instruction|instruction_literal)\(0x104747u\)',body,re.M)
        if not boundary:
            raise ValueError('Missing original 104747 source continuation')
        prefix=body[:boundary.start()]
        keys={int(pc,16):int(key,16) for pc,key in re.findall(
            r'source_object_function_compiled_symbol_tu15\(s\.memory,(0x[0-9a-f]+)u,(0x[0-9a-f]+)u\)',prefix)}
        if keys!=DISPATCH_KEYS:
            raise ValueError('Authentic initial named function operands changed')
        sites.append(re.findall(r's\.(?:services\.instruction|instruction_literal)\((0x[0-9a-f]+u)\)',prefix))
        for pc,route in LABELS.items():
            branch='goto L'+pc+';'
            if prefix.count(branch)!=1:
                raise ValueError('Missing exact original branch to '+pc)
            prefix=prefix.replace(branch,'return Route::'+route+';',1)
        if 'goto ' in prefix:
            raise ValueError('Unknown initial dispatch continuation')
        prefixes.append(prefix.replace('static void run_1046c0(','static Route reference(',1)+'return Route::extended;\n}')
    native=set(re.findall(r'0x104[67][0-9a-f]{2}u',(ROOT/FRAME).read_text()))
    if len(set(sites[0]))!=32 or any(s!=sites[0] for s in sites) or native!=set(sites[0]):
        raise ValueError('Initial dispatch source callback sites changed')

    basic=re.findall(r'\bSymbol\{0x[0-9a-f]+u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/SYMBOLS).read_text())
    object_rows=re.findall(r'ObjectFunctionSymbol\{(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/JOIN).read_text())
    names={}
    for key,name in basic+[(mcc_name,name) for _,_,_,mcc_name,name in object_rows]:
        if names.setdefault(int(key,16),name)!=name:
            raise ValueError('Conflicting live object-function name binding')
    relevant={int(pc,16):(int(original,16),int(mcc,16)) for pc,original,mcc,_,_ in object_rows
              if int(pc,16) in {*DISPATCH_KEYS,0x104af7}}
    if len(names)!=108 or any(relevant.get(pc,(None,))[0]!=key for pc,key in DISPATCH_KEYS.items()):
        raise ValueError('Exact initial dispatch named bindings changed')
    mcc_keys={key:mcc for key,mcc in relevant.values()}
    mcc_keys[0x1d0007bb]=mcc_keys[0x1d0007ba]+1
    chunks=['static std::string fixture_name(std::uint32_t key){switch(key){'+''.join(
        f'case 0x{key:x}u:return {json.dumps(name)};' for key,name in sorted(names.items()))+
        'default:throw std::runtime_error("Missing exact 1046C0 fixture name");}}',
        'static std::uint32_t fixture_key(std::uint32_t key){switch(key){'+''.join(
        f'case 0x{key:x}u:return 0x{value:x}u;' for key,value in sorted(mcc_keys.items()))+'default:return key;}}']
    for n,name in enumerate(('current','fresh')):
        chunks+=['namespace '+name+' {','using namespace halo2::physics_world_runtime_detail;',states[n],
            prefixes[n].replace(' reference(',' raw(',1),prefixes[n+2].replace(' reference(',' literal(',1),'}',
            f'static Route run_reference({name}::WorldState&s,unsigned v){{return v&2u?{name}::literal(s):{name}::raw(s);}}']
    (scratch/'object_ratio_caller_dispatch_reference.inc').write_text('\n'.join(chunks)+'\n')
    return dict(generation=command,original_owners=list(map(digest,parents)),continuation_prefixes=list(map(digest,prefixes)),
                original_callbacks=32,exact_registry_keys=DISPATCH_KEYS,registry_names=len(names),continuations=6,
                completed_parent_return=False)


PROFILE=SourceProfile(UNIT,GEN,FRAME,DRIVER,prepare,inputs,'Object ratio initial dispatch',(
    ('wrong-signed-compare','s.flags.zf||s.flags.sf!=s.flags.of','s.flags.zf||s.flags.cf'),
    ('drop-datum-salt','step(0x1046ddu);s.set(7,read(g[4]+0x18u),4,0);','step(0x1046ddu);s.set(7,read(g[4]+0x18u)&0xffffu,4,0);'),
    ('wrong-definition-slot','read(g[1]+g[0]+8u)','read(g[1]+g[0]+4u)'),
    ('skip-late-selector','step(0x104735u);return s.flags.zf;','step(0x104735u);return false;'),
),extra_pins=('tools/validate_native_object_ratio_caller_dispatch_source_tu15.py',
              'tools/validate_native_object_ratio_caller_setup_tu15.py',
              'tools/validate_native_object_record_ratio_source_tu15.py',
              'tools/validate_native_object_record_ratio_public_tu15.py'))


if __name__=='__main__':
    raise SystemExit(main(PROFILE))
