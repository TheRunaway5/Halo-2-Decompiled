"""Bind the complete 22AD60 authored emitter bound."""

import hashlib
import re

import native_world_leaves_replacements as world


OWNER = '22ad60'
PIN = 'cb88089969ce6b086e6ccf5eab88bb220d28537679462f58b659ea62139992a1'
PREPARED_PIN = '539474adc72e67316a496165cf83ee1e2039ccdbd004935118b884e563117be1'
INCLUDE = '#include "rendering/effects/native_tu15_particle_emitter_bound.hpp"\n'
NAMESPACE = 'halo2::native_particle_emitter_bound_detail'
BASIS = 'halo2::native_particle_transform_basis_detail'
RESUMES = ('22ad61','22ad64','22ad6a','22ad6d','22ad79','22ad87','22ad91','22ad99','22ada0','22ada3','22ada4')
HOOK = ('\n    // BEGIN native authored emitter bound 22ad60\n'
        f'    switch ({NAMESPACE}::bridge(s)) {{\n'
        f'    case {NAMESPACE}::done: return;\n'
        + ''.join(f'    case 0x{site}u: goto L{site};\n' for site in RESUMES)
        + '    default: break;\n    }\n'
        '    // END native authored emitter bound 22ad60\n')


def admitted(canonical):
    owner=world.body(canonical,OWNER)
    if (not owner or owner[1]!='WorldState' or world.digest(owner)!=PIN or
            'void source_run_effect_particle_frame_tu15(' not in canonical or
            canonical.count('case 0x22ad60u:run_22ad60(s);break;')!=1 or
            canonical.count('if(target==selected(0x22ad60u,s.image)){run_22ad60(s);return;}')!=1):
        return False
    elements={int(value,16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)',owner[0])}
    mapping=world.mapping(canonical)
    return elements=={0,1,4,8,0x44,0x46a0d0} and all(mapping.get(element,element)==element for element in elements)


def install(canonical, output):
    if not admitted(canonical):
        return output,[]
    found=world.body(output,OWNER)
    if not found or hashlib.sha256(found[0].encode()).hexdigest()!=PREPARED_PIN:
        return output,[]
    body=found[0]
    anchor='s.push(0x22ad60u,s.reg(1,4,0));'
    if body.count(anchor)!=1:
        raise ValueError('Emitter bound saved ECX push drifted')
    body=body.replace(anchor,f'{BASIS}::push_saved(s,0x22ad60u,s.reg(1,4,0));',1)
    for site in RESUMES:
        if site in ('22ad91','22ada0'):
            continue
        anchor=f'\ns.instruction_literal(0x{site}u);'
        if body.count(anchor)!=1:
            raise ValueError(f'Emitter bound resume {site} drifted')
        body=body.replace(anchor,f'\nL{site}: '+anchor[1:],1)
    inside=body.index('{')+1
    return INCLUDE+output[:found.start()]+body[:inside]+HOOK+body[inside:]+output[found.end():],[OWNER]
