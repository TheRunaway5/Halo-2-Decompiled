"""Bind the actual TU1.5 World position transform, preserving its retained body."""

import hashlib
import re


INCLUDE = '#include "physics/runtime/native_tu15_world_position_transform_2e4690.hpp"\n'
MARKER = 'static void run_2e0ba0(ObservedWorldState &s)'
BODY = re.compile(r'^' + re.escape(MARKER) + r' \{.*?^\}',re.M | re.S)
PIN = 'fe559cc5c2d5d4ab3ad9bc66d2d4f0fe9ca0783b7050be3abf68efca01a786fa'
HOOK = '\n    if (halo2::native_world_position_transform_2e4690_detail::bridge(s)) return;\n'
OPERANDS = {'_mm_mul_ps(': 'halo2::native_tu15_leaves::ordered_mul_ps(',
            '_mm_add_ps(': 'halo2::native_tu15_leaves::ordered_add_ps('}


def original(canonical):
    bodies = BODY.findall(canonical)
    if len(bodies) != 1 or hashlib.sha256(bodies[0].encode()).hexdigest() != PIN:
        raise ValueError('World 2E4690 original body changed')
    sites = re.findall(r'instruction\(world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)\)',bodies[0])
    if len(sites) != 26 or len(set(sites)) != 26 or any(int(b,16)-int(a,16)!=0x3af0 for a,b in sites):
        raise ValueError('World 2E4690 observation sites changed')
    return bodies[0]


def translated(canonical):
    text = original(canonical)
    for old,new in OPERANDS.items():
        if text.count(old)!=3:
            raise ValueError(f'World 2E4690 ordered operands changed for {old}')
        text = text.replace(old,new)
    return text


def install(canonical, alias):
    selector = 'case 0x2e0ba0u:return 0x2e4690u;'
    public_dispatch = 'case 0x2e0ba0u:run_2e0ba0(s);break;'
    source_dispatch = 'if(target==selected(0x2e0ba0u,s.image)){run_2e0ba0(s);return;}'
    prepared_dispatch = 'if(target==(s.image==SourcePhysicsContactImage::MapPack11?0x2e0ba0u:0x2e4690u)){run_2e0ba0(s);return;}'
    if (canonical.count(selector)!=1 or canonical.count(source_dispatch)!=1 or alias.count(prepared_dispatch)!=1 or
            any(text.count(public_dispatch)!=1 for text in (canonical,alias)) or
            'void source_run_physics_world_runtime_selected_observations(' not in alias):
        raise ValueError('World 2E4690 dispatch route changed')
    body = BODY.search(alias)
    if not body:
        raise ValueError('World 2E4690 prepared body absent')
    raw = original(canonical)
    fallback = translated(canonical)
    insertion = fallback.index('{')+1
    bound = fallback[:insertion]+HOOK+fallback[insertion:]
    if body[0]==bound and alias.count(INCLUDE)==1:
        return alias,['2e4690']
    if body[0]!=raw or INCLUDE in alias:
        raise ValueError('World 2E4690 prepared body changed')
    return INCLUDE+alias[:body.start()]+bound+alias[body.end():],['2e4690']
