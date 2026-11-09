"""Authenticate complete1E6E80 source bodies before binding the shared polygon."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('082145925101dfaf8392f515eb68f07bbde923d44c5c266dc023d30d44815bcd',
      '1d112ff567c1185a3a1d19074e6e915f00e03566a0fbfd067affd4f8192479cd')
RESOLVER_PINS=('6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',)
PREFIX='\n    if(halo2::native_bsp_surface_polygon_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_bsp_surface_polygon.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_bsp_surface_polygon(source):
    matches=list(re.finditer(r'static void run_1e6e80\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1E6E80 polygon body')
    current=matches[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete1E6E80 polygon body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if current==original:
        start=current.index('{')+1;source=source[:matches[0].start()]+current[:start]+PREFIX+current[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
