"""Bind the original 42CE0 frame setup and both exit paths."""

from hashlib import sha256

from native_contrail_vertex_emission_replacements import CANONICAL, owner


CANONICAL_OPEN = '76a1a8debd8baa6b8643ca16fc9c0fd3d256ee5d05f550aa0e15a9d04d01f39a'
PREPARED_OPEN = '8243eb6ab6e0f0af66032a6042ce457263a4b02e52d3aa16d5de7e894f16a912'
CANONICAL_CLOSE = 'b18a25a3ac492c54cdae5646b96247a8ee03f26a06ed521e1c4d82d9f97c488a'
PREPARED_CLOSE = '2c04931af55947bbf90092606d3c4ace91438ee0871c6487a7d893e15e5f4f80'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_draw_frame.hpp"\n'
ENTRY = 'static void run_42ce0(WorldState &s) {\n'
RELOAD = 'L42d8b: '
SUCCESS = 'L43681: '
SKIP = 'L436df: '
END = 'throw std::runtime_error("Original solver fell outside authenticated CFG");'
OPEN_HOOK = ('    {\n'
             '        using namespace halo2::native_contrail_draw_frame_detail;\n'
             '        switch (open(s)) {\n'
             '        case Start::retained: break;\n'
             '        case Start::points: goto L42d90;\n'
             '        case Start::endcaps: goto L43681;\n'
             '        }\n'
             '    }\n')
SUCCESS_HOOK = '\n    if (halo2::native_contrail_draw_frame_detail::close(s,true)) return;\n'
SKIP_HOOK = '\n    if (halo2::native_contrail_draw_frame_detail::close(s,false)) return;\n'
BOUNDARIES = ('s.instruction_literal(0x4303du); s.push(0x4303du,0x43042u);run_bcd80(s);',
              's.instruction_literal(0x430c7u); switch(canonical_branch(',
              's.instruction_literal(0x43573u); s.push(0x43573u,0x43578u);run_42c40(s);')


def phases(body):
    if (not body.startswith(ENTRY) or any(body.count(label) != 1 for label in (RELOAD, SUCCESS, SKIP, END))
            or not body.index(RELOAD) < body.index(SUCCESS) < body.index(SKIP) < body.index(END)):
        raise ValueError('Expected the original contrail frame boundary labels')
    start, finish = body.index(RELOAD), body.index(SUCCESS)
    return body[:start], body[start:finish], body[finish:body.index(END)], body[body.index(END):]


def ordered(prefix):
    for op, helper, count in (('mul', 'mul', 2), ('add', 'sum', 1), ('sub', 'difference', 1)):
        token = f'_mm_{op}_ss('
        if prefix.count(token) != count:
            raise ValueError(f'Unexpected contrail frame {op} operand sites')
        prefix = prefix.replace(token, f'halo2::native_tu15_leaves::ordered_{helper}_ss(')
    return prefix


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    canonical_open, _, canonical_close, _ = phases(original[0])
    start, middle, finish, ending = phases(current[0])
    pinned = ((original[0], CANONICAL), (canonical_open, CANONICAL_OPEN), (canonical_close, CANONICAL_CLOSE),
              (start, PREPARED_OPEN), (finish, PREPARED_CLOSE))
    if (INCLUDE in prepared or any(sha256(value.encode()).hexdigest() != pin for value, pin in pinned)
            or any(middle.count(call) != 1 for call in BOUNDARIES)):
        raise ValueError('Contrail draw frame differs from the pinned source')
    start = ordered(start).replace(ENTRY, ENTRY + OPEN_HOOK, 1)
    finish = finish.replace(SUCCESS, SUCCESS + SUCCESS_HOOK, 1).replace(SKIP, SKIP + SKIP_HOOK, 1)
    return INCLUDE + prepared[:current.start()] + start + middle + finish + ending + prepared[current.end():]
