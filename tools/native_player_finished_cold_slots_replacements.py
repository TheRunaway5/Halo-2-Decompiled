"""Bind authenticated unsampled TU1.5 player-finished slot families."""

import re

from native_model_marker_lookup_replacements import function, digest


INPUT_PIN = '3c3275518491da1508b31bab1bb5878ceb2b3dc9289662dc2ee2ee37294cb38c'
NAMESPACE = 'halo2::native_player_finished_cold_detail'
FRAME = 'halo2::native_player_finished_detail'
FLAG_INCLUDE = '#include "gameplay/multiplayer/native_tu15_player_finished_flag_slots.hpp"\n'
EVENT_INCLUDE = '#include "gameplay/multiplayer/native_tu15_player_finished_event_slots.hpp"\n'
POSE_INCLUDE = '#include "gameplay/multiplayer/native_tu15_player_finished_pose_slots.hpp"\n'


def success_resumes(first_pop):
    return tuple(first_pop + step for step in (1, 2, 7, 11, 12, 13))


def bind(body, entry, frame, method, resumes=(), transfers=()):
    for pc in resumes:
        literal = f's.instruction_literal(0x{pc:x}u);'
        pattern = rf'(?m)^(L{pc:x}: )?{re.escape(literal)}'
        def resume(match):
            return (match[1] or '') + f'L_native_finished_cold_{pc:x}: {literal}'
        body, count = re.subn(pattern, resume, body)
        if count != 1:
            raise ValueError(f'Original player-finished slot continuation {pc:x} drifted')

    anchor = f'L{entry:x}: s.instruction_literal(0x{entry:x}u);'
    if body.count(anchor) != 1:
        raise ValueError(f'Expected exactly one player-finished slot entry {entry:x}')
    cases = ''.join(f'    case 0x{pc:x}u: goto L_native_finished_cold_{pc:x};\n' for pc in resumes)
    cases += ''.join(f'    case 0x{pc:x}u: goto L{pc:x};\n' for pc in transfers)
    hook = (f'L{entry:x}:\n    switch ({NAMESPACE}::bridge(s, [&](const auto *cursor) {{\n'
            f'        return {NAMESPACE}::{frame}<WorldState>(s,cursor).{method};\n'
            '    })) {\n'
            f'    case {FRAME}::done: return;\n'
            f'    case {FRAME}::restore_all: goto L1f0ed4;\n'
            + cases + '    default: break;\n    }\n'
            f's.instruction_literal(0x{entry:x}u);')
    return body.replace(anchor, hook, 1)


def bind_flags(body):
    frame = 'FlagFrame'
    for entry, mask in ((0x1f0b09, 1), (0x1f0b64, 4)):
        resumes = tuple(entry + step for step in (7, 19, 20, 21, 28, 34, 38, 39, 40))
        body = bind(body, entry, frame, f'consume_engine_bit<0x{entry:x}u,{mask}u>()', resumes)
    body = bind(body, 0x1f0b35, frame, 'consume_second_engine_bit()',
                (0x1f0b3b, 0x1f0b4e, 0x1f0b4f, 0x1f0b50, 0x1f0b58, 0x1f0b59, 0x1f0b5d, 0x1f0b60))
    for entry, mask in ((0x1f0d4d, 0x200000), (0x1f0d6e, 0x10000), (0x1f0dc1, 0x20000000)):
        body = bind(body, entry, frame, f'test_engine_bit<0x{entry:x}u,0x{mask:x}u>()',
                    (entry + 10, *success_resumes(entry + 16)))
    for entry, mask in ((0x1f0de2, 0x40), (0x1f0e02, 8)):
        body = bind(body, entry, frame, f'test_engine_high_byte<0x{entry:x}u,0x{mask:x}u>()',
                    (entry + 6, *success_resumes(entry + 15)))
    body = bind(body, 0x1f0e22, frame, 'unit_threshold()',
                (0x1f0e2a, 0x1f0e31, *success_resumes(0x1f0e37)))
    body = bind(body, 0x1f0e48, frame, 'position_flag()', (0x1f0e4e,), (0x1f085b,))
    body = bind(body, 0x1f0e84, frame, 'test_team<true>()',
                (0x1f0e90, 0x1f0e99, 0x1f0ea5, 0x1f0eae), (0x1f0ece,))
    body = bind(body, 0x1f0eb0, frame, 'test_team<false>()', (0x1f0ebc, 0x1f0ec5), (0x1f0ece,))
    return bind(body, 0x1f0ece, frame, 'publish_team_result()')


def bind_stack_writes(body, saves):
    for pc, value in saves.items():
        anchor = f's.push(0x{pc:x}u,0x{value:x}u);'
        if body.count(anchor) != 1:
            raise ValueError(f'Original player-finished stack write {pc:x} drifted')
        body = body.replace(anchor, f'{FRAME}::save_word(s,0x{pc:x}u,0x{value:x}u);', 1)
    return body


def bind_events(body):
    frame = 'EventFrame'
    timers = (
        (0x1f0a8b, 0x1d, 'eax', 0x1f0aac),
        (0x1f0c83, 0x1f, 'eax', 0x1f0ca4),
        (0x1f0e55, 0x1c, 'edx', 0x1f0e73),
    )
    saves = {0x1f09af: 0x1f09b4, 0x1f0c38: 0x1f0c3d, 0x1f0d8f: 0,
             0x1f0d91: 4, 0x1f0d93: 0x1f0d98, 0x1f0da3: 0x1f0da8}
    for entry, event, time_reg, success in timers:
        time_read = entry + (5 if time_reg == 'eax' else 6)
        successor = time_read + 20
        saves[successor - 5] = successor
        body = bind(body, entry, frame,
                    f'test_recent_event<0x{entry:x}u,0x{event:x}u,{FRAME}::{time_reg},'
                    f'0x{success:x}u>(run_1eec80)',
                    (time_read, time_read + 4, successor, *success_resumes(success)))

    body = bind(body, 0x1f09af, frame, 'test_unit_unavailable(run_cad40)',
                (0x1f09b4, *success_resumes(0x1f09be)))
    body = bind(body, 0x1f0c04, frame, 'test_elapsed(run_cad40)',
                (0x1f0c0b, 0x1f0c17, 0x1f0c1f, 0x1f0c20, 0x1f0c21, 0x1f0c22,
                 0x1f0c29, 0x1f0c33, 0x1f0c34, 0x1f0c3d, 0x1f0c49, 0x1f0c55,
                 0x1f0c5b, 0x1f0c5f, 0x1f0c65, *success_resumes(0x1f0c72)))
    body = bind(body, 0x1f0d8f, frame,
                'test_award(run_1f1160,[](WorldState &state) { dispatch(state,0xf6850u,0x1f0da3u,false); })',
                (0x1f0d91, 0x1f0d93, 0x1f0d98, 0x1f0da3, 0x1f0da8, *success_resumes(0x1f0db0)))
    increment = ('w(s.memory,s.observe,0x1f0c43u,(0x1a4u+s.gp[5]),4,'
                 's.flags.increment(r(s.memory,(0x1a4u+s.gp[5]),4),4,false));')
    if body.count(increment) != 1:
        raise ValueError('Original player-finished elapsed increment drifted')
    return bind_stack_writes(body.replace(increment,
                                         f'{NAMESPACE}::increment_elapsed(s,0x1f0c43u,0x1a4u+s.gp[5]);', 1), saves)


def bind_poses(body):
    frame = 'PoseFrame'
    body = bind(body, 0x1f06f2, frame, 'test_wrapped_angles()',
                (0x1f06f9, 0x1f0705, 0x1f070f, 0x1f0715, 0x1f0716, 0x1f0717,
                 0x1f071d, 0x1f0721, 0x1f0722, 0x1f0728, 0x1f072f, 0x1f0730,
                 0x1f073c, 0x1f0744, 0x1f074b, 0x1f0753, 0x1f075b, 0x1f0761,
                 0x1f0769, 0x1f076f, 0x1f077b, 0x1f0783, 0x1f0792, 0x1f0796,
                 0x1f079e, 0x1f07aa, 0x1f07b2), (0x1f07be,))
    calls = ','.join(f'[](WorldState &state) {{ dispatch(state,0xbbe00u,0x{pc:x}u,false); }}'
                     for pc in (0x1f07ec, 0x1f0808))
    body = bind(body, 0x1f07cf, frame, f'test_effect_position({calls})',
                (0x1f07d6, 0x1f07e2, 0x1f07f1, 0x1f07f5, 0x1f07f6, 0x1f07f7, 0x1f07f8,
                 0x1f07ff, 0x1f0800), (0x1f080d,))
    body = bind(body, 0x1f0953, frame, 'test_unit_position()',
                (0x1f095a, 0x1f0966, 0x1f096f, 0x1f0977, 0x1f097a, 0x1f097d, 0x1f0980,
                 0x1f0981, 0x1f0982, 0x1f0983, 0x1f0986, 0x1f098a, 0x1f0991, 0x1f0992,
                 0x1f0998, 0x1f099b, 0x1f099e, 0x1f09a2, 0x1f09a6, 0x1f09aa), (0x1f080d,))
    body = bind(body, 0x1f080d, frame, 'test_squared_distance()',
                (0x1f0815, 0x1f081b, 0x1f0823, 0x1f0829, 0x1f0831, 0x1f0837), (0x1f085b,))
    body = bind(body, 0x1f09cf, frame, 'test_heading()',
                (0x1f09d6, 0x1f09e2, 0x1f09ec, 0x1f09f0, 0x1f09f1, 0x1f09f2, 0x1f09f3,
                 0x1f09fa, 0x1f0a01, 0x1f0a07, 0x1f0a0e, 0x1f0a0f, 0x1f0a1b, 0x1f0a23,
                 0x1f0a2a, 0x1f0a32, 0x1f0a40, 0x1f0a51, 0x1f0a61, 0x1f0a68, 0x1f0a70,
                 0x1f0a76, 0x1f0a84), (0x1f0ece,))
    return bind_stack_writes(body, {0x1f07ec: 0x1f07f1, 0x1f0808: 0x1f080d})


def install(prepared):
    owner = function(prepared, 'run_1f0670')
    if digest(owner) != INPUT_PIN:
        return prepared, False
    if prepared.count(owner) != 1:
        raise ValueError('Expected exactly one authenticated player-finished owner')
    return FLAG_INCLUDE + EVENT_INCLUDE + POSE_INCLUDE + prepared.replace(
        owner, bind_poses(bind_events(bind_flags(owner))), 1), True
