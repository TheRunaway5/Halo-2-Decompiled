#!/usr/bin/env python3
"""Independent complete-body death/physical-node correspondence, read-only.

Reuses the frozen general instruction matcher without modifying it or relying
on supplied annotation names/comments. The explicit owner selection is pinned
separately from that matcher. This does not execute a death or physical body.
"""
import json,sys
from pathlib import Path
import research_mp_sandbox_update_functions as original
original.ENTRIES=(0xe6050,0xe6dc0,0x1e8e60,0x1c70d0,0xc5190,0xeff50,
 0xe2520,0xe2ce0,0xe3190,0xe3270,0xe3a10,0x1d7310,0x1d42d0,
 0xbdf80,0x110bf0,0x1d1940,0xcb410,0xcbdc0)
if __name__=='__main__':
 if '--output' not in sys.argv:raise SystemExit('--output required for independent scope')
 original.main()
 path=Path(sys.argv[sys.argv.index('--output')+1]);report=json.loads(path.read_text())
 report['owner_selection_tool_sha256']=original.sha(__file__)
 report['instruction_matcher_tool_sha256']=report.pop('tool_sha256')
 path.write_text(json.dumps(report,indent=2)+'\n')
