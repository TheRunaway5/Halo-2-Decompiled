"""Bind the two 250AC0 matrix call instructions to their exact local owners."""

import hashlib

import native_particle_transform_projection_replacements as projection


BASIS = projection.basis
PREPARED_PIN = '9d4c0ed293f9791e89548e6c7ec1f0374cc8a1b6cd3f9e9bc44d0f0e59d698ed'
CALLEE_PINS = {
    '1458a0':'0befa697cc977dd00ffd4c68660536e2658729aa62fc60cc7ab6976ae48ff7d1',
    '1459b0':'f2fce1f1a65a118e65a069b35c2e8087b65bf0edb538071d9415eaac84bd1140',
}
INCLUDE = '#include "rendering/effects/native_tu15_particle_transform_matrix_calls.hpp"\n'
NAMESPACE = 'halo2::native_particle_transform_matrix_calls_detail'


def install(canonical, output):
    if not BASIS.admitted(canonical):
        return output, []
    parent = BASIS.world.body(output, BASIS.OWNER)
    if not parent or hashlib.sha256(parent[0].encode()).hexdigest() != PREPARED_PIN:
        return output, []
    for callee, pin in CALLEE_PINS.items():
        body = BASIS.world.body(output, callee)
        if not body or hashlib.sha256(body[0].encode()).hexdigest() != pin:
            return output, []
    bound = parent[0]
    for site, result, callee in projection.CALLS:
        anchor = f'L{site}: s.instruction_literal(0x{site}u); {BASIS.NAMESPACE}::push_saved(s,0x{site}u,0x{result}u);run_{callee}(s);'
        if bound.count(anchor) != 1:
            raise ValueError(f'Particle matrix call {site} changed its prepared owner or ABI')
        hook = (f'L{site}:\n    if ({NAMESPACE}::call<0x{site}u>(s,run_{callee}))goto L{result};\n'
                + anchor.removeprefix(f'L{site}: '))
        bound = bound.replace(anchor,hook,1)
    return INCLUDE + output[:parent.start()] + bound + output[parent.end():], [row[0] for row in projection.CALLS]
