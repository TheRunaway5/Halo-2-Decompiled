"""Bind the 251F70 location transform walk around both original callee bodies."""

import hashlib
import re

import native_world_leaves_replacements as world


OWNER = '251f70'
PIN = '1504fdff83d40241a7392effe824566eb869917047ce2bc7da543aa24092bf28'
PREPARED_PIN = '8ee44339d50ba04648f617e94dd647366b4599b1701cd2f47266d681527a649f'
CALLEES = {
    '178070':'79f2057c7d084fc3a1598f1af9db2e2c45b04b60957600ccd1d4e6f9b07cbd46',
    '250ac0':'cdfcd4f73414ae9160dedf414f670912f9fbf9d8465b68d46c034abab9cbff50',
}
INCLUDE = '#include "rendering/effects/native_tu15_particle_update_location_transforms.hpp"\n'
NAMESPACE = 'halo2::native_particle_update_location_transforms_detail'
BASIS = 'halo2::native_particle_transform_basis_detail'
RESUMES = ('251f71','251f72','251f73','251f74','251f79','251f7e','251f84','251f8c','251f93',
           '251f9b','251f9e','251fa1','251fa5','251fb7','251fba','251fc7','251fc8','251fc9','251fca')
CALLS = (('251f79','251f7e','178070'), ('251fb2','251fb7','250ac0'))
HOOK = ('\n    // BEGIN native particle location transforms 251f70\n'
        f'    switch ({NAMESPACE}::bridge(s,run_178070,run_250ac0)) {{\n'
        f'    case {NAMESPACE}::done: return;\n'
        + ''.join(f'    case 0x{site}u: goto L{site};\n' for site in RESUMES)
        + '    default: break;\n    }\n'
        '    // END native particle location transforms 251f70\n')


def admitted(canonical):
    owner = world.body(canonical, OWNER)
    if (not owner or owner[1] != 'WorldState' or world.digest(owner) != PIN or
            'void source_run_effect_particle_frame_tu15(' not in canonical or
            canonical.count('case 0x251f70u:run_251f70(s);break;') != 1 or
            canonical.count('s.push(0x179fe1u,selected(0x179fe6u,s.image));run_251f70(s);') != 1 or
            canonical.count('if(target==selected(0x251f70u,s.image)){run_251f70(s);return;}') != 1):
        return False
    numbers={int(value,16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)',owner[0])}
    mapping=world.mapping(canonical)
    return all(mapping.get(number,number)==number for number in numbers)


def install(canonical, output):
    if not admitted(canonical):
        return output,[]
    owner=world.body(output,OWNER)
    if not owner or hashlib.sha256(owner[0].encode()).hexdigest()!=PREPARED_PIN:
        return output,[]
    for callee,pin in CALLEES.items():
        candidate=world.body(output,callee)
        if not candidate or hashlib.sha256(candidate[0].encode()).hexdigest()!=pin:
            return output,[]
    body=owner[0]
    for site,word in (*((f'{0x251f70+i:x}',f's.reg({reg},4,0)') for i,reg in enumerate((3,5,6,7))),
                      *((site,f'0x{returned}u') for site,returned,_ in CALLS)):
        before=f's.push(0x{site}u,{word});'
        if body.count(before)!=1:
            raise ValueError(f'Particle location transform saved stack word at {site} drifted')
        body=body.replace(before,f'{BASIS}::push_saved(s,0x{site}u,{word});',1)
    for site in RESUMES:
        anchor=f'\ns.instruction_literal(0x{site}u);'
        if body.count(anchor)!=1:
            raise ValueError(f'Particle location transform continuation {site} drifted')
        body=body.replace(anchor,f'\nL{site}: '+anchor[1:],1)
    inside=body.index('{')+1
    return INCLUDE+output[:owner.start()]+body[:inside]+HOOK+body[inside:]+output[owner.end():],[OWNER]
