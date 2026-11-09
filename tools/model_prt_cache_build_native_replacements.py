"""Authenticate the complete23670 reference body before native sampling."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands

BODY_SHA256 = 'c426cfebe239e3719ba66c3055aaeca29fe185f507c2cd5f96bee9ea12ee0bb5'
BODY_PINS = {BODY_SHA256,'0310e78f4944f8be0f2c6c37ba4ce5944b203716aa0f1fd1c5f4c3bdec934489'}
PREFIX = ('\n    if(!prt_helpers_detail::reference()) {\n'
          '        prt_cache_build_detail::bridge(s);return;\n'
          '    }')


def replace_prt_cache_build(source):
    pattern = r'static void run_23670\(WorldState &s\) \{.*?\n\}'
    matches = list(re.finditer(pattern, source, re.S))
    if len(matches) != 1:
        raise ValueError('Expected one complete PRT cache build23670')
    body = matches[0].group()
    start = body.index('{') + 1
    bound = body[start:].startswith(PREFIX)
    original = body[:start] + body[start + len(PREFIX):] if bound else body
    normalized = original.replace('WorldState &s', 'STATE &s')
    if hashlib.sha256(normalized.encode()).hexdigest() not in BODY_PINS:
        raise ValueError('Complete PRT cache build23670 changed before native binding')
    require_identity_operands(source,original)
    include = '#include "rendering/models/native_tu15_prt_cache_build.hpp"\n'
    if include not in source:
        source = include + source
    if not bound:
        source = source.replace(body, body[:start] + PREFIX + body[start:], 1)
    return source
