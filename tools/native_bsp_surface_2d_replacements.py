"""Authenticate whole2A0230 bodies before installing the shared native tree."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity
PINS=('9692a8fe9194037f480ab2e1625a633fc93b7e1c55bb2694307f78895dcdb8c5',
      'bf6fe25f9776e26c552948ab41f2cab5d2b7a8f61c05465bbdf47f916f9bf2bd')
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
               'cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61')
PREFIX='\n    if(halo2::native_bsp_surface_2d_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_bsp_surface_2d.hpp"\n'
def canonical_body(body):
    at=body.index('{')+1
    return body[:at]+body[at+len(PREFIX):]if body[at:].startswith(PREFIX)else body
def replace_bsp_surface_2d(source):
    matches=list(re.finditer(r'static void run_2a0230\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete2A0230 tree body')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete2A0230 tree body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==body:
        at=body.index('{')+1;source=source[:matches[0].start()]+body[:at]+PREFIX+body[at:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
