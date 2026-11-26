"""Bind the common lifetime setup and vector normalization of Death Cycle 252480."""

import re

import native_world_leaves_replacements as world


OWNER = '252480'
STAGE = '2524cd'
PIN = 'ab5610fa3f803fbc32a50ec9888c67e9f67a886e2eb6918c2a38af7db86a293b'
SOURCE_OWNER = 'void source_run_death_cycle_runtime_tu15('
INCLUDE = '#include "rendering/effects/native_tu15_particle_velocity_properties.hpp"\n'
NAMESPACE = 'halo2::native_particle_velocity_properties_detail'
PREFIX = 'native_particle_velocity_properties_prefix_reference_' + OWNER
SUFFIX = 'native_particle_velocity_properties_suffix_reference_' + OWNER
SWITCH = 's.instruction_literal(0x2524cdu);'
SWITCH_LABEL = 'L2524cd: '
FIRST_SUFFIX = 'L252cc9: '
SECOND_SUFFIX = 'L252cce: '
CHILD = 'run_24ffe0(s);'
REPLAY = 'halo2::native_world_leaves_detail::replay_child(s,0x24ffe0u);'
ESTIMATE = 'source_estimate(s,'
DIRECT = '[](auto &value,auto){run_24ffe0(value);}'
PREFIX_HOOK = (
    '\n    // BEGIN native particle velocity properties setup\n'
    f'    if ({NAMESPACE}::bridge_prefix(s,{PREFIX},{DIRECT}))goto L2524cd;\n'
    '    // END native particle velocity properties setup'
)


def suffix_hook(store):
    return (
        '\n    // BEGIN native particle velocity properties finish\n'
        f'    if ({NAMESPACE}::bridge_suffix(s,{store},\n'
        f'        [](auto &value,bool first,auto &estimate){{{SUFFIX}(value,first,estimate);}},\n'
        f'        {DIRECT},[](auto &value,std::uint32_t input){{return source_estimate(value,input);}}))return;\n'
        '    // END native particle velocity properties finish\n'
    )


def admitted(canonical):
    body = world.body(canonical, OWNER)
    if not body or body[1] != 'WorldState' or SOURCE_OWNER not in canonical or world.digest(body) != PIN:
        return False
    mapping = world.mapping(canonical)
    words = {int(word, 16) for word in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', body[0])}
    return all(mapping.get(word, word) == word for word in words)


def extract(body):
    begin = body.index('{') + 1
    end = body.index(SWITCH, begin)
    suffix = body.index(FIRST_SUFFIX, end)
    result = body.index('throw std::runtime_error("Original solver fell outside authenticated CFG");', suffix)
    return begin, end, suffix, result


def normalized(body, suffix):
    for operation, expected, namespace in (
            ('mul', 9 if suffix else 1, 'native_tu15_leaves'),
            ('add', 2 if suffix else 0, 'native_world_leaves_detail')):
        generic = f'_mm_{operation}_ss('
        ordered = f'halo2::{namespace}::ordered_{operation}_ss('
        if body.count(generic) + body.count(ordered) != expected:
            raise ValueError(f'Particle velocity properties {operation}ss destinations drifted')
        body = body.replace(generic, ordered)
    return body


def prefix_reference(span, replay=True):
    if span.count(CHILD) != 2:
        raise ValueError('Particle velocity setup expected two authored curve calls')
    if replay:
        span = span.replace(CHILD, REPLAY)
    name = PREFIX if replay else PREFIX + '_live'
    return f'static void {name}(WorldState &s) {{\n' + normalized(span, False) + '}\n'


def suffix_reference(span, replay=True):
    if span.count(CHILD) != 1 or span.count(ESTIMATE) != 1:
        raise ValueError('Particle velocity finish child or reciprocal estimate drifted')
    if replay:
        span = span.replace(CHILD, REPLAY)
    name = SUFFIX if replay else SUFFIX + '_live'
    return (f'static void {name}(WorldState &s, bool store_first, auto &estimate) {{\n'
            '    if (!store_first) goto L252cce;\n'
            + normalized(span, True).replace(ESTIMATE, 'estimate(') + '}\n')


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    found = world.body(output, OWNER)
    if not found or found[1] != 'WorldState' or PREFIX_HOOK in found[0] or 'selected(' in found[0]:
        return output, []
    body = found[0]
    begin, end, tail, result = extract(body)
    prefix, suffix = body[begin:end], body[tail:result]
    if tuple(span.count('s.instruction_literal(') for span in (prefix,suffix)) != (20,45):
        return output, []
    refs = prefix_reference(prefix) + '\n' + suffix_reference(suffix) + '\n'
    finish = normalized(suffix, True)
    for label, store in ((FIRST_SUFFIX, 'true'), (SECOND_SUFFIX, 'false')):
        finish = finish.replace(label, label + suffix_hook(store), 1)
    replacement = (body[:begin] + PREFIX_HOOK + normalized(prefix, False) + SWITCH_LABEL
                   + body[end:tail] + finish + body[result:])
    return INCLUDE + output[:found.start()] + refs + replacement + output[found.end():], [STAGE]
