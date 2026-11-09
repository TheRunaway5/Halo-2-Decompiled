"""Authenticate complete1707A0 leaves before binding the native pose gather."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('20588ec7ed106df5ca1b3ddebf3bff8f407ab846e1cc68947773c456c4684de4',
      'ac55688e7aa36e1c7b7e5a94b5c8a983d33d90a1beab7b19bfd029bf6cf9b913')
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',)
PREFIX='\n    if(halo2::native_model_default_pose_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_model_default_pose.hpp"\n'

def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body

def replace_model_default_pose(source):
    matches=list(re.finditer(r'static void run_1707a0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete1707A0 pose body')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete1707A0 pose body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
