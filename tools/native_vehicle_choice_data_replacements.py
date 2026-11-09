"""Authenticate complete vehicle selectors before their typed DATA admission.

The existing instruction-observing adapter remains the exact reference path.
This binder does not describe that older adapter as a native DATA kernel.
"""
import hashlib,re
from generated_routine_ownership import function_bodies,qualified_dispatch_entries
from native_vehicle_choice_kernel_tu15 import PINS,INCLUDE as REFERENCE_INCLUDE,route,install_native_vehicle_choice
from native_halfword_bit_query_replacements import canonical_body as canonical_halfword_query

INCLUDE='#include "gameplay/objects/native_tu15_vehicle_choice.hpp"\n'
DISPATCH_PIN='5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'
MARKER='// typed vehicle choice DATA admission\n'
def prefix(family):
    return MARKER+f'if(halo2::native_vehicle_choice_detail::bridge(s,halo2::NativeTu15VehicleChoiceFamily::{family}))return;\n'

def canonical_body(body):
    for family,_ in PINS.values():
        original=route(family);admitted=original.replace('    vehicle_choice_native_tu15(',prefix(family)+'    vehicle_choice_native_tu15(',1)
        if admitted in body:body=body.replace(admitted,original,1)
    if MARKER in body:raise ValueError('Changed vehicle DATA admission prefix')
    return body

def remove_vehicle_choice_data(source):
    source=source.replace(INCLUDE,'')
    for signature,body in reversed(list(function_bodies(source))):
        if signature[1] in {f'{entry:x}'for entry in PINS}:
            source=source[:signature.start()]+canonical_body(body)+source[signature.start()+len(body):]
    if MARKER in source:raise ValueError('Vehicle DATA prefix outside its owning body')
    return source

def replace_vehicle_choice_data(source):
    if source.count(INCLUDE)>1:raise ValueError('Duplicate vehicle DATA include')
    include_at=source.find(INCLUDE)
    original=remove_vehicle_choice_data(source)
    # Existing canonical generation owns and authenticates the full retained
    # bodies, including all self-call operands and compiled symbol sites.
    install_native_vehicle_choice(original)
    if original.count(REFERENCE_INCLUDE)!=1:raise ValueError('Vehicle DATA requires its retained owner include')
    owners=qualified_dispatch_entries(original,'tu15')
    dispatcher_pin=next(iter(owners.values()))['dispatcher_sha256']if owners else None
    if dispatcher_pin!=DISPATCH_PIN:
        # This exact independently qualified fresh dispatcher adds only FA5D0;
        # retain its complete original body authority before accepting the map.
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',original)
        extra=next((body for signature,body in function_bodies(original)if signature[1]=='fa5d0'),None)
        case='case 0xfa5d0u:run_fa5d0(s);return;'
        if not(dispatcher_pin=='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'and dispatcher and
            dispatcher[0].count(case)==1 and hashlib.sha256(dispatcher[0].replace(case,'',1).encode()).hexdigest()==DISPATCH_PIN and extra and
            hashlib.sha256(canonical_halfword_query(extra).encode()).hexdigest()=='caa1c7398c487d028f00ed11ed4a20a15937a234e7b424a4ea15406150796346'):
            raise ValueError('Vehicle selector complete dispatcher changed')
    for entry,(family,pin)in PINS.items():
        owner=owners.get(f'{entry:x}')
        if not owner or owner['entry']!=entry:
            raise ValueError('Vehicle selector actual dispatcher identity changed')
        old=route(family)
        if original.count(old)!=1:raise ValueError('Vehicle selector route is absent or duplicated')
        original=original.replace(old,old.replace('    vehicle_choice_native_tu15(',prefix(family)+'    vehicle_choice_native_tu15(',1),1)
    # Later independent generator overlays may prepend their own includes.
    # Preserve an already-installed include's exact preamble position rather
    # than reordering unrelated source on an idempotent binding check.
    return original[:include_at]+INCLUDE+original[include_at:]if include_at>=0 else INCLUDE+original
