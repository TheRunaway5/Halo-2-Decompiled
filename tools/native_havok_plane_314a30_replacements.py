"""Bind the complete authenticated Live Support Havok plane constructor."""

import hashlib
import re

from native_mopp_ray_traversal_replacements import function


ENTRY = '314a30'
PIN = '709b82327b9db90dcc38e86ec52e3f3c5122042bab49218b0108f3b2639cbd61'
PREPARED = 'ba6296b3224132ee11bad32779445ab0e6f6cb7dfdb9d7167f53ea391efa4bbc'
ESTIMATE_PIN = 'bb50fdcd1d22250d871032ff227a462ef42c30981de20b9f23137bb1c60efd89'
INCLUDE = '#include "physics/runtime/native_tu15_havok_plane_314a30.hpp"\n'
DETAIL = 'halo2::native_havok_plane_314a30_detail'
TU15 = 's.image==SourcePhysicsContactImage::TitleUpdate15'
ESTIMATE = '[](auto &state,std::uint32_t input){return source_estimate(state,input);}'
HOOK = f'\n    if ({DETAIL}::bridge(s,{ESTIMATE})) return;\n'


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def retained(body):
    for original, ordered, count in (
        ('_mm_mul_ps(', 'halo2::native_tu15_leaves::ordered_mul_ps(', 5),
        ('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(', 4),
        ('_mm_sub_ps(', 'halo2::native_world_leaves_detail::ordered_sub_ps(', 3),
        ('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(', 4),
    ):
        if body.count(original) != count:
            raise ValueError('Prepared 314A30 SSE operands changed')
        body = body.replace(original, ordered)
    body, loads = re.subn(r'=v\(s\.memory,(\(0x[0-9a-f]+u\+s\.gp\[[024]\]\))\);',
                         lambda m: f'=({TU15}?{DETAIL}::load_aligned(s.memory,{m[1]}):v(s.memory,{m[1]}));', body)
    body, stores = re.subn(r'\bvw\((s\.memory,s\.observe,0x[0-9a-f]+u,\(0x[0-9a-f]+u\+s\.gp\[[14]\]\),s\.xmm\[\d\])\);',
                          lambda m: f'if({TU15}){{{DETAIL}::store_aligned({m[1]});}}else{{{m[0]}}}', body)
    push = 's.push(0x314a30u,s.reg(5,4,0));'
    estimate = 's.xmm[2]=_mm_move_ss(s.xmm[2],_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(source_estimate(s,u(_mm_cvtss_f32(s.xmm[0])))))));'
    if (loads, stores) != (5, 3) or body.count(push) != 1 or body.count(estimate) != 1:
        raise ValueError('Prepared 314A30 access or estimate changed')
    body = body.replace(push, f'if({TU15}){{{DETAIL}::push_frame(s);}}else{{{push}}}', 1)
    return body.replace(
        estimate,
        f'if({TU15}){{auto estimate={ESTIMATE};{DETAIL}::estimate_length(s,estimate);}}else{{{estimate}}}', 1)


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    original = function(canonical, 'run_' + ENTRY)
    prepared = function(output, 'run_' + ENTRY)
    route = f'case 0x{ENTRY}u:run_{ENTRY}(s);break;'
    if (sha(original) != PIN or sha(prepared) != PREPARED or INCLUDE in output or
            canonical.count(route) != 1 or output.count(route) != 1 or output.count(prepared) != 1 or
            any(sha(function(text, 'source_estimate')) != ESTIMATE_PIN for text in (canonical, output))):
        raise ValueError('Authenticated 314A30 Live Support route changed')
    body = retained(prepared)
    opening = body.index('{') + 1
    return INCLUDE + output.replace(prepared, body[:opening] + HOOK + body[opening:], 1), [ENTRY]
