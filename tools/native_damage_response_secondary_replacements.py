"""Bind the selected D8330 secondary response and preserve mixed child dispatch."""
import re

import native_damage_due_response_replacements as dispatcher
import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_response_secondary.hpp"\n'
NAMESPACE = 'halo2::native_damage_response_secondary_detail'
REFERENCE = 'native_damage_response_secondary_reference_d73e0'
ENTRIES = {'d787e': 'load', 'd788a': 'scaled'}
FIRST = '\nLd787e:'
END = '\nLd79a9:'
CALLS = (
    ('1ed0b0', 'd78bf', 'd880f', 'd78c4', 'd8814', '1f19d0'),
    ('d7cd0', 'd791a', 'd886a', 'd791f', 'd886f', 'd8c20'),
    ('a87e0', 'd7989', 'd88d9', 'd798e', 'd88de', 'b1c30'),
    ('a87e0', 'd799f', 'd88ef', 'd79a4', 'd88f4', 'b1c30'),
)
RESET = 'dispatch(s,target,0xd78f1u,false);'
MIXED = ('if(s.image==SourcePhysicsContactImage::TitleUpdate15 && s.services.required_owner)'
         's.required(0xd8889u,selected(0x1781f0u,s.image),false);else run_1781f0(s);')
OPERANDS = {0x518444: 0x51df54, 0xbb6e0: 0xbc820, 0xd78f6: 0xd8846, 0x1781f0: 0x179610}
for target, _, _, resume, resume_actual, target_actual in CALLS:
    OPERANDS[int(target, 16)] = int(target_actual, 16)
    OPERANDS[int(resume, 16)] = int(resume_actual, 16)
LITERAL = re.compile(r's\.services\.instruction\(damage_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u,s\)\);')
SELECTED = re.compile(r'selected\(0x([0-9a-f]+)u,s\.image\)')


def admitted(canonical):
    if not owner.admitted(canonical) or not dispatcher.dispatcher(canonical, False):
        return False
    definition = dispatcher.DISPATCH.findall(canonical)[0]
    mapping = prefix.world.mapping(canonical)
    return ('0xbb6e0u' not in definition and '0xbc820u' not in definition and
            all(mapping.get(key) == actual for key, actual in OPERANDS.items()))


def split(source):
    if source.count(FIRST) != 1 or source.count(END) != 1:
        raise ValueError('D8330 secondary response boundaries changed')
    before, rest = source.split(FIRST, 1)
    selected, after = rest.split(END, 1)
    return before, FIRST + selected, END + after


def reference(source, replay=True):
    _, selected, _ = split(source)
    sites = LITERAL.findall(selected)
    if len(sites) != 70 or len(set(sites)) != 70 or sites[0] != 'd87ce' or sites[-1] != 'd88f4':
        raise ValueError('D8330 secondary response observation sites changed')
    if selected.count('goto Ld7751;') != 2 or selected.count(MIXED) != 1 or selected.count(RESET) != 1:
        raise ValueError('D8330 secondary response continuation or ABI route changed')
    selected = selected.replace('goto Ld7751;', 'return;')
    for name, disc, actual, resume, _, target in CALLS:
        call = f'run_{name}(s);'
        original = f's.push(damage_observation_pc(0x{disc}u,0x{actual}u,s),selected(0x{resume}u,s.image));{call}'
        if selected.count(original) != 1:
            raise ValueError(f'D8330 secondary response child route changed at {actual}')
        if replay:
            hook = original.removesuffix(call) + f'halo2::native_object_tick_detail::replay(s,0x{target}u);'
            selected = selected.replace(original, hook, 1)
    if replay:
        selected = selected.replace(RESET, 'halo2::native_object_tick_detail::replay(s,target);', 1)
        selected = selected.replace(MIXED, 'halo2::native_object_tick_detail::replay(s,0x179610u);', 1)
    symbol = REFERENCE if replay else REFERENCE + '_live'
    cases = ''.join(f'    case {NAMESPACE}::Entry::{entry}: goto L{pc};\n' for pc, entry in ENTRIES.items())
    return (f'static void {symbol}(ObservedDamageState &s,{NAMESPACE}::Entry entry) {{\n'
            f'    switch (entry) {{\n{cases}    }}\n'
            f'    throw std::logic_error("Invalid damage secondary response entry");{selected}\n}}\n')


def install(canonical, emitted):
    if not admitted(canonical):
        return emitted, []
    original = owner.body(canonical)
    current = owner.body(emitted)
    if not current or owner.REFERENCE not in emitted:
        return emitted, []
    if not dispatcher.dispatcher(emitted, True):
        raise ValueError('D8330 secondary response dispatcher changed')
    _, expected, _ = split(original[1])
    expected, count = SELECTED.subn(
        lambda match: f'(s.image==SourcePhysicsContactImage::MapPack11?0x{match[1]}u:0x{OPERANDS[int(match[1],16)]:x}u)',
        expected)
    if count != 8:
        raise ValueError('D8330 secondary response operands changed')
    replay = reference(original[1])
    before, actual, after = split(current[1])
    children = ','.join(f'[](auto &v){{run_{child}(v);}}' for child in ('1ed0b0', 'd7cd0', '1781f0', 'a87e0'))
    dispatch = '[](auto &v,std::uint32_t target,std::uint32_t pc,bool tail){dispatch(v,target,pc,tail);}'
    intended = expected
    for pc, entry in ENTRIES.items():
        label = f'\nL{pc}:'
        if intended.count(label) != 1:
            raise ValueError(f'D8330 secondary response label {pc} changed')
        bridge = (f'\n    if ({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE},'
                  f'{children},{dispatch},damage_mulss,damage_addss)) goto Ld7751;\n')
        intended = intended.replace(label, label + bridge, 1)
    if REFERENCE in emitted:
        if emitted.count(INCLUDE) != 1 or emitted.count(replay) != 1 or actual != intended:
            raise ValueError('Prepared D8330 secondary response changed')
        return emitted, []
    if actual != expected or INCLUDE in emitted:
        raise ValueError('Specialized D8330 secondary response range changed')
    return (INCLUDE + emitted[:current.start(1)] + replay + before + intended + after + emitted[current.end(1):],
            ['d87ce', 'd87da'])
