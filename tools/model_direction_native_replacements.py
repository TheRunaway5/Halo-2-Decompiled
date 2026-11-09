"""Bind complete authenticated native direction clamping leaf."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands

PINS = {
    0x11a360: ('db5dcd71d7d0b580c8dcab62a83033b56ca16320963bc0908813f47875c02ba1', 'c4a91a881c13d35af6ac5fb8fd7c11f6797498b2bcda14ce6b5b52365bd5d063'),
}


def prefix(entry):
    return ('\n    if(!prt_helpers_detail::reference()) {\n'
            f'        model_direction_detail::bridge(s);return;\n'
            '    }')


def replace_model_direction(source):
    include='#include "rendering/models/native_tu15_model_direction.hpp"\n'
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
