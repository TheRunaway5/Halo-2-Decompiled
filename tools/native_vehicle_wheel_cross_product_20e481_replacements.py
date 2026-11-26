"""Bind the wheel cross-product and axis translation stage of TU1.5 motion."""

import re

from native_vehicle_wheel_offset_20e414_replacements import OWNER, digest


INCLUDE = '#include "physics/vehicles/native_tu15_vehicle_wheel_cross_product_20e481.hpp"\n'
ENTRY = '209b81'
END = '209bf4'
LABEL = 'L_wheel_cross_product_entry:'
STAGE = re.compile(r'^(?:L_wheel_cross_product_entry:|[^\n]*instruction(?:_literal)?\(0x209b81u\))'
                   r'[\s\S]*?(?=^L_wheel_force_entry:|^[^\n]*instruction(?:_literal)?\(0x209bf4u\))', re.M)
RAW_PIN = '044775b0b06f29dc566ab5fa9a8cc2a2072974f16ab9aabafaf6bcab2b53aac7'
PREPARED_PIN = '2ed9befb3d0848f7910b9974758800c5c92cbb506f7bfe86fe030ba1a66ff2aa'


def stage_of(source):
    body = OWNER.search(source)
    stage = STAGE.search(body[0]) if body else None
    if not stage:
        raise ValueError('Vehicle 20E481 wheel axis stage missing')
    return stage, body.start()


def prepared_literal(canonical):
    stage, _ = stage_of(canonical)
    if (digest(stage[0]) != RAW_PIN or canonical.count('case 0x209a50u:return 0x20e350u;') != 1 or
            canonical.count('case 0x209a50u:run_209a50(s);break;') != 1):
        raise ValueError('Vehicle 20E481 Original source or selected caller changed')
    stage, count = re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
        r's.instruction_literal(\1);', stage[0])
    if count != 23 or digest(stage) != PREPARED_PIN:
        raise ValueError('Vehicle 20E481 prepared axis stage changed')
    return stage


def prepared_reference(canonical):
    # Legacy SSE's destination selects the payload when both operands are NaN.
    # Give the native and retained branches the actual destination ordering.
    stage = prepared_literal(canonical)
    stage = stage.replace('_mm_add_ss(', 'halo2::native_world_leaves_detail::ordered_add_ss(')
    return stage.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')


def bridge_stage(canonical):
    stage = prepared_reference(canonical)
    entry, *sites = re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', stage)
    if entry != ENTRY:
        raise ValueError('Vehicle 20E481 cross-product entry changed')
    routes = ''.join(f'    case 0x{pc}u: goto L_cross_product_{pc};\n' for pc in [*sites, END])
    prefix = (LABEL + '\n'
              '    switch (halo2::native_vehicle_wheel_cross_product_20e481_detail::bridge(s)) {\n'
              '    case 0: break;\n' + routes +
              '    default: throw std::logic_error("Unknown vehicle wheel cross-product continuation");\n'
              '    }\n')
    for pc in sites:
        line = f'\ns.instruction_literal(0x{pc}u);'
        stage = stage.replace(line, f'\nL_cross_product_{pc}:{line}', 1)
    return prefix + stage + f'L_cross_product_{END}:\n'


def install(canonical, emitted):
    reference = prepared_literal(canonical)
    stage, offset = stage_of(emitted)
    bound = bridge_stage(canonical)
    if stage[0] == bound and emitted.count(INCLUDE) == 1:
        return emitted, ['20e481']
    if stage[0] != reference or INCLUDE in emitted:
        raise ValueError('Vehicle 20E481 emitted cross-product boundaries changed')
    return INCLUDE + emitted[:offset+stage.start()] + bound + emitted[offset+stage.end():], ['20e481']
