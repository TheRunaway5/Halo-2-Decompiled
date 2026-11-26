"""Bind the allocation, linkage and RNG prefix of the 250C60 particle birth."""

import importlib.util
from pathlib import Path


ENTRY = '250c60'
SPAN = '250e64'
PIN = '84b4df3bb24820ac367c0acb604ea0ba172d0d90595aac90fd42d9750955d151'
REFERENCE = 'native_particle_birth_prefix_reference_' + ENTRY
ANCHOR = 'L250e67: '
ALLOCATE = 'dispatch(s,target,0x250cb0u,false);'
RESOLVE = 'run_178070(s);'
INCLUDE = '#include "rendering/effects/native_tu15_particle_birth_prefix.hpp"\n'
PREFIX = (
    '\n    // BEGIN native particle birth allocation and RNG 250c60\n'
    '    using halo2::native_particle_birth_prefix_detail::Resume;\n'
    f'    switch (halo2::native_particle_birth_prefix_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t site){dispatch(value,target,site,false);},\n'
    '        [](auto &value){run_178070(value);})) {\n'
    '    case Resume::evaluate_properties: goto L250e67;\n'
    '    case Resume::return_without_birth: goto L2512c7;\n'
    '    case Resume::retained: break;\n'
    '    }\n'
    '    // END native particle birth allocation and RNG 250c60'
)


def world_helper():
    path = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_birth_prefix_metadata', path)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)
    return world


def admitted(canonical, world):
    body = world.body(canonical, ENTRY)
    return body and body[1] == 'WorldState' and world.digest(body) == PIN


def extract(owner):
    start = owner.index('{') + 1
    end = owner.index(ANCHOR, start)
    span = owner[start:end]
    if PREFIX in span:
        span = span.replace(PREFIX, '', 1)
    return start, end, span


def reference(span, replay=True):
    if (span.count('goto L250e67;') != 1 or span.count('goto L2512c7;') != 2 or
            span.count(ALLOCATE) != 1 or span.count(RESOLVE) != 1):
        raise ValueError('Particle birth prefix closure drifted')
    span = span.replace('goto L250e67;', 'return Resume::evaluate_properties;').replace(
        'goto L2512c7;', 'return Resume::return_without_birth;')
    if replay:
        replay_call = 'halo2::native_world_leaves_detail::replay_child(s,'
        span = span.replace(ALLOCATE, replay_call + 'target);', 1).replace(RESOLVE, replay_call + '0x178070u);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return ('static halo2::native_particle_birth_prefix_detail::Resume '
            f'{name}(WorldState &s) {{\n'
            '    using halo2::native_particle_birth_prefix_detail::Resume;\n'
            f'{span}    return Resume::evaluate_properties;\n}}\n')


def install(canonical, output):
    world = world_helper()
    if not admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0] or ANCHOR not in current[0]:
        return output, []
    start, _, span = extract(current[0])
    if 'selected(' in span or span.count('s.instruction_literal(') != 132:
        return output, []
    owner = current[0][:start] + PREFIX + current[0][start:]
    output = output[:current.start()] + reference(span) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
