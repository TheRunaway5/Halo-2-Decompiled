"""Authenticate all22830 operations, including the retired view-slot prefix."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands

ORIGINAL_BODY_SHA256='36d2c4f0204a71bed071d0a12e2ba7858299c943a7b1ddceeca93941ea8403c9'
BODY_PINS={'dcb61b2be90022c2027fc064f87f0faf198bea14537723f480208ed30388c6c2',
           '8ef88cfaa43c96518c482ea8cb134cfe79ecef9cb9f4792e3fbd67fcfb7f8e63'}
PREFIX=('\n    if(!prt_helpers_detail::reference()) {\n'
        '        prt_update_detail::bridge(s);return;\n'
        '    }')


def replace_prt_update(source):
    pattern=r'static void run_22830\(WorldState &s\) \{.*?\n\}'
    matches=list(re.finditer(pattern,source,re.S))
    if len(matches)!=1:raise ValueError('Expected one complete22830 probe update')
    body=matches[0].group();start=body.index('{')+1
    bound=body[start:].startswith(PREFIX)
    original=body[:start]+body[start+len(PREFIX):] if bound else body
    digest=hashlib.sha256(original.replace('WorldState &s','STATE &s').encode()).hexdigest()
    if digest==ORIGINAL_BODY_SHA256:
        # Authenticate every original operation before the earlier narrower
        # slot-reset transform retires its body. Its site-only check by itself
        # is insufficient to protect a whole-parent native replacement.
        from renderer_native_replacements import replace_model_view_slot_reset
        source=replace_model_view_slot_reset(source)
        return replace_prt_update(source)
    if digest not in BODY_PINS:raise ValueError('Complete22830 probe update changed before native binding')
    require_identity_operands(source,original)
    include='#include "rendering/models/native_tu15_prt_update.hpp"\n'
    if include not in source:source=include+source
    if not bound:source=source.replace(body,body[:start]+PREFIX+body[start:],1)
    return source
