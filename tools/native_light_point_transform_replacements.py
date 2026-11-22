"""Authenticate the complete 1454D0 light frustum point-transform body."""

import hashlib
import re

from native_selected_operand_binding import require_selected_identity


BODY_SHA256 = '15e6a5a73ea6ecec338700380de889a7ce40e5ea73041197228db99467369ee0'
RESOLVER_SHA256 = 'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981'
SITES = 41
INCLUDE = '#include "pstencil/native_tu15_light_point_transform.hpp"\n'
PREFIX = '\n    if(halo2::native_light_point_transform_detail::bridge(s))return;'
BODY = re.compile(r'static void run_1454d0\(WorldState &s\) \{.*?\n\}', re.S)


def canonical_body(body):
    opening = body.index('{') + 1
    if body[opening:].startswith(PREFIX):
        return body[:opening] + body[opening + len(PREFIX):]
    return body


def authenticate(source):
    matches = list(BODY.finditer(source))
    if len(matches) != 1:
        raise ValueError('Expected one complete 1454D0 transform body')
    match = matches[0]
    body = canonical_body(match[0])
    if hashlib.sha256(body.encode()).hexdigest() != BODY_SHA256:
        raise ValueError('Complete 1454D0 transform body drift')
    require_selected_identity(source, body, {RESOLVER_SHA256})
    return match, body


def replace_light_point_transform(source):
    if not BODY.search(source):
        return source
    match, body = authenticate(source)
    opening = body.index('{') + 1
    bound = body[:opening] + PREFIX + body[opening:]
    source = source[:match.start()] + bound + source[match.end():]
    if source.count(INCLUDE) > 1:
        raise ValueError('Duplicate native light transform include')
    return source if INCLUDE in source else INCLUDE + source
