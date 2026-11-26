"""Bind the selected-image D8330 countdown advance and depletion loop."""
import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_update_sections.hpp"\n'
REFERENCE = 'native_damage_update_sections_reference_d73e0'
NAMESPACE = 'halo2::native_damage_update_sections_detail'
SPANS = (
    ('pending', 'd758c', 'd75a4', {'countdown': 'd74a0', 'validation': 'd75a4'}, 'validation'),
    ('depletion', 'd7b06', 'd7b92', {'notify': 'd7b92'}, 'notify'),
)


def admitted(canonical):
    return (owner.admitted(canonical) and prefix.world.mapping(canonical).get(0x518444) == 0x51df54)


def exact_spill(source):
    original = ('w(s.memory,s.observe,damage_observation_pc(0xd7b55u,0xd8aa5u,s),'
                '(0x1cu+s.gp[4]),4,integer_spill(s.fp.st(0)));s.fp.pop();')
    if source.count(original) != 1:
        raise ValueError('Damage depletion x87 spill boundary changed')
    corrected = original.replace('integer_spill(s.fp.st(0))', 'integer_spill(s.fp.st(0),s.fp.status)')
    return source.replace(original, corrected, 1)


def reference(body):
    arms = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{first};\n'
                   for entry, first, _, _, _ in SPANS)
    spans = []
    for entry, first, end, exits, fallthrough in SPANS:
        start = body.index(f'\nL{first}:')
        source = body[start:body.index(f'\nL{end}:', start)]
        for outcome, dest in exits.items():
            source = source.replace(f'goto L{dest};', f'return {NAMESPACE}::Next::{outcome};')
        if entry == 'depletion':
            source = exact_spill(source)
        spans.append(source + f'\n    return {NAMESPACE}::Next::{fallthrough};')
    return (f'static {NAMESPACE}::Next {REFERENCE}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{arms}    }}\n'
            f'    throw std::logic_error("Invalid damage section entry");' + ''.join(spans) + '\n}\n')


def install(canonical, output):
    if not admitted(canonical) or REFERENCE in output:
        return output, []
    original = owner.body(canonical)
    found = owner.body(output)
    if not original or not found or owner.REFERENCE not in output:
        return output, []
    body = exact_spill(found[1])
    for entry, first, _, exits, _ in SPANS:
        label = f'\nL{first}:'
        if body.count(label) != 1:
            return output, []
        arms = ''.join(f'    case {NAMESPACE}::Next::{outcome}: goto L{dest};\n'
                       for outcome, dest in exits.items())
        hook = (f'\n    switch ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},damage_mulss)) {{\n'
                f'{arms}    case {NAMESPACE}::Next::retained: break;\n'
                '    default: throw std::logic_error("Invalid damage section continuation");\n'
                '    }\n')
        body = body.replace(label, label + hook, 1)
    output = INCLUDE + output[:found.start(1)] + reference(original[1]) + body + output[found.end(1):]
    return output, ['d84dc','d8a56']
