"""Bind the selected World cache initializer while retaining its original routes."""

import hashlib
import re


INCLUDE = '#include "physics/runtime/native_tu15_collision_cache_309060.hpp"\n'
DETAIL = 'halo2::native_collision_cache_309060_detail'
REFERENCE = 'native_collision_cache_reference_305560'
MARKER = 'static void run_305560(ObservedWorldState &s)'
BODY = re.compile(r'^' + re.escape(MARKER) + r' \{.*?^\}', re.M | re.S)
PIN = '18e0e52c9542ba0252769503226ee9883693aeb15d668be030afa0aad00dfebc'
PREPARED = 'e949b6a5a084830186a2b7d83ec6981413d7b868e1250a54a6e32b042e42d049'
VIRTUAL = ('30557a', '305589', '3055be', '30560f', '305634', '30563e')
DIRECT = {'2e0ba0': '2e4690', '2e0cc0': '2e47b0'}
SELECTED = {
    '305560': '309060', '30557d': '30907d', '30558c': '30908c', '30559e': '30909e',
    '3055c1': '3090c1', '3055ed': '3090ed', '305612': '309112', '305637': '309137',
    '305641': '309141', '418ae0': '461960', **DIRECT,
}
HOOK = (f'\n    if ({DETAIL}::bridge(s,{REFERENCE},'
        '\n        [](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);},'
        '\n        [](auto &v){run_2e0ba0(v);},[](auto &v){run_2e0cc0(v);})) return;\n')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def original(canonical):
    found = BODY.findall(canonical)
    if len(found) != 1 or digest(found[0]) != PIN:
        raise ValueError('Original World 309060 cache initializer changed')
    pairs = re.findall(r'instruction\(world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)\)', found[0])
    if (len(pairs) != 82 or len(set(pairs)) != 82 or
            any(int(tu15, 16) - int(disc, 16) != 0x3b00 for disc, tu15 in pairs)):
        raise ValueError('World 309060 instruction observations changed')
    return found[0]


def check_routes(text, canonical=False):
    if (text.count('case 0x305560u:run_305560(s);break;') != 1 or
            'void source_run_physics_world_runtime_selected_observations(' not in text):
        raise ValueError('Missing public World cache route')
    for child in DIRECT:
        if text.count(f'static void run_{child}(ObservedWorldState &s) {{') != 1:
            raise ValueError(f'Missing real World cache child {child}')
    if canonical:
        for disc, tu15 in SELECTED.items():
            if text.count(f'case 0x{disc}u:return 0x{tu15}u;') != 1:
                raise ValueError(f'World cache executable operand {disc} changed')
        target = 'if(target==selected(0x305560u,s.image)){run_305560(s);return;}'
    else:
        target = 'if(target==(s.image==SourcePhysicsContactImage::MapPack11?0x305560u:0x309060u)){run_305560(s);return;}'
    if text.count(target) != 1:
        raise ValueError('Missing internal World cache dispatcher route')


def ordered(body):
    if body.count('_mm_sub_ps(') != 2:
        raise ValueError('World 309060 cache subtraction sites changed')
    return body.replace('_mm_sub_ps(', 'halo2::native_world_leaves_detail::ordered_sub_ps(')


def reference(canonical, replay=True):
    body = original(canonical)
    if replay:
        for site in VIRTUAL:
            call = f'dispatch(s,target,0x{site}u,false);'
            if body.count(call) != 1:
                raise ValueError(f'World cache virtual call {site} changed')
            body = body.replace(call, 'halo2::native_object_tick_detail::replay(s,target);', 1)
        for child in DIRECT:
            call = f'run_{child}(s);'
            if body.count(call) != 1:
                raise ValueError(f'World cache direct call {child} changed')
            body = body.replace(call, f'halo2::native_object_tick_detail::replay(s,selected(0x{child}u,s.image));', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return ordered(body.replace(MARKER, f'static void {name}(ObservedWorldState &s)', 1)) + '\n'


def install(canonical, emitted):
    check_routes(canonical, canonical=True)
    check_routes(emitted)
    current = BODY.search(emitted)
    if not current:
        raise ValueError('Missing prepared World 309060 cache')
    oracle = reference(canonical)
    if REFERENCE in emitted:
        restored = current[0].replace(HOOK, '', 1).replace(
            'halo2::native_world_leaves_detail::ordered_sub_ps(', '_mm_sub_ps(')
        if (emitted.count(INCLUDE) != 1 or emitted.count(oracle) != 1 or
                current[0].count(HOOK) != 1 or digest(restored) != PREPARED):
            raise ValueError('Prepared World cache initializer changed')
        return emitted, ['309060']
    if digest(current[0]) != PREPARED or INCLUDE in emitted:
        raise ValueError('Specialized 309060 cache initializer differs from the pinned World route')
    head = current[0].index('{') + 1
    replacement = ordered(current[0][:head] + HOOK + current[0][head:])
    return INCLUDE + emitted[:current.start()] + oracle + replacement + emitted[current.end():], ['309060']
