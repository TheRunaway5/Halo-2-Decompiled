"""Bind both terminal entries to the native campaign plane-output merge."""
import re

import native_fog_plane_classifier_replacements as classifier
import native_fog_plane_projection_replacements as projection


INCLUDE = '#include "campaign/native_tu15_fog_plane_output.hpp"\n'
NAMESPACE = 'halo2::native_fog_plane_output_detail'
REFERENCE = 'native_fog_plane_output_reference_133b40'
ENTRIES = (('no_planes', '134042'), ('classified', '134045'))
FIRST = '\nL134042:'


def split(owner):
    if owner.count(FIRST) != 1:
        raise ValueError('133B40 plane output boundary changed')
    before, after = owner.split(FIRST, 1)
    return before, FIRST + after


def pin(selected):
    for intrinsic, operation, expected in (
            ('_mm_mul_ss', 'halo2::native_tu15_leaves::ordered_mul_ss', 21),
            ('_mm_add_ss', 'halo2::native_world_leaves_detail::ordered_add_ss', 24)):
        selected, count = re.subn(rf'\b{intrinsic}\(', f'{operation}(', selected)
        if count != expected:
            raise ValueError(f'133B40 plane output {intrinsic} sites changed')
    return selected


def reference_body(canonical):
    _, selected = split(classifier.source(canonical))
    cases = ''.join(f'        case {NAMESPACE}::Entry::{name}:goto L{pc};\n' for name, pc in ENTRIES)
    return (f'static void {REFERENCE}(WorldState &s,{NAMESPACE}::Entry start) {{\n'
            + '    switch(start) {\n' + cases + '    }\n'
            + '    throw std::logic_error("Unknown 133B40 plane output entry");'
            + pin(selected) + '\n')


def install(canonical, output):
    reference = reference_body(canonical)
    current = classifier.body(output)
    if (not current or output.count(classifier.CALL) != 1 or
            current[0].count(classifier.GEOMETRY_CALL) != 1 or
            output.count(classifier.reference_body(canonical)) != 1 or
            output.count(projection.reference_body(canonical)) != 1):
        raise ValueError('133B40 plane output requires original child, classifier and projection')
    _, original = split(classifier.source(canonical))
    expected = classifier.CURSOR.sub(r's.instruction_literal(\1);', original)
    installed = pin(expected)
    for name, pc in ENTRIES:
        label = '\nL' + pc + ':'
        if installed.count(label) != 1:
            raise ValueError(f'133B40 plane output entry {pc} changed')
        bridge = f'\n    if({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{name},{REFERENCE}))return;\n'
        installed = installed.replace(label, label + bridge, 1)
    before, actual = split(current[0])
    if REFERENCE in output:
        if output.count(INCLUDE) != 1 or output.count(reference) != 1 or actual != installed:
            raise ValueError('Prepared 133B40 plane output binding changed')
        return output, ['134042']
    if actual != expected or INCLUDE in output:
        raise ValueError('Specialized 133B40 plane output body changed')
    return INCLUDE + output[:current.start()] + reference + '\n' + before + installed + output[current.end():], ['134042']
