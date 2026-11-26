"""Bind the existing entries to the child-free campaign fog cluster exit."""
import re

from native_fog_resolution_replacements import body, raw_owner

INCLUDE = '#include "campaign/native_tu15_fog_cluster_finish.hpp"\n'
NAMESPACE = 'halo2::native_fog_cluster_finish_detail'
REFERENCE = 'native_fog_cluster_finish_reference_131ac0'
ENTRIES = (('bloom', '131fc7'), ('recorded', '132084'), ('uncovered', '13208c'))
CONTINUATIONS = (('another_cluster', '131c31'), ('resolve', '13262b'))
FIRST = 'L131fc7:'
LAST = 'L1320e5:'
CURSOR = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')


def split(owner):
    if owner.count(FIRST) != 1 or owner.count(LAST) != 1:
        raise ValueError('Campaign fog cluster completion boundary changed')
    before, after = owner.split(FIRST, 1)
    selected, after = after.split(LAST, 1)
    return before, FIRST + selected, LAST + after


def pin(selected):
    for intrinsic, ordered, expected in (
            ('_mm_mul_ss', 'halo2::native_tu15_leaves::ordered_mul_ss', 5),
            ('_mm_add_ss', 'halo2::native_world_leaves_detail::ordered_add_ss', 6)):
        selected, count = re.subn(rf'\b{intrinsic}\(', f'{ordered}(', selected)
        if count != expected:
            raise ValueError(f'Fog cluster {intrinsic} sites changed')
    return selected


def reference_body(canonical):
    _, selected, _ = split(raw_owner(canonical))
    for path, pc in CONTINUATIONS:
        selected, count = re.subn(rf'goto L{pc};', f'return {NAMESPACE}::Next::{path};', selected)
        if count != 1:
            raise ValueError(f'Campaign fog cluster edge to {pc} changed')
    cases = ''.join(f'        case {NAMESPACE}::Entry::{entry}:goto L{pc};\n' for entry, pc in ENTRIES)
    return (f'static {NAMESPACE}::Next {REFERENCE}(WorldState &s,{NAMESPACE}::Entry start) {{\n'
            + '    switch(start) {\n' + cases
            + '    }\n    throw std::logic_error("Unknown campaign fog cluster entry");\n'
            + pin(selected) + '}\n')


def install(canonical, output):
    reference = reference_body(canonical)
    current = body(output)
    if not current:
        raise ValueError('Missing specialized campaign environment owner 131AC0')
    before, selected, after = split(current[0])
    _, original, _ = split(raw_owner(canonical))
    expected = CURSOR.sub(r's.instruction_literal(\1);', original)
    installed = pin(expected)
    for entry, pc in ENTRIES:
        label = 'L' + pc + ':'
        bridge = (f'\n    switch({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{entry},{REFERENCE})) {{\n'
                  + ''.join(f'        case {NAMESPACE}::Next::{path}:goto L{dest};\n'
                            for path, dest in CONTINUATIONS)
                  + f'        case {NAMESPACE}::Next::fallback:break;\n'
                  + '    }\n')
        if installed.count(label) != 1:
            raise ValueError(f'Campaign fog cluster entry {pc} changed')
        installed = installed.replace(label, label + bridge, 1)
    if REFERENCE in output:
        if output.count(INCLUDE) != 1 or output.count(reference) != 1 or selected != installed:
            raise ValueError('Prepared fog cluster completion binding changed')
        return output, ['131fc7']
    if selected != expected or INCLUDE in output:
        raise ValueError('Specialized fog cluster completion body changed')
    return INCLUDE + output[:current.start()] + reference + '\n' + before + installed + after + output[current.end():], ['131fc7']
