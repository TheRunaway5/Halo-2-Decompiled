#!/usr/bin/env python3
"""Compare exact CombatAction 24B430 and its genuine local 1655F0 caller."""
import re
import subprocess
import sys

import native_object_definition_mode_query_replacements as definition
from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import ROOT, SourceProfile, digest, main, one
from validate_native_object_definition_mode_query_source_tu15 import CONTEXT, GENERATOR, UNIT
from validate_native_object_record_ratio_source_tu15 import SIGN_EXTEND, SIGN_EXTEND_PIN
from validate_native_object_table_ratio_frame_tu15 import BANKS, corpus

FRAME='src/gameplay/objects/native_tu15_object_table_ratio_source_frame.hpp'
DRIVER='tests/native_tu15_object_table_ratio_source_tests.cpp'
LEAF=re.compile(r'^static void run_24b430\(WorldState &s\) \{.*?^\}',re.M|re.S)
PARENT=re.compile(r'^static void run_1655f0\(WorldState &s\) \{.*?^\}',re.M|re.S)
LEAF_PINS=('0ca760e8a63172e2eb45d2979e7ea8b2c4e637c2d4f3ba8c5a7b255449eb5d54',
           '211beb1b6884e3a1c9227215667e7cdc9e7d5047f4f3dbe7d0ad9486d19bc5f6',
           '94914e737d8098008cfe3243a501b7678609d949998d5c32973c2e8422d01563')
PARENT_PINS=('c0092c440050a6c0502995b959f5c613aa301b11aa8fe225fdfef51d732f6e12',
             '7d49a618cea30869c73bbd885267e1a911e07a4f1bd4636af6a7699b78acc02a',
             '077f433ce4ccb15a0f360262cf59c8690467666ea8077cfaecc00d69814764f7')


def inputs(image):
    leaf=corpus(image)
    parent=[]
    for n,row in enumerate(leaf[:96]):
        banks={bank['address']:bytearray.fromhex(bank['before']) for bank in row['banks']}
        options=0x61000c00+0x180-0x61000000
        banks[0x61000000][options:options+4]=(8 if n//8%4==0 else 9).to_bytes(4,'little')
        parent.append(dict(row,entry=0x1655f0,name=f'object-mode-ratio-{n}',
                           banks=[dict(address=base,before=banks[base].hex()) for base,_ in BANKS]))
    return dict(leaf=leaf,parent=parent)


def prepare(scratch,require_binding=False):
    import native_object_table_ratio_replacements as binding
    fresh=scratch/'fresh.cpp'
    cmd=[sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--directory',str(scratch/'generation')]
    generated=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=900)
    if generated.returncode:
        raise ValueError('Genuine CombatAction generation failed: '+generated.stderr[-8000:])
    raw=[(ROOT/UNIT).read_text(),fresh.read_text()]
    if require_binding:
        for text in raw:
            binding.installed_entries(text)
            prepared,receipt=specialize(text)
            if receipt.get('native_object_table_ratio_entries')!=binding.installed_entries(prepared):
                raise ValueError('Installed CombatAction table-ratio specialization receipt differs')
    raw=[binding.remove_object_table_ratio(text) for text in raw]
    variants=raw+[specialize(text)[0] for text in raw]
    for text in variants:
        definition.installed_entries(text)
    leaf=[one(LEAF,text,'24B430 source body') for text in variants]
    parent=[one(PARENT,text,'1655F0 source body') for text in variants]
    ctx=[[one(pattern,text,'24B430 complete owner') for pattern in CONTEXT] for text in variants]
    helpers=[one(SIGN_EXTEND,text,'24B430 sign extension') for text in variants]
    if any(digest(s)!=SIGN_EXTEND_PIN for s in helpers):
        raise ValueError('24B430 genuine sign-extension helper changed')
    for rows,pins,number in ((leaf,LEAF_PINS,34),(parent,PARENT_PINS,8)):
        if tuple(map(digest,rows))!=(*pins,pins[-1]):
            raise ValueError('Complete 24B430/1655F0 body changed')
        operand_maps=[]
        for body,parts in zip(rows,ctx,strict=True):
            identity=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',parts[1]))
            operands=set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',body))
            if any(identity.get(operand)!=operand for operand in operands):
                raise ValueError('24B430/1655F0 nonidentity source operand')
            operand_maps.append(operands)
        if len(operand_maps[0])!=(16 if number==34 else 5) or any(operand_maps[1:]):
            raise ValueError('24B430/1655F0 source operand profiles changed')
        sites=[re.findall(r's\.(?:services\.instruction|instruction_literal)\((0x[0-9a-f]+u)\)',body) for body in rows]
        pattern=r'0x24b4[0-9a-f]{2}u' if number==34 else r'0x165(?:5[0-9a-f]{2}|60[0-9a-f])u'
        native=set(re.findall(pattern,(ROOT/FRAME).read_text()))
        if len(set(sites[0]))!=number or any(row!=sites[0] for row in sites) or set(sites[0])!=native:
            raise ValueError('24B430/1655F0 callback boundaries changed')
    if any(body.count('run_24b430(s);')!=1 or '0x165602u' not in body or '0x165607u' not in body for body in parent):
        raise ValueError('1655F0 actual local child edge changed')
    chunks=[]
    for n,name in enumerate(('current','fresh')):
        chunks+=['namespace '+name+' {','using namespace halo2::physics_world_runtime_detail;',ctx[n][0],ctx[n][1],helpers[n]]
        for key,index in (('raw',n),('literal',n+2)):
            chunks.append(leaf[index].replace('run_24b430(',key+'_leaf(',1))
            chunks.append(parent[index].replace('run_1655f0(',key+'_parent(',1).replace('run_24b430(s);',key+'_leaf(s);',1))
        chunks.append('}')
        for kind in ('leaf','parent'):
            chunks.append(f'static void run_{kind}({name}::WorldState&s,unsigned v){{'
                          f'if(v&2u){name}::literal_{kind}(s);else {name}::raw_{kind}(s);}}')
    (scratch/'object_table_ratio_reference.inc').write_text('\n'.join(chunks)+'\n')
    return dict(generation=cmd,leaf_pins=list(map(digest,leaf)),parent_pins=list(map(digest,parent)),
                owner_contexts=[[digest(value) for value in row] for row in ctx],
                exact_local_call=0x165602,exact_return=0x165607,real_child=0x24b430,
                child_result_supplied=False,leaf_sites=34,parent_sites=8)


PROFILE=SourceProfile(UNIT,GENERATOR,FRAME,DRIVER,prepare,inputs,'Object table ratio',(
    ('wrong-row-flag','g[0]+g[1]+0x170u,1)&8u','g[0]+g[1]+0x170u,1)&4u'),
    ('missing-zero-substitution','step(0x24b461u);s.set(0,1u,4,0);','step(0x24b461u);s.set(0,0u,4,0);'),
    ('unsigned-state','signed_word(s.reg(0,2,0))','s.reg(0,2,0)'),
    ('dropped-negative-clamp','else{step(0x24b49eu);s.set(0,s.flags.logic(0u,4),4,0);}',
     'else{step(0x24b49eu);s.set(0,signed_word(s.reg(1,2,0)),4,0);}'),
    ('wrong-local-return','s.push(0x165602u,0x165607u);','s.push(0x165602u,0x16560au);'),
),extra_pins=('tools/validate_native_object_table_ratio_frame_tu15.py',
              'tools/validate_native_object_table_ratio_source_tu15.py',
              'tools/native_object_definition_mode_query_replacements.py',
              'tools/native_object_table_ratio_replacements.py'))


if __name__=='__main__':
    raise SystemExit(main(PROFILE))
