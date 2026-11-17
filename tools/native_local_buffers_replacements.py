"""Bind the native TU1.5 303C70/303D00 local shape buffers (hkLocalArray of
256-byte shape buffers: construct, canonical run_3001d0; destroy, canonical
run_300260) in the physics world unit.

Each prefix is installed only into the build-local specialized copy, and only
when its canonical body matches its pin (state type normalized) and its
selected() operands (return addresses, the exception handler and the thread
memory pointer) map to the TU1.5 literals the adapter uses. The bridge
receives a verification reference: a copy of the canonical body with its
dispatch calls routed to the recorded child boundary. A changed or missing
body keeps its translated route.
"""
import hashlib
import re

STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
DETAIL = 'halo2::native_world_leaves_detail'
# canonical name -> (TU1.5 entry, pin, selected() operands canonical -> TU1.5,
#                    dispatch sites, bridge)
OWNERS = {
    '3001d0': ('303c70', '631a1ef1e043fc543f0e516958e8f8d271418f1b2dd6c7c4f8b2ac66151ba70d',
               {0x300222: 0x303cc2, 0x37b2b8: 0x37ed68, 0x487638: 0x48c938}, (0x30021f,), 'bridge_303c70'),
    '300260': ('303d00', '4ab35f67e153b474d3ea184bd5d284554769fcf40ea4d1677dd0a88a9f921e24',
               {0x3002af: 0x303d4f, 0x3002e9: 0x303d89, 0x37b2b8: 0x37ed68, 0x487638: 0x48c938},
               (0x3002ac, 0x3002e6), 'bridge_303d00'),
}


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
    """True when the canonical body and its operands qualify."""
    _, pin, operands, sites, _ = OWNERS[name]
    found = body(canonical, name)
    if not found or found[1] != STATE or digest(found) != pin:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(operands) or any(resolved.get(key) != value for key, value in operands.items()):
        return False
    return tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) == sites


def reference_name(name):
    return f'native_world_leaves_reference_{name}'


def reference(found, name):
    """The canonical body renamed, with its dispatch calls routed to the replay."""
    text = found[0].replace(f'static void run_{name}({STATE} &s) {{', f'static void {reference_name(name)}({STATE} &s) {{', 1)
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError(f'{name} reference kept an unrouted child')
    return text + '\n\n'


def prefix(name):
    entry, _, _, _, bridge = OWNERS[name]
    return (f'\n    // BEGIN native local buffers {entry}\n'
            f'    if({DETAIL}::{bridge}(s,[](auto &v){{{reference_name(name)}(v);}},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}}))return;\n'
            f'    // END native local buffers {entry}')


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
        replaced = reference(body(canonical, name), name) + current[0][:start] + text + current[0][start:]
        output = output[:current.start()] + replaced + output[current.end():]
        installed.append(entry)
    if installed and INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
