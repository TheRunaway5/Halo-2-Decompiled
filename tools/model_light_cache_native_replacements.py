"""Authenticate structured3DDC0 policy and its genuine pool-owner boundaries."""
import hashlib
import re
from native_selected_operand_binding import require_selected_identity

PINS=('dee08b89f3204e2605b98fd3e4837310d2368d18bc4f3d0f1d7af1510c5a3ce4',
      '121096f6bb5c705c14663954634908e0b4938a605bcaa4d87a2d47bae30bd09a',
      '47ff3eb4618ba5f9f7216c795e8d98d9f17dbd527920e856a4048ce0c89c1fe2',
      'a99e3e76704da4544023d6449ba6c3d35cbd5f51c19737ebe366f3730d68716b',
      'da2b22bbb7af9b6766966ca16a56592a94ddacb26da2ec2d316adbc5d68453f4',
      '0fd826957cf578513df3da574fed24ea0d585a518793d4cbc352b2683816f29a',
      '8547785997ed809ba61dc9023fc39e725ec678dee783c64bd62b3b371bced7e3',
      'e291d933d074f5d7f1b48cf56cff146d9b5a06511f5a1a9cb83296ab5abb5ed2',
      '40ed50f5028ccb009f0db038ae1ab94d56a6450d8dda7e58d2a5d5301fdfa29d',
      '4040c9f254c5f12e3d8c41afb5265f236547fc67d5bd71d910340ccfaa7735c2',
      '0a0a93498d2eb013180ea4ad91a94fc943bb9c975b70c8e4c43c5d10e02f2f06')
RESOLVER_PINS=('57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db',
               '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
               '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75',
               'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981')
PREFIX=('\n    if(!prt_helpers_detail::reference() && s.image==SourcePhysicsContactImage::TitleUpdate15 && s.fp.count<=4u) {\n'
        '        model_light_cache_detail::bridge(s);return;\n'
        '    }')
VALIDATION='\nif(source_has_presented_model_pose_tu15(s.memory))source_validate_presented_model_frame_tu15(s.memory,r(s.memory,s.gp[4]+4u,4),r(s.memory,0x4c6854u,4));'
COPIES=('src/rendering/models/source_model_render_tu15_generated.cpp',
        'src/physics/physics_live_support_tu15_generated.cpp',
        'src/runtime/source_map_world_support_tu15_generated.cpp',
        'src/gameplay/units/source_combat_action_tu15_generated.cpp',
        'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp')


def replace_model_light_cache(source):
    matches=list(re.finditer(r'static void run_3ddc0\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete3DDC0 body')
    body=matches[0].group();opening=body.index('{')+1
    anchor=opening+len(VALIDATION) if body[opening:].startswith(VALIDATION) else opening
    bound=body[anchor:].startswith(PREFIX)
    original=body[:anchor]+body[anchor+len(PREFIX):] if bound else body
    if hashlib.sha256(original.replace('WorldState &s','STATE &s').encode()).hexdigest() not in PINS:
        raise ValueError('Complete3DDC0 changed before native binding')
    require_selected_identity(source,original,RESOLVER_PINS)
    if not bound:source=source.replace(body,body[:anchor]+PREFIX+body[anchor:],1)
    include='#include "rendering/models/native_tu15_model_light_cache.hpp"\n'
    if include not in source:source=include+source
    return source
