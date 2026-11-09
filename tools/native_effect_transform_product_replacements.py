"""Authenticate the Effect owner's complete145560 helper and actual entry routes."""
import hashlib
from native_effect_slot_scan_replacements import body,ROUTE_PINS

OWNER_PIN='caa5772deb91b7f4837f8f1ba4b86b956552e55edfaad237fff5d2f85f35e68b'
PREFIX='\n    if(halo2::native_effect_transform_product_detail::try_native(m,a,s,o))return;'
INCLUDE='#include "native_tu15_effect_transform_product.hpp"\n'


def canonical(value):
    start=value.index('{')+1
    value=value[:start]+value[start+len(PREFIX):]if value[start:].startswith(PREFIX)else value
    for name in ('mul_ss','mul_ps','add_ps'):
        value=value.replace('halo2::native_tu15_leaves::ordered_'+name+'(', '_mm_'+name+'(')
    return value


def install_owner(header,dispatch):
    for name in ('native_weapon_effect_dispatch_tu15_run','native_weapon_effect_dispatch_tu15_owns_entry'):
        if hashlib.sha256(body(dispatch,name).encode()).hexdigest()!=ROUTE_PINS[name]:
            raise ValueError('Complete Effect transform entry route drift '+name)
    current=body(header,'marker_transform_product');original=canonical(current)
    if hashlib.sha256(original.encode()).hexdigest()!=OWNER_PIN:
        raise ValueError('Complete145560 Effect transform helper body drift')
    # Retain each diagnostic observation, but require two-address SSE operand
    # order even when the compiler could otherwise commute NaN payloads.
    for name in ('mul_ss','mul_ps','add_ps'):
        original=original.replace('_mm_'+name+'(', 'halo2::native_tu15_leaves::ordered_'+name+'(')
    start=original.index('{')+1
    return header.replace(current,original[:start]+PREFIX+original[start:],1)
