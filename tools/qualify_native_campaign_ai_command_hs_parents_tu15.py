#!/usr/bin/env python3
"""Lifecycle suffix mutation/sanitizer proof through genuine native HS children."""
from pathlib import Path
import qualify_native_campaign_hs_command_builtins_tu15 as shared

# Reuse the owned compiler/link lifecycle. Only the lifecycle object is
# replaced; the HS parser, evaluator and command builtins are real children.
shared.SOURCE=Path('src/campaign/ai/native_campaign_ai_command_lifecycle_tu15.cpp')
shared.MUTANTS=(
    ('wake_command','0x21186bu,p+8u,0','0x21186bu,p+8u,none'),
    ('completed_return','after==none?0u','after==none?2u'),
    ('yield_return','after==0xfffffffdu?1u','after==0xfffffffdu?2u'),
    ('sleep_return','after==0xfffffffdu?1u:2u','after==0xfffffffdu?1u:0u'),
    ('completed_state','0x2626e3u,c+0x30u,2,2','0x2626e3u,c+0x30u,1,2'),
    ('yield_state','0x2626fbu,c+0x30u,0,2','0x2626fbu,c+0x30u,1,2'),
    ('actor_context','0x26262eu,0x50eee4u,actor_id','0x26262eu,0x50eee4u,none'),
    ('context_retirement','0x262656u,0x50eee4u,none','0x262656u,0x50eee4u,actor_id'),
)
if __name__=='__main__':shared.main()
