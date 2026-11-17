"""Private strict VehicleUpdate D26C0 / actual TU1.5 D3750 source binder."""
import hashlib
import re

from native_vehicle_render_root_replacements import authenticate as authenticate_sibling

INCLUDE_FILE='native_tu15_vehicle_effect_ratio_source_owner.hpp'
INCLUDE=f'#include "physics/vehicles/{INCLUDE_FILE}"\n'
ANCHOR='#include "rendering/models/native_tu15_vehicle_render_root_source_owner.hpp"\n'
NAMESPACE='native_vehicle_effect_ratio_owner_detail'
ORDERED_NAMESPACE='native_vehicle_effect_ratio_ordered_detail'
MODELED_SHIFT='s.flags.shift(s.reg(0,4,0),0x4u,4)'
OBSERVED_SHIFT=f'halo2::{ORDERED_NAMESPACE}::shift_index(s,s.reg(0,4,0))'
SIGNATURE='static void run_d26c0(VehicleState &s) {'
BODY=re.compile(r'^'+re.escape(SIGNATURE)+r'.*?^\}',re.M|re.S)
CONTEXT=tuple(re.compile(expr,re.M|re.S) for expr in (
    r'^std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',
    r'^static void dispatch\(VehicleState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',
    r'^void source_run_physics_vehicle_update_runtime\(.*?^\}',
))
PINS={
    ('7ce128976a92d65eb1cf072c32570fe48531f676995c7705b841a105d489fd88',
     '1ad94227d3a1fefcce67499652d13611b97df348eef749f2bb19c323da59196a',
     'd52a1a4589a2d4a0179531ddc498125f06e394e36cb8df7f4a671c6a49692bdf',
     'effa8b2c7a7ce8d6948977f95352a9bf88c026ad9e116c78189910aebfd6241f'):False,
    ('036d309f5dd4d39d87fa2b27c05ae473f5501a96e1d11fae3cc30fba56b12e1c',
     '1ad94227d3a1fefcce67499652d13611b97df348eef749f2bb19c323da59196a',
     'a8a9f8851187b20c7e0ce414b04fcf6a586f28848eebe8d645c7f2472776bb13',
     '20ef009651ce29cf4e3920f30007800c04f39b9a23845f1c0c450d9276e263cc'):True,
}


def digest(text):return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    return f'\n    if(halo2::{NAMESPACE}::bridge<{str(literal).lower()}>(s))return;'


def ordered_body(body):
    if body.count(MODELED_SHIFT)!=1 or OBSERVED_SHIFT in body:
        raise ValueError('Exact VehicleUpdate D26FA shift expression drift')
    return body.replace(MODELED_SHIFT,OBSERVED_SHIFT,1)


def canonical_body(body):
    if not body.startswith(SIGNATURE):raise ValueError('Invalid VehicleUpdate ratio signature')
    opening=len(SIGNATURE)
    for tag in (False,True):
        overlay=prefix(tag)
        if body.startswith(overlay,opening):
            if body.count(OBSERVED_SHIFT)!=1 or MODELED_SHIFT in body:
                raise ValueError('Missing exact VehicleUpdate D26FA ordered fallback')
            retained=body[:opening]+body[opening+len(overlay):]
            return retained.replace(OBSERVED_SHIFT,MODELED_SHIFT,1)
    return body


def authenticate(source,*,intermediate=False):
    found=list(BODY.finditer(source))
    if not found:
        if INCLUDE_FILE in source or NAMESPACE in source or ORDERED_NAMESPACE in source:
            raise ValueError('Vehicle ratio overlay has no authenticated owner')
        return None
    contexts=[list(pattern.finditer(source)) for pattern in CONTEXT]
    if len(found)!=1 or any(len(rows)!=1 for rows in contexts) or source.count(ANCHOR)!=1:
        raise ValueError('Ambiguous VehicleUpdate ratio body, selected resolver, dispatcher, export or include')
    match=found[0];body=match[0];opening=len(SIGNATURE)
    tags=[tag for tag in (False,True) if body.startswith(prefix(tag),opening)]
    if (source.count(NAMESPACE)!=len(tags) or source.count(INCLUDE_FILE)!=len(tags) or
        source.count(ORDERED_NAMESPACE)!=len(tags) or
        (tags and source.count(INCLUDE+ANCHOR)!=1)):
        raise ValueError('Unowned or misplaced VehicleUpdate ratio prefix/include')
    retained=canonical_body(body)
    identity=(digest(retained),*(digest(rows[0][0]) for rows in contexts))
    if identity not in PINS:
        raise ValueError('Complete VehicleUpdate ratio body/resolver/dispatcher/export drift')
    literal=PINS[identity]
    sibling=authenticate_sibling(source)
    if sibling is None or sibling[-2:]!=(literal,True):
        raise ValueError('VehicleUpdate ratio owner requires preserved BBE70 binding')
    if tags and tags!=[literal] and not (intermediate and literal and tags==[False]):
        raise ValueError('VehicleUpdate ratio prefix does not match the authenticated callback format')
    return match,retained,literal,bool(tags)


def install(source):
    owner=authenticate(source)
    if owner is None:return source
    match,body,literal,installed=owner
    if installed:return source
    body=ordered_body(body)
    opening=len(SIGNATURE)
    result=(source[:match.start()]+body[:opening]+prefix(literal)+body[opening:]+source[match.end():])
    result=result.replace(ANCHOR,INCLUDE+ANCHOR,1)
    authenticate(result)
    return result


def remove(source):
    owner=authenticate(source)
    if owner is None or not owner[3]:return source
    match,body,_,_=owner
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    owner=authenticate(source)
    return ['d26c0'] if owner is not None and owner[3] else []


def rebind_prepared(source):
    owner=authenticate(source,intermediate=True)
    if owner is None or not owner[2] or not owner[3]:
        raise ValueError('VehicleUpdate ratio retag requires an already bound prepared owner')
    match=owner[0];body=match[0];opening=len(SIGNATURE)
    if body.startswith(prefix(False),opening):
        body=body[:opening]+prefix(True)+body[opening+len(prefix(False)):]
    result=source[:match.start()]+body+source[match.end():]
    authenticate(result)
    return result
