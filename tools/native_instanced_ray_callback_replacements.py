"""Authenticate paired actual24DEF0/24DF20 owners and their real query dependency."""
import hashlib
import re

import native_camera_instance_query_replacements as query

INCLUDE='#include "physics/runtime/native_tu15_instanced_ray_callback.hpp"\n'
PREFIXES={entry:'\n    if(halo2::native_instanced_ray_callback_binding_detail::'+name+'(s,[](auto&state,std::uint32_t target,std::uint32_t site,bool tail){dispatch(state,target,site,tail);}))return;'
          for entry,name in ((0x24def0,'forward'),(0x24df20,'collect'))}
BODIES={entry:re.compile(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',re.S)for entry in PREFIXES}
PRIMARY=('a85774d3c01bf3c1b2c18f44609848cd2b0ad0042d9f3a5753f35e8e1f5caf75',
         'c15a07edd2cabf8c875b0e6edd9ba26713157985ee3e28d673b47bd4ced9ab4d')
AI=('5c3cf03fc8a43278844a4926be8df238ef4622f8cfd7a59b22d65c88625b367d',
    'fc2dfe1f59f2b54c84e367a52293eed635de4aac2446888d41765dc3a66056e2')
# The complete16C000 body identifies each exact original body/resolver/dispatch
# context through query.authenticate. The literal AI projection is independently
# pinned there; it shares the exact callback spelling of Primary.
CONTEXTS={'7568d9b56c4f415710b83c34d67f00f849f604bafeb5a78e0c016909c294b17a':PRIMARY,
          'a3dcc61bf80a16d252b77f9dc767571955a7eb0bc668671ba6664c909f4ee49f':AI,
          '378022e00a98cf0ef5ba4ebeda6f980f57f0883cf708d963613fbaa942369952':PRIMARY}

def digest(source):return hashlib.sha256(source.encode()).hexdigest()

def prefix(entry):return PREFIXES[int(entry,16)if isinstance(entry,str)else entry]

def canonical_body(body,entry):
    if isinstance(entry,str):entry=int(entry,16)
    opening=body.index('{')+1;prefix=PREFIXES[entry]
    return body[:opening]+body[opening+len(prefix):]if body[opening:].startswith(prefix)else body

def authenticate(source):
    _,parent=query.authenticate(source)
    expected=CONTEXTS[digest(parent)];result=[]
    for (entry,pattern),pin in zip(BODIES.items(),expected,strict=True):
        found=list(pattern.finditer(source))
        if len(found)!=1:raise ValueError('Expected one complete local callback '+hex(entry))
        match=found[0];body=canonical_body(match[0],entry)
        if digest(body)!=pin:raise ValueError('Complete instanced-ray callback body/context drift '+hex(entry))
        result.append((entry,match,body))
    # A missing query definition or native child prefix must fail, even when
    # all callback bodies and the owning dispatcher themselves are unchanged.
    if query.replace_camera_instance_query(source)!=source:
        raise ValueError('Instanced-ray callbacks require the complete authenticated native16C000 owner')
    return result

def replace_instanced_ray_callbacks(source):
    if not any(pattern.search(source)for pattern in BODIES.values()):return source
    owners=authenticate(source)
    for entry,match,body in reversed(owners):
        if match[0]==body:
            opening=body.index('{')+1;bound=body[:opening]+PREFIXES[entry]+body[opening:]
            source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate instanced-ray callback include')
    if INCLUDE not in source:source=INCLUDE+source
    return source
