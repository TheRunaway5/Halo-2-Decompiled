"""Bind the spatial and planar 252480 random-bearing modes."""

import native_particle_velocity_properties_replacements as properties


STAGE = '2524d4'
MODES = (
    ('spatial','L2524d4: ','L252597: ','L252cc9',50,1),
    ('planar','L252597: ','L252628: ','L252cce',35,2),
)
INCLUDE = '#include "rendering/effects/native_tu15_particle_velocity_angles.hpp"\n'
NAMESPACE = 'halo2::native_particle_velocity_angles_detail'
MUL = 'halo2::native_tu15_leaves::ordered_mul_ss('
TRIG = ('[](long double value,bool cosine,std::uint16_t &status)'
        '{return cosine?native_fcos(value,status):native_fsin(value,status);}')


def alias(kind):
    return 'native_particle_velocity_angles_' + kind + '_reference_' + properties.OWNER


def hook(kind, exit_label):
    planar = 'true' if kind == 'planar' else 'false'
    return (
        f'\n    // BEGIN native particle velocity angles {kind}\n'
        f'    if ({NAMESPACE}::bridge<{planar}>(s,{alias(kind)},{properties.DIRECT},\n'
        f'        {TRIG},native_single_spill))goto {exit_label};\n'
        f'    // END native particle velocity angles {kind}\n'
    )


def extract(body, first, end):
    start = body.index(first) + len(first)
    return start, body.index(end,start)


def normalized(span, multiplies):
    if span.count('_mm_mul_ss(') + span.count(MUL) != multiplies:
        raise ValueError('Particle velocity angles MULSS destinations drifted')
    return span.replace('_mm_mul_ss(',MUL)


def reference(kind, exit_label, multiplies, span, replay=True):
    continuation = f'goto {exit_label};'
    if span.count(properties.CHILD) != 1 or span.count(continuation) != 1:
        raise ValueError('Particle velocity angle curve call or exit drifted')
    if replay:
        span = span.replace(properties.CHILD,properties.REPLAY)
    name = alias(kind) if replay else alias(kind) + '_live'
    return (f'static void {name}(WorldState &s) {{\n' +
            normalized(span,multiplies).replace(continuation,'return;') + '}\n')


def install(canonical, output):
    if not properties.admitted(canonical):
        return output,[]
    found = properties.world.body(output,properties.OWNER)
    if not found or properties.PREFIX_HOOK not in found[0] or hook('spatial','L252cc9') in found[0]:
        return output,[]
    body = found[0]
    refs = []
    for kind, first, after, exit_label, expected, multiplies in reversed(MODES):
        start, end = extract(body,first,after)
        span = body[start:end]
        if 'selected(' in span or span.count('s.instruction_literal(') != expected:
            return output,[]
        refs.append(reference(kind,exit_label,multiplies,span))
        body = body[:start] + hook(kind,exit_label) + normalized(span,multiplies) + body[end:]
    return INCLUDE + output[:found.start()] + '\n'.join(refs) + '\n' + body + output[found.end():],[STAGE]
