"""Authenticate complete13FE50 page DATA/ABI owners before native binding.

The third literal body is the exact independently exercised10-owner reference
generation, whose only transport difference is the local32A660 call. Selected
production operands additionally require the complete pinned image resolver.
"""
import hashlib
import importlib.util
from pathlib import Path
import re

BODY_PINS=(
    '9f7baf1b966181785b74d8cb4c1b82fc501beff0aed6ccc98a430240e7bbf5cf',
    '3bfaa3a4042e8d3b45aae965de558ea0157e46d3dbaf69d61fadad158ddfbfb5',
    'ce273066496b0ee1694f6ead6056091fa8c5bc1090bc56767f40c52552b57963',
)
SPECIALIZED_SHADOW_LITERAL_PIN='07f64750206aa8ff661ad1d9fc0f4970cf3d3c0e37507b804b05d9e2013c0bcc'
RESOLVER_PINS=('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',)
PREFIX='''    if(halo2::native_page_allocate_detail::run(s,
        [](auto &v,std::uint32_t owner,std::uint32_t site,bool tail){dispatch(v,owner,site,tail);}))return;
'''
INCLUDE='#include "physics/runtime/native_tu15_page_allocate_abi.hpp"\n'
DISPATCH_PINS={
    '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0':'primary',
    'fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e':'shadow',
    'cdcfa2670c25e9e8de4532a839c48d343873da83311ef01cf9ced7a0051564c1':'physical-reference',
}
SPECIALIZED_DISPATCH_PINS={
    '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052':'primary-specialized',
    'f4173cc280c10731be2223ca4125cfab0596610d7a77dd77e1eb6f2af97cfc3b':'shadow-specialized',
}
REFERENCE_ENTRIES={'12fe00','13fe00','13fe50','140310','16e6a0','16e770','16e860','16e8a0','2db4ca','32a660'}
FRESH_CASE='case 0xfa5d0u:run_fa5d0(s);return;'


def dispatcher(source):
    found=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{',source)
    if not found:raise ValueError('Missing complete13FE50 dispatcher')
    depth=0
    for at in range(source.index('{',found.start()),len(source)):
        if source[at]=='{':depth+=1
        elif source[at]=='}':
            depth-=1
            if depth==0:return source[found.start():at+1]
    raise ValueError('Incomplete13FE50 dispatcher')


def dispatch_context(source,specialized=False):
    value=dispatcher(source);pin=hashlib.sha256(value.encode()).hexdigest()
    context=DISPATCH_PINS.get(pin)or(SPECIALIZED_DISPATCH_PINS.get(pin)if specialized else None)
    if context=='physical-reference':
        entries=re.findall(r'static void run_([0-9a-f]+)\(WorldState &s\) \{.*?\n\}',source,re.S)
        if len(entries)!=len(REFERENCE_ENTRIES)or set(entries)!=REFERENCE_ENTRIES:raise ValueError('Complete13FE50 physical reference context drift')
    if not context and value.count(FRESH_CASE)==1:
        normalized=hashlib.sha256(value.replace(FRESH_CASE,'',1).encode()).hexdigest()
        if DISPATCH_PINS.get(normalized)=='primary'or specialized and SPECIALIZED_DISPATCH_PINS.get(normalized)=='primary-specialized':
            extra=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
            spec=importlib.util.spec_from_file_location('page_allocate_fresh_halfword_binding',Path(__file__).with_name('native_halfword_bit_query_replacements.py'))
            helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
            candidate=helper.canonical_body(extra[0])if extra else''
            if specialized:candidate=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',candidate)
            if hashlib.sha256(candidate.encode()).hexdigest()==helper.PIN:context='primary-fresh-specialized'if specialized else'primary-fresh'
    if not context:raise ValueError('Complete13FE50 dispatcher drift')
    if value.count('run_13fe50(s);')!=1:raise ValueError('Ambiguous13FE50 dispatcher target')
    return dict(variant=context,sha256=pin,bytes=len(value))


def body(source):
    matches=list(re.finditer(r'static void run_13fe50\(WorldState &s\) \{.*?\n\}',source,re.S))
    if len(matches)>1:raise ValueError('Expected one complete13FE50 page allocation body')
    return matches[0]if matches else None


def canonical_body(value):
    start=value.index('{\n')+2
    return value[:start]+value[start+len(PREFIX):]if value[start:].startswith(PREFIX)else value


def authenticate(source,value,specialized=False):
    candidate=value
    if specialized:candidate=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',candidate)
    pin=hashlib.sha256(candidate.encode()).hexdigest()
    if pin not in BODY_PINS and not(specialized and pin==SPECIALIZED_SHADOW_LITERAL_PIN):raise ValueError('Complete13FE50 page allocation body drift')
    if specialized and pin==SPECIALIZED_SHADOW_LITERAL_PIN:
        selected=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        if not selected or hashlib.sha256(selected[0].encode()).hexdigest()not in RESOLVER_PINS:raise ValueError('Complete specialized13FE50 image resolver drift')
    dispatch_context(source,specialized)
    spec=importlib.util.spec_from_file_location('page_allocate_selected_binding',Path(__file__).with_name('native_selected_operand_binding.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper);helper.require_selected_identity(source,candidate,RESOLVER_PINS)


def installed_entries(source):
    current=body(source)
    if not current or canonical_body(current[0])==current[0]:return[]
    authenticate(source,canonical_body(current[0]),True);return['13fe50']


def install_native_page_allocate(source):
    current=body(source)
    if not current:return source
    original=canonical_body(current[0]);authenticate(source,original)
    if original==current[0]:
        start=original.index('{\n')+2;source=source[:current.start()]+original[:start]+PREFIX+original[start:]+source[current.end():]
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Missing13FE50 page allocation include seam')
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
