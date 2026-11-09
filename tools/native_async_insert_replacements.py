"""Bind complete async insertion bodies, preserving their PC namespaces."""
import hashlib
import re
from native_async_order_replacements import SELECTED_PIN as ARENA_SELECTED_PIN

INCLUDE='#include "platform/native_tu15_async_order.hpp"\n'
PINS={
    '122020':('f3531bb2f9d4daaae2d61fce13760cd8acced3e69e591fb6744a20101d1134c6',ARENA_SELECTED_PIN),
    '1232c0':('645d313b524d1162c6b8e507493b1c3a9e013a343187b2ae800bcdcb3d87cc49','dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981'),
}


def prefix(entry):
    callback='run_1a5080(v);' if entry=='122020' else (
        'if(!native_async_order_detail::compare(v,0x1ab330u))dispatch(v,0x1ab330u,0x123339u,false);')
    return f'\n    if(native_async_order_detail::insert(s,0x{entry}u,[](auto &v){{{callback}}}))return;\n'


def canonical_body(body,entry):
    start=body.index('{')+1;native=prefix(entry)
    return body[:start]+body[start+len(native):] if body[start:].startswith(native) else body


def authenticate(source,entry):
    matches=list(re.finditer(r'static void run_'+entry+r'\(WorldState &s\) \{.*?\n\}',source,re.S))
    if len(matches)!=1:raise ValueError('Expected one complete async insertion owner '+entry)
    match=matches[0];body=canonical_body(match[0],entry)
    if hashlib.sha256(body.encode()).hexdigest()!=PINS[entry][0]:raise ValueError('Complete async insertion body changed '+entry)
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest()!=PINS[entry][1]:raise ValueError('Complete insertion selected resolver changed '+entry)
    return match,body


def install_native_async_insert(source):
    installed=False
    for entry in PINS:
        if not re.search(r'static void run_'+entry+r'\(WorldState &s\) \{',source):continue
        match,body=authenticate(source,entry);installed=True
        if match[0]==body:
            start=body.index('{')+1;source=source[:match.start()]+body[:start]+prefix(entry)+body[start:]+source[match.end():]
    if installed and INCLUDE not in source:source=INCLUDE+source
    return source
