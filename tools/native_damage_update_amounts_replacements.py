"""Bind the selected-image D8650 shield refill and armor input preparation."""
import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner
import native_damage_update_vitality_gate_replacements as gate


INCLUDE = '#include "gameplay/units/native_tu15_damage_update_amounts.hpp"\n'
REFERENCE = 'native_damage_update_amounts_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_amounts_detail'
ENTRIES = {'d7700': 'refill', 'd774a': 'clear', 'd7751': 'armor', 'd7794': 'defer'}
EXITS = {'d7ab8': 'notify', 'd79de': 'base_armor', 'd79f5': 'section_armor'}
OPERANDS = {0x518444: 0x51df54, 0x46507c: 0x46a11c, 0x465044: 0x46a0e4}


def admitted(canonical):
    mapping = prefix.world.mapping(canonical)
    return owner.admitted(canonical) and all(mapping.get(key) == pc for key, pc in OPERANDS.items())


def reference(body):
    arms = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{pc};\n'
                   for pc, entry in ENTRIES.items())
    start = body.index('\nif(s.services.instruction){s.services.instruction(damage_observation_pc(0xd7700u,')
    end = body.index('\nLd77a2:', start)
    source = '\nLd7700: ' + body[start+1:end]
    for target, outcome in EXITS.items():
        source = source.replace(f'goto L{target};', f'return {NAMESPACE}::Next::{outcome};')
    return (f'static {NAMESPACE}::Next {REFERENCE}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{arms}    }}\n'
            f'    throw std::logic_error("Invalid damage shield refill or armor input entry");{source}\n}}\n')


def install(canonical, output):
    if not admitted(canonical) or REFERENCE in output:
        return output, []
    original = owner.body(canonical)
    found = owner.body(output)
    if not original or not found or gate.REFERENCE not in output:
        return output, []
    body = found[1]
    for first, entry in ENTRIES.items():
        label = f'\nL{first}:'
        if body.count(label) != 1:
            return output, []
        arms = ''.join(f'    case {NAMESPACE}::Next::{outcome}: goto L{dest};\n'
                       for dest, outcome in EXITS.items())
        hook = (f'\n    switch ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},'
                'damage_mulss,damage_addss)) {\n'
                f'{arms}    case {NAMESPACE}::Next::retained: break;\n'
                '    }\n')
        body = body.replace(label, label + hook, 1)
    output = INCLUDE + output[:found.start(1)] + reference(original[1]) + body + output[found.end(1):]
    return output, ['d8650']
