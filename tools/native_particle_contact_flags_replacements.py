"""Bind the particle contact input cache and per-particle collision flags."""

import native_particle_contact_walk_replacements as walk


STAGE = '2c2ee0'
NEXT = 'L2c2fd4: '
REFERENCE = 'native_particle_contact_flags_reference_' + walk.OWNER
NAMESPACE = 'halo2::native_particle_contact_flags_detail'
INCLUDE = '#include "rendering/effects/native_tu15_particle_contact_flags.hpp"\n'
CHILD = 'run_176a40(s);'
HOOK = (
    '\n    // BEGIN native particle contact flags 2c2ee0\n'
    f'    switch ({NAMESPACE}::bridge_flags(s,{REFERENCE},[](auto &value,auto){{run_176a40(value);}})) {{\n'
    f'    case {NAMESPACE}::Resume::motion: goto L2c2fd4;\n'
    f'    case {NAMESPACE}::Resume::advance: goto L2c3375;\n'
    '    default: break;\n'
    '    }\n'
    '    // END native particle contact flags 2c2ee0\n'
)


def span(body):
    first = body.index(walk.COLLISION)
    end = body.index(NEXT, first)
    return body[first:end]


def reference(source, replay=True):
    if (source.count(CHILD) != 1 or source.count('goto L2c2fd4;') != 4 or
            source.count('goto L2c3375;') != 1):
        raise ValueError('Particle contact flags source branches or child drifted')
    if replay:
        source = source.replace(CHILD, 'halo2::native_world_leaves_detail::replay_child(s,0x176a40u);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    source = source.replace('goto L2c2fd4;', 'return Resume::motion;')
    source = source.replace('goto L2c3375;', 'return Resume::advance;', 1)
    return (f'static {NAMESPACE}::Resume {name}(WorldState &s) {{\n'
            f'    using {NAMESPACE}::Resume;\n'
            + source + '}\n')


def install(canonical, output):
    if not walk.admitted(canonical):
        return output, []
    found = walk.world.body(output, walk.OWNER)
    if not found or walk.COLLISION_LABEL not in found[0] or HOOK in found[0]:
        return output, []
    source = span(found[0])
    if 'selected(' in source or source.count('s.instruction_literal(') != 72:
        return output, []
    body = found[0].replace(walk.COLLISION_LABEL, walk.COLLISION_LABEL + HOOK, 1)
    return INCLUDE + output[:found.start()] + reference(source) + '\n' + body + output[found.end():], [STAGE]
