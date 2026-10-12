#!/usr/bin/env python3
"""Authenticate complete solver-motion bodies; no annotation-name authority."""
import hashlib, json, sys
from pathlib import Path
import research_physics_world_step_20261010 as world

# Keep the verified motion callees in the full paired-body set. The solver and
# initialization bodies are dependencies, not claimed native implementations.
world.ENTRIES += (0x3218a0, 0x321a60, 0x2e0d30, 0x322ed0, 0x322d60)

if __name__ == '__main__':
    output = Path(sys.argv[sys.argv.index('--output') + 1])
    world.main()
    report = output / 'complete-report.json'
    result = json.loads(report.read_text())
    result.update(extension_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        extension_entries=[f'{p:08x}' for p in (0x3218a0, 0x321a60, 0x2e0d30, 0x322ed0, 0x322d60)],
        dependency_only_entries=['00322ed0', '00322d60'])
    report.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(extended_report_sha256=hashlib.sha256(report.read_bytes()).hexdigest())))
