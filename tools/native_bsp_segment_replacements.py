"""Authenticate whole1E6980 bodies and their already-qualified surface owner."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity
from native_bsp_leaf_surface_replacements import replace_bsp_leaf_surface

PINS=(
    'aa657acbe810555c37dfe8845a2a8d9ca0b79f50089945b364c46583e9c7b2fc',
    '8088326e28af2fc2f6758f2d3fdcda2e0a49f6cde32739fb2389a67d2f7841ac',
    '8789d8544c3843a456e80e01fb8564dc31d32b0824cec63eb91c2f71017c14f9',
    '1b8287f4191cb1dc725972a84134245883b4277f9a9287a6526fb12e108b6755',
    '040f3b33913d3226c72aedb4fb61bedea3e5cde553cf8bb649fc8c528b09e528',
)
RESOLVER_PINS=(
    '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
    '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
    'cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61',
)
PREFIX='\n    if(halo2::native_bsp_segment_detail::bridge(s,[](auto&state,std::uint32_t target,std::uint32_t site,bool tail){dispatch(state,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_bsp_segment.hpp"\n'
ORIGINAL_SPILL='u(spill(s.fp.st(0)))'
ORDERED_SPILL='halo2::native_bsp_segment_detail::spill_reference(s.fp.st(0),s.fp.status)'


def canonical_body(body):
    start=body.index('{')+1
    original=body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body
    return original.replace(ORDERED_SPILL,ORIGINAL_SPILL)


def replace_bsp_segment(source):
    matches=list(re.finditer(r'static void run_1e6980\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1E6980 segment body')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete1E6980 segment body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    # This admission also authenticates the complete unit dispatcher and its
    # original selected resolver; unknown/deactivated child routes stay closed.
    if replace_bsp_leaf_surface(source)!=source:raise ValueError('Native1E6980 requires its authenticated native1E6CB0 owner')
    bound=current;start=bound.index('{')+1
    if not bound[start:].startswith(PREFIX):bound=bound[:start]+PREFIX+bound[start:]
    bound=bound.replace(ORIGINAL_SPILL,ORDERED_SPILL)
    source=source[:matches[0].start()]+bound+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
