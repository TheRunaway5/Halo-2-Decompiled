"""Bind the native TU1.5 collision ray 16C620 (native_tu15_collision_ray.hpp).

The prefix is installed only into the build-local specialized copy, and only
when the complete canonical 16C620 body matches one of its pins (state type
normalized) and every selected() operand of that body is the identity under
TU1.5. Both translated owners are pinned: live support (direct children) and
AI frame (children through dispatch/required). Every other unit reaches the
live-support body through its required route. A changed or missing body keeps
its translated route.
"""
import hashlib
import re

PINS = {
    # Live support after its 16C9BE/16CDFB m32 stores publish C1 (2026-11-16).
    '0784014552a5e98b40fd0b8e5f627f9f945c2d0eb53ad02090d862e16c82c457': 'live support',
    '16f0eef89b50e4041344deccf738508a521a7e3c63a91c60920cf1dfa356453e': 'AI frame',
}
INCLUDE = '#include "physics/runtime/native_tu15_collision_ray.hpp"\n'
PREFIX = ('\n    // BEGIN native collision ray 16c620\n'
          '    if(halo2::native_collision_ray_detail::bridge(s,[](auto &v){run_16c620(v);}))return;\n'
          '    // END native collision ray 16c620')


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def identity_operands(text, found):
    operands = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    if not operands:
        return True
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)
    if not resolver:
        return False
    mapping = {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}
    return all(mapping.get(value) == value for value in operands)


def route(canonical):
    """The pinned owner name of this unit's 16C620 body, or None."""
    found = body(canonical, '16c620')
    if not found:
        return None
    kind = PINS.get(digest(found))
    return kind if kind and identity_operands(canonical, found) else None


def install(canonical, output):
    """Return (output, installed entries)."""
    if not route(canonical):
        return output, []
    current = body(output, '16c620')
    if not current or PREFIX in current[0]:
        return output, []
    start = current[0].index('{') + 1
    replaced = current[0][:start] + PREFIX + current[0][start:]
    output = output[:current.start()] + replaced + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, ['16c620']
