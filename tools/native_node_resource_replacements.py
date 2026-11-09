"""Authenticate complete1E5AF0 eligibility/tail bodies before native binding."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('40da305bee8c363748b87f9ec5e1764dec320a6a9a53d97c8a2a603c5efcb322',
      '6917d1981670463206f0bae57f6c3c237ed8645686a0120189281881cb9feefe')
RESOLVER_PINS=('92991b7c4472fda6e9a7534fe8981ad8f1305ecfacc5704b918cc68ee2925fa2',
               '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657')
PREFIX='\n    if(halo2::native_node_resource_detail::bridge(s,[](auto &r){run_126200(r);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_node_resource.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_node_resource(source):
    matches=list(re.finditer(r'static void run_1e5af0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1E5AF0 node-resource owner')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete1E5AF0 node-resource body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
