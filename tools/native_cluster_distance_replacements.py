"""Bind the native TU1.5 cluster distance flood (131470, campaign environment unit).

The prefix is installed only into the build-local specialized copy, and only
when the complete canonical 131470 body matches its pin (state type
normalized) and the unit keeps its dispatched 14DD70 route (no direct body).
The verification reference is the unmodified canonical body with only its
child call routed to the replay (HALO2_VERIFY_CLUSTER_DISTANCE). A changed
body keeps its translated route.
"""
import hashlib
import re

PIN = '64cdb7125a7d3ca72a8fe7d096ae0f4d82b2ac36d55356d33ea148054a1b72b8'
INCLUDE = '#include "physics/runtime/native_tu15_cluster_distance_flood.hpp"\n'
DETAIL = 'halo2::native_object_tick_detail'
REFERENCE = 'native_cluster_distance_reference_131470'
REFERENCES_BEGIN = '// BEGIN native cluster distance reference\n'
REFERENCES_END = '// END native cluster distance reference\n'
FIRST_BODY = re.compile(r'^static void run_[0-9a-f]+\(WorldState &s\) \{$', re.M)
PREFIX = ('\n    // BEGIN native cluster distance 131470\n'
          f'    if(halo2::native_cluster_distance_detail::bridge(s,[](auto &v){{{REFERENCE}(v);}},\n'
          '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;\n'
          '    // END native cluster distance 131470')


def body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def digest(found):
    return hashlib.sha256(found[0].replace(found[1] + ' &s', 'STATE &s').encode()).hexdigest()


def reference(found):
    text = found[0].replace(f'static void run_131470({found[1]} &s) {{', f'void {REFERENCE}(WorldState &s) {{', 1)
    text = re.sub(r'\bdispatch\(s,(target|0x[0-9a-f]+u),0x[0-9a-f]+u,false\)', DETAIL + r'::replay(s,\1)', text)
    if re.search(r'\brun_[0-9a-f]+\(|\bdispatch\(|required\(', text) or not text.startswith('void '):
        raise ValueError('Cluster distance reference kept an unrouted child')
    return text + '\n'


def admitted(canonical):
    found = body(canonical, '131470')
    return bool(found and digest(found) == PIN and not body(canonical, '14dd70')
                and 'dispatch(s,target,0x13155du,false)' in found[0])


def install(canonical, output):
    """Return (output, installed entries)."""
    if not admitted(canonical):
        return output, []
    current = body(output, '131470')
    if not current or PREFIX in current[0]:
        return output, []
    start = current[0].index('{') + 1
    output = output[:current.start()] + current[0][:start] + PREFIX + current[0][start:] + output[current.end():]
    first = FIRST_BODY.search(output)
    if not first or REFERENCES_BEGIN in output:
        raise ValueError('Cluster distance reference needs one unbound unit')
    block = REFERENCES_BEGIN + reference(body(canonical, '131470')) + REFERENCES_END
    output = output[:first.start()] + block + output[first.start():]
    if INCLUDE not in output:
        output = INCLUDE + output
    return output, ['131470']
