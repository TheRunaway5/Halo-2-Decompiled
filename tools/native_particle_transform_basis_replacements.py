"""Bind the integer frame and ordered basis copies in Effect Frame 250AC0."""

import hashlib
import re

import native_world_leaves_replacements as world


OWNER = '250ac0'
PIN = '60c19c12370731ea89d42abda3ca0b15afac2d0b70d14842ac433bf3fa468919'
PREPARED_PIN = '60d865f06102541ba141cec75c4b101311a1d7fb704327eda5f640de22da50ac'
CALLER_PIN = '1504fdff83d40241a7392effe824566eb869917047ce2bc7da543aa24092bf28'
SOURCE_OWNER = 'void source_run_effect_particle_frame_tu15('
INCLUDE = '#include "rendering/effects/native_tu15_particle_transform_basis.hpp"\n'
NAMESPACE = 'halo2::native_particle_transform_basis_detail'
RESUMES = ('250acc','250acd','250ace','250ad3','250ada','250add','250ae0','250ae3','250ae6',
           '250aeb','250af0','250af3','250af6','250af9','250afc','250b01','250b06','250b09',
           '250b0c','250b0f','250b12','250b17','250b1e','250b21','250b24','250b27','250b2a',
           '250b34','250b3a','250b3e','250b41','250b44','250b48','250b4c')
TRANSFORM = '250b50'
LEAVE = '250c4d'
HOOK = (
    '\n    // BEGIN native particle transform basis 250ac0\n'
    f'    switch ({NAMESPACE}::bridge(s)) {{\n'
    + ''.join(f'    case 0x{site}u: goto L{site};\n' for site in (*RESUMES, TRANSFORM, LEAVE))
    + '    default: break;\n'
    '    }\n'
    '    // END native particle transform basis 250ac0\n'
)


def admitted(canonical):
    owner = world.body(canonical, OWNER)
    caller = world.body(canonical, '251f70')
    if (not owner or owner[1] != 'WorldState' or world.digest(owner) != PIN or
            not caller or caller[1] != 'WorldState' or world.digest(caller) != CALLER_PIN or
            SOURCE_OWNER not in canonical or canonical.count('case 0x250ac0u:run_250ac0(s);break;') != 1):
        return False
    elements = re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', owner[0] + caller[0])
    mapping = world.mapping(canonical)
    return all(mapping.get(int(value,16), int(value,16)) == int(value,16) for value in elements)


def retained(body):
    for site, reg in (('250acb',3), ('250acc',5), ('250acd',7)):
        anchor = f's.push(0x{site}u,s.reg({reg},4,0));'
        if body.count(anchor) != 1:
            raise ValueError(f'Particle transform basis saved register at {site} drifted')
        body = body.replace(anchor, f'{NAMESPACE}::push_saved(s,0x{site}u,s.reg({reg},4,0));', 1)
    return body


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    found = world.body(output, OWNER)
    if not found or hashlib.sha256(found[0].encode()).hexdigest() != PREPARED_PIN:
        return output, []
    body = retained(found[0])
    for site in RESUMES:
        anchor = f'\ns.instruction_literal(0x{site}u);'
        if body.count(anchor) != 1:
            raise ValueError(f'Particle transform basis successor {site} drifted')
        body = body.replace(anchor, f'\nL{site}: ' + anchor[1:],1)
    middle = body.index('{') + 1
    bound = body[:middle] + HOOK + body[middle:]
    return INCLUDE + output[:found.start()] + bound + output[found.end():], [OWNER]
