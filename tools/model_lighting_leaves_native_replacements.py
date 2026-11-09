"""Bind complete authenticated model lighting/default and lookup leaves."""
import hashlib
import re
from model_prt_binding_common import require_identity_operands
from model_default_lighting_native_replacements import replace_model_default_lighting,PREFIX as DEFAULT_PREFIX

PINS = {
    0x2c390: ('3eb51e7de6bffb7f440a9558b75c2fc40ab0a2b1dc6f56edb656774249b271dc', 'c1ccbde1667c0711a49230dab086cb78d25d545d43d60dc970c7a89cd24539e8'),
    0xe7bc0: ('e3aa6271dc0e046c62899760288020f78ee4067afc7afa62d16b47d1493dc40f', '3b89865967d4d300017a6027d14806df40327f1a2dfbe8983ca8572aba4231ce'),
    0xbcdf0: ('72f1d275edd6cbecd9d69cc64b5c0d79a2e2220d186d732c8c20669815e2b7e6', '466cbe631bbf08e05d4240147371df20fe8abd77fb71868b8f8f552b0be23a37'),
}


def prefix(entry):
    if entry==0x2c390:return DEFAULT_PREFIX
    return ('\n    if(!prt_helpers_detail::reference()) {\n'
            f'        model_lighting_leaves_detail::bridge(s,0x{entry:x}u);return;\n'
            '    }')


def replace_model_lighting_leaves(source):
    include='#include "rendering/models/native_tu15_model_lighting_leaves.hpp"\n'
    if include not in source:source=include+source
    for entry,pins in PINS.items():
        if entry==0x2c390:continue
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
    return replace_model_default_lighting(source)
