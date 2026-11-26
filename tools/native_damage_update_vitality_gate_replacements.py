"""Bind the selected-image damage vitality scale and shield gate."""
import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_update_vitality_gate.hpp"\n'
REFERENCE = 'native_damage_update_vitality_gate_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_vitality_gate_detail'
ENTRIES = {'d767b': 'clear', 'd7689': 'preserve'}
EXITS = {'d7751': 'armor', 'd77a2': 'recovery', 'd7700': 'shield'}
OPERANDS = {0x4edf18: 0x4f3218, 0x465030: 0x46a0d0, 0x465048: 0x46a0e8,
            0xd76ca: 0xd861a, 0x159ef0: 0x15b2d0}


def admitted(canonical):
    mapping = prefix.world.mapping(canonical)
    return owner.admitted(canonical) and all(mapping.get(key) == pc for key, pc in OPERANDS.items())


def reference(body, replay=True):
    arms = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{pc};\n'
                   for pc, entry in ENTRIES.items())
    start = body.index('\nLd767b:')
    end = body.index('\nif(s.services.instruction){s.services.instruction(damage_observation_pc(0xd7700u,', start)
    source = body[start:end]
    for target, outcome in EXITS.items():
        source = source.replace(f'goto L{target};', f'return {NAMESPACE}::Next::{outcome};')
    if replay:
        source = source.replace('run_159ef0(s);', 'halo2::native_object_tick_detail::replay(s,0x15b2d0u);')
    suffix = '' if replay else '_live'
    return (f'static {NAMESPACE}::Next {REFERENCE}{suffix}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{arms}    }}\n'
            f'    throw std::logic_error("Invalid damage vitality gate entry");{source}\n'
            f'    return {NAMESPACE}::Next::shield;\n}}\n')


def install(canonical, output):
    if not admitted(canonical) or REFERENCE in output:
        return output, []
    original = owner.body(canonical)
    found = owner.body(output)
    if not original or not found:
        return output, []
    body = found[1]
    exit_instruction = '\nif(s.services.instruction){s.services.instruction(damage_observation_pc(0xd7700u,'
    if body.count(exit_instruction) != 1:
        return output, []
    body = body.replace(exit_instruction, '\nLd7700: ' + exit_instruction[1:], 1)
    for first, entry in ENTRIES.items():
        label = f'\nL{first}:'
        if body.count(label) != 1:
            return output, []
        arms = ''.join(f'    case {NAMESPACE}::Next::{outcome}: goto L{dest};\n'
                       for dest, outcome in EXITS.items())
        hook = (f'\n    switch ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},'
                '[](auto &value){run_159ef0(value);})) {\n'
                f'{arms}    case {NAMESPACE}::Next::retained: break;\n'
                '    default: throw std::logic_error("Invalid damage vitality gate continuation");\n'
                '    }\n')
        body = body.replace(label, label + hook, 1)
    output = INCLUDE + output[:found.start(1)] + reference(original[1]) + body + output[found.end(1):]
    return output, ['d85cb','d85d9']
