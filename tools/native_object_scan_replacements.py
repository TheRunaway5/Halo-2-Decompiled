"""Authenticate complete BCEE0 bodies before binding the shared native iterator."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('d589abf3ce319e87f0ab652415b019622a2ab4b85bf0e3b89f08df3cf0b0fa51',
      '8709ac852672de3ad79db36da682b2349cd4162dff16fbfb68dfb9b72ad6f673')
RESOLVER_PINS=('618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a',
               '25374df84ccd5e4a87864e627625292b8e24bfe99d99360c3417970b12c99a59',
               '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657')
PREFIX='\n    if(halo2::native_object_scan_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_object_scan.hpp"\n'


def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body


def replace_object_scan(source):
    matches=list(re.finditer(r'static void run_bcee0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete BCEE0 iterator body')
    body=matches[0][0];original=canonical_body(body)
    if hashlib.sha256(original.encode()).hexdigest()not in PINS:raise ValueError('Complete BCEE0 iterator body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if original==body:
        start=body.index('{')+1;source=source[:matches[0].start()]+body[:start]+PREFIX+body[start:]+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
