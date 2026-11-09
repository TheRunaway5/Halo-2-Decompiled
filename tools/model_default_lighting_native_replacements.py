"""Authenticate every compatible TU1.5 copy of the typed2C390 defaults."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

# These exact older C1/spill representations retain their original bodies.
# Native qualification uses unchanged physical instructions, not their older
# translated floating-status publication as an oracle.
PINS=('3eb51e7de6bffb7f440a9558b75c2fc40ab0a2b1dc6f56edb656774249b271dc',
      'c1ccbde1667c0711a49230dab086cb78d25d545d43d60dc970c7a89cd24539e8',
      '457f2ad4ecb06d6a55e9b181d5d4e0e54984e06c3e4d6068f87c09904018f04e',
      '32e18272fb809d6394406d889e23280cad1a9632dbf950eccf73b2a3d03c6f9e',
      '3a828a97215c3f645f20353884bea66ccee5434dbe4651178c9911bdcfcb67a6',
      '8045d6907e20a552d4ec0c6b4114b72dda1b1a553b2df483007e7022dd6fb5ec')
RESOLVER_PINS=('57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db',
               '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
               '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',
               '892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb')
OLD_PREFIX=('\n    if(!prt_helpers_detail::reference()) {\n'
            '        model_lighting_leaves_detail::bridge(s,0x2c390u);return;\n'
            '    }')
PREFIX=('\n    if(!prt_helpers_detail::reference() && s.image==SourcePhysicsContactImage::TitleUpdate15 && s.fp.count<=6u) {\n'
        '        model_lighting_leaves_detail::bridge(s,0x2c390u);return;\n'
        '    }')


def replace_model_default_lighting(source):
    matches=list(re.finditer(r'static void run_2c390\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete2C390 body')
    body=matches[0].group();start=body.index('{')+1
    installed=next((prefix for prefix in (PREFIX,OLD_PREFIX) if body[start:].startswith(prefix)),None)
    original=body[:start]+body[start+len(installed):] if installed else body
    if hashlib.sha256(original.replace('WorldState &s','STATE &s').encode()).hexdigest() not in PINS:
        raise ValueError('Complete2C390 body changed before native binding')
    require_selected_identity(source,original,RESOLVER_PINS)
    if installed!=PREFIX:source=source.replace(body,original[:start]+PREFIX+original[start:],1)
    include='#include "rendering/models/native_tu15_model_lighting_leaves.hpp"\n'
    if include not in source:source=include+source
    return source
