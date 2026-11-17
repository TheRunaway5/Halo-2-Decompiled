"""Bind the native TU1.5 object sphere query closure (BD080/BA970/BAA40).

The prefix is installed only into the build-local specialized copy, and only
when the complete canonical BD080 body matches one of its pins (state type
normalized), every BA970/BAA40 body the unit carries matches its pin, and
every selected() operand of those bodies is the identity under TU1.5. The
pin fixes the unit's original 14D0E0 route, which the closure keeps: the
direct body where the unit has one, else the unit's dispatch (s.required).
A changed or missing body keeps its translated route.
"""
import hashlib
import re

# BD080 variants: live support (literal operands, direct callees), AI frame
# (direct 14D0E0/BA970, dispatched BAA40), combat action and scene device
# (identical: dispatched 14D0E0/BA970, direct BAA40).
BD080_PINS = {
    '166c22e0fb724cc6d622674d91cfe79d309e51f4cbd03601dec01a262c4da6a8': 'direct',
    'ba7c0e0ed4501edae646d4f1c7bd56a3e69368921fedda497a50d7f6237151cc': 'direct',
    'c2bb1ab3109109307779e6317cb393a3452295fd9ea2dab1ffdda0271d9b044b': 'dispatch',
}
CALLEE_PINS = {
    'ba970': {'18e32701cea3b39caafab60d3b44490b579bdd7be9a3c13fa23cd2e977667378',
              'cc5a1abcaea01d2ecf4b5c3fbce5f7cde511e39119645f1e0d5c96fe5a9bd20b'},
    'baa40': {'8da10d078321a4170b0432b395a49fc2e85dc84e561d9ea429bc66fa88f76f83',
              '70e3e5146ef8e40d40faae142ba7c3334fbaa7929a2e5cefa2ecb9fa26e06ecb'},
}
INCLUDE = '#include "physics/runtime/native_tu15_object_sphere_query.hpp"\n'
FLOOD = {
    'direct': '[](auto &v){run_14d0e0(v);}',
    'dispatch': '[](auto &v){dispatch(v,0x14d0e0u,0xbd114u,false);}',
}


def prefix(route):
    return ('\n    // BEGIN native object sphere query bd080\n'
            '    if(halo2::native_object_sphere_query_detail::bridge(s,[](auto &v){run_bd080(v);},\n'
            f'        {FLOOD[route]}))return;\n'
            '    // END native object sphere query bd080')


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def identity_operands(text, bodies):
    operands = {int(value, 16) for found in bodies for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    if not operands:
        return True
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)
    if not resolver:
        return False
    mapping = {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}
    return all(mapping.get(value) == value for value in operands)


def route(canonical):
    """The authenticated 14D0E0 route of this unit, or None."""
    found = body(canonical, 'bd080')
    if not found:
        return None
    kind = BD080_PINS.get(digest(found))
    if not kind:
        return None
    bodies = [found]
    for name, pins in CALLEE_PINS.items():
        callee = body(canonical, name)
        if callee:
            if digest(callee) not in pins:
                return None
            bodies.append(callee)
        elif f'run_{name}(s);' in found[0]:
            return None
    if kind == 'direct' and not body(canonical, '14d0e0'):
        return None
    if kind == 'dispatch' and body(canonical, '14d0e0'):
        return None
    return kind if identity_operands(canonical, bodies) else None


def install(canonical, output):
    """Return (output, installed entries)."""
    kind = route(canonical)
    if not kind:
        return output, []
    current = body(output, 'bd080')
    text = prefix(kind)
    if not current or text in current[0]:
        return output, []
    start = current[0].index('{') + 1
    replaced = current[0][:start] + text + current[0][start:]
    output = output[:current.start()] + replaced + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, ['bd080']
