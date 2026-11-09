"""Authenticate complete1E5810 bodies before binding the shared native search."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('1b201bbce1fd388c010fdb5953da0bc239b5ad7c86a68acf818454b9a3c3ebad',
      '257ac2e9b7c280edf10932f08fd999718b45866117554632be1eed3b27a602d7')
RESOLVER_PINS=('92991b7c4472fda6e9a7534fe8981ad8f1305ecfacc5704b918cc68ee2925fa2',
               '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657')
PREFIX='\n    if(halo2::native_record_search_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_record_search.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_record_search(source):
    matches=list(re.finditer(r'static void run_1e5810\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1E5810 search body')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete1E5810 search body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
