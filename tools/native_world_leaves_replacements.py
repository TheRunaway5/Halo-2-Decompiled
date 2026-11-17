"""Bind the native TU1.5 physics world leaves (1F5FE0 collision surface polygon,
2E4550 relative transform, 303BA0 record copy, 2E47B0 transposed rotation).

The prefix is installed only into the build-local specialized copy of the
physics world unit, and only when the complete canonical body matches its pin
(state type normalized) and every selected() operand it uses maps to the
TU1.5 literal the native leaf reads. A changed or missing body keeps its
translated route. The leaf is called with the translated body as its
verification reference (re-entered with the native route bypassed).
"""
import hashlib
import re

# canonical name -> (pin, TU1.5 entry, bridge, selected() operands canonical -> TU1.5)
PINS = {
    '1f16f0': ('3097f1b5c67981997c756bbe5d72337fdef56f6211547789e316f598a47ce862', '1f5fe0', 'bridge_1f5fe0',
               {0x486570: 0x48b880, 0x465030: 0x46a0d0, 0x45d238: 0x4620d0}),
    '2e0a60': ('55f4ef2e24b85265d8a26ee3084ba99d9901b1da85b90f92e1560a2b801e3985', '2e4550', 'bridge_2e4550', {}),
    '300100': ('a338cf2eeba4c5cc1b6eded33e05c8bc9ceacc1041cc6ea6630845be5ccf2681', '303ba0', 'bridge_303ba0', {}),
    '2e0cc0': ('32776d387bf6764d699f02a5e5762bd532a6ca9be7d7801c0eed5f57945cab9d', '2e47b0', 'bridge_2e47b0', {}),
}
# Packed SSE: the first operand owns the NaN payload when both are NaNs. As for
# 2E44D0/2E46E0, the translated reference keeps explicit destinations too;
# without instruction callbacks the compiler may otherwise commute the operands.
ORDERED = {
    '_mm_mul_ps(': 'halo2::native_tu15_leaves::ordered_mul_ps(',
    '_mm_add_ps(': 'halo2::native_tu15_leaves::ordered_add_ps(',
    '_mm_sub_ps(': 'halo2::native_world_leaves_detail::ordered_sub_ps(',
}
INCLUDE = '#include "physics/runtime/native_tu15_world_leaves.hpp"\n'
STATE = 'ObservedWorldState'


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
    """Canonical names whose bodies match their pins and operand bindings."""
    resolved = mapping(canonical)
    names = []
    for name, (pin, _, _, operands) in PINS.items():
        found = body(canonical, name)
        if not found or found[1] != STATE or digest(found) != pin:
            continue
        used = {int(value, 16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s\.image\)', found[0])}
        if used != set(operands) or any(resolved.get(key) != value for key, value in operands.items()):
            continue
        names.append(name)
    return names


def reference(text):
    """The translated body with explicit-destination packed SSE operations."""
    for old, new in ORDERED.items():
        text = text.replace(old, new)
    return text


def prefix(name):
    _, entry, bridge, _ = PINS[name]
    return (f'\n    // BEGIN native world leaf {entry}\n'
            f'    if(halo2::native_world_leaves_detail::{bridge}(s,[](auto &v){{run_{name}(v);}}))return;\n'
            f'    // END native world leaf {entry}')


def install(canonical, output):
    """Return (output, installed TU1.5 entries)."""
    installed = []
    for name in admitted(canonical):
        current = body(output, name)
        text = prefix(name)
        if not current or text in current[0]:
            continue
        start = current[0].index('{') + 1
        replaced = current[0][:start] + text + reference(current[0][start:])
        output = output[:current.start()] + replaced + output[current.end():]
        installed.append(PINS[name][1])
    if installed and INCLUDE not in output:
        output = INCLUDE + output
    return output, installed
