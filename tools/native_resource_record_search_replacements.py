"""Bind complete126190 resource-record searches to their shared native kernel."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('d14b05471f2e74620ccc80366f49861f7e116299b7d7ecee6c8d7fb0e23e4513',
      'ef9dd1f97c21d296b3d18f68d88d2806aa6f157a1f502086f90514a7b2877098')
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',)
PREFIX='\n    if(halo2::native_resource_record_search_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_resource_record_search.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_resource_record_search(source):
    matches=list(re.finditer(r'static void run_126190\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete126190 resource-record search body')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete126190 resource-record search body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
