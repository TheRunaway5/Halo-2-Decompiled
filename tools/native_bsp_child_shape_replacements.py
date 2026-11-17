"""Bind the native TU1.5 1F7C10 BSP child shape (canonical run_1f3320, the
structure collision shape's getChildShape) in the physics world unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its selected()
operands (return addresses, the structure's globals, the empty shape's vtable,
the direct calls routed through the unit's dispatch) map to the TU1.5 literals
the adapter uses, its dispatch sites are the expected ones and its translated
children (1F76A0, 1F5FE0, 283450) exist. 1F76A0 and 1F5FE0 are bound at their
own bodies by their binders, so the adapter enters them through the unit's
route like any caller. The bridge receives a verification reference: a copy
of the canonical body with every child and dispatch call routed to the
recorded child boundary. A changed or missing body keeps its translated route.
"""
import hashlib
import re

NAME = '1f3320'
ENTRY = '1f7c10'
PIN = '48a50ef25372131d0b61459d57de6160250a8e8b62aba74a50ef85d87a2cd519'
OPERANDS = {
    0x1f3331: 0x1f7c21, 0x4e7910: 0x4ecc10, 0x1f3370: 0x1f7c60, 0x4e7918: 0x4ecc18, 0x2e4e10: 0x2e88b0, 0x1f33c1: 0x1f7cb1,
    0x1f33d8: 0x1f7cc8, 0x183a00: 0x184e20, 0x1f33ef: 0x1f7cdf, 0x1f3400: 0x1f7cf0, 0x1f340a: 0x1f7cfa, 0x1f1ed0: 0x1f67c0,
    0x1f344e: 0x1f7d3e, 0x1f2a40: 0x1f7330, 0x1f345f: 0x1f7d4f, 0x45d2b4: 0x462180,
}
SITES = (0x1f33bc, 0x1f33ea, 0x1f33fe, 0x1f3405, 0x1f3449, 0x1f345a)
# Translated children: canonical name -> TU1.5 entry (replay_child target), in
# the bridge's order (filter, polygon, is_static).
CHILDREN = {'1f2db0': 0x1f76a0, '1f16f0': 0x1f5fe0, '27f9f0': 0x283450}
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_1f3320'
DETAIL = 'halo2::native_world_leaves_detail'


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def mapping(text):
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)
    if not resolver:
        return {}
    return {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}


def admitted(canonical):
    """True when the canonical body, its operands and its children qualify."""
    found = body(canonical, NAME)
    if not found or found[1] != STATE or digest(found) != PIN:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(OPERANDS) or any(resolved.get(key) != value for key, value in OPERANDS.items()):
        return False
    if tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) != SITES:
        return False
    return all(body(canonical, child) and body(canonical, child)[1] == STATE for child in CHILDREN)


def reference(found):
    """The canonical body renamed, with every child routed to the replay."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    for child, entry in CHILDREN.items():
        text = text.replace(f'run_{child}(s)', f'{DETAIL}::replay_child(s,0x{entry:x}u)')
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError('1F7C10 reference kept an unrouted child')
    return text + '\n\n'


def prefix():
    lanes = ''.join(f',[](auto &v){{run_{child}(v);}}' for child in CHILDREN)
    return (f'\n    // BEGIN native BSP child shape {ENTRY}\n'
            f'    if({DETAIL}::bridge_1f7c10(s,[](auto &v){{{REFERENCE}(v);}}{lanes},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}}))return;\n'
            f'    // END native BSP child shape {ENTRY}')


def install(canonical, output):
    """Return (output, installed TU1.5 entries)."""
    if not admitted(canonical):
        return output, []
    current = body(output, NAME)
    text = prefix()
    if not current or text in current[0]:
        return output, []
    start = current[0].index('{') + 1
    replaced = reference(body(canonical, NAME)) + current[0][:start] + text + current[0][start:]
    output = output[:current.start()] + replaced + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [ENTRY]
