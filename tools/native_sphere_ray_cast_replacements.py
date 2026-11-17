"""Exact complete2E8DD0 source owner and owning context authentication."""
import hashlib
import re
import native_instanced_ray_child_replacements as context_owner

BODY=re.compile(r'static void run_2e8dd0\(WorldState &s\) \{.*?\n\}',re.S)
PIN='455f182e913e774969e2a3bd348c5e8f93e048ada793c1bfc254a8e3c7c4dc8e'
FRESH_PIN='6e4eb51a6134799bcbc5981ca5bc352b38869f33f031046be037fe6842677d17'
SINGLE_SPILL_PIN='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
SINGLE_SPILL=re.compile(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_sphere_ray_cast_binding_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_sphere_ray_cast.hpp"\n'
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def original_projection(body):
    """Verify the exact fresh C1/spill emission delta; never rewrite source."""
    pin=digest(body)
    if pin==PIN:return body
    if pin!=FRESH_PIN:raise ValueError('Complete sphere-ray body drift')
    projected=body.replace('s.fp.status&=~0x200u;','').replace(
        'native_single_spill(s.fp.st(0),s.fp.status)','u(spill(s.fp.st(0)))')
    if digest(projected)!=PIN:raise ValueError('Fresh sphere delta extends beyond exact C1/spill emission')
    return projected
def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Missing or ambiguous complete sphere-ray owner')
    match=found[0];body=canonical_body(match[0])
    original_projection(body)
    if digest(body)==FRESH_PIN:
        helpers=list(SINGLE_SPILL.finditer(source))
        if len(helpers)!=1 or digest(helpers[0][0])!=SINGLE_SPILL_PIN:raise ValueError('Fresh sphere single-spill helper drift')
    # This established sibling authenticates the whole real resolver and
    # dispatcher, including its exactly pinned fresh FA5D0 case. No generic
    # dispatcher stripping or new baseline is introduced here.
    context_owner.authenticate(source)
    context_owner.require_selected_identity(source,body,context_owner.RESOLVER_PINS)
    if source.count(INCLUDE)>1 or(PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete sphere-ray source overlay')
    return match,body
def replace_sphere_ray_cast(source):
    if not BODY.search(source):
        if PREFIX in source or INCLUDE in source:raise ValueError('Sphere-ray overlay has no complete owner')
        return source
    match,body=authenticate(source);opening=body.index('{')+1
    source=source[:match.start()]+body[:opening]+PREFIX+body[opening:]+source[match.end():]
    return source if INCLUDE in source else INCLUDE+source
def remove_sphere_ray_cast(source):
    match,body=authenticate(source)
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
