"""Authenticate complete BC200 and native authored/default/RGB channel bodies."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS = {
    0xbcd80: ('a0560134b47b70b9b901698b63a2f85630693f7bda22d3d699ea8a30ad6139dc',),
    0xbc200: ('538abebc8f6b73ef7247e2a9a84198ab3c3428d9600f2b6671092eda94b1d14a', '8e7dafba2efb4aeb6e6500d3fedfd5bf4f9c6b311ace1fe300a24d2ed2ff82ea', '9eaf5e64ff34f241368eafeb13261e5705751a0f77d2a94a749d9f5b93ce047d', 'd21345bfaa6b65ee4318b84812a40189757e2abb76204b3e4348087a0ac927af'),
    0x1d8c80: ('1ff74d8cb566c45d092f06a681ec6c06c47427fd43fca4016bb4f19e1bf3f777', '936ae1a2bbd50b69506546f5196f09f0b30f5d2b670392e1263f154e249d8757'),
    0x1d8d80: ('7fc618e3fadc40c809dc64c967ff3788bedb4d530aa1a2b607b524cee37d6dbe', 'c3338df7d7f26c0844b04eb9d48a4075e7048ca787948d092cd377a739f63a6e'),
}
RESOLVER_PINS = ('57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db', '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657', '618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a')


def prefix(entry):
    return ('\n    if(!prt_helpers_detail::reference() && s.image==SourcePhysicsContactImage::TitleUpdate15) {\n'
            f'        model_light_source_detail::bridge(s,0x{entry:x}u);return;\n'
            '    }')


def replace_model_light_source(source):
    found=False
    for entry,pins in PINS.items():
        matches=list(re.finditer(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',source,re.S))
        if not matches:continue
        if len(matches)!=1:raise ValueError(f'Expected one complete light-source body{entry:x}')
        found=True;body=matches[0].group();start=body.index('{')+1;native=prefix(entry)
        bound=body[start:].startswith(native)
        original=body[:start]+body[start+len(native):] if bound else body
        normalized=original.replace('WorldState &s','STATE &s')
        if hashlib.sha256(normalized.encode()).hexdigest() not in pins:
            raise ValueError(f'Complete light-source body{entry:x} changed before native binding')
        require_selected_identity(source,original,RESOLVER_PINS)
        if not bound:source=source.replace(body,body[:start]+native+body[start:],1)
    include='#include "rendering/models/native_tu15_model_light_source.hpp"\n'
    if found and include not in source:source=include+source
    return source
