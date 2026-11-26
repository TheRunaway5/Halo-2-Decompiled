"""Bind only the pinned canonical Contact copy of the collision cache."""

import importlib.util
from pathlib import Path
import re

world_path = Path(__file__).with_name('native_collision_cache_309060_replacements.py')
world_spec = importlib.util.spec_from_file_location('native_world_collision_cache_binding', world_path)
world = importlib.util.module_from_spec(world_spec)
world_spec.loader.exec_module(world)


MARKER = 'static void run_305560(State &s)'
BODY = re.compile(r'^' + re.escape(MARKER) + r' \{.*?^\}', re.M | re.S)
PIN = 'f3f6cbb830da4d7a41d15ad9d302603b8798fef63ddc4aa9f29ee8a33a6341c1'
PREPARED = 'ed912e733178f4e6dcd42ed5e0c88eb9634597619222b334aa363152425ad026'
REFERENCE = 'native_contact_collision_cache_reference_305560'
HOOK = (f'\n    if ({world.DETAIL}::bridge_contact(s,{REFERENCE},'
        '\n        [](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);},'
        '\n        [](auto &v){run_2e0ba0(v);},[](auto &v){run_2e0cc0(v);})) return;\n')


def original(canonical):
    bodies = BODY.findall(canonical)
    if len(bodies) != 1 or world.digest(bodies[0]) != PIN:
        raise ValueError('Original Contact 305560 collision cache changed')
    pcs = re.findall(r'services\.instruction\((0x[0-9a-f]+u)\)', bodies[0])
    if len(pcs) != 82 or len(set(pcs)) != 82:
        raise ValueError('Original Contact 305560 observation sites changed')
    return bodies[0]


def check_routes(text, canonical=False):
    if (text.count('case 0x305560u:run_305560(s);break;') != 1 or
            'void source_run_physics_contact_runtime(' not in text):
        raise ValueError('Missing public Contact cache route')
    for child in world.DIRECT:
        if text.count(f'static void run_{child}(State &s) {{') != 1:
            raise ValueError(f'Missing real Contact cache child {child}')
    if canonical:
        for disc, tu15 in world.SELECTED.items():
            if text.count(f'case 0x{disc}u:return 0x{tu15}u;') != 1:
                raise ValueError(f'Contact cache executable operand {disc} changed')
        target = 'if(target==selected(0x305560u,s.image)){run_305560(s);return;}'
    else:
        target = 'if(target==(s.image==SourcePhysicsContactImage::MapPack11?0x305560u:0x309060u)){run_305560(s);return;}'
    if text.count(target) != 1:
        raise ValueError('Missing internal Contact cache dispatcher route')


def reference(canonical, replay=True):
    body = original(canonical)
    if replay:
        for site in world.VIRTUAL:
            call = f'dispatch(s,target,0x{site}u,false);'
            if body.count(call) != 1:
                raise ValueError(f'Contact cache virtual call {site} changed')
            body = body.replace(call, 'halo2::native_object_tick_detail::replay(s,target);', 1)
        for child in world.DIRECT:
            call = f'run_{child}(s);'
            if body.count(call) != 1:
                raise ValueError(f'Contact cache direct call {child} changed')
            body = body.replace(call, f'halo2::native_object_tick_detail::replay(s,selected(0x{child}u,s.image));', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return world.ordered(body.replace(MARKER, f'static void {name}(State &s)', 1)) + '\n'


def install(canonical, emitted):
    check_routes(canonical, canonical=True)
    check_routes(emitted)
    current = BODY.search(emitted)
    if not current:
        raise ValueError('Missing prepared Contact 305560 cache')
    retained = reference(canonical)
    if REFERENCE in emitted:
        restored = current[0].replace(HOOK, '', 1).replace(
            'halo2::native_world_leaves_detail::ordered_sub_ps(', '_mm_sub_ps(')
        if (emitted.count(world.INCLUDE) != 1 or emitted.count(retained) != 1 or
                current[0].count(HOOK) != 1 or world.digest(restored) != PREPARED):
            raise ValueError('Prepared Contact cache initializer changed')
        return emitted, ['305560']
    if world.digest(current[0]) != PREPARED or world.INCLUDE in emitted:
        raise ValueError('Specialized 305560 cache differs from the pinned Contact route')
    head = current[0].index('{') + 1
    native = world.ordered(current[0][:head] + HOOK + current[0][head:])
    return world.INCLUDE + emitted[:current.start()] + retained + native + emitted[current.end():], ['305560']
