"""Bind the fog member fold after the genuine vector-angle dispatch."""
import re

import native_fog_plane_classifier_replacements as classifier


INCLUDE = '#include "campaign/native_tu15_fog_plane_projection.hpp"\n'
NAMESPACE = 'halo2::native_fog_plane_projection_detail'
REFERENCE = 'native_fog_plane_projection_reference_133b40'
ENTRIES = (('project', '133d8f'), ('advance', '13400e'))
CONTINUATIONS = (('member', '133d50'), ('plane', '134020'))
FIRST = '133d8f'
END = '\nL134020:'


def split(owner, specialized, prepared=False):
    callback = (f's.instruction_literal(0x{FIRST}u);' if specialized else
                f'if(s.services.instruction){{s.services.instruction(0x{FIRST}u);}}')
    start = '\nL' + FIRST + ':' if prepared else '\n' + callback
    if owner.count(start) != 1 or owner.count(END) != 1:
        raise ValueError('133B40 plane projection boundaries changed')
    before, after = owner.split(start, 1)
    selected, after = after.split(END, 1)
    return before, start + selected, END + after


def pin(selected):
    for intrinsic, operation, expected in (
            ('_mm_mul_ss', 'halo2::native_tu15_leaves::ordered_mul_ss', 24),
            ('_mm_add_ss', 'halo2::native_world_leaves_detail::ordered_add_ss', 21)):
        selected, count = re.subn(rf'\b{intrinsic}\(', f'{operation}(', selected)
        if count != expected:
            raise ValueError(f'133B40 plane projection {intrinsic} sites changed')
    return selected


def reference_body(canonical):
    _, selected, _ = split(classifier.source(canonical), False)
    selected, count = re.subn(r'goto L133d50;', f'return {NAMESPACE}::Next::member;', selected)
    if count != 1:
        raise ValueError('133B40 projected member-loop edge changed')
    cases = ''.join(f'        case {NAMESPACE}::Entry::{name}:goto L{pc};\n' for name, pc in ENTRIES)
    return (f'static {NAMESPACE}::Next {REFERENCE}(WorldState &s,{NAMESPACE}::Entry start) {{\n'
            + '    switch(start) {\n' + cases + '    }\n    throw std::logic_error("Unknown 133B40 projection entry");\n'
            + '\nL' + FIRST + ':' + pin(selected).removeprefix('\n')
            + f'\nreturn {NAMESPACE}::Next::plane;\n}}\n')


def install(canonical, output):
    reference = reference_body(canonical)
    current = classifier.body(output)
    if (not current or output.count(classifier.CALL) != 1 or
            current[0].count(classifier.GEOMETRY_CALL) != 1 or
            output.count(classifier.reference_body(canonical)) != 1):
        raise ValueError('133B40 plane projection requires the original child and certified classifier')
    _, original, _ = split(classifier.source(canonical), False)
    expected = classifier.CURSOR.sub(r's.instruction_literal(\1);', original)
    installed = '\nL' + FIRST + ':' + pin(expected).removeprefix('\n')
    for name, pc in ENTRIES:
        label = '\nL' + pc + ':'
        if installed.count(label) != 1:
            raise ValueError(f'133B40 projection entry {pc} changed')
        switch = (f'\n    switch({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{name},{REFERENCE})) {{\n'
                  + ''.join(f'        case {NAMESPACE}::Next::{path}:goto L{dest};\n'
                            for path, dest in CONTINUATIONS)
                  + f'        case {NAMESPACE}::Next::fallback:break;\n'
                  + '    }\n')
        installed = installed.replace(label, label + switch, 1)
    prepared = REFERENCE in output
    before, current_range, after = split(current[0], True, prepared)
    if prepared:
        if output.count(INCLUDE) != 1 or output.count(reference) != 1 or current_range != installed:
            raise ValueError('Prepared 133B40 plane projection binding changed')
        return output, ['133d8f']
    if current_range != expected or INCLUDE in output or not before.endswith(classifier.GEOMETRY_CALL):
        raise ValueError('Specialized 133B40 plane projection body changed')
    return INCLUDE + output[:current.start()] + reference + '\n' + before + installed + after + output[current.end():], ['133d8f']
