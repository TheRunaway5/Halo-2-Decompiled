"""Bind the selected D8330 due-response route after the original countdown."""
import hashlib
import re

import native_damage_update_entry_replacements as prefix
import native_damage_update_return_replacements as owner


INCLUDE = '#include "gameplay/units/native_tu15_damage_due_response.hpp"\n'
NAMESPACE = 'halo2::native_damage_due_response_detail'
REFERENCE = 'native_damage_due_response_reference_d73e0'
FIRST = '\nLd74dc: ;\n// END native pending response continuation'
END = '\nLd758c:'
DISPATCH = re.compile(r'^static void dispatch\(ObservedDamageState &s,[^\n]+$', re.M)
DISPATCH_PINS = ('3e678740b41bca175413cce5a694b22e76de19cc7f3d0cd8f535ea9dd2c92407',
                 '9dc5c5396ce6354dd8a44008bbf15774b18ff7045d2f751a9de7beb021c062ee')
CALLS = {
    'db570': ('d7522', 'd8472', 'd7527', 'd8477', 'dc4c0'),
    'db710': ('d7552', 'd84a2', 'd7557', 'd84a7', 'dc660'),
    'dd280': ('d7573', 'd84c3', 'd7578', 'd84c8', 'de1d0'),
}
NOTIFY = 'dispatch(s,target,0xd7587u,false);'
OPERANDS = {0x4e78d0: 0x4ecbd0, 0xd81c0: 0xd9110, 0xd758c: 0xd84dc}
for target, (_, _, resume, resume_actual, target_actual) in CALLS.items():
    OPERANDS[int(target, 16)] = int(target_actual, 16)
    OPERANDS[int(resume, 16)] = int(resume_actual, 16)
SELECTED = re.compile(r'selected\(0x([0-9a-f]+)u,s\.image\)')
LITERAL = re.compile(r's\.services\.instruction\(damage_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u,s\)\);')


def dispatcher(unit, prepared):
    definition = DISPATCH.findall(unit)
    return (len(definition) == 1 and '0xd9110u' not in definition[0] and '0xd81c0u' not in definition[0] and
            definition[0].endswith('s.required(site,target,tail);}') and
            hashlib.sha256(definition[0].encode()).hexdigest() == DISPATCH_PINS[prepared])


def admitted(canonical):
    if not owner.admitted(canonical) or not dispatcher(canonical, False):
        return False
    mapping = prefix.world.mapping(canonical)
    return all(mapping.get(key) == value for key, value in OPERANDS.items())


def split(source):
    if source.count(FIRST) != 1 or source.count(END) != 1:
        raise ValueError('D8330 due-response boundaries changed')
    before, rest = source.split(FIRST, 1)
    selected, after = rest.split(END, 1)
    return before + FIRST, selected, END + after


def reference(body, replay=True):
    _, source, _ = split(body)
    sites = LITERAL.findall(source)
    if len(sites) != 52 or len(set(sites)) != 51 or sites.count('d848d') != 2:
        raise ValueError('D8330 due-response observation sites changed')
    if source.count('goto Ld758c;') != 2 or source.count(NOTIFY) != 1:
        raise ValueError('D8330 due-response continuation or required route changed')
    source = source.replace('goto Ld758c;', 'return;')
    for name, (pc, actual, resume, _, target) in CALLS.items():
        call = f'run_{name}(s);'
        original = f's.push(damage_observation_pc(0x{pc}u,0x{actual}u,s),selected(0x{resume}u,s.image));{call}'
        if source.count(original) != 1:
            raise ValueError(f'D8330 due-response direct route changed at {actual}')
        if replay:
            source = source.replace(call, f'halo2::native_object_tick_detail::replay(s,0x{target}u);', 1)
    if replay:
        source = source.replace(NOTIFY, 'halo2::native_object_tick_detail::replay(s,target);', 1)
    name = REFERENCE if replay else REFERENCE + '_live'
    return f'static void {name}(ObservedDamageState &s) {{{source}\n}}\n'


def install(canonical, emitted):
    if not admitted(canonical):
        return emitted, []
    original = owner.body(canonical)
    current = owner.body(emitted)
    if not current or owner.REFERENCE not in emitted:
        return emitted, []
    if not dispatcher(emitted, True):
        raise ValueError('D8330 selected D9110 required dispatcher changed')
    _, expected, _ = split(original[1])
    expected, count = SELECTED.subn(
        lambda match: f'(s.image==SourcePhysicsContactImage::MapPack11?0x{match[1]}u:0x{OPERANDS[int(match[1], 16)]:x}u)',
        expected)
    if count != 6:
        raise ValueError('D8330 due-response operands changed')
    replay = reference(original[1])
    before, actual, after = split(current[1])
    direct = ','.join(f'[](auto &v){{run_{name}(v);}}' for name in CALLS)
    required = '[](auto &v,std::uint32_t target,std::uint32_t pc,bool tail){dispatch(v,target,pc,tail);}'
    hook = f'\n    if ({NAMESPACE}::bridge(s,{REFERENCE},{direct},{required})) goto Ld758c;'
    intended = hook + expected
    if REFERENCE in emitted:
        if emitted.count(INCLUDE) != 1 or emitted.count(replay) != 1 or actual != intended:
            raise ValueError('Prepared D8330 due-response binding changed')
        return emitted, []
    if actual != expected or INCLUDE in emitted:
        raise ValueError('Specialized D8330 due-response range changed')
    return (INCLUDE + emitted[:current.start(1)] + replay + before + intended + after + emitted[current.end(1):],
            ['d842c'])
