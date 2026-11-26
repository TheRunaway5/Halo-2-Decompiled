"""Bind the selected World MOPP quantizer to its exact traversal handoff."""

import hashlib
import re


ENTRY = '30a1e0'
CANONICAL = '3066e0'
CHILD = '305e40'
ACTUAL_CHILD = '309940'
INCLUDE = '#include "physics/runtime/native_tu15_mopp_quantized_aabb_30a1e0.hpp"\n'
REFERENCE = 'native_mopp_quantized_aabb_reference_3066e0'
MARKER = 'static void run_3066e0(ObservedWorldState &s)'
BODY = re.compile(r'^' + re.escape(MARKER) + r' \{.*?^\}', re.M | re.S)
PIN = '603a0c0db5ca2c3e230f586fed96db1080946d64908afec080f85f2890e2b479'
PREPARED = 'b3d397c9bf22ac735a06352c009ce35bfa388caaa11f5b80836025891c4a11ae'
SELECTED = {CANONICAL: ENTRY, CHILD: ACTUAL_CHILD, '30680c': '30a30c'}
HOOK = ('\n    if (halo2::native_mopp_quantized_aabb_30a1e0_detail::bridge('
        f's,{REFERENCE},[](auto &v){{run_{CHILD}(v);}})) return;\n')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def original(canonical):
    matches = BODY.findall(canonical)
    if len(matches) != 1 or digest(matches[0]) != PIN:
        raise ValueError('Original selected World 30A1E0 MOPP quantizer changed')
    sites = re.findall(r'instruction\(world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)\)', matches[0])
    if (len(sites) != 100 or len(set(sites)) != 100 or
            any(int(tu15, 16) - int(disc, 16) != 0x3b00 for disc, tu15 in sites)):
        raise ValueError('Selected World MOPP quantizer instruction positions changed')
    return matches[0]


def check_routes(text, canonical=False):
    if (text.count(f'case 0x{CANONICAL}u:run_{CANONICAL}(s);break;') != 1 or
            text.count(f'static void run_{CHILD}(ObservedWorldState &s) {{') != 1 or
            'void source_run_physics_world_runtime_selected_observations(' not in text):
        raise ValueError('Missing selected World MOPP quantizer route or traversal')
    if canonical:
        for disc, tu15 in SELECTED.items():
            if text.count(f'case 0x{disc}u:return 0x{tu15}u;') != 1:
                raise ValueError(f'Selected World MOPP quantizer executable operand {disc} changed')
        target = f'if(target==selected(0x{CANONICAL}u,s.image)){{run_{CANONICAL}(s);return;}}'
    else:
        target = (f'if(target==(s.image==SourcePhysicsContactImage::MapPack11?0x{CANONICAL}u:0x{ENTRY}u))'
                  f'{{run_{CANONICAL}(s);return;}}')
    if text.count(target) != 1:
        raise ValueError('Missing internal MOPP quantizer dispatcher route')


def reference(canonical, replay=True):
    body = original(canonical)
    call = f'selected(0x30680cu,s.image));run_{CHILD}(s);'
    if body.count(call) != 1:
        raise ValueError('Selected World MOPP quantizer traversal handoff changed')
    name = REFERENCE
    if replay:
        body = body.replace(call, (f'selected(0x30680cu,s.image));'
                                  f'halo2::native_object_tick_detail::replay(s,0x{ACTUAL_CHILD}u);'), 1)
    else:
        name += '_live'
    return body.replace(MARKER, f'static void {name}(ObservedWorldState &s)', 1) + '\n'


def install(canonical, emitted):
    check_routes(canonical, canonical=True)
    check_routes(emitted)
    current = BODY.search(emitted)
    if not current:
        raise ValueError('Missing prepared World MOPP quantizer')
    retained = reference(canonical)
    if REFERENCE in emitted:
        if (emitted.count(INCLUDE) != 1 or emitted.count(retained) != 1 or
                current[0].count(HOOK) != 1 or digest(current[0].replace(HOOK, '', 1)) != PREPARED):
            raise ValueError('Prepared World MOPP quantizer changed')
        return emitted, [ENTRY]
    if digest(current[0]) != PREPARED or INCLUDE in emitted:
        raise ValueError('MOPP quantizer operand specialization differs from the pinned selected route')
    head = current[0].index('{') + 1
    native = current[0][:head] + HOOK + current[0][head:]
    return INCLUDE + emitted[:current.start()] + retained + native + emitted[current.end():], [ENTRY]
