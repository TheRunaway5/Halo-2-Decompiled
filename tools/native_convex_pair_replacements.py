"""Bind the native TU1.5 convex pair owners in the physics world unit: the
2F8630 closest point (canonical run_2f4bd0, the GSK agent's query) and the
2E4FE0 swept pair (canonical run_2e14e0, which calls 2F8630).

Each prefix is installed only into the build-local specialized copy, and only
when its canonical body matches its pin (state type normalized), its selected()
operands (return addresses, the profiler stream and timer names, constants,
the direct call routed through the unit's dispatch) map to the TU1.5 literals
the adapter uses, its dispatch sites are the expected ones, its translated
children exist and the children it runs inline match their pins (2E4550, as
tools/native_world_leaves_replacements.py checks it, and 2E4760 run_2e0c70).
The bridge receives a verification reference: a copy of the canonical body with
every translated child and dispatch call routed to the recorded child
boundary; the inline children stay their translated bodies (2E4550's native
route is bypassed inside a reference). Both the reference and the translated
routes keep explicit destinations for their SSE operations (ORDERED), and so
does 2E4760's translated body, which the reference runs. A changed or missing
body keeps its translated route.
"""
import hashlib
import importlib.util
from pathlib import Path
import re

STATE = 'ObservedWorldState'
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
DETAIL = 'halo2::native_world_leaves_detail'
# canonical name -> (TU1.5 entry, pin, selected() operands canonical -> TU1.5,
#                    dispatch sites, translated children canonical -> TU1.5 entry,
#                    inline children, bridge)
OWNERS = {
    '2f4bd0': ('2f8630', '1f05ec18f911ed4ed6ede615c2efea722f09bbb320a96bb66faf75ab92ff21ea',
               {0x487674: 0x48c974, 0x487678: 0x48c978, 0x4192bc: 0x41cd3c, 0x4192b4: 0x41cd34, 0x465048: 0x46a0e8,
                0x4650f0: 0x46a190, 0x2f4cde: 0x2f873e, 0x2f4d0a: 0x2f876a, 0x2f4d28: 0x2f8788, 0x2f4d3c: 0x2f879c},
               (), {'30ae40': 0x30e930, '309860': 0x30d350}, ('2e0a60', '2e0c70'), 'bridge_2f8630'),
    '2e14e0': ('2e4fe0', '785e1214184f88330c20ce3e1ae9af639d6313309a408c7fdf187c1c709ac9fe',
               {0x2e1513: 0x2e5013, 0x2e162f: 0x2e512f, 0x2e1576: 0x2e5076, 0x2e1696: 0x2e5196, 0x2e178e: 0x2e528e,
                0x2e1795: 0x2e5295, 0x2e17b1: 0x2e52b1, 0x2e17ca: 0x2e52ca, 0x2e17ed: 0x2e52ed, 0x305650: 0x309150},
               (0x2e1510, 0x2e162c, 0x2e1792, 0x2e17ac), {'27f920': 0x283380, '305560': 0x309060, '2f4bd0': 0x2f8630},
               ('2e0a60',), 'bridge_2e4fe0'),
}
# 2E4760 (run_2e0c70): run inline by 2F8630's adapter; its translated body is
# the reference's, and is given explicit-destination SSE.
FORWARD = ('2e0c70', '879b5a694bd14b6ba40d1b6350c1c2c3a389b7da0043cdefc876aaa251a52924')

# SSE: the first operand owns the NaN payload when both are NaNs. As for the
# world leaves, the translated routes and their references keep explicit
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


def load(name):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def forward_admitted(canonical):
    found = body(canonical, FORWARD[0])
    return bool(found) and found[1] == STATE and digest(found) == FORWARD[1]


def admitted(canonical, name):
    """True when the canonical body, its operands and its children qualify."""
    _, pin, operands, sites, children, inline, _ = OWNERS[name]
    found = body(canonical, name)
    if not found or found[1] != STATE or digest(found) != pin:
        return False
    used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
    resolved = mapping(canonical)
    if used != set(operands) or any(resolved.get(key) != value for key, value in operands.items()):
        return False
    if tuple(int(s, 16) for s in re.findall(r'dispatch\(s,target,0x([0-9a-f]+)u,false\)', found[0])) != sites:
        return False
    if not all(body(canonical, child) and body(canonical, child)[1] == STATE for child in children):
        return False
    if '2e0a60' in inline and '2e0a60' not in load('native_world_leaves_replacements').admitted(canonical):
        return False
    return '2e0c70' not in inline or forward_admitted(canonical)


def reference_name(name):
    return f'native_world_leaves_reference_{name}'


def reference(found, name):
    """The canonical body renamed, with every child routed to the replay."""
    _, _, _, _, children, inline, _ = OWNERS[name]
    text = found[0].replace(f'static void run_{name}({STATE} &s) {{', f'static void {reference_name(name)}({STATE} &s) {{', 1)
    for child, entry in children.items():
        text = text.replace(f'run_{child}(s)', f'{DETAIL}::replay_child(s,0x{entry:x}u)')
    text = re.sub(r'dispatch\(s,target,0x[0-9a-f]+u,false\)', f'{DETAIL}::replay_child(s,target)', text)
    text = ordered(text)
    if set(re.findall(r'\brun_([0-9a-f]+)\(', text)) - set(inline) or re.search(r'\bdispatch\(|required\(', text) or \
            not text.startswith('static void '):
        raise ValueError(f'{name} reference kept an unrouted child')
    return text + '\n\n'


def prefix(name):
    entry, _, _, _, children, _, bridge = OWNERS[name]
    lanes = ''.join(f',[](auto &v){{run_{child}(v);}}' for child in children)
    dispatch = (',[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}'
                if OWNERS[name][3] else '')
    return (f'\n    // BEGIN native convex pair {entry}\n'
            f'    if({DETAIL}::{bridge}(s,[](auto &v){{{reference_name(name)}(v);}}{lanes}{dispatch}))return;\n'
            f'    // END native convex pair {entry}')


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
    if installed:
        forward = body(output, FORWARD[0])
        if forward and forward_admitted(canonical) and ordered(forward[0]) != forward[0]:
            output = output[:forward.start()] + ordered(forward[0]) + output[forward.end():]
        if INCLUDE not in output:
            output = INCLUDE + output
    return output, installed
