"""Bind both complete 178110 owners while preserving their child call routes."""

import re

import native_world_leaves_replacements as world


OWNER = '178110'
OWNERS = {
    'death': ('void source_run_death_cycle_runtime_tu15(', 'ce82cacd88d59cb5625b0829a39047d3fcca7b719b61a71c34edd808e810f53b'),
    'frame': ('void source_run_effect_particle_frame_tu15(', '70d7134fa564be602c98263559c1599c7730b9be87b690184051cde3ef077e2c'),
}
CHILDREN = {'251930': '17813e', '2520a0': '17816b', '176a40': '1781f9', '251c80': '178220'}
INCLUDE = '#include "rendering/effects/native_tu15_particle_location_prepare.hpp"\n'
NAMESPACE = 'halo2::native_particle_location_prepare_detail'
REFERENCE = 'native_particle_location_prepare_reference_' + OWNER
LITERAL = re.compile(r'\bs\.instruction_literal\(0x([0-9a-f]+)u\)')


def route(canonical):
    body = world.body(canonical, OWNER)
    if not body or body[1] != 'WorldState':
        return None
    pins = world.digest(body)
    mapping = world.mapping(canonical)
    words = {int(word, 16) for word in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', body[0])}
    if not all(mapping.get(word, word) == word for word in words):
        return None
    return next((kind for kind, (owner, pin) in OWNERS.items() if owner in canonical and pin == pins), None)


def local(kind, name):
    return kind == 'death' or name == '176a40'


def call(kind, name, site, state='s'):
    if local(kind, name):
        return f'run_{name}({state});'
    target = 'target' if state == 's' else f'0x{name}u'
    return f'dispatch({state},{target},0x{site}u,false);'


def direct(kind):
    return ''.join(f'            if constexpr (decltype(child)::value == 0x{name}u) {call(kind,name,site,"value")}\n'
                   for name,site in CHILDREN.items())


def hook(kind):
    return ('\n    // BEGIN native particle location prepare 178110\n'
            f'    if ({NAMESPACE}::bridge(s,{REFERENCE},[](auto &value,auto child){{\n'
            + direct(kind) + '        }))return;\n'
            '    // END native particle location prepare 178110')


def normalized(body):
    for operation, namespace in (('add','native_world_leaves_detail'), ('mul','native_tu15_leaves')):
        generic = f'_mm_{operation}_ss('
        ordered = f'halo2::{namespace}::ordered_{operation}_ss('
        if body.count(generic) + body.count(ordered) != 1:
            raise ValueError(f'Particle location prepare {operation}ss destination drifted')
        body = body.replace(generic, ordered)
    return body


def reference(kind, body, replay=True):
    suffix = '' if replay else '_live'
    body = body.replace('run_' + OWNER + '(', REFERENCE + suffix + '(', 1)
    if replay:
        for name,site in CHILDREN.items():
            child = call(kind, name, site)
            target = f'0x{name}u' if local(kind,name) else 'target'
            if body.count(child) != 1:
                raise ValueError(f'Particle location prepare {kind} child {name} drifted')
            body = body.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,{target});', 1)
    return normalized(body)


def install(canonical, output):
    kind = route(canonical)
    if not kind:
        return output, []
    body = world.body(output, OWNER)
    if not body or body[1] != 'WorldState' or hook(kind) in body[0] or 'selected(' in body[0]:
        return output, []
    if len(set(LITERAL.findall(body[0]))) != 93:
        return output, []
    first = body[0].index('{') + 1
    replacement = body[0][:first] + hook(kind) + normalized(body[0][first:])
    refs = reference(kind, body[0]) + '\n\n'
    return INCLUDE + output[:body.start()] + refs + replacement + output[body.end():], [OWNER]
