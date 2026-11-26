"""Bind the child-free accumulation block in campaign environment 131AC0."""
import re

from native_fog_resolution_replacements import body, raw_owner

INCLUDE = '#include "campaign/native_tu15_fog_accumulation.hpp"\n'
NAMESPACE = 'halo2::native_fog_accumulation_detail'
REFERENCE = 'native_fog_accumulation_reference_131ac0'
START = 'L131d38:'
END = '131f80'
CONTINUATIONS = (('patchy_fog', '131f80'), ('sky_and_bloom', '131fc7'), ('no_weight', '13208c'))
ORDERS = {
    '_mm_mul_ss': 'halo2::native_tu15_leaves::ordered_mul_ss',
    '_mm_add_ss': 'halo2::native_world_leaves_detail::ordered_add_ss',
}
CURSOR = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')


def end(cursor):
    return f'\n{cursor} s.set(0,r(s.memory,(0x24u+s.gp[2]),4),4,0);'


RAW_END = end('if(s.services.instruction){s.services.instruction(0x131f80u);}')
SPECIALIZED_END = end('s.instruction_literal(0x131f80u);')


def split(owner, marker):
    if owner.count(START) != 1 or owner.count(marker) != 1:
        raise ValueError('Campaign fog accumulation boundary changed')
    before, interval = owner.split(START, 1)
    selected, after = interval.split(marker, 1)
    return before, selected, after


def pin(selected):
    for intrinsic, ordered in ORDERS.items():
        selected, count = re.subn(rf'\b{intrinsic}\(', f'{ordered}(', selected)
        if count != 21:
            raise ValueError(f'Wrong number of campaign fog {intrinsic} operands ({count})')
    return selected


def reference_body(canonical):
    _, selected, _ = split(raw_owner(canonical), RAW_END)
    selected = pin(selected)
    last_pc = {'sky_and_bloom': 3, 'no_weight': 1}
    for path, pc in CONTINUATIONS[1:]:
        selected, count = re.subn(rf'goto L{pc};', f'return {NAMESPACE}::Next::{path};', selected)
        if count != last_pc[path]:
            raise ValueError(f'Unexpected number of fog accumulation edges to {pc}')
    return (f'static {NAMESPACE}::Next {REFERENCE}(WorldState &s) {{\n'
            + selected + f'\nreturn {NAMESPACE}::Next::patchy_fog;\n}}\n')


def install(canonical, output):
    reference = reference_body(canonical)
    current = body(output)
    if not current:
        raise ValueError('Missing specialized campaign environment owner 131AC0')
    _, selected, _ = split(raw_owner(canonical), RAW_END)
    expected = CURSOR.sub(r's.instruction_literal(\1);', selected)
    bridge = (f'\n    switch({NAMESPACE}::bridge(s,{REFERENCE})) {{\n'
              + ''.join(f'        case {NAMESPACE}::Next::{path}:goto L{pc};\n'
                        for path, pc in CONTINUATIONS)
              + f'        case {NAMESPACE}::Next::fallback:break;\n'
              + '    }\n')
    terminal_label = '\nL' + END + ':' + SPECIALIZED_END[1:]
    if REFERENCE in output:
        if output.count(INCLUDE) != 1 or output.count(reference) != 1:
            raise ValueError('Prepared fog accumulation reference changed')
        _, interval, _ = split(current[0], terminal_label)
        if interval != bridge + pin(expected):
            raise ValueError('Prepared fog accumulation source changed')
        return output, ['131d38']
    before, interval, after = split(current[0], SPECIALIZED_END)
    if expected != interval or INCLUDE in output or terminal_label in output:
        raise ValueError('Specialized fog accumulation input changed')
    installed = before + START + bridge + pin(interval) + terminal_label + after
    return INCLUDE + output[:current.start()] + reference + '\n' + installed + output[current.end():], ['131d38']
