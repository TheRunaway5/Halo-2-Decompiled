"""Bind the two 252480 velocity mode bodies that draw a 148DE0 vector."""

import native_particle_velocity_properties_replacements as properties


STAGE = '252628'
OUTWARD = 'L252628: '
INWARD = 'L252672: '
END = 'L2526ee: '
MODES = (
    ('outward', OUTWARD, INWARD, '25263e', 17),
    ('inward', INWARD, END, '25267f', 29),
)
INCLUDE = '#include "rendering/effects/native_tu15_particle_velocity_direction.hpp"\n'
NAMESPACE = 'halo2::native_particle_velocity_direction_detail'
MUL = 'halo2::native_tu15_leaves::ordered_mul_ss('


def alias(kind):
    return 'native_particle_velocity_direction_' + kind + '_reference_' + properties.OWNER


def hook(kind):
    inward = 'true' if kind == 'inward' else 'false'
    return (
        f'\n    // BEGIN native particle velocity direction {kind}\n'
        f'    if ({NAMESPACE}::bridge<{inward}>(s,{alias(kind)},{properties.DIRECT},\n'
        '        [](auto &value,std::uint32_t target,std::uint32_t at){dispatch(value,target,at,false);}))goto L252cce;\n'
        f'    // END native particle velocity direction {kind}\n'
    )


def extract(body, first, end):
    start = body.index(first) + len(first)
    return start, body.index(end,start)


def normalized(span):
    if span.count('_mm_mul_ss(') + span.count(MUL) != 3:
        raise ValueError('Particle velocity direction MULSS destinations drifted')
    return span.replace('_mm_mul_ss(',MUL)


def reference(kind, site, span, replay=True):
    dispatch = f'dispatch(s,target,0x{site}u,false);'
    if span.count(dispatch) != 1 or span.count(properties.CHILD) != 1 or span.count('goto L252cce;') != 1:
        raise ValueError('Particle velocity direction child calls or exit drifted')
    if replay:
        span = span.replace(properties.CHILD,properties.REPLAY)
        span = span.replace(dispatch,'halo2::native_world_leaves_detail::replay_child(s,target);')
    name = alias(kind) if replay else alias(kind) + '_live'
    return (f'static void {name}(WorldState &s) {{\n' +
            normalized(span).replace('goto L252cce;', 'return;') + '}\n')


def install(canonical, output):
    if not properties.admitted(canonical):
        return output,[]
    found = properties.world.body(output,properties.OWNER)
    if not found or properties.PREFIX_HOOK not in found[0] or hook('outward') in found[0]:
        return output,[]
    body = found[0]
    refs = []
    for kind, first, after, site, expected in reversed(MODES):
        start, end = extract(body,first,after)
        span = body[start:end]
        if 'selected(' in span or span.count('s.instruction_literal(') != expected:
            return output,[]
        refs.append(reference(kind,site,span))
        body = body[:start] + hook(kind) + normalized(span) + body[end:]
    return INCLUDE + output[:found.start()] + '\n'.join(refs) + '\n' + body + output[found.end():],[STAGE]
