#!/usr/bin/env python3
"""Genuine moving-contact proof compiled against the shared producer/query union."""
import hashlib,json,sys
from pathlib import Path
import validate_native_moving_shape_observed_actual_tu15 as observed
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 index=sys.argv.index('--source-union');stage=Path(sys.argv[index+1]).resolve();del sys.argv[index:index+2]
 output=Path(sys.argv[sys.argv.index('--output')+1]);manifest=stage/'source-pins.json';pin=sha(manifest);data=json.loads(manifest.read_text())
 assert all(sha(stage/'staged'/n)==row['sha256']for n,row in data['files'].items())
 observed.BASE=observed.INTERFACE=observed.WRITERS=stage/'staged'
 observed.parent.full.EXTRA=(*observed.parent.full.EXTRA,'native_tu15_closest_segments.cpp','native_tu15_simplex_triangle.cpp')
 tools=(Path(__file__),Path(observed.__file__),Path(observed.parent.__file__))
 tool_pins={str(p.relative_to(ROOT)):sha(p)for p in tools}
 rc=observed.main()
 report=output/'report.json';r=json.loads(report.read_text())
 # The inherited c58 checkpoint is a provenance reference. The actual shared
 # solver includes independently qualified query continuations; do not label
 # its entire arithmetic as unchanged c58.
 r['inherited_solver_reference_sha256']=r.pop('solver_arithmetic_baseline_sha256')
 r['inherited_solver_reference']=r.pop('solver_arithmetic_baseline')
 r['compiled_shared_solver_cpp_sha256']=sha(stage/'staged/src/physics/native_tu15_convex_simplex_solver.cpp')
 r['shared_union_manifest_sha256']=pin;r['shared_union_tool_pins']=tool_pins
 r['shared_union_inputs_unchanged']=sha(manifest)==pin and all(sha(stage/'staged'/n)==row['sha256']for n,row in data['files'].items())and all(sha(ROOT/n)==v for n,v in tool_pins.items())
 if not r['shared_union_inputs_unchanged']:r['passed']=False;rc=1
 r['scope']='Selected genuine2E5320 same typed retained-observer production helper on the exact shared candidate/query/producer source union. Full retained banks/FP,64 SourcePC/preimage/value/order events and538 semantic reads; no Source execution services. Wider domains and World/live remain separate.'
 report.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
