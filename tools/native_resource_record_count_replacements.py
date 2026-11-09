"""Bind the complete literal126480 inactive-resource counter."""
import hashlib
import re

PIN='311bbb4d2b537c5ac53ef49cf18b7444b18f232317a64a94740c34c6a46bdc50'
PREFIX='\n    if(halo2::native_resource_record_count_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_resource_record_count.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_resource_record_count(source):
    matches=list(re.finditer(r'static void run_126480\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete126480 inactive-resource counter')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()!=PIN:raise ValueError('Complete126480 resource counter body drift')
    if 'selected('in original:raise ValueError('Unexpected selected resource-counter operands')
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
