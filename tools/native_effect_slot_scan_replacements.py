"""Authenticate actual16EA60 DATA/ABI owners and compatible production routes."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

OWNER_PIN='010012091284d7b1647dcd5a76f9552ecfa7659a4935dd9c348e6494ed545fde'
OWNER_PREFIX='\n    if(halo2::native_effect_slot_scan_detail::try_native(m,a.gp,a.eflags,s,o))return;'
OWNER_INCLUDE='#include "native_tu15_effect_slot_scan.hpp"\n'
ROUTE_PINS={
    'native_weapon_effect_integer_tu15_run':'b475db943c84c623592dd313ccd3446057db50ea46a49019b501f1b6e173eee2',
    'native_weapon_effect_dispatch_tu15_run':'206700cab7683b4858b18940d2db2f0ab15dc1bc0d06b2507a34697dee349b20',
    'native_weapon_effect_dispatch_tu15_owns_entry':'e57801cc206e04cc9eb9d0795f886281dd9e2ae90ae733b728fa14eb535e16ec',
}
PINS=('09f1fba08b6d76ef4d658f7d6db06f94b5823a8f8ffe93ff708fe19231a3598c',
      '58cbbfd9414e5b43c3ce0dba539b8a1a0c896623398684ae9fdf521002801f07')
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
               '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',
               '1fba98c3ce8762b3dd2a174bf640765e538220ef098a3c9115de3aaeb2c0391f',
               '03e2514fde9d92aa28b30dc7c13ef0cc2fcaf7c320d20644d66603ba6e6b640a')
INCLUDE='#include "halo2/native_weapon_effect_dispatch_tu15.hpp"\n'
PREFIX='''
    // Native16EA60 owns only the integer file; keep live FP/MMX in place.
    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
        auto flags=s.other_flags|s.flags.bits();
        const auto publish=[&]{s.other_flags=flags&~0x8d5u;s.flags.load(flags);};
        try {native_weapon_effect_integer_tu15_run(s.memory,0x16ea60u,s.gp,flags,s.services,s.observe);}
        catch(...) {publish();throw;}
        publish();return;
    }
'''


def body(source,name):
    match=re.search(r'(?:static )?(?:void|bool) '+name+r'\([^;]*?\)\s*\{',source,re.S)
    if not match:raise ValueError('Missing complete native effect owner '+name)
    depth=1;end=match.end()
    while depth:
        if source[end]=='{':depth+=1
        elif source[end]=='}':depth-=1
        end+=1
    return source[match.start():end]


def canonical_body(value,prefix=PREFIX):
    start=value.index('{')+1
    return value[:start]+value[start+len(prefix):]if value[start:].startswith(prefix)else value


def install_owner(header,dispatch):
    for name,pin in ROUTE_PINS.items():
        if hashlib.sha256(body(dispatch,name).encode()).hexdigest()!=pin:
            raise ValueError('Complete native effect entry route drift '+name)
    current=body(header,'find_effect_slot');original=canonical_body(current,OWNER_PREFIX)
    if hashlib.sha256(original.encode()).hexdigest()!=OWNER_PIN:raise ValueError('Complete16EA60 Effect helper body drift')
    if original==current:
        start=current.index('{')+1;header=header.replace(current,current[:start]+OWNER_PREFIX+current[start:],1)
    return header


def install_native_effect_slot_scan(source):
    matches=list(re.finditer(r'static void run_16ea60\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete16EA60 source route')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete16EA60 source body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==current:
        start=current.index('{')+1;source=source[:matches[0].start()]+current[:start]+PREFIX+current[start:]+source[matches[0].end():]
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Missing16EA60 runtime include seam')
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
