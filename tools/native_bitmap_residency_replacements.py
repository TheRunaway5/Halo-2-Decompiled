"""Bind the complete TU1.5 bitmap residency owner and its original children."""

from hashlib import sha256
import re


BODY = re.compile(r'^static void run_12f910\(WorldState &s\) \{.*?^\}(?=\n\nstatic void run_1300b0)', re.M | re.S)
PIN = '53504f27959ac6aff5ece6b46a7b886865a4917b0edc0286b3c72595677e8650'
PREPARED = 'faab43ae7c4ffd1c4a2328a0c5a499a3a66815daff56a0bd17fe1914aa623153'
HEADER = 'rendering/models/native_tu15_bitmap_residency.hpp'
INCLUDE = f'#include "{HEADER}"\n'
CHILDREN = (('12f9cc', '12f330', '12f9d1'), ('12fa25', '12f7c0', '12fa2a'),
            ('12fa42', '12f620', '12fa47'), ('12fa9a', '12f4a0', '12fa9f'))
PUSH_SITES = ('12f91f', '12f926', '12f939', '12f9a5', '12f9cc', '12fa25', '12fa42',
              '12fa83', '12fa88', '12fa8d', '12fa92', '12fa99', '12fa9a')
PUSH = re.compile(r'\bs\.push\((0x([0-9a-f]+)u,[^;\n]+)\);')
ATOMIC_PUSH = ('if(s.image==SourcePhysicsContactImage::TitleUpdate15)'
               'halo2::native_two_bone_solve::push_word(s,')
HOOK = ('\n    if(halo2::native_bitmap_residency_detail::bridge(s,[](auto &v,std::uint32_t target) {\n'
        '        switch(target) {\n' + ''.join(f'        case 0x{p}u: run_{p}(v); return;\n' for _, p, _ in CHILDREN) +
        '        default: throw std::logic_error("Unknown bitmap residency child");\n'
        '        }\n    }))return;\n')


def digest(text):
    return sha256(text.encode()).hexdigest()


def retained(body):
    for name, target, count in (('_mm_add_ss', 'native_world_leaves_detail::ordered_add_ss', 2),
                                ('_mm_mul_ss', 'native_tu15_leaves::ordered_mul_ss', 1)):
        if body.count(f'{name}(') != count:
            raise ValueError(f'Bitmap residency operand {name} changed')
        body = body.replace(f'{name}(', f'halo2::{target}(')
    if tuple(match[2] for match in PUSH.finditer(body)) != PUSH_SITES:
        raise ValueError('Bitmap residency stack store sites changed')
    # Commit the stack only after the provider accepts the store. The
    # committed stack is visible to any subsequent observer rejection.
    return PUSH.sub(lambda match: f'{ATOMIC_PUSH}{match[1]});else {match[0]}', body)


def bound_body(prepared):
    body = retained(prepared)
    opening = body.index('{') + 1
    return body[:opening] + HOOK + body[opening:]


def install(canonical, output):
    original = BODY.search(canonical)
    if 'void source_run_physics_live_support_tu15(' not in canonical or not original:
        return output, []
    if digest(original[0]) != PIN:
        raise ValueError('Canonical bitmap residency body changed')
    if canonical.count('case 0x12f910u:run_12f910(s);break;') != 1 or canonical.count('case 0x12f910u:run_12f910(s);return;') != 1:
        raise ValueError('Bitmap residency dispatch changed')
    for site, target, resume in CHILDREN:
        call = f's.push(0x{site}u,0x{resume}u);run_{target}(s);'
        if original[0].count(call) != 1 or canonical.count(f'static void run_{target}(WorldState &s) {{') != 1:
            raise ValueError(f'Bitmap residency child {target} changed')
    prepared = BODY.search(output)
    if not prepared or digest(prepared[0]) != PREPARED or INCLUDE in output:
        raise ValueError('Prepared bitmap residency body changed')
    return INCLUDE + output[:prepared.start()] + bound_body(prepared[0]) + output[prepared.end():], ['12f910']
