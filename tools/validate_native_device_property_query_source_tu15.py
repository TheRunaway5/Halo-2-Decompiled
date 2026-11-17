#!/usr/bin/env python3
"""Compare all 171 device property source sites in genuine MapWorld profiles."""
import json
import re
import subprocess
import sys
from pathlib import Path

import native_device_property_query_replacements as binding
from specialize_native_operands import specialize
from validate_native_device_property_query_frame_tu15 import corpus
from validate_native_flight_property_query_source_tu15 import (
    ROOT,SYMBOLS,JOIN,SourceProfile,STATE,HELPER,HELPER_PIN,digest,one,main)

UNIT='src/runtime/source_map_world_support_tu15_generated.cpp'
GEN='tools/generate_map_world_support_tu15.py'
FRAME='src/gameplay/objects/native_tu15_device_property_query_source_frame.hpp'
DRIVER='tests/native_tu15_device_property_query_source_tests.cpp'
BODY=re.compile(r'^static void run_10be20\(WorldState &s\) \{.*?^\}',re.M|re.S)
BODY_PINS=('3b6fca521e2de89dc0dc754a9246f070b191acb27d956504ed1ed7bb1122106f',
 '21750a90d6aeab3834664b4662200482198b182afb7e3910a4a9a472452fc420',
 'eb5bd46cbfedf1ff11b76698359eb71bc2056a67104471ba9d7d797492464eda',
 '5449e7be2724f6a759b0f3e9b8b3c03eabe1ce41a18a819bc131df355ed4556d')
CONTEXTS=(('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46', '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9', 'f754c70cbc7f2e502ea1e757ef67688c89a38f38b665e7e2ba66f711571dc84c', '7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53', '198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554'), ('457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575', '5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d', 'e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4', '4f4ae0165fbbbb4d37d893eb82f487055376425b7aebf9385dc0ac152f184739', '766f168a1357ae9fb01edc1e8489cb71741e3971db052a6ae4d80a0799646c49'), ('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46', '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9', 'abcda7bb63b840d4f2ce25ec163dd66c9964b2a928af4ac897bd7b33765aa599', '7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53', '198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554'), ('457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575', '5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d', 'e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4', '4f4ae0165fbbbb4d37d893eb82f487055376425b7aebf9385dc0ac152f184739', '766f168a1357ae9fb01edc1e8489cb71741e3971db052a6ae4d80a0799646c49'))
PATTERNS=(STATE,re.compile(r'^std::uint32_t selected\([^\n]+',re.M),
 re.compile(r'^static void dispatch\([^\n]+',re.M),
 re.compile(r'^void source_run_map_world_support_tu15\(.*?^\}',re.M|re.S),
 re.compile(r'^bool source_map_world_support_tu15_owns_entry\([^\n]+',re.M))
SYMBOL_ROWS=((0x10be5c,0x6000560,0x600056c,'locked'),(0x10be73,0x40005ab,0x40005b7,'loop'),
 (0x10be81,0x5000561,0x500056d,'delay'),(0x10bf83,0xf00055e,0xf00056a,'change_in_power'),
 (0x10bf8e,0x1200055f,0x1200056b,'change_in_position'))


def prepare(scratch,require_binding=False):
    current=(ROOT/UNIT).read_text();fresh=scratch/'fresh.cpp'
    cmd=[sys.executable,'-B',str(ROOT/GEN),'--output',str(fresh),'--manifest',str(scratch/'generation.json')]
    ran=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=600)
    if ran.returncode:raise ValueError('Genuine MapWorld generation failed: '+ran.stderr[-7000:])
    actual=[current,fresh.read_text()]
    if require_binding:
        for text in actual:
            binding.installed_entries(text)
            specialized,receipt=specialize(text)
            if receipt.get('native_device_property_query_entries')!=binding.installed_entries(specialized):
                raise ValueError('Actual MapWorld specializer missed 10BE20')
    raw=[binding.remove_device_property_query(text) for text in actual]
    sources=raw+[specialize(value)[0]for value in raw]
    bodies=[one(BODY,text,'MapWorld device property body')for text in sources]
    if tuple(map(digest,bodies))!=BODY_PINS:raise ValueError('Complete MapWorld device property body/profile changed')
    contexts=[tuple(digest(one(pattern,text,'MapWorld device property context'))for pattern in PATTERNS)for text in sources]
    if tuple(contexts)!=CONTEXTS:raise ValueError('Complete correlated MapWorld device property context changed')
    sites=[re.findall(r's\.(?:services\.instruction|instruction_literal)\((0x[0-9a-f]+u)\)',body)for body in bodies]
    native=set(re.findall(r'0x10(?:be|bf|c0)[0-9a-f]{2}u',(ROOT/FRAME).read_text()))
    if len(set(sites[0]))!=171 or any(row!=sites[0]for row in sites)or set(sites[0])!=native:
        raise ValueError('Device property exact source boundaries changed')
    resolver=one(PATTERNS[1],current,'current MapWorld resolver')
    literals=set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',bodies[0]))
    table=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolver))
    if len(literals)!=40 or any(table.get(value)!=value for value in literals):
        raise ValueError('Device property current identity operands changed')
    helper=one(HELPER,raw[1],'fresh MapWorld single spill')
    if digest(helper)!=HELPER_PIN:raise ValueError('Fresh MapWorld single-spill helper changed')
    sign_pattern=re.compile(r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',re.M|re.S)
    sign_helpers=[one(sign_pattern,text,'MapWorld sign extension')for text in raw]
    if any(digest(value)!='cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b'for value in sign_helpers):
        raise ValueError('Complete MapWorld sign extension changed')
    joins=(ROOT/JOIN).read_text()
    for site,key,mcc,name in SYMBOL_ROWS:
        expected=f'ObjectFunctionSymbol{{0x{site:x}u,0x{key:x}u,0x{mcc:x}u,0x{mcc:x}u,"{name}"}}'
        if joins.count(expected)!=1:raise ValueError('Device property named compiled-symbol join changed')
    names={}
    pairs=re.findall(r'\bSymbol\{0x[0-9a-f]+u,0x([0-9a-f]+)u,"([^"]+)"\}',(ROOT/SYMBOLS).read_text())
    pairs+=re.findall(r'ObjectFunctionSymbol\{0x[0-9a-f]+u,0x[0-9a-f]+u,0x[0-9a-f]+u,0x([0-9a-f]+)u,"([^"]+)"\}',joins)
    for key,name in pairs:
        if names.setdefault(int(key,16),name)!=name:raise ValueError('Conflicting complete compiled-symbol registry')
    if len(names)!=108:raise ValueError('Incomplete compiled-symbol registry')
    chunks=['static std::string fixture_name(std::uint32_t p){switch(p){'+''.join(
        f'case 0x{k:x}u:return {json.dumps(v)};'for k,v in sorted(names.items()))+
        'default:throw std::runtime_error("Undeclared device symbol fixture");}}',
        'static std::uint32_t fixture_key(std::uint32_t p){switch(p){'+''.join(
        f'case 0x{key:x}u:return 0x{mcc:x}u;'for _,key,mcc,_ in SYMBOL_ROWS)+'default:return p;}}']
    for variant,ns in enumerate(('current','fresh')):
        chunks+=['namespace '+ns+' {','using namespace halo2::physics_world_runtime_detail;',
                 one(STATE,raw[variant],'real MapWorld LiveState'),sign_helpers[variant]]
        if variant:chunks.append(helper)
        else:chunks.append(resolver)
        for fmt,body in (('raw',bodies[variant]),('literal',bodies[variant+2])):
            chunks.append(body.replace('run_10be20(',fmt+'(',1))
        chunks.append('}')
        chunks.append(f'static void run_reference({ns}::WorldState&s,unsigned variant){{'
                      f'if(variant&2u){ns}::literal(s);else {ns}::raw(s);}}')
        flag='true'if variant else'false';spill=ns+'::native_single_spill'if variant else'nullptr'
        chunks.append(f'static void run_native({ns}::WorldState&s,unsigned variant){{'
                      'using namespace halo2::native_device_property_query_detail;'
                      f'if(variant&2u)run_source_frame<true,{flag}>(s,{spill});'
                      f'else run_source_frame<false,{flag}>(s,{spill});}}')
    (scratch/'device_property_query_reference.inc').write_text('\n'.join(chunks)+'\n')
    return dict(generation=cmd,body_sha256=BODY_PINS,context_sha256=CONTEXTS,identity_operands=40,
                complete_registry_names=108,raw_prepared_sites=171,retained_semantic_operations_changed=False,
                installed_context_authenticated=require_binding)


DEVICE=SourceProfile(UNIT,GEN,FRAME,DRIVER,prepare,corpus,'Device query',(
    ('wrong-lahf-test','s.reg(0,1,8)&0x44u','s.reg(0,1,8)&0x40u'),
    ('stale-spill-policy','if constexpr(Fresh)w(s.memory','if constexpr(false)w(s.memory'),
    ('missing-c1-policy','if constexpr(Fresh)s.fp.status&=~0x200u;',''),
    ('missing-live-publish-branch','step(0x10c08du);',''),
    ('wrong-al-return','step(0x10c09au);s.set(0,s.reg(3,1,0),1,0);','step(0x10c09au);s.set(0,0u,1,0);'),
    ('wrong-ret-cleanup','s.gp[4]+=16u','s.gp[4]+=12u'),
),extra_pins=('tools/validate_native_device_property_query_source_tu15.py',
             'tools/native_device_property_query_replacements.py','tools/native_map_world_property_query_replacements.py',
             'src/gameplay/objects/native_tu15_device_property_query_source_owner.hpp'))

if __name__=='__main__':raise SystemExit(main(DEVICE))
