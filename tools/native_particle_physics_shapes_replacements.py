"""Bind Effect Particle Frame's complete 2C40D0 shape iteration."""

import re

import native_world_leaves_replacements as world


OWNER = '2c40d0'
MOTION = '2c2ad0'
PIN = 'dacaf6b3a7989fa61bb40b1530302cfe1649a798a96c4589921d61d90e020edc'
SOURCE_OWNER = 'void source_run_effect_particle_frame_tu15('
INCLUDE = '#include "rendering/effects/native_tu15_particle_physics_shapes.hpp"\n'
NAMESPACE = 'halo2::native_particle_physics_shapes_detail'
REFERENCE = 'native_particle_physics_shapes_reference_' + OWNER
CALL = 'run_2c2ad0(s);'
REPLAY = 'halo2::native_world_leaves_detail::replay_child(s,0x2c2ad0u);'
HOOK = (
    '\n    // BEGIN native particle physics shape iteration 2c40d0\n'
    f'    if ({NAMESPACE}::bridge(s,{REFERENCE},[](auto &value,auto){{run_2c2ad0(value);}}))return;\n'
    '    // END native particle physics shape iteration 2c40d0'
)


def admitted(canonical):
    body = world.body(canonical,OWNER)
    if not body or body[1] != 'WorldState' or SOURCE_OWNER not in canonical or world.digest(body) != PIN:
        return False
    caller = world.body(canonical,'250450')
    if (not caller or caller[0].count('s.push(0x250757u,selected(0x25075cu,s.image));run_2c40d0(s);') != 1 or
            canonical.count('case 0x2c40d0u:run_2c40d0(s);break;') != 1):
        return False
    selected = {int(value,16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)',body[0])}
    mapping = world.mapping(canonical)
    return selected == {8,0xc,0x10,0x14,0x18,0x2c40fc} and all(mapping.get(value,value) == value for value in selected)


def reference(body, replay=True):
    if body.count('s.push(0x2c40f7u,0x2c40fcu);' + CALL) != 1:
        raise ValueError('Particle physics shape iteration original motion call drifted')
    name = REFERENCE if replay else REFERENCE + '_live'
    body = body.replace('run_' + OWNER + '(', name + '(', 1)
    return body.replace(CALL,REPLAY,1) if replay else body


def install(canonical, output):
    if not admitted(canonical):
        return output,[]
    found = world.body(output,OWNER)
    child = world.body(output,MOTION)
    if not found or HOOK in found[0] or 'selected(' in found[0] or not child or '::bridge_2c2ad0(' not in child[0]:
        return output,[]
    body = found[0]
    if len(set(re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)',body))) != 28:
        return output,[]
    boundary = body.index('{') + 1
    result = body[:boundary] + HOOK + body[boundary:]
    return INCLUDE + output[:found.start()] + reference(body) + '\n\n' + result + output[found.end():],[OWNER]
