"""Authenticate complete33B590 service-backed clock owners before binding."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('fd37a54436139686ef059fe7a0da6c18e2d8ab8288e46d0060e04a7efaf64f79',
      '82a9eef1a26d0782ee07c810efc76ac9ace106cfee4c1d70dbf2cd4e4bf190bd')
RESOLVER_PINS=('1abb58ddfc49422a2c66de29d9dc787b82346a9cb9998f83db5d5ee87f04b98a',
               '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75')
PREFIX='\n    if(halo2::native_millisecond_clock_detail::bridge(s))return;'
INCLUDE='#include "platform/native_tu15_millisecond_clock.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_millisecond_clock(source):
    matches=list(re.finditer(r'static void run_33b590\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete33B590 clock body')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete33B590 clock body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
