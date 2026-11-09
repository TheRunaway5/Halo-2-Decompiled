"""Authenticate complete126060 resource-page ownership and child dispatch."""
import hashlib
import re
from native_halfword_bit_query_replacements import canonical_body as canonical_halfword_bit_query

PIN='12c280df93713aafbc755bec9efe7d94ece3da988d323b8b5a48fd0c51ef2aef'
DISPATCH_PIN='5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'
PREFIX='\n    if(halo2::native_resource_page_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_resource_page.hpp"\n'


def canonical_body(value):
    start=value.index('{')+1
    return value[:start]+value[start+len(PREFIX):]if value[start:].startswith(PREFIX)else value


def replace_resource_page(source):
    matches=list(re.finditer(r'static void run_126060\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete126060 resource-page owner')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()!=PIN:raise ValueError('Complete126060 resource-page body drift')
    if 'selected('in original:raise ValueError('Unexpected selected resource-page operands')
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete resource-page child dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    if pin!=DISPATCH_PIN:
        from specialize_native_operands import ALLOCATOR_OWNER_FRESH_DISPATCH_PIN,ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_BODY_PIN,leaf_body,digest
        extra=leaf_body(source,'fa5d0')
        if not(pin==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN and dispatcher[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
               digest(dispatcher[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==DISPATCH_PIN and
               extra and digest(canonical_halfword_bit_query(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN):
            raise ValueError('Complete126060 child dispatcher drift')
    if original==current:
        start=current.index('{')+1;source=source[:matches[0].start()]+current[:start]+PREFIX+current[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
