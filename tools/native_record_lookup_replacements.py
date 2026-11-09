"""Bind the complete literal1E3270 body to its typed descriptor/object lookup."""
import hashlib
import re

PIN='d4e2cbc2eb377e47991d6f16dbe2bf1a43c0d3b5d90ae24cfbd4e399dad47f34'
PREFIX='\n    if(halo2::native_record_lookup_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_record_lookup.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_record_lookup(source):
    matches=list(re.finditer(r'static void run_1e3270\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1E3270 body')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()!=PIN:raise ValueError('Complete1E3270 body drift')
    if 'selected('in original:raise ValueError('Unexpected selected lookup operands require authentication')
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
