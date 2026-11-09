"""Bind complete1236A0 promotion with genuine mutex and insertion boundaries."""
import hashlib
import re

INCLUDE='#include "platform/native_tu15_async_promote.hpp"\n'
BODY_PIN='77a9046329fec25fb377ce81125ab496fddd147a410c967d91c9976d340aac8c'
PREFIX=('\n    if(native_async_promote_detail::promote(s,[](auto&v){\n'
        '        if(!native_async_order_detail::insert(v,0x1232c0u,[](auto&c){\n'
        '            if(!native_async_order_detail::compare(c,0x1ab330u))dispatch(c,0x1ab330u,0x123339u,false);\n'
        '        }))dispatch(v,0x1232c0u,0x1236f5u,false);\n'
        '    }))return;\n')


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):] if body[start:].startswith(PREFIX) else body


def authenticate(source):
    matches=list(re.finditer(r'static void run_1236a0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if len(matches)!=1:raise ValueError('Expected one complete1236A0 promotion body')
    match=matches[0];body=canonical_body(match[0])
    if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete1236A0 promotion body changed')
    return match,body


def install_native_async_promote(source):
    if not re.search(r'static void run_1236a0\(WorldState &s\) \{',source):return source
    match,body=authenticate(source)
    if match[0]==body:
        start=body.index('{')+1;source=source[:match.start()]+body[:start]+PREFIX+body[start:]+source[match.end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
