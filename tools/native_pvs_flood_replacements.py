"""Bind the native TU1.5 PVS closure (1532A0/170C80/170FB0/44680).

The prefix is installed only into the build-local specialized copy, and only
when all four complete canonical bodies match their pins (state type
normalized). A changed or missing body keeps its translated route.
"""
import hashlib
import re

PINS = {
    '1532a0': '9c1b08df4fd245a19fc400cfcdf52b90a68c0096f3fb8b26dedfdeba99db069c',
    '170c80': '21205505bcf1ca9c86513460ed516b9d106eef3e9bcefe2d23fa340ee2b08222',
    '170fb0': 'ae526cd0576f8c8beb9a4de12e34c3c2157d1a0072c1bf57c2ee5eda86efdc62',
    '44680': '84f5b21a96a5f0dd8f99733abc11003ad744b266fd9999b96568932759661fed',
}
INCLUDE = '#include "physics/runtime/native_tu15_pvs_flood.hpp"\n'
PREFIX = ('\n    // BEGIN native PVS flood1532a0\n'
          '    if(halo2::native_pvs_flood_detail::bridge(s,[](auto &v){run_1532a0(v);}))return;\n'
          '    // END native PVS flood1532a0')


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def canonical_matches(text):
    for name, pin in PINS.items():
        found = body(text, name)
        if not found:
            return False
        normalized = found[0].replace(found[1] + ' &s', 'STATE &s')
        if hashlib.sha256(normalized.encode()).hexdigest() != pin:
            return False
    return True


def install(canonical, output):
    """Return (output, installed entries)."""
    if not canonical_matches(canonical):
        return output, []
    current = body(output, '1532a0')
    if not current or PREFIX in current[0]:
        return output, []
    start = current[0].index('{') + 1
    replaced = current[0][:start] + PREFIX + current[0][start:]
    output = output[:current.start()] + replaced + output[current.end():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, ['1532a0']
