"""Bind the particle contact notification and final method/flag continuation."""

import native_particle_contact_query_replacements as query
import native_particle_contact_response_replacements as response
import native_particle_contact_walk_replacements as walk


STAGE = '2c3248'
REFERENCE = 'native_particle_contact_actions_reference_' + walk.OWNER
NAMESPACE = 'halo2::native_particle_contact_actions_detail'
INCLUDE = '#include "rendering/effects/native_tu15_particle_contact_actions.hpp"\n'
CHILDREN = ('178070', '250310', '250380', '2502b0')
METHODS = ('2c326b', '2c3281', '2c328a', '2c3313', '2c3328', '2c333d')
DIRECT = ''.join(f'            if constexpr (decltype(child)::value == 0x{name}u) run_{name}(value);\n'
                 for name in CHILDREN)
HOOK = (
    '\n    // BEGIN native particle contact actions 2c3248\n'
    f'    if ({NAMESPACE}::bridge_actions(s,{REFERENCE},[](auto &value,auto child){{\n'
    + DIRECT + '        },\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t at){dispatch(value,target,at,false);}))goto L2c3375;\n'
    '    // END native particle contact actions 2c3248\n'
)


def extract(body):
    first = body.index(response.LABEL) + len(response.LABEL)
    last = body.index(query.REJECT, first)
    return first, last


def reference(source, replay=True):
    if source.count('goto L2c3375;') != 3:
        raise ValueError('Particle contact actions advance routes drifted')
    source = source.replace('goto L2c3375;', 'return;')
    if replay:
        for name in CHILDREN:
            call = f'run_{name}(s);'
            if source.count(call) != (2 if name == '178070' else 1):
                raise ValueError(f'Particle contact actions child {name} drifted')
            source = source.replace(call, f'halo2::native_world_leaves_detail::replay_child(s,0x{name}u);')
        for site in METHODS:
            call = f'dispatch(s,target,0x{site}u,false);'
            if source.count(call) != 1:
                raise ValueError(f'Particle contact actions method {site} drifted')
            source = source.replace(call, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n' + source + '}\n'


def install(canonical, output):
    if not walk.admitted(canonical):
        return output, []
    found = walk.world.body(output, walk.OWNER)
    if not found or response.HOOK not in found[0] or HOOK in found[0]:
        return output, []
    body = found[0]
    first, last = extract(body)
    source = body[first:last]
    if 'selected(' in source or source.count('s.instruction_literal(') != 80:
        return output, []
    replacement = body[:first] + HOOK + body[first:]
    return INCLUDE + output[:found.start()] + reference(source) + '\n' + replacement + output[found.end():], [STAGE]
