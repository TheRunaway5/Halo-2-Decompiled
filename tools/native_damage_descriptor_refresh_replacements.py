"""Bind the selected TU1.5 damage descriptor refresh in the D8330 owner."""
import re

import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_descriptor_refresh.hpp"\n'
NAMESPACE = 'halo2::native_damage_descriptor_refresh_detail'
REFERENCE = 'native_damage_descriptor_refresh_reference_d73e0'
FIRST = '\nLd75a4:'
END = '\nLd7621:'
CALLS = {
    '1e83a0': ('d75d8', 'd8528', 'd75dd', 'd852d', '1eccc0'),
    'd7cd0': ('d7606', 'd8556', 'd760b', 'd855b', 'd8c20'),
    'd7e00': ('d7615', 'd8565', 'd761a', 'd856a', 'd8d50'),
}
OPERANDS = {0x4fcdc8: 0x5020c8}
for target, (_, _, resume, resume_actual, target_actual) in CALLS.items():
    OPERANDS[int(target, 16)] = int(target_actual, 16)
    OPERANDS[int(resume, 16)] = int(resume_actual, 16)
LITERAL = re.compile(r's\.services\.instruction\(damage_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u,s\)\);')
SELECTED = re.compile(r'selected\(0x([0-9a-f]+)u,s\.image\)')


def split(source):
    if source.count(FIRST) != 1 or source.count(END) != 1:
        raise ValueError('D8330 descriptor refresh boundaries changed')
    prefix_source, rest = source.split(FIRST, 1)
    selected, after = rest.split(END, 1)
    return prefix_source + FIRST, selected, END + after


def admitted(canonical):
    if not owner.admitted(canonical):
        return False
    mapping = prefix.world.mapping(canonical)
    return all(mapping.get(key) == actual for key, actual in OPERANDS.items())


def reference(source, replay=True):
    _, selected, _ = split(source)
    jump = 'goto Ld7621;'
    sites = LITERAL.findall(selected)
    if (selected.count(jump) != 1 or len(sites) != 35 or len(set(sites)) != 35 or
            (sites[0], sites[-1]) != ('d84f4', 'd856a')):
        raise ValueError('D8330 original descriptor refresh calls or observation sites changed')
    selected = selected.replace(jump, 'return;', 1)
    for name, (pc, actual, resume, _, target) in CALLS.items():
        call = f'run_{name}(s);'
        original = f's.push(damage_observation_pc(0x{pc}u,0x{actual}u,s),selected(0x{resume}u,s.image));{call}'
        if selected.count(original) != 1:
            raise ValueError(f'D8330 descriptor child route changed at {actual}')
        if replay:
            selected = selected.replace(call, f'halo2::native_object_tick_detail::replay(s,0x{target}u);', 1)
    symbol = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {symbol}(ObservedDamageState &s) {{\n{selected.lstrip()}\n}}\n'


def install(canonical, output):
    if not admitted(canonical):
        return output, []
    original = owner.body(canonical)
    current = owner.body(output)
    if not current or owner.REFERENCE not in output:
        return output, []
    _, expected, _ = split(original[1])
    expected, count = SELECTED.subn(
        lambda match: f'(s.image==SourcePhysicsContactImage::MapPack11?0x{match[1]}u:0x{OPERANDS[int(match[1], 16)]:x}u)',
        expected)
    if count != 4:
        raise ValueError('D8330 descriptor operand count changed')
    before, selected, after = split(current[1])
    replay = reference(original[1])
    children = ','.join(f'[](auto &v){{run_{name}(v);}}' for name in CALLS)
    hook = f'\n    if ({NAMESPACE}::bridge(s,{REFERENCE},{children})) goto Ld7621;\n'
    intended = hook + expected.lstrip(' ')
    if REFERENCE in output:
        if output.count(replay) != 1 or output.count(INCLUDE) != 1 or selected != intended:
            raise ValueError('Prepared D8330 descriptor refresh changed')
        return output, []
    if selected != expected or INCLUDE in output:
        raise ValueError('Specialized D8330 descriptor refresh changed')
    return (INCLUDE + output[:current.start(1)] + replay + before + intended + after + output[current.end(1):],
            ['d84f4'])
