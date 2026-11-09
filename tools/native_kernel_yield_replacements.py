"""Authenticate the complete2DBF74 wrapper and its actual import dispatcher."""
import hashlib,re
from native_halfword_bit_query_replacements import canonical_body as canonical_halfword_bit_query

PIN='d6497ece40ee9bfbdf4c108fb878701f7a705a1316fdc3187eda7bb673d94f0b'
PREFIX='\n    if(halo2::native_kernel_yield_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;'
INCLUDE='#include "platform/native_tu15_kernel_yield.hpp"\n'
DISPATCH_PINS=('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
    '1b8d505cdeea18175b3a47977b6a77666802dbb23dc4d6631e4afda11a58d680',
    '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a')

def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body

def replace_kernel_yield(source):
    matches=list(re.finditer(r'static void run_2dbf74\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete2DBF74 kernel-yield wrapper')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()!=PIN:raise ValueError('Complete2DBF74 kernel-yield body drift')
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete2DBF74 import dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    if pin not in DISPATCH_PINS:
        from specialize_native_operands import ALLOCATOR_OWNER_FRESH_DISPATCH_PIN,ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_BODY_PIN,leaf_body,digest,ALLOCATOR_OWNER_DISPATCH_PIN
        extra=leaf_body(source,'fa5d0')
        if not(pin==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN and dispatcher[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
               digest(dispatcher[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==ALLOCATOR_OWNER_DISPATCH_PIN and
               extra and digest(canonical_halfword_bit_query(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN):
            raise ValueError('Complete2DBF74 import dispatcher drift')
    if current==original:
        start=current.index('{')+1;bound=current[:start]+PREFIX+current[start:]
        source=source[:matches[0].start()]+bound+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
