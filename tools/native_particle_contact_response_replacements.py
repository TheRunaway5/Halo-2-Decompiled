"""Bind the particle contact response loop and impulse calculation."""

import native_particle_contact_query_replacements as query
import native_particle_contact_walk_replacements as walk


STAGE = '2c30b8'
REFERENCE = 'native_particle_contact_response_reference_' + walk.OWNER
NAMESPACE = 'halo2::native_particle_contact_response_detail'
INCLUDE = '#include "rendering/effects/native_tu15_particle_contact_response.hpp"\n'
END = 's.instruction_literal(0x2c3248u);'
LABEL = 'L2c3248: '
CHILDREN = ('24ffe0', '2c4110')
DIRECT = ''.join(f'            if constexpr (decltype(child)::value == 0x{name}u) run_{name}(value);\n'
                 for name in CHILDREN)
HOOK = (
    '\n    // BEGIN native particle contact response 2c30b8\n'
    f'    if ({NAMESPACE}::bridge_response(s,{REFERENCE},[](auto &value,auto child){{\n'
    + DIRECT + '    }))goto L2c3248;\n'
    '    // END native particle contact response 2c30b8\n'
)


def extract(body):
    first = body.index(query.END) + len(query.END)
    last = body.index(END, first)
    return first, last


def normalized(source):
    for mnemonic, expected, namespace in (
            ('mul', 16, 'native_tu15_leaves'), ('add', 5, 'native_world_leaves_detail')):
        op = f'_mm_{mnemonic}_ss('
        ordered = f'halo2::{namespace}::ordered_{mnemonic}_ss('
        if source.count(op) + source.count(ordered) != expected:
            raise ValueError(f'Particle contact response {mnemonic}ss destinations drifted')
        source = source.replace(op, ordered)
    return source


def reference(source, replay=True):
    if replay:
        for name in CHILDREN:
            call = f'run_{name}(s);'
            if source.count(call) != 1:
                raise ValueError(f'Particle contact response child {name} drifted')
            source = source.replace(call, f'halo2::native_world_leaves_detail::replay_child(s,0x{name}u);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n' + query.END + normalized(source) + '}\n'


def install(canonical, output):
    if not walk.admitted(canonical):
        return output, []
    found = walk.world.body(output, walk.OWNER)
    if not found or query.HOOK not in found[0] or HOOK in found[0]:
        return output, []
    body = found[0]
    first, end = extract(body)
    source = body[first:end]
    if 'selected(' in source or source.count('s.instruction_literal(') != 85:
        return output, []
    replacement = body[:first] + HOOK + normalized(source) + LABEL + body[end:]
    return INCLUDE + output[:found.start()] + reference(source) + '\n' + replacement + output[found.end():], [STAGE]
