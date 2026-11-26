"""Bind the live TU1.5 convex contact setup and keep its authored child routes."""
import re

from native_model_marker_lookup_replacements import digest, function


INCLUDE = '#include "physics/runtime/native_tu15_convex_process_prefix.hpp"\n'
NAMESPACE = 'halo2::native_convex_process_prefix_detail'
CONTINUATION = NAMESPACE + '::Continuation'
REFERENCE = 'native_convex_process_reference_2f8b60'
NAME = 'run_2f8b60'
SIGNATURE = 'static void run_2f8b60(WorldState &s)'
CANONICAL_PIN = 'e7771ea6750494822fe13e464f3c4226f157fa13af9e5f422a4e318f406c1335'
PREPARED_PIN = 'c6b6064eb26a1fca8817b2e0b38f8bef99b85adbec1ae74c358cfb90cfd1ef39'
CALLER_PIN = '44f90283d363b214816b5b7f8db13b6e964d04509b9ad6857461b886a6af2a1d'
CALL_SITE = 's.push(0x2e5650u,0x2e5655u);run_2f8b60(s);'
UNIT = 'void source_run_physics_live_support_tu15('
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)
COMPOSE = 's.instruction_literal(0x2f8c8eu);'
REMOVE = 'L2f8e64: s.instruction_literal(0x2f8e64u);'
COMPOSE_ROUTE = '{const auto target=0x2e4550u;s.push(0x2f8c8eu,0x2f8c93u);dispatch(s,target,0x2f8c8eu,false);}'
REMOVE_ROUTE = '{const auto target=0x30ca60u;s.push(0x2f8e70u,0x2f8e75u);dispatch(s,target,0x2f8e70u,false);}'
MULTIPLY = 's.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[1]);'
ORDERED_MULTIPLY = MULTIPLY.replace('_mm_mul_ps', 'halo2::native_tu15_leaves::ordered_mul_ps')
ORIGINAL_CLOCK = ('if(!s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");'
                  '{const auto x=s.services.timestamp_counter();')
HOOK = (f'\n    switch ({NAMESPACE}::prefix(s,{REFERENCE})) {{\n'
        f'    case {CONTINUATION}::compose_transforms: goto L2f8c8e;\n'
        f'    case {CONTINUATION}::remove_cached: goto L2f8e64;\n'
        f'    case {CONTINUATION}::retained: break;\n'
        '    }\n')


def replace_once(source, before, after):
    if source.count(before) != 1:
        raise ValueError('Expected one pinned 2F8B60 site: ' + before)
    return source.replace(before, after, 1)


def check_routes(source):
    for route in ('case 0x2f8b60u:return 0x2f8b60u;',
                  'case 0x2f8b60u:run_2f8b60(s);return;',
                  'case 0x2f8b60u:run_2f8b60(s);break;', CALL_SITE):
        if source.count(route) != 1:
            raise ValueError('Live 2F8B60 dispatch route changed: ' + route)


def reference(prepared, replay=True):
    head, separator, _ = prepared.partition(COMPOSE)
    if not separator or head.count(ORIGINAL_CLOCK) != 3:
        raise ValueError('2F8B60 profiling or continuation changed')
    head = replace_once(head, 'goto L2f8e64;', f'return {CONTINUATION}::remove_cached;')
    head = replace_once(head, MULTIPLY, ORDERED_MULTIPLY)
    if replay:
        head = head.replace(ORIGINAL_CLOCK, '{const auto x=clocks.read();')
        signature = f'{CONTINUATION} {REFERENCE}(WorldState &s,{NAMESPACE}::ClockSamples &clocks)'
    else:
        signature = f'static {CONTINUATION} fixture_convex_process_live_prefix(WorldState &s)'
    head = replace_once(head, SIGNATURE, signature)
    return head + f'return {CONTINUATION}::compose_transforms;\n}}'


def install(canonical, emitted):
    if UNIT not in canonical:
        return emitted, False
    raw = function(canonical, NAME)
    if digest(raw) != CANONICAL_PIN or digest(function(canonical, 'run_2e5320')) != CALLER_PIN:
        raise ValueError('Canonical 2F8B60 body or direct caller changed')
    check_routes(canonical)
    check_routes(emitted)
    current = function(emitted, NAME)
    if HOOK in current:
        original = replace_once(current, HOOK, '\n')
        original = replace_once(original, 'L2f8c8e: ' + COMPOSE, COMPOSE)
        original = replace_once(original, 's.instruction_literal(0x2f8c13u); ' + ORDERED_MULTIPLY, 's.instruction_literal(0x2f8c13u); ' + MULTIPLY)
        if digest(original) != PREPARED_PIN or emitted.count(INCLUDE) != 1 or emitted.count(reference(original)) != 1:
            raise ValueError('Installed 2F8B60 staging changed')
        return emitted, True
    if digest(current) != PREPARED_PIN or INCLUDE in emitted or REFERENCE in emitted:
        raise ValueError('Prepared 2F8B60 body changed')
    if COMPOSE_ROUTE not in current or REMOVE_ROUTE not in current or REMOVE not in current:
        raise ValueError('2F8B60 stage lost a required original child')
    oracle = reference(current)
    prefix, tail = current.split(COMPOSE, 1)
    prefix = replace_once(prefix, MULTIPLY, ORDERED_MULTIPLY)
    hooked = prefix.replace(SIGNATURE + ' {\n', SIGNATURE + ' {' + HOOK, 1)
    hooked += 'L2f8c8e: ' + COMPOSE + tail
    first = FIRST_BODY.search(emitted)
    if not first:
        raise ValueError('Missing shared WorldState partition for the convex process replay')
    output = emitted.replace(current, hooked, 1)
    return INCLUDE + output[:first.start()] + oracle + '\n\n' + output[first.start():], True
