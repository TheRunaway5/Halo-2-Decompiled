"""Bind Death Cycle's complete 17BB10 particle location update."""

import re

import native_world_leaves_replacements as world


OWNER = '17bb10'
PIN = 'c72153f9f75e6acc4b8b309a38b23b22067790bd52dbf1f5177cef90bcc206c8'
SOURCE_OWNER = 'void source_run_death_cycle_runtime_tu15('
INCLUDE = '#include "rendering/effects/native_tu15_particle_location_update.hpp"\n'
NAMESPACE = 'halo2::native_particle_location_update_detail'
REFERENCE = 'native_particle_location_update_reference_' + OWNER
CHILDREN = {'178020': 1, '178070': 1, '13a6b0': 1, '178920': 1, '17dce0': 2, '17ba50': 1, '178110': 1}
METHODS = ('17bb7e', '17bb87')
ROUTES = {0x17bbcb, 0x17bc47, 0x17bcc2, 0x17bcfc}
DIRECT = ''.join(f'            if constexpr (decltype(child)::value == 0x{name}u) run_{name}(value);\n'
                 for name in CHILDREN)
HOOK = (
    '\n    // BEGIN native particle location update 17bb10\n'
    f'    if ({NAMESPACE}::bridge(s,{REFERENCE},[](auto &value,auto child){{\n'
    + DIRECT + '        },\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t at){dispatch(value,target,at,false);},\n'
    '        [](auto &value,std::uint32_t target){return canonical_branch(target,value.image);}))return;\n'
    '    // END native particle location update 17bb10'
)


def admitted(canonical):
    body = world.body(canonical, OWNER)
    if not body or body[1] != 'WorldState' or SOURCE_OWNER not in canonical or world.digest(body) != PIN:
        return False
    mapping = world.mapping(canonical)
    words = {int(word, 16) for word in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', body[0])}
    if not all(mapping.get(word, word) == word for word in words):
        return False
    resolver = re.search(r'std::uint32_t canonical_branch\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',
                         canonical, re.S)
    if not resolver:
        return False
    routes = {int(a, 16): int(b, 16) for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}
    return (all(routes.get(route) == route for route in ROUTES) and
            all(a == b for a,b in routes.items() if a in ROUTES or b in ROUTES))


def normalized(source):
    for mnemonic, expected, namespace in (
            ('mul', 6, 'native_tu15_leaves'), ('add', 4, 'native_world_leaves_detail')):
        generic = f'_mm_{mnemonic}_ss('
        ordered = f'halo2::{namespace}::ordered_{mnemonic}_ss('
        if source.count(generic) + source.count(ordered) != expected:
            raise ValueError(f'Particle location update {mnemonic}ss destinations drifted')
        source = source.replace(generic, ordered)
    return source


def reference(source, replay=True):
    name = REFERENCE if replay else REFERENCE + '_live'
    source = source.replace('run_' + OWNER + '(', name + '(', 1)
    if replay:
        for child, count in CHILDREN.items():
            call = f'run_{child}(s);'
            if source.count(call) != count:
                raise ValueError(f'Particle location update child {child} drifted')
            source = source.replace(call, f'halo2::native_world_leaves_detail::replay_child(s,0x{child}u);')
        for site in METHODS:
            call = f'dispatch(s,target,0x{site}u,false);'
            if source.count(call) != 1:
                raise ValueError(f'Particle location update method {site} drifted')
            source = source.replace(call, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)
    return normalized(source)


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    found = world.body(output, OWNER)
    if not found or found[1] != 'WorldState' or HOOK in found[0] or 'selected(' in found[0]:
        return output, []
    body = found[0]
    if body.count('s.instruction_literal(') != 167:
        return output, []
    first = body.index('{') + 1
    replacement = body[:first] + HOOK + normalized(body[first:])
    refs = reference(body) + '\n\n'
    return INCLUDE + output[:found.start()] + refs + replacement + output[found.end():], [OWNER]
