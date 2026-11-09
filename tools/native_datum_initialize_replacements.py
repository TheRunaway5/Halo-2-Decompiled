"""Authenticate complete actual16E860 initializer bodies and owner routing."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

BODY_PINS=(
    '92c336953667b4acd03faa0bb57555e385d5e44ecec5c36e7df28cf51f002606',
    '62f9fcd664f4a87ae98a998b27f7b388fb40c08e2309883516661636c1fea7b1',
)
RESOLVER_PINS=(
    '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
    '6c1712e8d8fc5b40670906a4f8907878207cebf37423b5779283b14db7cec862',
    'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',
)
DISPATCH_PINS=(
    '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
    '74486deb593da5c844c9bc27f3383d026c7527e8672e8426c5b2d474dab2f482',
    'fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e',
)
# The genuine physical-allocation reference generator owns this complete
# ten-function composition. Admit its exact mapping/fallback and entry set;
# smaller or altered compositions do not inherit production admission.
PHYSICAL_ALLOCATION_DISPATCH_PIN='cdcfa2670c25e9e8de4532a839c48d343873da83311ef01cf9ced7a0051564c1'
PHYSICAL_ALLOCATION_ENTRIES=('12fe00','13fe00','13fe50','140310','16e6a0','16e770','16e860','16e8a0','2db4ca','32a660')
PREFIX='\n    if(halo2::native_datum_specific_detail::initialize(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_datum_specific.hpp"\n'

def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body

def replace_datum_initialize(source):
    found=list(re.finditer(r'static void run_16e860\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not found:return source
    if len(found)!=1:raise ValueError('Expected one complete16E860 initializer body')
    current=found[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in BODY_PINS:raise ValueError('Complete16E860 initializer body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete16E860 dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    if pin==PHYSICAL_ALLOCATION_DISPATCH_PIN:
        entries=re.findall(r'static void run_([0-9a-f]+)\(WorldState &s\) \{.*?\n\}',source,re.S)
        if len(entries)!=len(PHYSICAL_ALLOCATION_ENTRIES)or set(entries)!=set(PHYSICAL_ALLOCATION_ENTRIES):
            raise ValueError('Complete physical-allocation datum entry context drift')
    elif pin not in DISPATCH_PINS:
        from specialize_native_operands import ALLOCATOR_OWNER_FRESH_DISPATCH_PIN,ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_BODY_PIN,leaf_body,digest,ALLOCATOR_OWNER_DISPATCH_PIN
        from native_halfword_bit_query_replacements import canonical_body as halfword_body
        extra=leaf_body(source,'fa5d0')
        if not(pin==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN and dispatcher[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
               digest(dispatcher[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==ALLOCATOR_OWNER_DISPATCH_PIN and
               extra and digest(halfword_body(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN):
            raise ValueError('Complete16E860 dispatcher drift')
    if current==original:
        start=current.index('{')+1;bound=current[:start]+PREFIX+current[start:]
        source=source[:found[0].start()]+bound+source[found[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
