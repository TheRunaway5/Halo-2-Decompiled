"""Bind the native TU1.5 1F76A0 shape key filter (whether the child shape a
shape key names may still collide; canonical run_1f2db0) in the physics world
unit.

The prefix is installed only into the build-local specialized copy, and only
when the canonical body matches its pin (state type normalized), its
selected() operands (the jump table, the globals, 185AA0 and its return
address) map to the TU1.5 literals the logic and adapter use, the unit's
canonical_branch() maps the jump table's four TU1.5 targets to the body's
four labels, and its only call is the one dispatch site. The bridge receives
the unit's canonical_branch() (for an unknown jump-table entry) and a
verification reference: a copy of the canonical body with its dispatch call
routed to the recorded child boundary. A changed or missing body keeps its
translated route.
"""
import hashlib
import re

NAME = '1f2db0'
ENTRY = '1f76a0'
PIN = 'f7bf834a9bebadc270559fc16ee8a37441e54e75e4404128ee53b87d6819715e'
OPERANDS = {0x185aa0: 0x186e70, 0x1f2e12: 0x1f7702, 0x1f2ebc: 0x1f77ac, 0x46fb64: 0x474c24, 0x4e78d0: 0x4ecbd0,
            0x4e7910: 0x4ecc10, 0x4e7918: 0x4ecc18, 0x4f4850: 0x4f9b50, 0x52625c: 0x52be70}
SITES = (0x1f2e0d,)
# The jump table's TU1.5 targets -> the canonical labels of the body's cases.
TARGETS = {0x1f76c1: 0x1f2dd1, 0x1f7711: 0x1f2e21, 0x1f775b: 0x1f2e6b, 0x1f77a7: 0x1f2eb7}
STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
REFERENCE = 'native_world_leaves_reference_1f2db0'
DETAIL = 'halo2::native_world_leaves_detail'


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def mapping(text, function='selected'):
    resolver = re.search(r'std::uint32_t ' + function + r'\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', text, re.S)
    if not resolver:
        return {}
    return {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}


def admitted(canonical):
    """True when the canonical body, its operands, jump targets and call site qualify."""
    found = body(canonical, NAME)
    if not found or found[1] != STATE or digest(found) != PIN:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(OPERANDS) or any(resolved.get(key) != value for key, value in OPERANDS.items()):
        return False
    branches = mapping(canonical, 'canonical_branch')
    if any(branches.get(key) != value for key, value in TARGETS.items()):
        return False
    if tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) != SITES:
        return False
    return not re.search(r'\brun_[0-9a-f]+\(', found[0][found[0].index('{'):])


def reference(found):
    """The canonical body renamed, with its dispatch call routed to the replay."""
    text = found[0].replace(f'static void run_{NAME}({STATE} &s) {{', f'static void {REFERENCE}({STATE} &s) {{', 1)
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('static void '):
        raise ValueError('1F76A0 reference kept an unrouted child')
    return text + '\n\n'


def prefix():
    return (f'\n    // BEGIN native shape key filter {ENTRY}\n'
            f'    if({DETAIL}::bridge_1f76a0(s,[](auto &v){{{REFERENCE}(v);}},'
            f'[&s](std::uint32_t target){{(void)canonical_branch(target,s.image);}},'
            f'[](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,site,false);}}))return;\n'
            f'    // END native shape key filter {ENTRY}')


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
