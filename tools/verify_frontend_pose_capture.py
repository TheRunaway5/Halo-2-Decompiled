#!/usr/bin/env python3
"""Run the independent native menu pose comparator and record exact provenance."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--fixture", type=Path, required=True)
parser.add_argument("--report", type=Path, required=True)
parser.add_argument("--binary", type=Path, default=ROOT/"build-frontend/frontend_pose_capture_tests")
parser.add_argument("--maps", type=Path, default=ROOT/"assets/disc/maps")
parser.add_argument('--stages', type=Path)
parser.add_argument('--shared-pose',action='store_true')
args = parser.parse_args()
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
before = digest(args.binary)
command = [str(args.binary.resolve()), str(args.fixture.resolve()),
           str(args.maps.resolve()), str(args.report.resolve())]
if args.stages:command.append(str(args.stages.resolve()))
if args.shared_pose:
    assert args.stages,'Shared pose requires captured stages/control slot'
    command.append('--shared')
result = subprocess.run(command,
                        text=True, capture_output=True, cwd=ROOT)
print(result.stdout, end="")
print(result.stderr, end="")
assert before == digest(args.binary), "comparison executable changed during execution"
if result.returncode not in (0, 2):
    raise SystemExit(result.returncode)
report = json.loads(args.report.read_text())
provenance = json.loads(args.fixture.with_suffix(".provenance.json").read_text())
assert provenance["fixture_sha256"] == digest(args.fixture), "fixture differs from capture provenance"
report.update(comparison_binary_sha256=before, fixture_sha256=digest(args.fixture),
              captured_provenance=provenance, native_exit_code=result.returncode,
              node_matrices_verified_byte_exact=report["byte_exact"],
              limitations=provenance["limits"])
if args.stages:
    assert provenance['stage_fixture_sha256'] == digest(args.stages), 'stage fixture differs from original capture'
    report.update(stage_fixture_sha256=digest(args.stages),
        native_pose_stage=('Production composite primary/initial world IK/deferred hierarchy helper, with actual captured frame/root/slot0, literal source primary/grip inputs, and retained weapon local fields initialized from source-selected startup idle at frame0.' if args.shared_pose else
            'Literal primary+grip with captured deferred flag and actual model mask; unit world IK and rendered interpolation remain independent.'))
args.report.write_text(json.dumps(report, indent=2)+"\n")
print(f"Report: {args.report}; exact={report['byte_exact']}")
raise SystemExit(result.returncode)
