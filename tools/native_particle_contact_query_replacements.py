"""Bind the particle sweep inputs, real collision query, and query result gates."""

import native_particle_contact_walk_replacements as walk
import native_particle_contact_flags_replacements as flags


STAGE = '2c2fd4'
REFERENCE = 'native_particle_contact_query_reference_' + walk.OWNER
NAMESPACE = 'halo2::native_particle_contact_query_detail'
INCLUDE = '#include "rendering/effects/native_tu15_particle_contact_query.hpp"\n'
END = 'L2c30b8: '
REJECT = 'L2c334a: '
CALL = 'dispatch(s,target,0x2c308bu,false);'
MUL = 'halo2::native_tu15_leaves::ordered_mul_ss('
HOOK = (
    '\n    // BEGIN native particle contact sweep query 2c2fd4\n'
    f'    switch ({NAMESPACE}::bridge_query(s,{REFERENCE},\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t at){dispatch(value,target,at,false);})) {\n'
    f'    case {NAMESPACE}::Resume::response: goto L2c30b8;\n'
    f'    case {NAMESPACE}::Resume::advance: goto L2c3375;\n'
    '    default: break;\n'
    '    }\n'
    '    // END native particle contact sweep query 2c2fd4\n'
)


def extract(body):
    begin = body.index(flags.NEXT) + len(flags.NEXT)
    end = body.index(END, begin)
    rejected = body.index(REJECT, end)
    next_span = body.index(walk.COUNTDOWN, rejected)
    return begin, end, rejected, next_span


def normalized(source):
    if source.count('_mm_mul_ss(') + source.count(MUL) != 6:
        raise ValueError('Particle contact sweep MULSS destinations drifted')
    return source.replace('_mm_mul_ss(', MUL)


def reference(source, tails, replay=True):
    if source.count(CALL) != 1 or tails.count('goto L2c3375;') != 2:
        raise ValueError('Particle contact sweep query call or rejection continuations drifted')
    if replay:
        source = source.replace(CALL, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    tails = tails.replace('goto L2c3375;', 'return Resume::advance;')
    return (f'static {NAMESPACE}::Resume {name}(WorldState &s) {{\n'
            f'    using {NAMESPACE}::Resume;\n'
            + normalized(source) + '    return Resume::response;\n' + tails + '}\n')


def install(canonical, output):
    if not walk.admitted(canonical):
        return output, []
    found = walk.world.body(output, walk.OWNER)
    if not found or flags.HOOK not in found[0] or HOOK in found[0]:
        return output, []
    body = found[0]
    begin, end, reject, next_span = extract(body)
    source, tails = body[begin:end], body[reject:next_span]
    if 'selected(' in source + tails or tuple(s.count('s.instruction_literal(') for s in (source,tails)) != (52,8):
        return output, []
    replacement = body[:begin] + HOOK + normalized(source) + body[end:]
    replay = reference(source, tails)
    return INCLUDE + output[:found.start()] + replay + '\n' + replacement + output[found.end():], [STAGE]
