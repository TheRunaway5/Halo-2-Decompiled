"""Bind the two complete particle drain count bodies in TU1.5 Effect Frame."""

import re

import native_world_leaves_replacements as world


OWNERS = {
    '178240': ('f1dde1f7d93e5b311ea53b89afff709e0f3f2e75fc756b0cb459ed1ee3652e63',
               'locations', 24),
    '252060': ('9989a1b8ed28819afb44be7f81e561d4b832720fdbf4f90a47cc71c645e1dc13',
               'emitters', 18),
}
SOURCE_OWNER = 'void source_run_effect_particle_frame_tu15('
INCLUDE = '#include "rendering/effects/native_tu15_particle_drain_counts.hpp"\n'
NAMESPACE = 'halo2::native_particle_drain_counts_detail'
CALL = 'run_252060(s);'
REPLAY = 'halo2::native_world_leaves_detail::replay_child(s,0x252060u);'
LITERAL = re.compile(r'\bs\.instruction_literal\(0x([0-9a-f]+)u\)')


def admitted(canonical):
    if SOURCE_OWNER not in canonical:
        return False
    mapping = world.mapping(canonical)
    for name, (pin, _, _) in OWNERS.items():
        body = world.body(canonical, name)
        if not body or body[1] != 'WorldState' or world.digest(body) != pin:
            return False
        selected = re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', body[0])
        if any(mapping.get(int(key, 16), int(key, 16)) != int(key, 16) for key in selected):
            return False
    return True


def reference_name(name):
    return 'native_particle_drain_counts_reference_' + name


def reference(name, body):
    replay = body.replace('run_' + name + '(', reference_name(name) + '(', 1)
    return replay.replace(CALL, REPLAY) if name == '178240' else replay


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    bodies = {name: world.body(output, name) for name in OWNERS}
    if any(not body or body[1] != 'WorldState' or 'selected(' in body[0]
           or len(set(LITERAL.findall(body[0]))) != OWNERS[name][2]
           or reference_name(name) in output for name, body in bodies.items()):
        return output, []
    if bodies['178240'][0].count(CALL) != 1:
        return output, []
    for name, body in sorted(bodies.items(), key=lambda item: item[1].start(), reverse=True):
        _, operation, _ = OWNERS[name]
        args = f's,{reference_name(name)}'
        if name == '178240':
            args += ',[](auto &value){run_252060(value);}'
        start = body[0].index('{') + 1
        hook = f'\n    if ({NAMESPACE}::bridge_{operation}({args}))return;'
        replacement = reference(name, body[0]) + '\n\n' + body[0][:start] + hook + body[0][start:]
        output = output[:body.start()] + replacement + output[body.end():]
    return INCLUDE + output, list(OWNERS)
