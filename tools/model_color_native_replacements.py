"""Bind complete authenticated native RGB pack/unpack leaves."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands

PINS = {
    0x1346c0: ('8c3c7f1eee900149a43ef1ef94a78c191153c1517342f3a621cd147bca0fc5b7', '7220352867467e74fd4860a7ba52a8eeffefd7ecff1b62186039eba0ecab6c9c'),
    0x1349e0: ('47d422172108a56aa2c649c3cfb3b593653d9e470b26e3622393b37b9667a8ca', 'fe433715b56efa7a05923b82d905050cd3735c5d893533f2e288b05cc90229d7'),
}


def prefix(entry):
    return ('\n    if(!prt_helpers_detail::reference()) {\n'
            f'        model_color_detail::bridge(s,0x{entry:x}u);return;\n'
            '    }')


def replace_model_color(source):
    include='#include "rendering/models/native_tu15_model_color.hpp"\n'
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
