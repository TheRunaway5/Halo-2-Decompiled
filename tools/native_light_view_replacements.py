"""Authenticate the full TU1.5 134C00 view collection body and dispatcher."""

import hashlib
import re

from native_selected_operand_binding import require_selected_identity


BODY_SHA256 = 'e9d7286e221d061f898dc17942008fa95added5a6ddabf8496f7fb92e19fd4ef'
RESOLVER_SHA256 = 'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981'
DISPATCH_SHA256 = 'fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e'
SITES = 84
INCLUDE = '#include "pstencil/native_tu15_light_view.hpp"\n'
PREFIX = ('\n    if(halo2::native_light_view_detail::bridge(s,[](auto& child,auto target,auto site)'
          '{dispatch(child,target,site,false);}))return;')
BODY = re.compile(r'static void run_134c00\(WorldState &s\) \{.*?\n\}', re.S)
DISPATCH = re.compile(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}')


def canonical_body(body):
    opening = body.index('{') + 1
    if body[opening:].startswith(PREFIX):
        return body[:opening] + body[opening + len(PREFIX):]
    return body


def authenticate(source):
    matches = list(BODY.finditer(source))
    if len(matches) != 1:
        raise ValueError('Expected one complete 134C00 view collection body')
    match = matches[0]
    body = canonical_body(match[0])
    dispatcher = DISPATCH.search(source)
    if hashlib.sha256(body.encode()).hexdigest() != BODY_SHA256 or not dispatcher or \
            hashlib.sha256(dispatcher[0].encode()).hexdigest() != DISPATCH_SHA256:
        raise ValueError('Complete 134C00 light view context drift')
    require_selected_identity(source, body, {RESOLVER_SHA256})
    return match, body


def replace_light_view(source):
    if not BODY.search(source):
        return source
    match, body = authenticate(source)
    opening = body.index('{') + 1
    bound = body[:opening] + PREFIX + body[opening:]
    source = source[:match.start()] + bound + source[match.end():]
    if source.count(INCLUDE) > 1:
        raise ValueError('Duplicate native light view include')
    return source if INCLUDE in source else INCLUDE + source
