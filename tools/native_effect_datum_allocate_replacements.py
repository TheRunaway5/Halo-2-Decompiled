"""Authenticate the shared DATA allocator's complete Effect and TU1.5 routes."""
import hashlib
import re
from native_effect_slot_scan_replacements import body,ROUTE_PINS as INTEGER_ROUTES
from native_selected_operand_binding import require_selected_identity

OWNER_PIN='a7b2ed13f8ab4238b9ca3e48924b5a8440ecc95676dd1fc81e41f905ff52077e'
OWNER_PREFIX='\n    if(halo2::native_effect_datum_allocate_detail::try_native(m,a.gp,a.eflags,s,o))return;'
OWNER_INCLUDE='#include "native_tu15_effect_datum_allocate.hpp"\n'
PINS=('e0b249bf1ca59c52c3e4c749154dbf9daa6293a9d857573b51a265478805e8ce',
      '831e42eb7e7511cadd5b1d35084d716d43e4bcd96e14ebd91d6d81fe13b7e20e')
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
               '489a49c7d9c1dffca065efd9a7446d828f0a54361a713e4c92e6daae9ac96a28',
               'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',
               '618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a',
               '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4')
ROUTE_PINS=dict(INTEGER_ROUTES)
ROUTE_PINS['native_weapon_effect_datum_allocate_tu15_run']='f69c1432507551b2a5c667fc7f69275787dcb8437d3182b27fbffaec7efd8faf'


def canonical_owner(value):
    start=value.index('{')+1
    return value[:start]+value[start+len(OWNER_PREFIX):]if value[start:].startswith(OWNER_PREFIX)else value


def install_owner(source):
    for name,pin in ROUTE_PINS.items():
        if hashlib.sha256(body(source,name).encode()).hexdigest()!=pin:
            raise ValueError('Complete native datum entry route drift '+name)
    current=body(source,'allocate_effect_datum');original=canonical_owner(current)
    if hashlib.sha256(original.encode()).hexdigest()!=OWNER_PIN:raise ValueError('Complete16E6A0 Effect helper body drift')
    if original==current:
        start=current.index('{')+1;source=source.replace(current,current[:start]+OWNER_PREFIX+current[start:],1)
    if OWNER_INCLUDE not in source:source=OWNER_INCLUDE+source
    return source


def canonical_body(value):
    from native_weapon_effect_datum_bridge_tu15 import bridge
    prefix=bridge('tu15');start=value.index('{')+2
    return value[:start]+value[start+len(prefix):]if value[start:].startswith(prefix)else value


def install_native_effect_datum_allocate(source):
    from native_weapon_effect_datum_bridge_tu15 import bridge,INCLUDE
    matches=list(re.finditer(r'static void run_16e6a0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete16E6A0 source route')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete16E6A0 source body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==current:
        start=current.index('{')+2
        source=source[:matches[0].start()]+current[:start]+bridge('tu15')+current[start:]+source[matches[0].end():]
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        if source.count(anchor)!=1:raise ValueError('Missing16E6A0 runtime include seam')
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
