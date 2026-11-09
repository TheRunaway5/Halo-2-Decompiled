"""Bind complete3DD00 with native RGB-row and RGB-pack composition."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('68a08b1a1c333f5fbe805f0bcbe9e88e311e8a4f4691be82424e7ef63d6089cc',
      'e162bd3d88e90a9fcd305e2ca70cf6cd176d7491d676ebc7ac9ebb5d0d923603',
      '6b5013cee4c40ca3527d992ba2ba47330fa15454bfe901424b4144c9e2a6cee2')
RESOLVER_PINS=('6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
               '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75')
PREFIX=('\n    if(!prt_helpers_detail::reference() && s.image==SourcePhysicsContactImage::TitleUpdate15 && s.fp.count<=4u) {\n'
        '        model_color_publish_detail::bridge(s);return;\n'
        '    }')


def replace_model_color_publish(source):
    matches=list(re.finditer(r'static void run_3dd00\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete color-publish parent')
    body=matches[0].group();start=body.index('{')+1
    bound=body[start:].startswith(PREFIX)
    original=body[:start]+body[start+len(PREFIX):] if bound else body
    if hashlib.sha256(original.replace('WorldState &s','STATE &s').encode()).hexdigest() not in PINS:
        raise ValueError('Complete3DD00 changed before native binding')
    require_selected_identity(source,original,RESOLVER_PINS)
    if not bound:source=source.replace(body,body[:start]+PREFIX+body[start:],1)
    include='#include "rendering/models/native_tu15_model_color_publish.hpp"\n'
    if include not in source:source=include+source
    return source
