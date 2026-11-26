"""Bind TU1.5 particle contact setup, eligibility walk, and return."""

import re

import native_world_leaves_replacements as world


OWNER = '2c2d90'
STAGE = '2c2e92'
PIN = '23b526d4524b16985b90e9542125f6bed85abb47070bdc8abcb8beca0a79dc02'
SOURCE_OWNER = 'void source_run_effect_particle_frame_tu15('
INCLUDE = '#include "rendering/effects/native_tu15_particle_contact_walk.hpp"\n'
NAMESPACE = 'halo2::native_particle_contact_walk_detail'
SETUP = 'native_particle_contact_setup_reference_' + OWNER
WALK = 'native_particle_contact_walk_reference_' + OWNER
CHILDREN = ('178070', '13a6b0', '176a40', '24ffe0')
PARTICLE = 'L2c2e92: '
COUNTDOWN = 'L2c3366: '
ADVANCE = 'L2c3375: '
EPILOGUE = 'L2c3384: '
COLLISION = 's.instruction_literal(0x2c2ee0u);'
COLLISION_LABEL = 'L2c2ee0: '


def admitted(canonical):
    body = world.body(canonical, OWNER)
    if not body or body[1] != 'WorldState' or SOURCE_OWNER not in canonical or world.digest(body) != PIN:
        return False
    mapping = world.mapping(canonical)
    used = {int(word, 16) for word in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', body[0])}
    return all(mapping.get(word, word) == word for word in used)


def extract(body):
    start = body.index('{') + 1
    begin = body.index(PARTICLE, start)
    collision = body.index(COLLISION, begin)
    countdown = body.index(COUNTDOWN, collision)
    end = body.index('throw std::runtime_error', countdown)
    return start, begin, collision, countdown, end


def references(setup, prefilter, tail, replay=True):
    setup = setup.replace('goto L2c3384;', 'return Resume::epilogue;', 1)
    if replay:
        for target in CHILDREN:
            call = f'run_{target}(s);'
            if setup.count(call) != 1:
                raise ValueError(f'Particle contact walk child {target} drifted')
            setup = setup.replace(call, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);')
    suffix = '' if replay else '_live'
    common = f'    using {NAMESPACE}::Resume;\n'
    setup = (f'static {NAMESPACE}::Resume {SETUP}{suffix}(WorldState &s) {{\n'
             + common + setup + '    return Resume::particle;\n}\n')
    walk = (f'static {NAMESPACE}::Resume {WALK}{suffix}(WorldState &s, {NAMESPACE}::Start start) {{\n'
            + common + f'    using {NAMESPACE}::Start;\n'
            '    if (start == Start::advance) goto L2c3375;\n'
            '    if (start == Start::epilogue) goto L2c3384;\n'
            + prefilter + '    return Resume::collision;\n'
            + tail.replace('return;', 'return Resume::returned;', 1) + '}\n')
    return setup, walk


DIRECT = ''.join(f'            if constexpr (decltype(child)::value == 0x{target}u) run_{target}(value);\n'
                 for target in CHILDREN)
SETUP_HOOK = ('\n    // BEGIN native particle contact setup\n'
              f'    using {NAMESPACE}::Resume;\n'
              f'    switch ({NAMESPACE}::bridge_setup(s,{SETUP},[](auto &value,auto child){{\n'
              + DIRECT + '        })) {\n'
              '    case Resume::particle: goto L2c2e92;\n'
              '    case Resume::epilogue: goto L2c3384;\n'
              '    default: break;\n'
              '    }\n'
              '    // END native particle contact setup')


def walk_hook(start):
    return (f'\n    switch ({NAMESPACE}::bridge_walk(s,{NAMESPACE}::Start::{start},{WALK})) {{\n'
            '    case Resume::collision: goto L2c2ee0;\n'
            '    case Resume::returned: return;\n'
            '    default: break;\n'
            '    }\n')


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    found = world.body(output, OWNER)
    if not found or found[1] != 'WorldState' or SETUP_HOOK in found[0] or 'selected(' in found[0]:
        return output, []
    body = found[0]
    start, begin, collision, countdown, end = extract(body)
    spans = (body[start:begin], body[begin:collision], body[countdown:end])
    if tuple(span.count('s.instruction_literal(') for span in spans) != (71, 18, 15):
        return output, []
    refs = '\n'.join(references(*spans)) + '\n'
    for label, entry in ((PARTICLE, 'particle'), (ADVANCE, 'advance'), (EPILOGUE, 'epilogue')):
        body = body.replace(label, label + walk_hook(entry), 1)
    body = body.replace(COLLISION, COLLISION_LABEL + COLLISION, 1)
    body = body[:start] + SETUP_HOOK + body[start:]
    return INCLUDE + output[:found.start()] + refs + body + output[found.end():], [STAGE]
