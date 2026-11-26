"""Bind the original x87 angle paths and outer plane advance in 133B40."""
import hashlib
import re

import native_fog_plane_classifier_replacements as classifier
import native_fog_plane_output_replacements as output
import native_fog_plane_projection_replacements as projection


INCLUDE = '#include "campaign/native_tu15_fog_plane_transform.hpp"\n'
NAMESPACE = 'halo2::native_fog_plane_transform_detail'
REFERENCE = 'native_fog_plane_transform_reference_133b40'
ENTRIES = (('angular', '133c2f'), ('explicit_normal', '133cab'), ('advance', '134020'))
EXITS = (('normal', '133d1d'), ('another_plane', '133bd4'), ('output', '134045'))
HELPERS = (
    ('native_read64', 'std::uint64_t', '5365e17a8ead608ed6628a60f2943303f8e10222a5162aadc6561e6e0f00bdc0'),
    ('native_single_spill', 'std::uint32_t', 'e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'),
    ('native_fcos', 'long double', 'cf89f62db28bc71dd32f516c2c4f9cfaa508914770c6b8b8101b6869383cfb9c'),
    ('native_fsin', 'long double', '610539eda393c4b305d8f76738a9ddd0408a21ede2fd054fdb1c7d8f4c05fe42'),
)


def helper_bodies(canonical):
    proofs = []
    for name, signature, pin in HELPERS:
        matches = re.findall(rf'^{signature} {name}\(.*?\n\}}', canonical, re.M | re.S)
        if len(matches) != 1 or hashlib.sha256(matches[0].encode()).hexdigest() != pin:
            raise ValueError(f'133B40 x87 helper {name} changed')
        proofs.append(matches[0] + '\n')
    return ''.join(proofs)


def split(owner, specialized):
    angular = '\nL133c2f:' if specialized else '\nif(s.services.instruction){s.services.instruction(0x133c2fu);}'
    positions = (angular, '\nL133d1d:', '\nL134020:', '\nL134042:')
    if any(owner.count(marker) != 1 for marker in positions):
        raise ValueError('133B40 plane transform boundaries changed')
    cuts = [0, *(owner.index(marker) for marker in positions), len(owner)]
    if cuts != sorted(cuts):
        raise ValueError('133B40 plane transform boundaries moved')
    return [owner[a:b] for a, b in zip(cuts, cuts[1:])]


def pin_angles(angles):
    angles, count = re.subn(r'\b_mm_mul_ss\(', 'halo2::native_tu15_leaves::ordered_mul_ss(', angles)
    if count != 3:
        raise ValueError('133B40 explicit normal multiply sites changed')
    return angles


def replace_exits(selected, names):
    for name in names:
        pc = dict(EXITS)[name]
        selected, count = re.subn(rf'goto L{pc};', f'return {NAMESPACE}::Next::{name};', selected)
        if count != (2 if name == 'normal' else 1):
            raise ValueError(f'133B40 transform continuation to {pc} changed')
    return selected


def reference_body(canonical):
    _, angles, _, advance, _ = split(classifier.source(canonical), False)
    cases = ''.join(f'        case {NAMESPACE}::Entry::{name}:goto L{pc};\n' for name, pc in ENTRIES)
    return (f'static {NAMESPACE}::Next {REFERENCE}(WorldState &s,{NAMESPACE}::Entry start) {{\n'
            + '    switch(start) {\n' + cases + '    }\n'
            + '    throw std::logic_error("Unknown 133B40 plane transform entry");\n'
            + 'L133c2f:' + replace_exits(pin_angles(angles), ('normal',)).removeprefix('\n')
            + f'\nreturn {NAMESPACE}::Next::normal;'
            + replace_exits(advance, ('another_plane', 'output')) + '\n}\n')


def bind(selected, entry, pc, exits):
    label = '\nL' + pc + ':'
    if selected.count(label) != 1:
        raise ValueError(f'133B40 transform entry {pc} changed')
    bridge = (f'\n    switch({NAMESPACE}::bridge<native_fsin,native_fcos,native_single_spill>'
              + f'(s,{NAMESPACE}::Entry::{entry},{REFERENCE})) {{\n'
              + ''.join(f'        case {NAMESPACE}::Next::{name}:goto L{dest};\n'
                        for name, dest in EXITS if name in exits)
              + f'        case {NAMESPACE}::Next::fallback:break;\n'
              + '        default:throw std::logic_error("Invalid 133B40 plane transform continuation");\n'
              + '    }\n')
    return selected.replace(label, label + bridge, 1)


def install(canonical, emitted):
    helpers = helper_bodies(canonical)
    current = classifier.body(emitted)
    if (not current or emitted.count(classifier.CALL) != 1 or
            current[0].count(classifier.GEOMETRY_CALL) != 1 or
            any(emitted.count(stage.reference_body(canonical)) != 1 for stage in (classifier, projection, output)) or
            any(emitted.count(definition) != 1 for definition in helpers.split('\n}\n') if definition)):
        raise ValueError('133B40 plane transform requires the pinned x87 helpers and prior stages')
    reference = reference_body(canonical)
    _, raw_angles, _, raw_advance, _ = split(classifier.source(canonical), False)
    expected_angles = '\nL133c2f:' + classifier.CURSOR.sub(r's.instruction_literal(\1);', raw_angles).removeprefix('\n')
    expected_advance = classifier.CURSOR.sub(r's.instruction_literal(\1);', raw_advance)
    installed_angles = pin_angles(expected_angles)
    for entry, pc in ENTRIES[:2]:
        installed_angles = bind(installed_angles, entry, pc, ('normal',))
    installed_advance = bind(expected_advance, *ENTRIES[2], ('another_plane', 'output'))
    before, angles, retained, advance, after = split(current[0], True)
    if REFERENCE in emitted:
        if (emitted.count(INCLUDE) != 1 or emitted.count(reference) != 1 or
                angles != installed_angles or advance != installed_advance):
            raise ValueError('Prepared 133B40 plane transform binding changed')
        return emitted, ['133c2f']
    if angles != expected_angles or advance != expected_advance or INCLUDE in emitted:
        raise ValueError('Specialized 133B40 plane transform body changed')
    return (INCLUDE + emitted[:current.start()] + reference + '\n' + before + installed_angles
            + retained + installed_advance + after + emitted[current.end():], ['133c2f'])
