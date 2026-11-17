"""Bind the native TU1.5 per-tick object update owners (B8F40/B9090/B9180/BE4A0/10DD10).

The prefix is installed only into the build-local specialized copy of the
live-support unit, and only when the complete canonical body matches its pin
(state type normalized) and every child the closure enters keeps its direct
body in the unit. Each bound owner also receives its verification reference:
the unmodified canonical body with only its child calls routed to the
verification replay (HALO2_VERIFY_OBJECT_TICK). A changed or missing body
keeps its translated route.
"""
import hashlib
import re

PINS = {
    'b8f40': '573e010b6ba6d3ba520ba865317d77d7f2ef1647904c71ffa4ba438a147ec9ad',
    'b9090': '3ca014ac67c2d72bb7694677419299de8458f883e73373fe035abaa56b4ed5be',
    'b9180': 'f777a5394fb8d15570c8a5574a11b6dbea720ee37790c9e093bc1efccd7d20a2',
    'be4a0': '526d2f878f467c89a06de1be618a33055996637f9317db385b8a5f775b9b4dcb',
    '10dd10': 'eb5b57a0e5897192183f85bd0a53b4559f8c9ae975fb8ad1121746e0587f8271',
}
# Direct children of each closure, in the bridge argument order.
CHILDREN = {
    'b8f40': ('be4a0', '10e630'),
    'b9090': ('be850', '10e710', '102910'),
    'b9180': ('be4a0', 'be850', '102910', 'ba3e0', 'ba490', 'c13b0'),
    'be4a0': ('1d5790', '10dd10', 'c0910', 'be4a0', '1d6fb0'),
    '10dd10': (),
}
DISPATCHING = {'be4a0', '10dd10'}
INCLUDE = '#include "physics/runtime/native_tu15_object_tick.hpp"\n'
DETAIL = 'halo2::native_object_tick_detail'
REFERENCES_BEGIN = '// BEGIN native object tick references\n'
REFERENCES_END = '// END native object tick references\n'
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def reference_name(name):
    return f'native_object_tick_reference_{name}'


def prefix(name):
    lambdas = [f'[](auto &v){{{reference_name(name)}(v);}}']
    if name in DISPATCHING:
        lambdas.append('[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}')
    lambdas += [f'[](auto &v){{run_{child}(v);}}' for child in CHILDREN[name]]
    return (f'\n    // BEGIN native object tick {name}\n'
            f'    if({DETAIL}::bridge_{name}(s,' + ',\n        '.join(lambdas) + '))return;\n'
            f'    // END native object tick {name}')


def reference(found, name):
    """The canonical body with only its child calls routed to the replay."""
    text = found[0].replace(f'static void run_{name}({found[1]} &s) {{', f'void {reference_name(name)}(WorldState &s) {{', 1)
    text = re.sub(r'\brun_([0-9a-f]+)\(s\)', DETAIL + r'::replay(s,0x\1u)', text)
    text = re.sub(r'\bdispatch\(s,(target|0x[0-9a-f]+u),0x[0-9a-f]+u,false\)', DETAIL + r'::replay(s,\1)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('void '):
        raise ValueError(f'Object tick reference {name} kept an unrouted child')
    return text + '\n'


def admitted(canonical):
    """Owners whose canonical body matches its pin and whose children exist."""
    names = []
    for name, pin in PINS.items():
        found = body(canonical, name)
        if not found or digest(found) != pin:
            continue
        if not all(body(canonical, child) for child in CHILDREN[name]):
            continue
        names.append(name)
    return names


def install(canonical, output):
    """Return (output, installed entries)."""
    names = admitted(canonical)
    installed, references = [], []
    for name in names:
        current = body(output, name)
        text = prefix(name)
        if not current or text in current[0]:
            continue
        start = current[0].index('{') + 1
        output = output[:current.start()] + current[0][:start] + text + current[0][start:] + output[current.end():]
        references.append(reference(body(canonical, name), name))
        installed.append(name)
    if not installed:
        return output, []
    first = FIRST_BODY.search(output)
    if not first or REFERENCES_BEGIN in output:
        raise ValueError('Object tick references need one unbound live-support unit')
    block = REFERENCES_BEGIN + ''.join(references) + REFERENCES_END
    output = output[:first.start()] + block + output[first.start():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
