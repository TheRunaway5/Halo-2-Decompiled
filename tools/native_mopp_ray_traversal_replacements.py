"""Private exact TU1.5 30AA70 source installer; no production binding yet."""
import hashlib
import importlib.util
from pathlib import Path
import re

_spec=importlib.util.spec_from_file_location('_mopp_ray_owner_functions',Path(__file__).with_name('native_model_marker_lookup_replacements.py'))
_shared=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_shared)
function=_shared.function
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
BODY=re.compile(r'static void run_30aa70\(WorldState &s\) \{.*?\n\}',re.S)
PINS={
    'ai':{'0965ba2dfbc6d947bea64e48f8224b3f85d36efbe9c39739f46d7ec012d0064d',
          '33e3dbf70856230662c38036abc235fc57cba33a544e9d4eb746e1853e4ec8f7'},
    # 7705b4f4...: the Death body after its four FSTP ST(0) sites (30AE00,
    # 30B2C7, 30B6AB, 30B6AD) gained the C1 clear the AI-frame copy and the
    # hardware already have (docs/native_replacement/16C620_collision_ray.md).
    'death':{'f5d6ae21129685622ea289febf4dae15317283b7cfb2c14c299d3853fc25070e',
             '0073fcd26126caf3b2005b45d23e2b213819144739a1da60a9acc0ff8036570d',
             '7705b4f435c057f959617f5ef867fce610b934cec04ce7ec1a1f2d7c1c10e333'},
}
# Full resolver, dispatcher and complete LiveState. The two original bodies
# differ at94 explicit legacy C1 clears: never normalize or conflate them.
CONTEXTS={
    ('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
     '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b',
     'c3e351629369a688a5ca40be8dfcaf725482d008a3050759060cacd2a2bed275'):'ai',
    ('cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61',
     '2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20',
     '68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3'):'death',
}
INCLUDE='#include "physics/runtime/native_tu15_mopp_ray_traversal.hpp"\n'
PREFIX='\n    if(halo2::native_mopp_ray_traversal_detail::bridge(s,[](auto&child,auto target,auto site){dispatch(child,target,site,false);}))return;'
def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body
def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete MOPP ray owner')
    match=found[0];body=canonical_body(match[0]);owner=re.search(r'^struct LiveState :.*?^\};',source,re.M|re.S)
    if not owner or source.count('using WorldState=LiveState;')!=1:raise ValueError('Complete MOPP ray State ownership drift')
    resolver=function(source,'selected');dispatcher=function(source,'dispatch')
    context=(digest(resolver),digest(dispatcher),digest(owner[0]))
    if context not in CONTEXTS:raise ValueError('Complete MOPP ray resolver/dispatcher/State context drift')
    kind=CONTEXTS[context]
    if digest(body)not in PINS[kind]or function(source,'run_30aa70')!=match[0]:raise ValueError('Complete MOPP ray body drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{context[0]})
    return match,body,kind
def replace_mopp_ray_traversal(source):
    if not BODY.search(source):
        if INCLUDE in source or PREFIX in source or re.search(r'\brun_30aa70\s*\(',source):raise ValueError('MOPP ray reference/overlay has no complete owner')
        return source
    match,body,_=authenticate(source)
    if body==match[0]:
        start=body.index('{')+1;source=source[:match.start()]+body[:start]+PREFIX+body[start:]+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate MOPP ray native include')
    return source if INCLUDE in source else INCLUDE+source
