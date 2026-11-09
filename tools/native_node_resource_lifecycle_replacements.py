"""Authenticate complete1E5C80 bodies, resolvers and child handoff spelling."""
import hashlib
import importlib.util
from pathlib import Path
import re

def _sibling(filename):
    spec=importlib.util.spec_from_file_location('node_lifecycle_'+Path(filename).stem,Path(__file__).with_name(filename))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper

canonical_halfword_bit_query=_sibling('native_halfword_bit_query_replacements.py').canonical_body
require_selected_identity=_sibling('native_selected_operand_binding.py').require_selected_identity

PINS=('7cf1b9b1a510000321239f60c3d2b5a4dbc34d63165cde5a0daf09eb586a5160',
      '94cb7d44a0102c0225c2d87d126e1035a79a3c4c4e988ad309cdd0af95435873',
      'c7a1edf84543a4b66636d9e39649d597ff878ed262f5aa40a8e605a062e3a2a0')
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
               '92991b7c4472fda6e9a7534fe8981ad8f1305ecfacc5704b918cc68ee2925fa2')
PREFIX='\n    if(halo2::native_node_resource_lifecycle_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_node_resource_lifecycle.hpp"\n'
DISPATCH_PINS=('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
               '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
               '1b8d505cdeea18175b3a47977b6a77666802dbb23dc4d6631e4afda11a58d680',
               # Same unique case/target maps and fallback, in the generator's
               # numeric order, with identity operands literal.
               '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a',
               'fc929a52f9f3ad3400cec16b6fa42f0b36a05ecd5215d94bd786d58bf81e02ae')
def canonical_body(value):
    start=value.index('{')+1
    return value[:start]+value[start+len(PREFIX):]if value[start:].startswith(PREFIX)else value
def replace_node_resource_lifecycle(source):
    matches=list(re.finditer(r'static void run_1e5c80\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1E5C80 lifecycle owner')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete1E5C80 lifecycle body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete1E5C80 child dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    if pin not in DISPATCH_PINS:
        context=_sibling('specialize_native_operands.py')
        ALLOCATOR_OWNER_FRESH_DISPATCH_PIN=context.ALLOCATOR_OWNER_FRESH_DISPATCH_PIN
        ALLOCATOR_OWNER_FRESH_CASE=context.ALLOCATOR_OWNER_FRESH_CASE
        ALLOCATOR_OWNER_FRESH_BODY_PIN=context.ALLOCATOR_OWNER_FRESH_BODY_PIN
        leaf_body,digest,ALLOCATOR_OWNER_DISPATCH_PIN=context.leaf_body,context.digest,context.ALLOCATOR_OWNER_DISPATCH_PIN
        extra=leaf_body(source,'fa5d0')
        if not(pin==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN and dispatcher[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
               digest(dispatcher[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==ALLOCATOR_OWNER_DISPATCH_PIN and
               extra and digest(canonical_halfword_bit_query(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN):
            raise ValueError('Complete1E5C80 child dispatcher drift')
    if original==current:
        start=current.index('{')+1;source=source[:matches[0].start()]+current[:start]+PREFIX+current[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
