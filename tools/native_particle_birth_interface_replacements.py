"""Bind both entrances to the remaining 250C60 particle birth continuation."""

import importlib.util
from pathlib import Path


ENTRY = '250c60'
SPAN = '25113a'
START = 's.instruction_literal(0x25113au);'
EARLY = 'L2512c7: '
REFERENCE = 'native_particle_birth_interface_reference_' + ENTRY
EPILOGUE = 'native_particle_birth_epilogue_reference_' + ENTRY
INCLUDE = '#include "rendering/effects/native_tu15_particle_birth_interface.hpp"\n'
DIRECT = ('13a6b0', '176e70', '177f10', '178070', '250170')
DISPATCH = ('251158', '251187', '2511cb', '25125b', '25126e', '251291', '2512a9', '2512b2')
ROUTES = ''.join(f'            if constexpr (decltype(child)::value == 0x{target}u) run_{target}(value);\n'
                 for target in DIRECT)
HOOK = (
    '\n    // BEGIN native particle birth interface 25113a\n'
    f'    if(halo2::native_particle_birth_interface_detail::bridge(s,{REFERENCE},\n'
    '        [](auto &value,auto child){\n' + ROUTES + '        },\n'
    '        [](auto &value,std::uint32_t target,std::uint32_t site){dispatch(value,target,site,false);}))return;\n'
    '    // END native particle birth interface 25113a\n'
)
EARLY_HOOK = (
    '\n    // BEGIN native particle birth early epilogue 2512c7\n'
    f'    if(halo2::native_particle_birth_interface_detail::bridge_epilogue(s,{EPILOGUE}))return;\n'
    '    // END native particle birth early epilogue 2512c7\n'
)


def prefix_helper():
    path = Path(__file__).with_name('native_particle_birth_prefix_replacements.py')
    spec = importlib.util.spec_from_file_location('particle_birth_interface_metadata', path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def extract(owner):
    start = owner.index(START)
    early = owner.index(EARLY, start) + len(EARLY)
    return start, early, owner[start:early], owner[early:].removeprefix(EARLY_HOOK)


def replay_interface(span):
    if span.count('goto L2512c7;') != 1:
        raise ValueError('Particle birth shared epilogue route drifted')
    for target in DIRECT:
        child = f'run_{target}(s);'
        if span.count(child) != 1:
            raise ValueError(f'Particle birth interface child {target} drifted')
        span = span.replace(child, f'halo2::native_world_leaves_detail::replay_child(s,0x{target}u);', 1)
    for site in DISPATCH:
        child = f'dispatch(s,target,0x{site}u,false);'
        if span.count(child) != 1:
            raise ValueError(f'Particle birth interface dispatch {site} drifted')
        span = span.replace(child, 'halo2::native_world_leaves_detail::replay_child(s,target);', 1)
    return span


def references(interface, epilogue):
    return (f'static void {REFERENCE}(WorldState &s) {{\n' + replay_interface(interface) + epilogue + '\n' +
            f'static void {EPILOGUE}(WorldState &s) {{\n' + epilogue + '\n')


def install(canonical, output):
    prefix = prefix_helper()
    world = prefix.world_helper()
    if not prefix.admitted(canonical, world):
        return output, []
    current = world.body(output, ENTRY)
    if not current or HOOK in current[0] or START not in current[0] or EARLY not in current[0]:
        return output, []
    start, early, interface, epilogue = extract(current[0])
    if ('selected(' in interface + epilogue or interface.count('s.instruction_literal(') != 118 or
            epilogue.count('s.instruction_literal(') != 6):
        return output, []
    owner = current[0][:start] + HOOK + current[0][start:early] + EARLY_HOOK + current[0][early:]
    output = output[:current.start()] + references(interface, epilogue) + '\n' + owner + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [SPAN]
