"""Authenticate the entire legacy-named Arena TU1.5 async comparison."""
import hashlib
import re

BODY_PIN='71bfc46f27a962cbb94fe9ff23eed13dd7c2d885a8b580d9a542b26b529238ac'
SELECTED_PIN='ff495ade2e6eb33688d00bae80af91364ff47d20914d31c36d283928ae73bbae'
PREFIX='\n    if(native_async_order_detail::compare(s,0x1a5080u))return;\n'
INCLUDE='#include "platform/native_tu15_async_order.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):] if body[start:].startswith(PREFIX) else body


def authenticate(source):
    matches=list(re.finditer(r'static void run_1a5080\(WorldState &s\) \{.*?\n\}',source,re.S))
    if len(matches)!=1:raise ValueError('Expected one complete Arena async comparator')
    match=matches[0];body=canonical_body(match[0])
    if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:
        raise ValueError('Complete async comparison body changed')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest()!=SELECTED_PIN:
        raise ValueError('Complete Arena selected resolver changed')
    mapping={int(a,16):int(b,16) for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver[0])}
    if mapping.get(0x1a5080)!=0x1ab330 or mapping.get(0x1a47d0)!=0x1aaa80:
        raise ValueError('Async comparison identity or read callback changed')
    return match,body


def install_native_async_order(source):
    match,body=authenticate(source)
    if match[0]==body:
        start=body.index('{')+1
        source=source[:match.start()]+body[:start]+PREFIX+body[start:]+source[match.end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
