#!/usr/bin/env python3
"""Complete original death-camera controller correspondence, read only."""
import json
import sys
from pathlib import Path
import research_mp_sandbox_update_functions as matcher

matcher.ENTRIES = (0x1573b0, 0x157520, 0x23fb00, 0x23ff50, 0x23fea0,
                   0x23fd40, 0x170e20, 0x172a50, 0x1732b0)

if __name__ == '__main__':
    if '--output' not in sys.argv:
        raise SystemExit('--output required')
    matcher.main()
    path = Path(sys.argv[sys.argv.index('--output') + 1])
    report = json.loads(path.read_text())
    report['owner_selection_tool_sha256'] = matcher.sha(__file__)
    report['instruction_matcher_tool_sha256'] = report.pop('tool_sha256')
    path.write_text(json.dumps(report, indent=2) + '\n')
