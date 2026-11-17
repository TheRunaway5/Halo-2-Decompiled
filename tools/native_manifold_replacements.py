"""Bind the native TU1.5 manifold owners of 304350's children in the physics
world unit: the 30CF40 manifold report (canonical run_309450) and the 304130
contact weld (canonical run_300650).

Each prefix is installed only into the build-local specialized copy, and only
when its canonical body matches its pin (state type normalized), its selected()
operands (return addresses, constants, the direct call routed through the
unit's dispatch) map to the TU1.5 literals the adapter uses, its dispatch sites
are the expected ones and its translated children exist. The bridge receives a
verification reference: a copy of the canonical body with every child (the
translated children and the dispatch calls) routed to the recorded child
boundary. Both the reference and the translated route keep explicit
destinations for their SSE operations (ORDERED). A changed or missing body
keeps its translated route.
"""
import hashlib
import re

STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
DETAIL = 'halo2::native_world_leaves_detail'
# canonical name -> (TU1.5 entry, pin, selected() operands canonical -> TU1.5,
#                    dispatch sites, translated children canonical -> TU1.5 entry, bridge)
OWNERS = {
    '309450': ('30cf40', 'e47f56fca44bbc2bff27b94c1eab4bace45dd259fc6008f6308f8cadd62bb86f',
               {0x3094a7: 0x30cf97, 0x30965d: 0x30d14d, 0x418a60: 0x41c500}, (0x30965a,), {'2df710': 0x2e31e0},
               'bridge_30cf40'),
    '300650': ('304130', '008de998f59acd9421838585ac596cb4dec025b55b886d7d8bf87755bcdaf418',
               {0x2ffd60: 0x303800, 0x3006ab: 0x30418b}, (0x3006a6,), {}, 'bridge_304130'),
}

# SSE: the first operand owns the NaN payload when both are NaNs. As for the
# world leaves, the translated route and its reference keep explicit
# destinations; without instruction callbacks the compiler may otherwise
# commute the operands.
ORDERED = {
    '_mm_mul_ps(': 'halo2::native_tu15_leaves::ordered_mul_ps(',
    '_mm_add_ps(': 'halo2::native_tu15_leaves::ordered_add_ps(',
    '_mm_sub_ps(': 'halo2::native_world_leaves_detail::ordered_sub_ps(',
    '_mm_add_ss(': 'halo2::native_world_leaves_detail::ordered_add_ss(',
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


def admitted(canonical, name):
    """True when the canonical body, its operands and its children qualify."""
    _, pin, operands, sites, children, _ = OWNERS[name]
    found = body(canonical, name)
    if not found or found[1] != STATE or digest(found) != pin:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(operands) or any(resolved.get(key) != value for key, value in operands.items()):
        return False
    if tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) != sites:
        return False
    return all(body(canonical, child) and body(canonical, child)[1] == STATE for child in children)


def reference_name(name):
    return f'native_world_leaves_reference_{name}'


def reference(found, name):
    """The canonical body renamed, with every child routed to the replay."""
    _, _, _, _, children, _ = OWNERS[name]
    text = found[0].replace(f'static void run_{name}({STATE} &s) {{', f'static void {reference_name(name)}({STATE} &s) {{', 1)
    for child, entry in children.items():
        text = text.replace(f'run_{child}(s)', f'{DETAIL}::replay_child(s,0x{entry:x}u)')
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    text = ordered(text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError(f'{name} reference kept an unrouted child')
    return text + '\n\n'


def prefix(name):
    entry, _, _, _, children, bridge = OWNERS[name]
    lanes = ''.join(f',[](auto &v){{run_{child}(v);}}' for child in children)
    return (f'\n    // BEGIN native manifold {entry}\n'
            f'    if({DETAIL}::{bridge}(s,[](auto &v){{{reference_name(name)}(v);}}{lanes},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}}))return;\n'
            f'    // END native manifold {entry}')


def install(canonical, output):
    """Return (output, installed TU1.5 entries)."""
    installed = []
    for name, (entry, *_rest) in OWNERS.items():
        if not admitted(canonical, name):
            continue
        current = body(output, name)
        text = prefix(name)
        if not current or text in current[0]:
            continue
        start = current[0].index('{') + 1
        replaced = reference(body(canonical, name), name) + current[0][:start] + text + ordered(current[0][start:])
        output = output[:current.start()] + replaced + output[current.end():]
        installed.append(entry)
    if installed and INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
