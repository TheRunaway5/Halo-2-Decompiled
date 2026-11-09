"""Authenticate the complete235E0 cache lookup before native routing."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands

BODY_SHA256 = '14012f05fc331034dff0d366a263169a9bdde07a20592ab99c983aed4c2520d3'
BODY_PINS = {BODY_SHA256,'2adeedd6cbe1f9f9f7896f9d6093d2487dd6631ddbda61b657c6ee7c3b3226f4'}
PREFIX = ('\n    if(!prt_helpers_detail::reference()) {\n'
          '        prt_cache_detail::bridge(s);return;\n'
          '    }')


def replace_prt_cache(source):
    matches = list(re.finditer(r'static void run_235e0\(WorldState &s\) \{.*?\n\}', source, re.S))
    if len(matches) != 1: raise ValueError('Expected one complete235E0 cache lookup')
    body = matches[0].group(); start = body.index('{') + 1
    bound = body[start:].startswith(PREFIX)
    original = body[:start] + body[start + len(PREFIX):] if bound else body
    if hashlib.sha256(original.replace('WorldState &s','STATE &s').encode()).hexdigest() not in BODY_PINS:
        raise ValueError('Complete235E0 cache lookup changed before native binding')
    require_identity_operands(source,original)
    include = '#include "rendering/models/native_tu15_prt_cache.hpp"\n'
    if include not in source: source = include + source
    if not bound: source = source.replace(body, body[:start] + PREFIX + body[start:], 1)
    return source
