"""Bind the native TU1.5 1D7260 body motion snapshot (canonical run_1d29a0: a
havok component's snapshot of one body's position and velocities) in the
physics world unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its selected()
operands (the position query's return address, the zero vector) map to the
TU1.5 literals the adapter uses, it makes no dispatch call and its translated
child (1D8B20, the position query) exists. The adapter enters the child
through the unit's route. The bridge receives a verification reference: a
copy of the canonical body with the child routed to the recorded child
boundary. A changed or missing body keeps its translated route.
"""
import hashlib
import re

NAME = '1d29a0'
ENTRY = '1d7260'
PIN = '7d856a18a3fa9e4a95aa89cdd3045c36c314eaa5ef894f8df2c04d602dcb6e08'
OPERANDS = {0x1d29c1: 0x1d7281, 0x46fc44: 0x474d04}
SITES = ()
# Translated children: canonical name -> TU1.5 entry (replay_child target).
CHILDREN = {'1d4260': 0x1d8b20}
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_1d29a0'
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
    """True when the canonical body, its operands and its child qualify."""
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
    """The canonical body renamed, with the child routed to the replay."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    for child, entry in CHILDREN.items():
        text = text.replace(f'run_{child}(s)', f'{DETAIL}::replay_child(s,0x{entry:x}u)')
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError('1D7260 reference kept an unrouted child')
    return text + '\n\n'


def prefix():
    lanes = ''.join(f',[](auto &v){{run_{child}(v);}}' for child in CHILDREN)
    return (f'\n    // BEGIN native body motion snapshot {ENTRY}\n'
            f'    if({DETAIL}::bridge_1d7260(s,[](auto &v){{{REFERENCE}(v);}}{lanes}))return;\n'
            f'    // END native body motion snapshot {ENTRY}')


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
