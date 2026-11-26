"""Bind the exact TU1.5 contrail point update at its real call boundary."""

from hashlib import sha256

from split_native_translation_unit import BODY


CANONICAL = '5720e9e398bb80795d3e0ea25636721b7a4bffdab684af008824e29b1b0ac287'
PREPARED = '87056c82ece676cbff388b39b394d7a1afadc1c34bd5de3cb4d36cfed368879d'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_point_update.hpp"\n'
ENTRY = 'static void run_17f3d0(WorldState &s) {\n'
DIRECT_CALL = 's.instruction_literal(0x17ecc7u); s.push(0x17ecc7u,0x17ecccu);run_17f3d0(s);'
PUBLIC_ENTRY = 'case 0x17f3d0u:run_17f3d0(s);break;'
HOOK = ('    if (halo2::native_contrail_point_update_detail::bridge(s,\n'
        '        [](auto &state, std::uint32_t target, std::uint32_t pc) { dispatch(state,target,pc,false); },\n'
        '        [](auto &state) { run_219330(state); },\n'
        '        native_single_spill)) return;\n')


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_17f3d0']
    if len(matches) != 1:
        raise ValueError('Expected one contrail point update owner at 17F3D0')
    return matches[0]


def ordered(body):
    # Scalar x86 instructions preserve destination operand priority for NaNs.
    for op, count in (('mul', 2), ('add', 1)):
        token = f'_mm_{op}_ss('
        if body.count(token) != count:
            raise ValueError(f'Unexpected contrail point {op} operand sites')
        body = body.replace(token, f'halo2::native_tu15_leaves::ordered_{op}_ss(')
    return body


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    if tuple(sha256(match[0].encode()).hexdigest() for match in (original, current)) != (CANONICAL, PREPARED):
        raise ValueError('Contrail point update differs from the pinned source')
    if (INCLUDE in prepared or not current[0].startswith(ENTRY) or
            prepared.count(DIRECT_CALL) != 1 or prepared.count(PUBLIC_ENTRY) != 1):
        raise ValueError('Contrail point update boundaries changed')
    bound = ordered(current[0]).replace(ENTRY, ENTRY + HOOK, 1)
    return INCLUDE + prepared[:current.start()] + bound + prepared[current.end():]
