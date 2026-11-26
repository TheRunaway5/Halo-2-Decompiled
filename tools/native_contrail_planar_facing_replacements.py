"""Bind the original vertical and horizontal 42CE0 orientation targets."""

from hashlib import sha256

from native_contrail_vertex_emission_replacements import CANONICAL, owner


CANONICAL_VERTICAL = 'f8ab5e3a9c723df1e74ef9dc4bcea250c6dfa72e8293a38bd735513212343f77'
PREPARED_VERTICAL = 'ba3620fbaa59e917356e2d827474f8552ca6d2e85c7ca5831efb720244442e11'
CANONICAL_HORIZONTAL = 'f8c46d3ec2fdf402ddf7164863a35ef9fde08ef0de0ad92c95fce85386e27fe7'
PREPARED_HORIZONTAL = '38dbe95d681fa5625b6846577ce5812d351df8a6b8172907e461035a293cab78'
INCLUDE = '#include "rendering/effects/native_tu15_contrail_planar_facing.hpp"\n'
VERTICAL = 'L430ce: '
HORIZONTAL = 'L431c8: '
CAMERA = 'L432bd: '
REJOIN = 'L43542: '
FINAL_NORMAL = ('L4353c: s.instruction_literal(0x4353cu); '
                'w(s.memory,s.observe,0x4353cu,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));\n')
TABLE = ('s.instruction_literal(0x430c7u); switch(canonical_branch(r(s.memory,(0x436f0u+s.gp[0]*4u),4),s.image))'
         '{case 0x430ceu:goto L430ce;case 0x431c8u:goto L431c8;case 0x432bdu:goto L432bd;'
         'case 0x436dfu:goto L436df;default:throw std::runtime_error("Required original solver dispatch target unavailable");}')


def spans(body):
    labels = (VERTICAL, HORIZONTAL, CAMERA, REJOIN)
    if any(body.count(label) != 1 for label in labels):
        raise ValueError('Expected one of each authored orientation boundary')
    a, b, c, d = (body.index(label) for label in labels)
    if not a < b < c < d:
        raise ValueError('Authored orientation boundary order changed')
    return body[a:b], body[b:c]


def ordered(block, vertical):
    counts = (3, 1, 5) if vertical else (4, 2, 6)
    for (op, name), count in zip((('mul', 'mul'), ('add', 'sum'), ('sub', 'difference')), counts, strict=True):
        token = f'_mm_{op}_ss('
        if block.count(token) != count:
            raise ValueError(f'Unexpected planar contrail {op} operand sites')
        block = block.replace(token, f'halo2::native_tu15_leaves::ordered_{name}_ss(')
    return block


def hook(vertical):
    flag = 'true' if vertical else 'false'
    return (f'\n    if (halo2::native_contrail_planar_facing_detail::bridge(s,{flag},native_single_spill))\n'
            '        goto L43542;\n')


def install(canonical, prepared):
    original, current = owner(canonical), owner(prepared)
    original_vertical, original_horizontal = spans(original[0])
    vertical, horizontal = spans(current[0])
    check = ((original[0], CANONICAL), (original_vertical, CANONICAL_VERTICAL), (vertical, PREPARED_VERTICAL),
             (original_horizontal, CANONICAL_HORIZONTAL), (horizontal, PREPARED_HORIZONTAL))
    if (any(sha256(block.encode()).hexdigest() != pin for block, pin in check) or INCLUDE in prepared
            or current[0].count(TABLE) != 1 or current[0].count(FINAL_NORMAL) != 1):
        raise ValueError('Planar contrail orientation differs from the pinned source')
    position = current.start() + current[0].index(VERTICAL)
    bound = (ordered(vertical, True).replace(VERTICAL, VERTICAL + hook(True), 1) +
             ordered(horizontal, False).replace(HORIZONTAL, HORIZONTAL + hook(False), 1))
    return INCLUDE + prepared[:position] + bound + prepared[position + len(vertical) + len(horizontal):]
