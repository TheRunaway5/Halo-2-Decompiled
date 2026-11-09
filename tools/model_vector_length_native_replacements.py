"""Authenticate complete native TU1.5 magnitude/distance bodies and x87 ABI."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS = {
    0x24570: ('79f34239d1b83564a4193bb332ee975973654bbdffc67b9827d9140050b6573b', 'c8484e2c89f06a0cd38ea6914fb8117bf920246911db26e524d976a8911368dd'),
    0x3ea20: ('1d7868ab4029db65022cd0de7a2f204957f367520f7eb31cc4a8434a729ac759', 'ac5cf086cda79eaaee223aa1f8654889c77483f5f2e2839c055446f6821fe5bd'),
}
RESOLVER_PINS = ('57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657', '618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a')


def prefix(entry):
    depth=3 if entry==0x24570 else 4
    return ('\n    if(!prt_helpers_detail::reference() && s.image==SourcePhysicsContactImage::TitleUpdate15'
            f' && s.fp.count<={depth}u) {{\n'
            f'        model_vector_length_detail::bridge(s,0x{entry:x}u);return;\n'
            '    }')


def replace_model_vector_length(source):
    found=False
    for entry,pins in PINS.items():
        matches=list(re.finditer(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',source,re.S))
        if not matches:continue
        if len(matches)!=1:raise ValueError(f'Expected one complete vector-length body{entry:x}')
        found=True;body=matches[0].group();start=body.index('{')+1;native=prefix(entry)
        bound=body[start:].startswith(native)
        original=body[:start]+body[start+len(native):] if bound else body
        normalized=original.replace('WorldState &s','STATE &s')
        if hashlib.sha256(normalized.encode()).hexdigest() not in pins:
            raise ValueError(f'Complete vector-length body{entry:x} changed before native binding')
        require_selected_identity(source,original,RESOLVER_PINS)
        if not bound:source=source.replace(body,body[:start]+native+body[start:],1)
    include='#include "rendering/models/native_tu15_model_vector_length.hpp"\n'
    if found and include not in source:source=include+source
    return source
