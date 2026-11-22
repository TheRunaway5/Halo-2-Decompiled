"""Bind the native TU1.5 convex pair owners in the physics world unit: the
2F8630 closest point (canonical run_2f4bd0, the GSK agent's query) and the
2E4FE0 swept pair (canonical run_2e14e0, which calls 2F8630), plus the 30E930
closest-feature simplex controller and its 30DE60 hull fallback.

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

SIMPLEX = '30ae40'
SIMPLEX_ENTRY = '30e930'
HULL = '30a370'
HULL_ENTRY = '30de60'
SIMPLEX_INCLUDE = '#include "physics/runtime/native_tu15_simplex_owner_source.hpp"\n'
SIMPLEX_PINS = (
    '4831de546a5dc35bdbd910bbf393eb7438a992a4b2cb8d7c2718ba1a5fed8113',
    '20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2',
    'c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c',
    '7c93aac13aa7930e203cbe9cd111ec6a112037e49cf659f8fd6781ba20792ec2',
)
HULL_PINS = (
    '08aea2d6c59de447d6dcebd6c3bc4182e5b47b25da1d1b8d28e20d1d7503abf7',
    SIMPLEX_PINS[1], SIMPLEX_PINS[2],
    'e5779c8ed411532bc15334539a7814df2a584d81946b32d413bb5346d166e5c6',
    'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
)
SIMPLEX_PREFIX = '''
    // BEGIN native simplex controller 30E930
    const auto simplex_child = [](auto &state, std::uint32_t target, std::uint32_t site) {
        switch(site) {
        case 0x30e96du:
        case 0x30e985u:
            if(handoff_native_contact_features(state,target,site))return;
            break;
        case 0x30ea7bu:
            run_309d40(state);return;
        case 0x30eaccu:
            if(!handoff_native_simplex_helpers(state,target,site))run_30eb30(state);
            return;
        case 0x30eb3fu:
        case 0x30ecb0u:
            run_309ae0(state);return;
        case 0x30eb85u:
        case 0x30ece2u:
            if(!handoff_native_simplex_helpers(state,target,site))run_309900(state);
            return;
        case 0x30f59au:
            run_30a370(state);return;
        default:
            break;
        }
        // World dispatch accepts canonical call-site labels. The retained
        // parent adapter publishes the selected TU1.5 label before the call.
        dispatch(state,target,site-0x3af0u,false);
    };
    if(halo2::native_simplex_source_owner_detail::bridge(s,simplex_child))return;
    // END native simplex controller 30E930'''
HULL_PREFIX = '''
    // BEGIN native simplex hull controller 30DE60
    const auto hull_child = [](auto &state, std::uint32_t target, std::uint32_t site) {
        switch(site) {
        case 0x30e29cu:
            run_309d40(state);return;
        case 0x30e2fdu:
            run_30eb30(state);return;
        case 0x30e370u:
        case 0x30e4e1u:
            run_309ae0(state);return;
        case 0x30e3b6u:
            if(!handoff_native_simplex_helpers(state,target,site))run_309900(state);
            return;
        case 0x30e513u:
            run_309900(state);return;
        default:
            dispatch(state,target,site-0x3af0u,false);return;
        }
    };
    if(halo2::native_simplex_source_owner_detail::bridge_hull(s,hull_child))return;
    // END native simplex hull controller 30DE60'''

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


def install_simplex(canonical, output):
    """Bind the retained World controllers to their original child routes."""
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}', canonical)
    dispatcher = re.search(r'static void dispatch\(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}', canonical)
    estimate = re.search(r'std::uint32_t source_estimate\(ObservedWorldState &s,std::uint32_t input\) \{[^}]+\}', canonical)
    branch = re.search(r'^std::uint32_t canonical_branch[^\n]+', canonical, re.M)
    extend = re.search(r'^std::uint32_t sign_extend[^\n]+\n[\s\S]*?^}', canonical, re.M)
    owners = (
        (SIMPLEX, SIMPLEX_ENTRY, (estimate,), SIMPLEX_PINS, SIMPLEX_PREFIX),
        (HULL, HULL_ENTRY, (branch, extend), HULL_PINS, HULL_PREFIX),
    )
    installed = []
    for name, entry, helpers, pins, prefix in owners:
        context = body(canonical, name), resolver, dispatcher, *helpers
        if not all(context) or tuple(hashlib.sha256(match[0].encode()).hexdigest() for match in context) != pins:
            continue
        current = body(output, name)
        if not current or prefix in current[0]:
            continue
        opening = current.start() + current[0].index('{') + 1
        output = output[:opening] + prefix + output[opening:]
        installed.append(entry)
    if installed and SIMPLEX_INCLUDE not in output:
        output = SIMPLEX_INCLUDE + output
    return output, installed


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
    output, simplex = install_simplex(canonical, output)
    return output, installed + simplex
