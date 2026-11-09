"""Authenticate complete126200 resource-touch bodies and their owner dispatch."""
import hashlib
import re
from native_halfword_bit_query_replacements import canonical_body as canonical_halfword_bit_query
from native_selected_operand_binding import require_selected_identity

PINS=('c82c3c915129ce9b0780d943146767afc1d4cf5c8e26d70985f54f152010cd6c',
      '0bc7265be405eb56b62fb2697d3fce8e52a1b851d2ee1a0d411d2952245d086c',
      'd5d86143ba2527fd03b7c265df8effe5c2151685b0908efd800555684228b084',
      # Complete literal forms derived only from those authenticated selected
      # bodies after proving every used operand maps to itself in TU1.5.
      'c2dd1466da5c9f0897fd1054b0063bf9d5c6c2e68860e66a99e1cab4020d6bdf',
      'f7719772012031eafc718fdea1bfab22a076de9fd8deb13ca049021dcc6f46aa')
RESOLVER_PINS=('92991b7c4472fda6e9a7534fe8981ad8f1305ecfacc5704b918cc68ee2925fa2',
               '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657')
DISPATCH_PINS=('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
               '1b8d505cdeea18175b3a47977b6a77666802dbb23dc4d6631e4afda11a58d680',
               '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
               # Root-qualified complete numeric-order literal switch forms;
               # their exact case/target maps and fallback remain unchanged.
               '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a',
               'fc929a52f9f3ad3400cec16b6fa42f0b36a05ecd5215d94bd786d58bf81e02ae')
PREFIX='\n    if(halo2::native_resource_touch_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_resource_touch.hpp"\n'


def canonical_body(value):
    start=value.index('{')+1
    return value[:start]+value[start+len(PREFIX):]if value[start:].startswith(PREFIX)else value


def replace_resource_touch(source):
    matches=list(re.finditer(r'static void run_126200\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete126200 resource-touch owner')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete126200 resource-touch body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete126200 child dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    if pin not in DISPATCH_PINS:
        from specialize_native_operands import ALLOCATOR_OWNER_FRESH_DISPATCH_PIN,ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_BODY_PIN,leaf_body,digest,ALLOCATOR_OWNER_DISPATCH_PIN
        extra=leaf_body(source,'fa5d0')
        if not(pin==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN and dispatcher[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
               digest(dispatcher[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==ALLOCATOR_OWNER_DISPATCH_PIN and
               extra and digest(canonical_halfword_bit_query(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN):
            raise ValueError('Complete126200 child dispatcher drift')
    if original==current:
        start=current.index('{')+1;source=source[:matches[0].start()]+current[:start]+PREFIX+current[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
