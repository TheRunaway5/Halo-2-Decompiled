"""Authenticate FA5D0 native routing and its ordered retained shift primitive."""
import hashlib
import re

PIN='caa1c7398c487d028f00ed11ed4a20a15937a234e7b424a4ea15406150796346'
PREFIX='\n    if(halo2::native_halfword_bit_query_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_halfword_bit_query.hpp"\n'
ORIGINAL_SHIFT='s.flags.shift(s.reg(0,4,0),s.reg(1,1,0),4)'
ORDERED_SHIFT='halo2::native_halfword_bit_query_detail::shifted_mask(s,s.reg(0,4,0),s.reg(1,1,0))'


def canonical_body(body):
    start=body.index('{')+1
    value=body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body
    return value.replace(ORDERED_SHIFT,ORIGINAL_SHIFT)


def installed_entries(source):
    match=re.search(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S)
    if not match:return []
    current=match[0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()!=PIN:raise ValueError('Complete FA5D0 receipt body drift')
    return ['fa5d0']if PREFIX in current and ORDERED_SHIFT in current else []


def replace_halfword_bit_query(source):
    matches=list(re.finditer(r'static void run_fa5d0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete FA5D0 halfword query')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()!=PIN:raise ValueError('Complete FA5D0 halfword query drift')
    bound=current
    start=bound.index('{')+1
    if not bound[start:].startswith(PREFIX):bound=bound[:start]+PREFIX+bound[start:]
    bound=bound.replace(ORIGINAL_SHIFT,ORDERED_SHIFT)
    source=source[:matches[0].start()]+bound+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
