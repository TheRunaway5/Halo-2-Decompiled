"""Bind the entry and transform publication span of emitter owner 250780."""

import importlib.util
from pathlib import Path


ENTRY = '250780'
SPAN = '2508a2'
PIN = 'a1a444f646f139d284b4ed135144ae0b0f217fc7609d14b03984a846ff710e81'
REFERENCE = 'native_emitter_transform_reference_' + ENTRY
NEXT = 's.instruction_literal(0x2508a5u);'
LABEL = 'L2508a5_after_native_transform:\n'
INCLUDE = '#include "rendering/effects/native_tu15_emitter_transform.hpp"\n'
PREFIX = (
    '\n    // BEGIN native particle emitter transform 250780\n'
    '    using halo2::native_emitter_transform_detail::Resume;\n'
    f'    switch (halo2::native_emitter_transform_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value){run_178070(value);}, [](auto &value){run_250400(value);},\n'
    '        [](auto &value){run_176a30(value);})) {\n'
    '    case Resume::transform: goto L2508a5_after_native_transform;\n'
    '    case Resume::emission: goto L25097b;\n'
    '    case Resume::retained: break;\n'
    '    }\n'
    '    // END native particle emitter transform 250780'
)
CHILDREN = ('178070', '250400', '176a30')


def world_helper():
    path = Path(__file__).with_name('native_world_leaves_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_emitter_transform_metadata', path)
    world = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(world)
    return world


def admitted(canonical, world):
    body = world.body(canonical, ENTRY)
    return body and body[1] == 'WorldState' and world.digest(body) == PIN


def extract(owner):
    start, end = owner.index('{') + 1, owner.index(NEXT)
    span = owner[start:end]
    if span.startswith(PREFIX):
        if span.count(LABEL) != 1:
            raise ValueError('Emitter transform continuation drifted')
        # Subsequent owners can install their entry at the transform handoff.
        span = span[len(PREFIX):span.index(LABEL)]
        span = span.replace('halo2::native_tu15_leaves::ordered_mul_ss(', '_mm_mul_ss(')
    return start, end, span


def ordered(span):
    if span.count('_mm_mul_ss(') != 3:
        raise ValueError('Emitter position MULSS sites drifted')
    return span.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')


def reference(span, replay=True):
    if span.count('goto L25097b;') != 1:
        raise ValueError('Emitter transform exit drifted')
    span = ordered(span).replace('goto L25097b;', 'return Resume::emission;', 1)
    for target in CHILDREN:
        child = f'run_{target}(s);'
        if span.count(child) != 1:
            raise ValueError(f'Emitter transform child {target} drifted')
        if replay:
            span = span.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return ('static halo2::native_emitter_transform_detail::Resume '
            f'{name}(WorldState &s) {{\n'
            '    using halo2::native_emitter_transform_detail::Resume;\n'
            f'{span}    return Resume::transform;\n}}\n')


def install(canonical, output):
    world = world_helper()
    if not admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREFIX in current[0] or NEXT not in current[0]:
        return output, []
    start, end, span = extract(current[0])
    if 'selected(' in span or span.count('s.instruction_literal(') != 92:
        return output, []
    owner = current[0][:start] + PREFIX + ordered(span) + LABEL + current[0][end:]
    output = output[:current.start()] + reference(span) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
