"""Bind the native TU1.5 2F36F0 child agent update (canonical run_2efc30, a
collection agent's child agents, one per child shape near body A) in the
physics world unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its selected()
operands (return addresses, the exception handler, the game's flags, the box
constants and the thread memory) map to the TU1.5 literals the adapter uses,
its dispatch sites (the virtual calls and the creation function) are the
expected ones, its translated children (2E31E0, 2F3430, 2F34D0 and 2F33B0)
exist, and the children it runs inline (2E4550 and 30BAD0) pass their own
checks: the world leaves binder's, and 30BAD0's pin and operand as the
collection agent binder pins them. 2F33B0 is bound at its own body by its
binder, so the adapter enters it through the unit's route like any caller. The
bridge receives a verification reference: a copy of the canonical body with
every translated child and dispatch call routed to the recorded child
boundary; 2E4550 and 30BAD0 stay their translated bodies there (2E4550's
native route is bypassed inside a reference). The reference and the
translated route keep explicit destinations for their packed SSE operations
(ORDERED, as the convex pair binder does; the harness's extracted 2E4550 needs
them too). A changed or missing body keeps its translated route.
"""
import hashlib
import importlib.util
from pathlib import Path
import re

NAME = '2efc30'
ENTRY = '2f36f0'
PIN = 'd24a06d5176278fad284c957012ac385779f4bbe7a48d2bc4dd996fe1d836e35'
OPERANDS = {
    0x2efc7a: 0x2f373a, 0x2efca9: 0x2f3769, 0x2efd61: 0x2f3821, 0x2efdfb: 0x2f38bb, 0x2eff01: 0x2f39c1, 0x2eff87: 0x2f3a47,
    0x2effb4: 0x2f3a74, 0x2effcd: 0x2f3a8d, 0x2effd7: 0x2f3a97, 0x2f003e: 0x2f3afe, 0x2f0077: 0x2f3b37, 0x2f00aa: 0x2f3b6a,
    0x2f0115: 0x2f3bd5, 0x2f0145: 0x2f3c05, 0x2f015f: 0x2f3c1f, 0x2f0176: 0x2f3c36, 0x2f0193: 0x2f3c53, 0x2f01b6: 0x2f3c76,
    0x2f01db: 0x2f3c9b, 0x2f0204: 0x2f3cc4, 0x2f021d: 0x2f3cdd, 0x2f0227: 0x2f3ce7, 0x2f026e: 0x2f3d2e, 0x2f0286: 0x2f3d46,
    0x2f02ce: 0x2f3d8e, 0x2f0302: 0x2f3dc2, 0x37aba6: 0x37e656, 0x45cac0: 0x461950, 0x46502c: 0x46a0cc, 0x4693a8: 0x46e468,
    0x487638: 0x48c938, 0x4878b8: 0x48cbb8,
}
SITES = (0x2efca6, 0x2efd5e, 0x2efdf8, 0x2eff84, 0x2effb2, 0x2effca, 0x2f0112, 0x2f0143, 0x2f015c, 0x2f0190, 0x2f01b3,
         0x2f01d8, 0x2f0202, 0x2f021a, 0x2f026b, 0x2f0283, 0x2f02ff)
# Translated children: canonical name -> TU1.5 entry (replay_child target), in
# the bridge's order (reserve, construct, destroy, sort).
CHILDREN = {'2df710': 0x2e31e0, '2ef970': 0x2f3430, '2efa10': 0x2f34d0, '2ef8f0': 0x2f33b0}
INLINE = ('2e0a60', '307fe0')
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_2efc30'
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


def used_operands(found):
    return {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}


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
    resolved = mapping(canonical)
    if used_operands(found) != set(OPERANDS) or any(resolved.get(key) != value for key, value in OPERANDS.items()):
        return False
    if tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) != SITES:
        return False
    if not all(body(canonical, child) and body(canonical, child)[1] == STATE for child in CHILDREN):
        return False
    name, pin, operands = load('native_collection_agent_replacements').NULL_AGENT
    null = body(canonical, name)
    if not null or null[1] != STATE or digest(null) != pin or used_operands(null) != set(operands) or \
            any(resolved.get(k) != v for k, v in operands.items()):
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
        raise ValueError('2F36F0 reference kept an unrouted child')
    return text + '\n\n'


def prefix():
    lanes = ''.join(f',[](auto &v){{run_{child}(v);}}' for child in CHILDREN)
    return (f'\n    // BEGIN native child agent update {ENTRY}\n'
            f'    if({DETAIL}::bridge_2f36f0(s,[](auto &v){{{REFERENCE}(v);}}{lanes},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}}))return;\n'
            f'    // END native child agent update {ENTRY}')


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
