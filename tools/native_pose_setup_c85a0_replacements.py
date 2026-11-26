"""Bind the authenticated TU1.5 pose preparation and its first-axis joins."""

import hashlib
import re


ENTRY = 'c85a0'
INCLUDE = '#include "physics/runtime/native_tu15_pose_setup_c85a0.hpp"\n'
DETAIL = 'halo2::native_pose_setup_c85a0'
BODY = re.compile(r'^static void run_c85a0\(WorldState &s\) \{.*?'
                  r'^throw std::runtime_error\("Original solver fell outside authenticated CFG"\);\n\}', re.M | re.S)
CANONICAL = '3655c3f355e61cf6c140d775631cb4e80942e194d4f663fe35fd2a13bd6819f4'
PREPARED = '7d7a31e1d0a822840ce00b82e7c572acffcfd32424685e0ec556b546845ada12'
HOOK = f'''
    switch({DETAIL}::run(s,[](auto &v){{run_c6e50(v);}},[](auto &v){{run_cdf30(v);}})) {{
    case {DETAIL}::Resume::returned: return;
    case {DETAIL}::Resume::authored_axis: goto Lc8704;
    case {DETAIL}::Resume::blend: goto Lc8827;
    case {DETAIL}::Resume::retained_entry: break;
    }}
'''
PUSH_SITES = ('c85bd', 'c85d3', 'c8606', 'c8677', 'c86c6', 'c86d6', 'c86d8', 'c86d9')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def bind_body(body):
    if digest(body) != PREPARED:
        raise ValueError('Prepared C85A0 pose owner changed')
    for site in PUSH_SITES:
        prefix = f's.push(0x{site}u,'
        pattern = re.compile(re.escape(prefix) + r'[^;]*\);')
        matches = pattern.findall(body)
        if len(matches) != 1:
            raise ValueError(f'C85A0 stack boundary {site} changed')
        original = matches[0]
        precise = original.replace('s.push(', 'halo2::native_two_bone_solve::push_word(s,', 1)
        body = body.replace(original, f'if({DETAIL}::selected(s)){{{precise}}}else{{{original}}}', 1)
    for site, lane in (('c867f', 0), ('c8685', 1)):
        original = f's.instruction_literal(0x{site}u); s.xmm[{lane}]=_mm_mul_ss('
        if body.count(original) != 1:
            raise ValueError(f'C85A0 scalar multiplication {site} changed')
        body = body.replace(original, original.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss('), 1)
    opening = body.index('{') + 1
    return body[:opening] + HOOK + body[opening:]


def install(canonical, output):
    if 'void source_run_physics_live_support_tu15(' not in canonical:
        return output, []
    original = BODY.findall(canonical)
    prepared = BODY.search(output)
    if len(original) != 1 or digest(original[0]) != CANONICAL or not prepared or INCLUDE in output:
        raise ValueError('Canonical C85A0 pose owner changed')
    for text in (canonical, output):
        for route in ('case 0xc85a0u:run_c85a0(s);break;', 'case 0xc85a0u:run_c85a0(s);return;',
                      's.push(0xc9027u,0xc902cu);run_c85a0(s);'):
            if text.count(route) != 1:
                raise ValueError('C85A0 public, nested, or direct caller route changed')
        for child in ('c6e50', 'cdf30'):
            if text.count(f'static void run_{child}(WorldState &s) {{') != 1:
                raise ValueError(f'C85A0 genuine call target {child} changed')
    return INCLUDE + output[:prepared.start()] + bind_body(prepared[0]) + output[prepared.end():], [ENTRY]
