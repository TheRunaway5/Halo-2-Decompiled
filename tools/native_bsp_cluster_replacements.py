"""Authenticate and install the native TU1.5 portal flood driver."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

INCLUDE = '#include "physics/runtime/native_tu15_bsp_cluster_flood.hpp"\n'
PREFIX = ('\n    if(native_bsp_cluster_detail::run(s,[](auto &v){run_14cea0(v);},\n'
          '        [](auto &v){run_14d0e0(v);}))return;\n')
PINS = {'b09b5cabdf27212f7787bea5412db6bd99ff3ecf079e09a1465836b10fe0e86b',
        '28829779a3301913bb7d832879726501957d6f8e7b4119324f905ae481c86ceb'}
SELECTED_PINS = {'1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
                 'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',
                 '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657'}


def install_native_bsp_cluster_flood(source):
    matches = list(re.finditer(r'static void run_14d0e0\((\w+) &s\) \{.*?\n\}', source, re.S))
    if not matches:
        return source
    if len(matches) != 1:
        raise ValueError('Expected exactly one TU1.5 cluster flood body')
    match = matches[0]
    body, state = match[0], match[1]
    start = body.index('{') + 1
    canonical = body
    if body[start:].startswith(PREFIX):
        canonical = body[:start] + body[start + len(PREFIX):]
    digest = hashlib.sha256(canonical.replace(state + ' &s', 'STATE &s').encode()).hexdigest()
    if digest not in PINS:
        raise ValueError('Authenticated TU1.5 cluster flood body changed before native binding')
    require_selected_identity(source,canonical,SELECTED_PINS)
    if canonical == body:
        body = body[:start] + PREFIX + body[start:]
        source = source[:match.start()] + body + source[match.end():]
    if INCLUDE not in source:
        source = INCLUDE + source
    return source
