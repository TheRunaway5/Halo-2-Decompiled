"""Bind complete authenticated native packed RGB stepping parent."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands

PINS = {
    0x3e4d0: ('d006a53b293b403d0b63d51dd2bd4d665b6f8014edef0a9b41774b161b881262', 'b7a46037fa0a26c5e0140b0d0b472ecb29a498e495a27821798630ab85c44511'),
}


def prefix(entry):
    return ('\n    if(!prt_helpers_detail::reference()) {\n'
            f'        model_color_step_detail::bridge(s);return;\n'
            '    }')


def replace_model_color_step(source):
    include='#include "rendering/models/native_tu15_model_color_step.hpp"\n'
    if include not in source:source=include+source
    for entry,pins in PINS.items():
        matches=list(re.finditer(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',source,re.S))
        if len(matches)!=1:raise ValueError(f'Expected one complete model leaf{entry:x}')
        body=matches[0].group();start=body.index('{')+1;native=prefix(entry)
        bound=body[start:].startswith(native)
        original=body[:start]+body[start+len(native):] if bound else body
        normalized=original.replace('WorldState &s','STATE &s')
        if hashlib.sha256(normalized.encode()).hexdigest() not in pins:
            raise ValueError(f'Complete model leaf{entry:x} changed before native binding')
        require_identity_operands(source,original)
        if not bound:source=source.replace(body,body[:start]+native+body[start:],1)
    return source
