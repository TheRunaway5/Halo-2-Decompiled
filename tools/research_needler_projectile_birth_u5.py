#!/usr/bin/env python3
"""Authenticate the complete Needler constructor bodies across TU1.1/TU1.5."""
import hashlib
import json
from pathlib import Path
import research_weapon_effect_u5 as correspondence

ROOT = Path(__file__).resolve().parents[1]


def main():
    output = ROOT / 'analysis/multiplayer/integration/needler-projectile-birth-u5-20261012'
    correspondence.OWNERS = (0xfc2e0, 0x101990, 0xffe20)
    correspondence.run(output)
    path = output / 'report.json'
    report = json.loads(path.read_text())
    report['schema'] = 'source-needler-projectile-birth-u5-v1'
    report['tool_sha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    report['inherited_correspondence_tool_sha256'] = hashlib.sha256(Path(correspondence.__file__).read_bytes()).hexdigest()
    report['scope'] = (
        'Complete normalized constructor/helper body and internal branch correspondence; '
        'TU1.1 constructor executes independently with declared generic storage. '
        'Needler flags9 skip the external speed-scale override service. '
        'No TU1.5 constructor execution or full allocation/guidance/contact proof is claimed.')
    for owner in report['owners']:
        for operand in owner['memory']:
            if operand['old_va'] in ('00465030', '004650c0'):
                assert operand['old_hex'] == operand['new_hex']
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(report=str(path), owners=len(report['owners']))), flush=True)


if __name__ == '__main__':
    main()
