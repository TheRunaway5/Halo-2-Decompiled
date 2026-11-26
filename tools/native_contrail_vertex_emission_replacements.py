"""Bind the shared per-point output stages in the authenticated 42CE0 owner."""

from hashlib import sha256

from split_native_translation_unit import BODY


CANONICAL = 'fe6eae11f8ec4b0c633f5ffd2cc03c8c72e652c88b1725e18bacc9a15df0175b'
PREPARED = '951d67999fd896c831721b03efb6fa04cca0d7f0e09633ce71b865ad1eaa0cef'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"\n'


def owner(text):
    matches = [match for match in BODY.finditer(text) if match[1] == 'run_42ce0']
    if len(matches) != 1:
        raise ValueError('Expected one contrail geometry owner at 42CE0')
    return matches[0]


def install(canonical, prepared):
    original, selected = owner(canonical), owner(prepared)
    if tuple(sha256(match[0].encode()).hexdigest() for match in (original, selected)) != (CANONICAL, PREPARED):
        raise ValueError('Contrail point output differs from the pinned source')
    body = selected[0]
    setup = 'L43542: '
    call = 's.instruction_literal(0x43573u); s.push(0x43573u,0x43578u);run_42c40(s);'
    if INCLUDE in prepared or body.count(setup) != 1 or body.count(call) != 1:
        raise ValueError('Contrail native output boundaries changed')

    namespace = 'halo2::native_contrail_vertex_emission_detail'
    body = body.replace(setup, setup + '\n'
        f'if ({namespace}::admitted(s)) {{\n'
        f'    {namespace}::VertexEmission(s).prepare_pair();\n'
        '    goto L_native_contrail_view_fade;\n}\n', 1)
    body = body.replace(call, 'L_native_contrail_view_fade: ' + call + '\n'
        f'if ({namespace}::admitted(s)) {{\n'
        f'    if ({namespace}::VertexEmission(s).finish_pair()) goto L42d8b;\n'
        '    goto L43681;\n}', 1)
    return INCLUDE + prepared[:selected.start()] + body + prepared[selected.end():]
