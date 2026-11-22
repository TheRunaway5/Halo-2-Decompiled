"""Authenticate the full TU1.5 31570 light admission body and dispatcher."""

import hashlib
import re


BODY_SHA256 = '55c18055cecfeddcfb89723df45f0c5d6788d2b1ef6547c14e99265eb0097ea7'
DISPATCH_SHA256 = '3b76018f37964f76fed09e6f65863a35f82a1a4e2fda6f91792a54158085cccc'
SITES = 78
INCLUDE = '#include "pstencil/native_tu15_light_admission.hpp"\n'
PREFIX = ('\n    if(halo2::native_light_admission_detail::bridge(s,[](auto& child,auto target,auto site)'
          '{dispatch(child,target,site,false);}))return;')
BODY = re.compile(r'static void run_31570\(WorldState &s\) \{.*?\n\}', re.S)
DISPATCH = re.compile(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}')


def canonical_body(body):
    opening = body.index('{') + 1
    if body[opening:].startswith(PREFIX):
        return body[:opening] + body[opening + len(PREFIX):]
    return body


def authenticate(source):
    matches = list(BODY.finditer(source))
    if len(matches) != 1:
        raise ValueError('Expected one complete 31570 light admission body')
    match = matches[0]
    body = canonical_body(match[0])
    dispatcher = DISPATCH.search(source)
    if hashlib.sha256(body.encode()).hexdigest() != BODY_SHA256 or not dispatcher or \
            hashlib.sha256(dispatcher[0].encode()).hexdigest() != DISPATCH_SHA256:
        raise ValueError('Complete 31570 light admission context drift')
    return match, body


def replace_light_admission(source):
    if not BODY.search(source):
        return source
    match, body = authenticate(source)
    opening = body.index('{') + 1
    bound = body[:opening] + PREFIX + body[opening:]
    source = source[:match.start()] + bound + source[match.end():]
    if source.count(INCLUDE) > 1:
        raise ValueError('Duplicate native light admission include')
    return source if INCLUDE in source else INCLUDE + source
