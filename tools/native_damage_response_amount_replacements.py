"""Bind the selected D8330 response amount and depletion handoff."""
import re

import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_response_amount.hpp"\n'
NAMESPACE = 'halo2::native_damage_response_amount_detail'
REFERENCE = 'native_damage_response_amount_reference_d73e0'
ENTRIES = {'d79a9': 'counter', 'd79de': 'load', 'd79f5': 'compare'}
EXITS = {'d7751': ('continue_flags', 1), 'd7ab8': ('depletion', 2)}
FIRST = '\nLd79a9:'
END = '\nLd7ab8:'
CALLS = (
    ('139ff0', 'd79a9', 'd88f9', 'd79ae', 'd88fe', '13b360'),
    ('a87e0', 'd79d4', 'd8924', 'd79d9', 'd8929', 'b1c30'),
    ('1ed0b0', 'd7a4b', 'd899b', 'd7a50', 'd89a0', '1f19d0'),
    ('139ff0', 'd7a96', 'd89e6', 'd7a9b', 'd89eb', '13b360'),
    ('a87e0', 'd7ab0', 'd8a00', 'd7ab5', 'd8a05', 'b1c30'),
)
OPERANDS = {0x518444: 0x51df54}
for target, _, _, resume, resume_actual, target_actual in CALLS:
    OPERANDS[int(target, 16)] = int(target_actual, 16)
    OPERANDS[int(resume, 16)] = int(resume_actual, 16)
LITERAL = re.compile(r's\.services\.instruction\(damage_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u,s\)\);')
SELECTED = re.compile(r'selected\(0x([0-9a-f]+)u,s\.image\)')


def admitted(canonical):
    if not owner.admitted(canonical):
        return False
    mapping = prefix.world.mapping(canonical)
    return all(mapping.get(key) == actual for key, actual in OPERANDS.items())


def split(source):
    if source.count(FIRST) != 1 or source.count(END) != 1:
        raise ValueError('D8330 damage amount boundaries changed')
    before, rest = source.split(FIRST, 1)
    selected, after = rest.split(END, 1)
    return before, FIRST + selected, END + after


def reference(source, replay=True):
    _, selected, _ = split(source)
    sites = LITERAL.findall(selected)
    if len(sites) != 60 or len(set(sites)) != 60 or sites[0] != 'd88f9' or sites[-1] != 'd8a05':
        raise ValueError('D8330 damage amount observation sites changed')
    for pc, (entry, count) in EXITS.items():
        jump = f'goto L{pc};'
        if selected.count(jump) != count:
            raise ValueError(f'D8330 damage amount continuation changed to {pc}')
        selected = selected.replace(jump, f'return {NAMESPACE}::Next::{entry};')
    for name, disc, actual, resume, _, target in CALLS:
        call = f'run_{name}(s);'
        original = f's.push(damage_observation_pc(0x{disc}u,0x{actual}u,s),selected(0x{resume}u,s.image));{call}'
        if selected.count(original) != 1:
            raise ValueError(f'D8330 damage amount child route changed at {actual}')
        if replay:
            hook = original.removesuffix(call) + f'halo2::native_object_tick_detail::replay(s,0x{target}u);'
            selected = selected.replace(original, hook, 1)
    symbol = REFERENCE if replay else REFERENCE + '_live'
    cases = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{pc};\n' for pc, entry in ENTRIES.items())
    return (f'static {NAMESPACE}::Next {symbol}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{cases}    }}\n'
            f'    throw std::logic_error("Invalid damage amount entry");{selected}\n'
            f'    return {NAMESPACE}::Next::depletion;\n}}\n')


def install(canonical, emitted):
    if not admitted(canonical):
        return emitted, []
    original = owner.body(canonical)
    current = owner.body(emitted)
    if not current or owner.REFERENCE not in emitted:
        return emitted, []
    _, expected, _ = split(original[1])
    expected, count = SELECTED.subn(
        lambda match: f'(s.image==SourcePhysicsContactImage::MapPack11?0x{match[1]}u:0x{OPERANDS[int(match[1], 16)]:x}u)',
        expected)
    if count != 6:
        raise ValueError('D8330 response amount operands changed')
    replay = reference(original[1])
    before, actual, after = split(current[1])
    exits = ''.join(f'    case {NAMESPACE}::Next::{entry}: goto L{pc};\n' for pc, (entry, _) in EXITS.items())
    children = ','.join(f'[](auto &v){{run_{child}(v);}}' for child in ('139ff0', '1ed0b0', 'a87e0'))
    intended = expected
    for pc, entry in ENTRIES.items():
        label = f'\nL{pc}:'
        if intended.count(label) != 1:
            raise ValueError(f'D8330 response amount label {pc} changed')
        bridge = (f'\n    switch ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},'
                  f'{children},damage_mulss,damage_addss)) {{\n'
                  f'{exits}    case {NAMESPACE}::Next::retained: break;\n    }}\n')
        intended = intended.replace(label, label + bridge, 1)
    if REFERENCE in emitted:
        if emitted.count(INCLUDE) != 1 or emitted.count(replay) != 1 or actual != intended:
            raise ValueError('Prepared D8330 response amount binding changed')
        return emitted, []
    if actual != expected or INCLUDE in emitted:
        raise ValueError('Specialized D8330 response amount range changed')
    return (INCLUDE + emitted[:current.start(1)] + replay + before + intended + after + emitted[current.end(1):],
            ['d88f9', 'd892e', 'd8945'])
