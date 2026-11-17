"""Bind the native TU1.5 1F6D10 structure AABB query (canonical run_1f2420, the
structure collision shape's queryAabb) in the physics world unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its selected()
operands (return addresses, the structure global, the box constants) map to
the TU1.5 literals the adapter uses, its one dispatch site (the definition's
queryObb) is the expected one, its translated children (2E9670, 1F6FA0 and
1F76A0) exist and the child it runs inline (2E4550) passes the world leaves
binder's own check. 1F76A0 is bound at its own body by its binder, so the
adapter enters it through the unit's route like any caller. The bridge
receives a verification reference: a copy of the canonical body with every
translated child and the dispatch call routed to the recorded child boundary;
2E4550 stays its translated body there (its native route is bypassed inside a
reference). The reference and the translated route keep explicit destinations
for their SSE operations (ORDERED, as the convex pair binder does). A changed
or missing body keeps its translated route.
"""
import hashlib
import importlib.util
from pathlib import Path
import re

NAME = '1f2420'
ENTRY = '1f6d10'
PIN = '31a5872a689392545433d68e42da4e1d8473e800d00cc0fff4710b4de1a5e3d4'
OPERANDS = {
    0x1f2442: 0x1f6d32, 0x1f2448: 0x1f6d38, 0x4e7918: 0x4ecc18, 0x46502c: 0x46a0cc, 0x465030: 0x46a0d0,
    0x1f25e4: 0x1f6ed4, 0x1f2631: 0x1f6f21, 0x1f265c: 0x1f6f4c,
}
SITES = (0x1f262e,)
# Translated children: canonical name -> TU1.5 entry (replay_child target), in
# the bridge's order (query_structure, filter_keys, filter).
CHILDREN = {'2e5bc0': 0x2e9670, '1f26b0': 0x1f6fa0, '1f2db0': 0x1f76a0}
INLINE = ('2e0a60',)
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_1f2420'
DETAIL = 'halo2::native_world_leaves_detail'
ORDERED = {
    '_mm_mul_ps(': 'halo2::native_tu15_leaves::ordered_mul_ps(',
    '_mm_add_ps(': 'halo2::native_tu15_leaves::ordered_add_ps(',
    '_mm_sub_ps(': 'halo2::native_world_leaves_detail::ordered_sub_ps(',
}


def ordered(text):
    """The translated text with explicit-destination SSE operations."""
    for old, new in ORDERED.items():
        text = text.replace(old, new)
    return text


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def mapping(text):
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)
    if not resolver:
        return {}
    return {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}


def load(name):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


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
    if not all(body(canonical, child) and body(canonical, child)[1] == STATE for child in CHILDREN):
        return False
    return '2e0a60' in load('native_world_leaves_replacements').admitted(canonical)


def reference(found):
    """The canonical body renamed, with every child routed to the replay."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    for child, entry in CHILDREN.items():
        text = text.replace(f'run_{child}(s)', f'{DETAIL}::replay_child(s,0x{entry:x}u)')
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    text = ordered(text)
    if set(re.findall(r'\brun_([0-9a-f]+)\(', text)) - set(INLINE) or re.search(r'\bdispatch\(|required\(', text) or \
            not text.startswith('static void '):
        raise ValueError('1F6D10 reference kept an unrouted child')
    return text + '\n\n'


def prefix():
    lanes = ''.join(f',[](auto &v){{run_{child}(v);}}' for child in CHILDREN)
    return (f'\n    // BEGIN native structure AABB query {ENTRY}\n'
            f'    if({DETAIL}::bridge_1f6d10(s,[](auto &v){{{REFERENCE}(v);}}{lanes},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}}))return;\n'
            f'    // END native structure AABB query {ENTRY}')


def install(canonical, output):
    """Return (output, installed TU1.5 entries)."""
    if not admitted(canonical):
        return output, []
    current = body(output, NAME)
    text = prefix()
    if not current or text in current[0]:
        return output, []
    start = current[0].index('{') + 1
    replaced = reference(body(canonical, NAME)) + current[0][:start] + text + ordered(current[0][start:])
    output = output[:current.start()] + replaced + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, [ENTRY]
