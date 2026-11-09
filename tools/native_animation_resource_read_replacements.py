"""Authenticate complete1260c0 animation-resource-read ownership and child dispatch."""
import hashlib
import re
from native_halfword_bit_query_replacements import canonical_body as canonical_halfword_bit_query

PIN='88d97d845f1572c8181d0a3c60503cb9465fae23739e9d02c53797902ab5f6c8'
DISPATCH_PIN='5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'
PREFIX='\n    if(halo2::native_animation_resource_read_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_animation_resource_read.hpp"\n'


ORIGINAL_SHIFT='s.flags.shift(s.reg(6,4,0),0x2u,4)'
ORDERED_SHIFT='halo2::native_animation_resource_read_detail::shifted_mode(s,s.reg(6,4,0))'
ORIGINAL_OFFSET='s.flags.shift(s.reg(0,4,0),s.reg(1,1,0),4)'
ORDERED_OFFSET='halo2::native_halfword_bit_query_detail::shifted_mask(s,s.reg(0,4,0),s.reg(1,1,0))'
def canonical_body(value):
    start=value.index('{')+1
    if value[start:].startswith(PREFIX):value=value[:start]+value[start+len(PREFIX):]
    return value.replace(ORDERED_SHIFT,ORIGINAL_SHIFT).replace(ORDERED_OFFSET,ORIGINAL_OFFSET)



def replace_animation_resource_read(source):
    matches=list(re.finditer(r'static void run_1260c0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1260c0 animation-resource-read owner')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()!=PIN:raise ValueError('Complete1260c0 animation-resource-read body drift')
    if 'selected('in original:raise ValueError('Unexpected selected animation-resource-read operands')
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete animation-resource-read child dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    if pin!=DISPATCH_PIN:
        from specialize_native_operands import ALLOCATOR_OWNER_FRESH_DISPATCH_PIN,ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_BODY_PIN,leaf_body,digest
        extra=leaf_body(source,'fa5d0')
        if not(pin==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN and dispatcher[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
               digest(dispatcher[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==DISPATCH_PIN and
               extra and digest(canonical_halfword_bit_query(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN):
            raise ValueError('Complete1260c0 child dispatcher drift')
    corrected=original.replace(ORIGINAL_SHIFT,ORDERED_SHIFT).replace(ORIGINAL_OFFSET,ORDERED_OFFSET)
    start=corrected.index('{')+1
    replacement=corrected[:start]+PREFIX+corrected[start:]
    source=source[:matches[0].start()]+replacement+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
