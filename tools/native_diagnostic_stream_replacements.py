"""Strict complete TU1.5 MapWorld diagnostic owner bindings."""
import hashlib
import importlib.util
from pathlib import Path
import re

_path=Path(__file__).with_name('native_model_marker_lookup_replacements.py')
_spec=importlib.util.spec_from_file_location('_diagnostic_owner_functions',_path)
_shared=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_shared)
function=_shared.function
def digest(text):return hashlib.sha256(text.encode()).hexdigest()

ENTRIES=(0x2e2d10,0x2e2e00,0x2e3080,0x2e30e0)
PINS={
 0x2e2d10:('b81eaa8d96080aacdf3285415568ebd7047d086683dc50adbc08d3dbddf1165a','be0af444b499b92d85db28cc89725430c16bbb1d463931cce6cede7295a68f2a'),
 0x2e2e00:('940b04ef2aa47780402eca42f7ca5ef9f8bf52a1e84f28029c265db3192e069e','f79a411c95092a58cb656cfe14e5db52857d23e1edd8f0d192b3754f07175445'),
 0x2e3080:('975800ab9fb6efbb2bcff97291f9e92cc5d6cc2f43cb8f5c012525628273766e','ed9715e375cb64494a2106167a5b6977bbcf1e78b16a086a39386fc0225b442a'),
 0x2e30e0:('24d568b6f0a27888e991faf976885b1ef816d52b48ad077631a2d7f053a9e267','615f688c7a1b5bd9459d11a952f7094294b63ec5d8d31551b1a07356304a43f8'),
}
CONTEXTS={
 ('6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
  'f754c70cbc7f2e502ea1e757ef67688c89a38f38b665e7e2ba66f711571dc84c',
  '68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3',
  '7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53',
  '198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554'),
 # Whole fresh source, generated from the same authenticated image. The
 # four complete bodies are exactly the old identity-literal projections.
 # Three public cases are sorted; an unused LiveState x87 helper's old C1
 # clear is absent. No normalization of either complete context is allowed.
 ('5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d',
  'e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4',
  'c3e351629369a688a5ca40be8dfcaf725482d008a3050759060cacd2a2bed275',
  '4f4ae0165fbbbb4d37d893eb82f487055376425b7aebf9385dc0ac152f184739',
  '766f168a1357ae9fb01edc1e8489cb71741e3971db052a6ae4d80a0799646c49'),
}
PUBLIC_PINS={
 'source_run_map_world_support_tu15':'7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53',
 'source_map_world_support_tu15_owns_entry':'198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554',
}
LIVE_STATE_PIN='68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3'
INCLUDE='#include "runtime/native_tu15_diagnostic_stream_admission.hpp"\n'
BODY=re.compile(r'static void run_(2e2d10|2e2e00|2e3080|2e30e0)\(WorldState &s\) \{.*?\n\}',re.S)

def prefix(entry):
    if isinstance(entry,str):entry=int(entry,16)
    if entry not in ENTRIES:raise ValueError('Unknown diagnostic owner')
    return '\n    if(halo2::native_diagnostic_stream_detail::bridge(s,0x'+f'{entry:x}'+'u,[](auto&child,auto target,auto site,bool tail){dispatch(child,target,site,tail);}))return;'

def canonical_body(body,entry):
    opening=body.index('{')+1;marker=prefix(entry)
    return body[:opening]+body[opening+len(marker):]if body[opening:].startswith(marker)else body

def authenticate(source):
    matches=list(BODY.finditer(source))
    if {int(match[1],16)for match in matches}!=set(ENTRIES)or len(matches)!=4:
        raise ValueError('Expected four complete MapWorld diagnostic owners')
    resolver=function(source,'selected');dispatcher=function(source,'dispatch')
    owner=re.search(r'^struct LiveState :.*?^\};',source,re.M|re.S)
    if not owner or source.count('using WorldState=LiveState;')!=1:
        raise ValueError('Complete diagnostic LiveState ownership drift')
    context=(digest(resolver),digest(dispatcher),digest(owner[0]),*(digest(function(source,name))for name in PUBLIC_PINS))
    if context not in CONTEXTS:raise ValueError('Complete diagnostic resolver/dispatcher/LiveState/public context drift')
    from native_selected_operand_binding import require_selected_identity
    authenticated=[]
    for match in matches:
        entry=int(match[1],16);body=canonical_body(match[0],entry)
        if function(source,f'run_{entry:x}')!=match[0]:raise ValueError('Diagnostic literal is not its complete compiled function')
        if digest(body)not in PINS[entry]:raise ValueError('Complete diagnostic body drift '+hex(entry))
        require_selected_identity(source,body,{digest(resolver)})
        authenticated.append((match,entry,body))
    return authenticated

def replace_diagnostic_stream(source):
    if not BODY.search(source):return source
    for match,entry,body in reversed(authenticate(source)):
        opening=body.index('{')+1;bound=body[:opening]+prefix(entry)+body[opening:]
        source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate diagnostic native include')
    return source if INCLUDE in source else INCLUDE+source
