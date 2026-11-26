"""Authenticate and bind the selected TU1.5 World pool-pressure owner."""

import hashlib
import re


INCLUDE = '#include "physics/runtime/native_tu15_world_pressure_2335b0.hpp"\n'
DETAIL = 'halo2::native_world_pressure_2335b0'
MARKER = 'static void run_22ecb0(ObservedWorldState &s)'
BODY = re.compile(r'^' + re.escape(MARKER) + r' \{.*?^\}', re.M | re.S)
CANONICAL = '5db7065b56e32ac99698189a6590ecf78c2d987addcbc03c397254a2abc0b4cd'
PREPARED = '4062fbadd506a8c0614e2db17eaafdfee5bc4e39c037a4039e38e0fd1abbc463'
HOOK = f'\n    if ({DETAIL}::run(s)) return;'
PUSH = 's.push(world_observation_pc(0x22ecb0u,0x2335b0u,s),s.reg(6,4,0));'
PUSH_PRECISE = (f'if ({DETAIL}::selected(s)) {DETAIL}::push_esi(s);\n'
                f'else {PUSH}')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def check_routes(text, canonical=False):
    if (text.count('case 0x22ecb0u:run_22ecb0(s);break;') != 1 or
            text.count('void source_run_physics_world_runtime_selected_observations(') != 1):
        raise ValueError('Missing public World pressure route')
    if canonical:
        for disc, tu15 in (('22ecb0', '2335b0'), ('f0384', 'f0384'), ('465094', '46a134')):
            if text.count(f'case 0x{disc}u:return 0x{tu15}u;') != 1:
                raise ValueError(f'World pressure operand {disc} changed')
        target = 'selected(0x22ecb0u,s.image)'
    else:
        target = '(s.image==SourcePhysicsContactImage::MapPack11?0x22ecb0u:0x2335b0u)'
    if text.count(f'if(target=={target}){{run_22ecb0(s);return;}}') != 1:
        raise ValueError('Missing internal World pressure route')


def install(canonical, emitted):
    check_routes(canonical, canonical=True)
    check_routes(emitted)
    original = BODY.findall(canonical)
    prepared = BODY.search(emitted)
    if len(original) != 1 or digest(original[0]) != CANONICAL or not prepared:
        raise ValueError('Original World pressure owner changed')
    sites = re.findall(r'instruction\(world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)\)', original[0])
    if (len(sites) != 16 or len(set(sites)) != 16 or
            any(int(tu15, 16) - int(disc, 16) != 0x4900 for disc, tu15 in sites)):
        raise ValueError('World pressure observation sites changed')
    if INCLUDE in emitted:
        restored = prepared[0].replace(HOOK, '', 1).replace(PUSH_PRECISE, PUSH, 1)
        if (emitted.count(INCLUDE) != 1 or prepared[0].count(HOOK) != 1 or
                prepared[0].count(PUSH_PRECISE) != 1 or digest(restored) != PREPARED):
            raise ValueError('Prepared World pressure bridge changed')
        return emitted, ['2335b0']
    body = prepared[0]
    if digest(body) != PREPARED or body.count(PUSH) != 1:
        raise ValueError('Specialized World pressure owner differs from its pinned route')
    head = body.index('{') + 1
    replacement = body[:head] + HOOK + body[head:].replace(PUSH, PUSH_PRECISE, 1)
    return INCLUDE + emitted[:prepared.start()] + replacement + emitted[prepared.end():], ['2335b0']
