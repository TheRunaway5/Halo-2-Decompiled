#!/usr/bin/env python3
"""Independent original kill-volume/death instruction correspondence.

Names and old image addresses select investigated roots only. Original image
bytes, complete internal branch topology and paired image operands are retained.
"""
import json,sys
from pathlib import Path
import research_mp_sandbox_update_functions as original
original.ENTRIES=(0xdefc0,0x1efef0,0x11db80,0x11da40,0x11d950,
 0xd7c60,0xd7cd0,0xd9180,0xd7e00,0xdac40,0xcb410,0xcbdc0,
 0xc5190,0xeff50,0xb8490,0xb8320,0xe6050,0xe6dc0)
if __name__=='__main__':
 if '--output'not in sys.argv:raise SystemExit('--output required')
 original.main()
 path=Path(sys.argv[sys.argv.index('--output')+1]);report=json.loads(path.read_text())
 report['owner_selection_tool_sha256']=original.sha(__file__)
 report['instruction_matcher_tool_sha256']=report.pop('tool_sha256')
 path.write_text(json.dumps(report,indent=2)+'\n')
