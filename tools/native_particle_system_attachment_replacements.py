"""Bind attachment transform preparation and nested updates in 1778D0."""

import importlib.util
from pathlib import Path


ENTRY = '1778d0'
SPAN = '177cd9'
PREPARE = 'L177cd9: '
RECURSE = 'L177da2: '
DRAIN = 'L177dd7: '
PREPARE_REFERENCE = 'native_particle_system_attachment_prepare_reference_' + ENTRY
RECURSE_REFERENCE = 'native_particle_system_attachment_recurse_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_system_attachment.hpp"\n'
NAMESPACE = 'halo2::native_particle_system_attachment_detail'
PREPARE_CHILDREN = ('178070', '178920', '178110')
RECURSE_CHILDREN = ('1778d0', '177fa0', '177020')
DISPATCH_SITES = ('177d65', '177d6c', '177d77')


def routes(children):
    return ''.join(f'            if constexpr (decltype(child)::value == 0x{child}u) run_{child}(value);\n'
                   for child in children)


PREPARE_HOOK = (
    '\n    // BEGIN native particle system attachment preparation 177cd9\n'
    f'    if ({NAMESPACE}::bridge_prepare(s,{PREPARE_REFERENCE},\n'
    '        [](auto &value,auto child){\n' + routes(PREPARE_CHILDREN) + '        },\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t site){dispatch(value,target,site,false);}))goto L177da2;\n'
    '    // END native particle system attachment preparation 177cd9\n'
)
RECURSE_HOOK = (
    '\n    // BEGIN native particle system attachment recursion 177da2\n'
    f'    using {NAMESPACE}::Completion;\n'
    f'    switch ({NAMESPACE}::bridge_recurse(s,{RECURSE_REFERENCE},\n'
    '        [](auto &value,auto child){\n' + routes(RECURSE_CHILDREN) + '        })) {\n'
    '    case Completion::next_attachment: goto L177985;\n'
    '    case Completion::drain: goto L177dd7;\n'
    '    case Completion::retained: break;\n'
    '    }\n'
    '    // END native particle system attachment recursion 177da2\n'
)


def system_helper():
    path = Path(__file__).with_name('native_particle_system_update_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_system_attachment_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    start = owner.index(PREPARE) + len(PREPARE)
    middle = owner.index(RECURSE, start)
    after_label = middle + len(RECURSE)
    end = owner.index(DRAIN, after_label)
    preparation = owner[start:middle].removeprefix(PREPARE_HOOK)
    recursion = owner[after_label:end].removeprefix(RECURSE_HOOK)
    return start, after_label, preparation, recursion


def replay(span, children, sites=()):
    system = system_helper()
    span = system.replay_children(span, children)
    for site in sites:
        call = f'dispatch(s,target,0x{site}u,false);'
        if span.count(call) != 1:
            raise ValueError(f'Particle system attachment method at {site} drifted')
        span = span.replace(call, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)
    return span


def reference_prepare(span, for_replay=True):
    if for_replay:
        span = replay(span, PREPARE_CHILDREN, DISPATCH_SITES)
    name = PREPARE_REFERENCE if for_replay else PREPARE_REFERENCE + '_live'
    return f'static void {name}(WorldState &s) {{\n' + span + '}\n'


def reference_recurse(span, for_replay=True):
    if span.count('goto L177985;') != 1:
        raise ValueError('Particle system attachment recursion return drifted')
    span = span.replace('goto L177985;', 'return Completion::next_attachment;', 1)
    if for_replay:
        span = replay(span, RECURSE_CHILDREN)
    name = RECURSE_REFERENCE if for_replay else RECURSE_REFERENCE + '_live'
    return (f'static {NAMESPACE}::Completion {name}(WorldState &s) {{\n'
            f'    using {NAMESPACE}::Completion;\n' + span + '    return Completion::drain;\n}\n')


def install(canonical, output):
    system = system_helper()
    world = system.world_helper()
    if not system.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or PREPARE_HOOK in current[0] or any(label not in current[0] for label in (PREPARE, RECURSE, DRAIN)):
        return output, []
    start, after_label, preparation, recursion = extract(current[0])
    if ('selected(' in preparation + recursion or preparation.count('s.instruction_literal(') != 58 or
            recursion.count('s.instruction_literal(') != 17):
        return output, []
    owner = current[0][:start] + PREPARE_HOOK + current[0][start:after_label] + RECURSE_HOOK + current[0][after_label:]
    refs = reference_prepare(preparation) + '\n' + reference_recurse(recursion) + '\n'
    output = output[:current.start()] + refs + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
