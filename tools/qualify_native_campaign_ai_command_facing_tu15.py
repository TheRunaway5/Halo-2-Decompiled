#!/usr/bin/env python3
"""Facing leaf mutation/sanitizer checks with actual native HS test dispatch."""
from pathlib import Path
import qualify_native_campaign_hs_command_builtins_tu15 as shared

shared.SOURCE=Path('src/campaign/ai/native_campaign_ai_command_facing_tu15.cpp')
shared.MUTANTS=(
    ('missing_context','if(g[1]!=0xffffffffu)', 'if(true)'),
    ('salted_index','(g[1]&0xffffu)*0xd4u', 'g[1]*0xd4u'),
    ('facing_enabled','g[0]+0x52u,enabled,1', 'g[0]+0x52u,0,1'),
    ('object_transient_clear','if(object)write', 'if(false && object)write'),
    ('facing_kind','g[0]+0x54u,object?1u:2u,2', 'g[0]+0x54u,2,2'),
    ('facing_target','g[0]+0x58u,g[7],4', 'g[0]+0x58u,0,4'),
    ('secondary_flag','g[0]+0x51u,0,1', 'g[0]+0x51u,1,1'),
    ('look_enabled','g[0]+0x46u,enabled,1', 'g[0]+0x46u,0,1'),
    ('look_kind','g[0]+0x48u,object?1u:2u,2', 'g[0]+0x48u,1,2'),
    ('look_target','g[0]+0x4cu,g[7],4', 'g[0]+0x4cu,0,4'),
)

if __name__=='__main__':shared.main()
