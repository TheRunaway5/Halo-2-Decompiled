"""Bind attachment selection and the two fixed contact bases in 1778D0."""

import importlib.util
from pathlib import Path


ENTRY = '1778d0'
SPAN = '177985'
REPEAT = 'L177985: '
FIRST = 'L177990: '
COMPUTE = 's.instruction_literal(0x177a2fu);'
CONTINUATION = 'L177a2f: '
FIXED = 'L177ad8: '
FALLBACK = 'L177b7c: '
FIRST_REFERENCE = 'native_particle_system_selection_first_reference_' + ENTRY
REPEAT_REFERENCE = 'native_particle_system_selection_repeat_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_system_selection.hpp"\n'
NAMESPACE = 'halo2::native_particle_system_selection_detail'
DISPATCH = 'dispatch(s,target,0x177a07u,false);'


def hook(repeat):
    flag = 'true' if repeat else 'false'
    mode = 'repeat' if repeat else 'first'
    reference = REPEAT_REFERENCE if repeat else FIRST_REFERENCE
    return (
        f'\n    // BEGIN native particle system {mode} attachment selection\n'
        f'    switch ({NAMESPACE}::bridge<{flag}>(s,{reference},\n'
        '        [](auto &value,std::uint32_t target,std::uint32_t site){dispatch(value,target,site,false);})) {\n'
        f'    case {NAMESPACE}::Selection::recursive_update: goto L177da2;\n'
        f'    case {NAMESPACE}::Selection::computed_basis: goto L177a2f;\n'
        f'    case {NAMESPACE}::Selection::velocity_basis: goto L177b7c;\n'
        f'    case {NAMESPACE}::Selection::prepared_basis: goto L177cd9;\n'
        f'    case {NAMESPACE}::Selection::retained: break;\n'
        '    }\n'
        f'    // END native particle system {mode} attachment selection\n'
    )


FIRST_HOOK = hook(False)
REPEAT_HOOK = hook(True)


def system_helper():
    path = Path(__file__).with_name('native_particle_system_update_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_system_selection_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    begin_repeat = owner.index(REPEAT) + len(REPEAT)
    first_label = owner.index(FIRST, begin_repeat)
    begin_first = first_label + len(FIRST)
    compute = owner.index(COMPUTE, begin_first)
    boundary = owner.find(CONTINUATION, begin_first, compute)
    end = boundary if boundary != -1 else compute
    fixed = owner.index(FIXED, compute)
    fallback = owner.index(FALLBACK, fixed)
    return (begin_repeat, begin_first, end,
            owner[begin_repeat:first_label].removeprefix(REPEAT_HOOK),
            owner[begin_first:end].removeprefix(FIRST_HOOK), owner[fixed:fallback])


def reference(reload, common, bases, repeat=False, replay=True):
    alternatives = {'L177da2': ('recursive_update', 4), 'L177b7c': ('velocity_basis', 2)}
    for target, (selection, count) in alternatives.items():
        branch = f'goto {target};'
        if common.count(branch) != count:
            raise ValueError(f'Particle system selection route {target} drifted')
        common = common.replace(branch, f'return Selection::{selection};')
    if common.count(DISPATCH) != 1 or bases.count('goto L177cd9;') != 2:
        raise ValueError('Particle system selection dispatch or basis closure drifted')
    if replay:
        common = common.replace(DISPATCH, 'halo2::native_world_leaves_detail::replay_child(s,target);')
    bases = bases.replace('goto L177cd9;', 'return Selection::prepared_basis;')
    name = REPEAT_REFERENCE if repeat else FIRST_REFERENCE
    if not replay:
        name += '_live'
    return (f'static {NAMESPACE}::Selection {name}(WorldState &s) {{\n'
            f'    using {NAMESPACE}::Selection;\n' + (reload if repeat else '') + common +
            '    return Selection::computed_basis;\n' + bases + '}\n')


def install(canonical, output):
    system = system_helper()
    world = system.world_helper()
    if not system.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or FIRST_HOOK in current[0] or any(
            label not in current[0] for label in (REPEAT, FIRST, COMPUTE, FIXED, FALLBACK)):
        return output, []
    start, first, end, reload, common, bases = extract(current[0])
    spans = reload, common, bases
    if any('selected(' in part for part in spans) or tuple(part.count('s.instruction_literal(') for part in spans) != (2, 42, 44):
        return output, []
    if any(current[0].count(f'goto {label};') != 1 for label in ('L177ad8', 'L177b2a')):
        return output, []
    owner = (current[0][:start] + REPEAT_HOOK + current[0][start:first] + FIRST_HOOK
             + current[0][first:end] + CONTINUATION + current[0][end:])
    refs = reference(reload, common, bases) + '\n' + reference(reload, common, bases, repeat=True) + '\n'
    output = output[:current.start()] + refs + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
