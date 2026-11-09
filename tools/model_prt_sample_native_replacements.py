"""Authenticate the complete22CD0 reference body before native sampling."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands

BODY_SHA256 = 'c01b3a9312476c8a9bfb57ed9861d171af17092834152802db3d14bb527dba8b'
BODY_PINS = {BODY_SHA256,'0451628ec0321ba5eb84321581daeb16ff92687e50200d5474c64da7d4d7a87d'}
PREFIX = ('\n    if(!prt_helpers_detail::reference()) {\n'
          '        prt_sample_detail::bridge(s);return;\n'
          '    }')


def replace_prt_sample(source):
    pattern = r'static void run_22cd0\(WorldState &s\) \{.*?\n\}'
    matches = list(re.finditer(pattern, source, re.S))
    if len(matches) != 1:
        raise ValueError('Expected one complete PRT sample22CD0')
    body = matches[0].group()
    start = body.index('{') + 1
    bound = body[start:].startswith(PREFIX)
    original = body[:start] + body[start + len(PREFIX):] if bound else body
    normalized = original.replace('WorldState &s', 'STATE &s')
    if hashlib.sha256(normalized.encode()).hexdigest() not in BODY_PINS:
        raise ValueError('Complete PRT sample22CD0 changed before native binding')
    require_identity_operands(source,original)
    include = '#include "rendering/models/native_tu15_prt_sample.hpp"\n'
    if include not in source:
        source = include + source
    if not bound:
        source = source.replace(body, body[:start] + PREFIX + body[start:], 1)
    return source
