"""Authenticate complete1E6CB0 parent bodies before shared native routing."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=(
    '94ffc606749a9b3c7d7167eff2bb4e8d6ba12f9ba6ef1620894f6aafca433d7d',
    '103e774e83d25bfa5453036025bf2d75037462acbf4d6350dc77948c99532efa',
    '03240e31c3b6553db5ec8ff1135459297bf66e37040f06420cb25558dedd03bd',
    'd7e293badf72c32bf9f5b9ba1975a5a92c40a25a316af00518b2730b376ba8d3',
    'e7de64af08a55e6864590f7b56b456ad9451026f13335f1184c908a75a6a83dc',
    # Current literal primary emission: independently hardware-qualified
    # FLD/FABS/FSTP C1 publication and exact native single spills.
    'c321544ee067f27ade04a7f89d79d8e0414aac6a73702584bed21a01ec45a0a0',
)
RESOLVER_PINS=(
    '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
    '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
    'cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61',
)
DISPATCH_PINS=(
    '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
    '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
    '2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20',
    # Authenticated identity maps in fresh generator order. The AI691-owner
    # map and death311-owner map retain every canonical target and fallback.
    '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a',
    '1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45',
)
PREFIX='\n    if(halo2::native_bsp_leaf_surface_detail::bridge(s,[](auto&state,std::uint32_t target,std::uint32_t site,bool tail){dispatch(state,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_bsp_leaf_surface.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_bsp_leaf_surface(source):
    matches=list(re.finditer(r'static void run_1e6cb0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1E6CB0 leaf-surface body')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete1E6CB0 leaf-surface body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not dispatcher:raise ValueError('Missing complete1E6CB0 child dispatcher')
    pin=hashlib.sha256(dispatcher[0].encode()).hexdigest()
    if pin not in DISPATCH_PINS:
        from specialize_native_operands import ALLOCATOR_OWNER_FRESH_DISPATCH_PIN,ALLOCATOR_OWNER_FRESH_CASE,ALLOCATOR_OWNER_FRESH_BODY_PIN,leaf_body,digest,ALLOCATOR_OWNER_DISPATCH_PIN
        from native_halfword_bit_query_replacements import canonical_body as halfword_body
        extra=leaf_body(source,'fa5d0')
        if not(pin==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN and dispatcher[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
               digest(dispatcher[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==ALLOCATOR_OWNER_DISPATCH_PIN and
               extra and digest(halfword_body(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN):
            raise ValueError('Complete1E6CB0 child dispatcher drift')
    if current==original:
        start=current.index('{')+1;source=source[:matches[0].start()]+current[:start]+PREFIX+current[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
