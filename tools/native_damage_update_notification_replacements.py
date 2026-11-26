"""Bind the selected-image damage depletion gate and real notification call."""
import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner
import native_damage_update_sections_replacements as sections


INCLUDE = '#include "gameplay/units/native_tu15_damage_update_notification.hpp"\n'
REFERENCE = 'native_damage_update_notification_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_notification_detail'
SPANS = (
    ('prepare', 'd7ab8', 'd7b06', {'notify': 'd7b92', 'deplete': 'd7b06'}, 'deplete'),
    ('notify', 'd7b92', 'd7c08', {'saved_notification': 'd7c14', 'updated': 'd7c08'}, 'updated'),
)
OPERANDS = {0x4650a8: 0x46a148, 0xd7bc1: 0xd8b11, 0xda150: 0xdb0a0}


def admitted(canonical):
    mapping = prefix.world.mapping(canonical)
    return owner.admitted(canonical) and all(mapping.get(key) == pc for key, pc in OPERANDS.items())


def reference(body, replay=True):
    arms = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{first};\n'
                   for entry, first, _, _, _ in SPANS)
    spans = []
    for _, first, end, exits, fallthrough in SPANS:
        start = body.index(f'\nL{first}:')
        source = body[start:body.index(f'\nL{end}:', start)]
        for outcome, dest in exits.items():
            source = source.replace(f'goto L{dest};', f'return {NAMESPACE}::Next::{outcome};')
        if replay:
            source = source.replace('run_da150(s);', 'halo2::native_object_tick_detail::replay(s,0xdb0a0u);')
        spans.append(source + f'\n    return {NAMESPACE}::Next::{fallthrough};')
    suffix = '' if replay else '_live'
    return (f'static {NAMESPACE}::Next {REFERENCE}{suffix}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{arms}    }}\n'
            f'    throw std::logic_error("Invalid damage notification entry");' + ''.join(spans) + '\n}\n')


def install(canonical, output):
    if not admitted(canonical) or REFERENCE in output:
        return output, []
    original = owner.body(canonical)
    found = owner.body(output)
    if not original or not found or sections.REFERENCE not in output:
        return output, []
    body = found[1]
    for entry, first, _, exits, _ in SPANS:
        label = f'\nL{first}:'
        if body.count(label) != 1:
            return output, []
        arms = ''.join(f'    case {NAMESPACE}::Next::{outcome}: goto L{dest};\n'
                       for outcome, dest in exits.items())
        hook = (f'\n    switch ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},'
                '[](auto &value){run_da150(value);},damage_addss)) {\n'
                f'{arms}    case {NAMESPACE}::Next::retained: break;\n'
                '    default: throw std::logic_error("Invalid damage notification continuation");\n'
                '    }\n')
        body = body.replace(label, label + hook, 1)
    output = INCLUDE + output[:found.start(1)] + reference(original[1]) + body + output[found.end(1):]
    return output, ['d8a08','d8ae2']
