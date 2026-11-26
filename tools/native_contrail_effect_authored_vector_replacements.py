"""Bind the authored 219E40 calculation on both sides of its genuine child."""

from hashlib import sha256

from native_contrail_effect_default_vector_replacements import AUTHORED, CANONICAL, CHILD, LABEL, owner


PREPARED = 'd5a54fea20afebb0ea5b1bf81f754396d6eb5fefdd20bbc7afb12c11ca9a42a5'
RESUME = 's.instruction_literal(0x219ec7u);'
CALL_LABEL = 'L_native_contrail_effect_authored_vector_call: '
BEFORE = ('    if (halo2::native_contrail_effect_launch_detail::prepare_authored_vector(s))\n'
          '        goto L_native_contrail_effect_authored_vector_call;\n')
AFTER = '\n    if (halo2::native_contrail_effect_launch_detail::finish_authored_vector(s)) return;'


def install(canonical, prepared):
    original, active = owner(canonical), owner(prepared)
    opening = LABEL + AUTHORED
    if (sha256(original[0].encode()).hexdigest() != CANONICAL or sha256(active[0].encode()).hexdigest() != PREPARED
            or any(active[0].count(token) != 1 for token in (opening, CHILD, RESUME))
            or CHILD + '\n' + RESUME not in active[0]):
        raise ValueError('Contrail authored vector differs from its pinned source')
    body = active[0].replace(opening, LABEL + BEFORE + AUTHORED, 1).replace(CHILD, CALL_LABEL + CHILD + AFTER, 1)
    return prepared[:active.start()] + body + prepared[active.end():]
