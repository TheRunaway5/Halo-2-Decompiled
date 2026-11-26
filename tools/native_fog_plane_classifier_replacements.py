"""Bind three no-child stages of the campaign fog plane classifier."""
import hashlib
import re


PIN = '0eb68e5db30a59ac125dd2d8146fbd2acc6c1dff15e552f3c4f389ce77475a7a'
INCLUDE = '#include "campaign/native_tu15_fog_plane_classifier.hpp"\n'
NAMESPACE = 'halo2::native_fog_plane_classifier_detail'
REFERENCE = 'native_fog_plane_classifier_reference_133b40'
CALL = 's.push(0x132183u,0x132188u);run_133b40(s);'
GEOMETRY_CALL = '{const auto target=0x11f7e0u;s.push(0x133d8au,0x133d8fu);dispatch(s,target,0x133d8au,false);}'
CURSOR = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
ENTRIES = (('initialize', '133b40'), ('classify', '133bd4'), ('normal', '133d1d'), ('member', '133d50'))
CONTINUATIONS = (
    ('classify', '133bd4'), ('no_planes', '134042'), ('angular', '133c2f'), ('explicit_normal', '133cab'),
    ('skip_plane', '134020'), ('end_members', '13400e'), ('geometry', '133d8a'),
)


def body(text):
    return re.search(r'static void run_133b40\(WorldState &s\) \{.*?\n\}', text, re.S)


def source(canonical):
    owner = body(canonical)
    if not owner:
        raise ValueError('Campaign fog plane classifier 133B40 is missing')
    normalized = owner[0].replace('WorldState &s', 'STATE &s')
    if hashlib.sha256(normalized.encode()).hexdigest() != PIN:
        raise ValueError('Campaign fog plane classifier 133B40 source changed')
    if canonical.count(CALL) != 1 or owner[0].count(GEOMETRY_CALL) != 1:
        raise ValueError('Campaign fog plane classifier 133B40 call route changed')
    return owner[0]


def split(owner, specialized):
    def anchor(pc):
        return (f's.instruction_literal(0x{pc}u);' if specialized else
                f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}}')
    markers = ('\n' + anchor('133b40'), '\nL133bd4:', '\n' + anchor('133c2f'),
               '\nL133d1d:', '\n' + anchor('133d8a'))
    if any(owner.count(marker) != 1 for marker in markers):
        raise ValueError('Campaign fog plane classifier 133B40 interval changed')
    offsets = [owner.index(marker) for marker in markers]
    if offsets != sorted(offsets):
        raise ValueError('Campaign fog plane classifier 133B40 intervals moved')
    cuts = [0, *offsets, len(owner)]
    return [owner[a:b] for a, b in zip(cuts, cuts[1:])]


def pinned(selected):
    for intrinsic, operation, expected in (
            ('_mm_mul_ss', 'halo2::native_tu15_leaves::ordered_mul_ss', 3),
            ('_mm_add_ss', 'halo2::native_world_leaves_detail::ordered_add_ss', 2)):
        selected, count = re.subn(rf'\b{intrinsic}\(', f'{operation}(', selected)
        if count != expected:
            raise ValueError(f'133B40 {intrinsic} instruction sites changed')
    return selected


def replace_exits(selected, names):
    for name in names:
        pc = dict(CONTINUATIONS)[name]
        selected, count = re.subn(rf'goto L{pc};', f'return {NAMESPACE}::Next::{name};', selected)
        if count != (3 if name == 'explicit_normal' else 1):
            raise ValueError(f'133B40 retained edge to {pc} changed')
    return selected


def reference_body(canonical):
    _, initialize, classify, _, normal, _ = split(source(canonical), False)
    cases = ''.join(f'        case {NAMESPACE}::Entry::{entry}:goto L{pc};\n' for entry, pc in ENTRIES)
    return (f'static {NAMESPACE}::Next {REFERENCE}(WorldState &s,{NAMESPACE}::Entry start) {{\n'
            + '    switch(start) {\n' + cases + '    }\n    throw std::logic_error("Unknown 133B40 native entry");\n'
            + 'L133b40:' + replace_exits(initialize, ('no_planes',)) + f'\nreturn {NAMESPACE}::Next::classify;'
            + replace_exits(classify, ('explicit_normal',)) + f'\nreturn {NAMESPACE}::Next::angular;'
            + replace_exits(pinned(normal), ('skip_plane', 'end_members'))
            + f'\nreturn {NAMESPACE}::Next::geometry;\n}}\n')


def bridge(start, paths):
    labels = dict(CONTINUATIONS)
    return (f'\n    switch({NAMESPACE}::bridge(s,{NAMESPACE}::Entry::{start},{REFERENCE})) {{\n'
            + ''.join(f'        case {NAMESPACE}::Next::{name}:goto L{labels[name]};\n' for name in paths)
            + f'        case {NAMESPACE}::Next::fallback:break;\n'
            + '        default:throw std::logic_error("Invalid 133B40 continuation");\n'
            + '    }\n')


def install(canonical, output):
    original = source(canonical)
    current = body(output)
    if not current or output.count(CALL) != 1 or current[0].count(GEOMETRY_CALL) != 1:
        raise ValueError('Specialized 133B40 caller or geometry dispatch changed')
    expected = CURSOR.sub(r's.instruction_literal(\1);', original)
    prologue, init, classifier, x87, normal, after = split(expected, True)
    classifier = classifier.replace('\nL133bd4:', '\nL133bd4:' + bridge('classify', ('angular', 'explicit_normal')), 1)
    normal_paths = ('skip_plane', 'end_members', 'geometry')
    normal = pinned(normal)
    for entry, pc in ENTRIES[2:]:
        label = '\nL' + pc + ':'
        if normal.count(label) != 1:
            raise ValueError(f'133B40 normal classification label {pc} changed')
        targets = normal_paths if entry == 'normal' else normal_paths[1:]
        normal = normal.replace(label, label + bridge(entry, targets), 1)
    installed = (prologue + bridge('initialize', ('classify', 'no_planes')) + init
                 + classifier + '\nL133c2f:' + x87.removeprefix('\n') + normal
                 + '\nL133d8a:' + after.removeprefix('\n'))
    reference = reference_body(canonical)
    if REFERENCE in output:
        if output.count(INCLUDE) != 1 or output.count(reference) != 1 or current[0] != installed:
            raise ValueError('Prepared 133B40 stage changed')
        return output, ['133bd4']
    if current[0] != expected or INCLUDE in output:
        raise ValueError('Specialized 133B40 body changed')
    return INCLUDE + output[:current.start()] + reference + '\n' + installed + output[current.end():], ['133bd4']
