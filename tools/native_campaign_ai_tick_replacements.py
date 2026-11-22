"""Preserve the owned TU1.5 AI tick prefix when regenerating its source unit."""

import hashlib
import re

from native_selected_operand_binding import require_selected_identity


INCLUDE = ('#include "campaign/ai/native_campaign_ai_tick.hpp"\n'
           '#include "campaign/ai/native_campaign_ai_behavior_candidate.hpp"\n'
           '#include "campaign/ai/native_campaign_ai_behavior_search.hpp"\n'
           '#include "campaign/ai/native_campaign_ai_decision_gathering.hpp"\n')
PREFIX = '''
    if (native_campaign_ai_tick::bridge(s, run_1d0470,
        {run_209f40, run_272220, run_1eb200, run_217e00, run_29d540},
        [](WorldState &state) { dispatch(state, 0x1eb4f0u, 0x1d06d3u, false); })) return;'''
PREFIXES = {
    '1d0670': PREFIX,
    '1b0020': '\n    if (native_campaign_ai_behavior_candidate::bridge(s, dispatch)) return;',
    '1b01e0': '\n    if (native_campaign_ai_behavior_search::bridge(s, run_1b0020)) return;',
    '1b08b0': ('\n    if (native_campaign_ai_decision_gathering::bridge(s, run_1b0720, run_1b06c0))'
               '\n        goto native_decision_gathered;'),
}
GATHERED = '\nnative_decision_gathered:'
CONTINUATION = '\nif(s.services.instruction){s.services.instruction(0x1b09e0u);}'
BODIES = {entry: re.compile(r'^static void run_' + entry + r'\(WorldState &s\) \{.*?\n\}', re.M | re.S)
          for entry in PREFIXES}
BODY = BODIES['1d0670']
BODY_SHA256 = {
    '1d0670': '28f426951dfc66a269a88a9a88c037e87de91df8cb1db63a32e4f79b44ad6a43',
    '1b0020': '17e8f4a98d9db1aa0174a720f6e19577a3104eebc05cbbff509ec5ae8da5f3fe',
    '1b01e0': 'e41013799961ec0efd8671e2aaef685b8e92303c81a44f12a515bbf11805998b',
    '1b08b0': '1f3f5b64aeeeef351c005845f3a2758a93db594c4d631bdd0976cfa085db9ce8',
}
RESOLVER_SHA256 = '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657'


def prefix(entry):
    return PREFIXES[entry]


def canonical_body(body, entry='1d0670'):
    opening = body.index('\n')
    route = prefix(entry)
    if body.startswith(route, opening):
        body = body[:opening] + body[opening + len(route):]
        if entry == '1b08b0':
            if body.count(GATHERED + CONTINUATION) != 1:
                raise ValueError('TU1.5 AI gathering continuation drifted')
            body = body.replace(GATHERED + CONTINUATION, CONTINUATION, 1)
    return body


def bind_body(source, entry):
    body_matches = list(BODIES[entry].finditer(source))
    if not body_matches:
        return source
    if len(body_matches) != 1:
        raise ValueError(f'Expected exactly one TU1.5 AI owner at {entry}')
    match = body_matches[0]
    body = canonical_body(match[0], entry)
    opening = body.index('\n')
    if hashlib.sha256(body.encode()).hexdigest() != BODY_SHA256[entry]:
        raise ValueError(f'Complete TU1.5 AI body changed at {entry}')
    require_selected_identity(source, body, {RESOLVER_SHA256})
    bound = body[:opening] + prefix(entry) + body[opening:]
    if entry == '1b08b0':
        if bound.count(CONTINUATION) != 1:
            raise ValueError('Expected a unique TU1.5 AI gathering continuation')
        bound = bound.replace(CONTINUATION, GATHERED + CONTINUATION, 1)
    return source[:match.start()] + bound + source[match.end():]


def replace_campaign_ai_tick(source):
    if not BODY.search(source):
        return source
    if source.count(INCLUDE) > 1:
        raise ValueError('Duplicate TU1.5 AI provider includes')
    for entry in BODIES:
        source = bind_body(source, entry)
    return source if INCLUDE in source else INCLUDE + source
