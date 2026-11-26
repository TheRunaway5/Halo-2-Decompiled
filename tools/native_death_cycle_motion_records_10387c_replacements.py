"""Bind the indexed record-motion variants through their original joins."""

import re

from native_death_cycle_opening_replacements import BODY, CANONICAL, OWNER, digest
from native_world_leaves_replacements import mapping


FIRST = '\nL10387c: '
END = '\nL103a6d: '
RAW_PIN = '8c9eab4d244109e14eb0467f60be74e438844184329195b2c5e6ed9e9e027cb2'
PREPARED_PIN = '3cd08bd0ea30335e8a149f838765bbbc28c562d84b1e8b14f7f40493521662f7'
INCLUDE = '#include "gameplay/units/native_tu15_death_cycle_motion_records_10387c.hpp"\n'
DETAIL = 'halo2::native_death_cycle_motion_records'
ENTRIES = (('10387c','primary'),('1038f5','alternate'),('103994','contextual'))
CALLS = (('1038aa','107610','1038af'),('1038eb','108040','1038f0'),('103938','107610','10393d'),
         ('103981','107610','103986'),('1039b7','107610','1039bc'),('1039d7','10b330','1039dc'),
         ('1039e7','108d10','1039ec'),('103a1e','107610','103a23'))
PUSHES = ('1038a2','1038a4','1038a9','1038aa','1038eb','103934','103936','103937','103938',
          '10397d','10397f','103980','103981','1039b2','1039b4','1039b6','1039b7','1039cf','1039d4',
          '1039d7','1039e4','1039e7','103a01','103a03','103a04','103a1a','103a1c','103a1d','103a1e')
STATUS = '(0x210u+s.gp[3]+s.gp[5]*1u)'
RMW = (('1038db',STATUS,4,'0xffffffffu','1u'),('103a49','(0x4u+s.gp[0])',1,'0xffu','1u'),
       ('103a4d',STATUS,4,'0xfffffffeu','0u'))


def span(body):
    start=body.index(FIRST)
    return slice(start,body.index(END,start))


def canonical_stage(canonical):
    found=BODY.search(canonical)
    if not found or digest(found[0])!=CANONICAL:
        raise ValueError('Canonical death-cycle owner changed')
    body=found[0]
    bounds=span(body)
    stage=body[bounds]
    if digest(stage)!=RAW_PIN or stage.count('s.services.instruction(')!=130:
        raise ValueError('Canonical death-cycle record-motion span changed')
    labels=set(re.findall(r'^L([0-9a-f]+):',stage,re.M))
    outer=body[:bounds.start]+body[bounds.stop:]
    incoming=[target for target in re.findall(r'goto L([0-9a-f]+);',outer) if target in labels]
    exits=[target for target in re.findall(r'goto L([0-9a-f]+);',stage) if target not in labels]
    if sorted(incoming)!=[pc for pc,_ in ENTRIES] or sorted(exits)!=['103cd3']+['1040f0']*16:
        raise ValueError('Death-cycle record-motion routes changed')
    for site,target,resume in CALLS:
        ret=f's.push(0x{site}u,selected(0x{resume}u,s.image));'
        route=(ret+'run_108040(s);' if site=='1038eb' else
               f'const auto target=selected(0x{target}u,s.image);{ret}dispatch(s,target,0x{site}u,false);')
        if stage.count(route)!=1:
            raise ValueError(f'Original death-cycle record-motion call {site} changed')
    operands={int(word,16) for word in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)',stage)}
    resolved=mapping(canonical)
    if any(resolved.get(operand)!=operand for operand in operands) or canonical.count('static void run_108040(WorldState &s) {')!=1:
        raise ValueError('Original death-cycle record-motion operands or direct child changed')
    return stage


def retained(stage):
    if tuple(re.findall(r'\bs\.push\(0x([0-9a-f]+)u',stage))!=PUSHES:
        raise ValueError('Prepared death-cycle record-motion stack writes changed')
    for pc in PUSHES:
        stage=stage.replace(f's.push(0x{pc}u,',f'halo2::native_two_bone_solve::push_word(s,0x{pc}u,',1)
    for pc,address,width,mask,bits in RMW:
        operand=f'(r(s.memory,{address},{width}))'
        operand+='|(0x1u)' if bits!='0u' else '&(0xfffffffeu)'
        original=f'w(s.memory,s.observe,0x{pc}u,{address},{width},s.flags.logic({operand},{width}));'
        updated=f'halo2::native_death_cycle_motion::update_field(s,0x{pc}u,{address},{width},{mask},{bits});'
        if stage.count(original)!=1:
            raise ValueError(f'Prepared death-cycle record-motion RMW {pc} changed')
        stage=stage.replace(original,updated,1)
    return stage


def hook(pc,entry):
    return (f'\nL{pc}:\n    switch ({DETAIL}::run(s,[](auto &v,std::uint32_t site,std::uint32_t target) {{\n'
            '        if (site==0x1038ebu) run_108040(v);\n'
            '        else dispatch(v,target,site,false);\n'
            f'    }},{DETAIL}::Entry::{entry})) {{\n'
            f'    case {DETAIL}::Continuation::expiration: goto L1040f0;\n'
            f'    case {DETAIL}::Continuation::apply_motion: goto L103cd3;\n'
            f'    case {DETAIL}::Continuation::reference: break;\n'
            '    }\n    ')


def new_body(body):
    bounds=span(body)
    phase=body[bounds]
    if digest(phase)!=PREPARED_PIN or phase.count('s.instruction_literal(')!=130:
        raise ValueError('Prepared death-cycle record-motion span changed')
    phase=retained(phase)
    for pc,entry in ENTRIES:
        label=f'\nL{pc}: '
        if phase.count(label)!=1:
            raise ValueError(f'Prepared death-cycle record-motion entry {pc} changed')
        phase=phase.replace(label,hook(pc,entry),1)
    return body[:bounds.start]+phase+body[bounds.stop:]


def install(canonical,output):
    if 'void source_run_death_cycle_runtime_tu15(' not in canonical:
        return output,[]
    canonical_stage(canonical)
    owner=BODY.search(output)
    if not owner or INCLUDE in output or output.count('static void run_108040(WorldState &s) {')!=1:
        raise ValueError('Expected death-cycle record-motion production owner is absent or already extended')
    return INCLUDE+output[:owner.start()]+new_body(owner[0])+output[owner.end():],[OWNER]
