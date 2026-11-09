"""Authenticate whole1E68E0 query setup and its qualified recursive owner."""
import hashlib,re
from native_selected_operand_binding import require_selected_identity
from native_bsp_segment_replacements import replace_bsp_segment,RESOLVER_PINS,INCLUDE as CHILD_INCLUDE

BODY_PINS=(
    '25fa5629202e56f3670bdfc18bc15f4c4cc962ca8ee67022f15f8286d1ba7df5',
    'd58e74682d54365ffd69e895c3c56148ad78409953e20a08b41ec02dea806fca',
)
PREFIX='\n    if(halo2::native_bsp_segment_setup_detail::bridge(s,[](auto&state,std::uint32_t target,std::uint32_t site,bool tail){dispatch(state,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_bsp_segment_setup.hpp"\n'
def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body

def replace_bsp_segment_setup(source):
    found=list(re.finditer(r'static void run_1e68e0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not found:return source
    if len(found)!=1:raise ValueError('Expected one complete1E68E0 setup body')
    current=found[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in BODY_PINS:raise ValueError('Complete1E68E0 setup body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if not re.search(r'static void run_1e6980\(WorldState &s\) \{',source):raise ValueError('Native query setup requires its complete recursive owner')
    if replace_bsp_segment(source)!=source:raise ValueError('Native query setup requires its authenticated native recursive owner')
    if current==original:
        start=current.index('{')+1;body=current[:start]+PREFIX+current[start:]
        source=source[:found[0].start()]+body+source[found[0].end():]
    if INCLUDE not in source:
        if source.count(CHILD_INCLUDE)!=1:raise ValueError('Expected one qualified recursive-owner include')
        source=source.replace(CHILD_INCLUDE,CHILD_INCLUDE+INCLUDE,1)
    return source
