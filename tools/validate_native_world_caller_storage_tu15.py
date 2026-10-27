#!/usr/bin/env python3
"""Require the world binding regression to reject the old overlapping stack.

The private mutation changes only the transport call-frame offset. Original
physics fixtures and expected outcomes are unchanged. Run via run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import validate_native_world_integration_tu15 as validation

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve()
    args.output.mkdir(parents=True,exist_ok=True)
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    overlay=owner/'source';overlay.mkdir()
    for folder in ['include','tools']:(overlay/folder).symlink_to(ROOT/folder,target_is_directory=True)
    validator=ROOT/'tools/validate_native_world_integration_tu15.py'
    paths=set(re.findall(r'\{ROOT\}/((?:src|tests)/[^"\s]+\.cpp)',validator.read_text()))
    paths.add('src/physics/native_tu15_world_integration.cpp')
    paths.add('src/physics/native_tu15_constraint_solver_internal.hpp')
    relative='src/physics/native_tu15_world_integration_residuals.cpp'
    for path in paths:
        target=overlay/path;target.parent.mkdir(parents=True,exist_ok=True)
        if path!=relative:target.symlink_to(ROOT/path)
    original=ROOT/relative;before=sha(original);text=original.read_text()
    needle='const auto call_stack=std::uint32_t(end)-0x200u,'
    assert text.count(needle)==1,'Requalify caller packet layout'
    mutated=overlay/relative;mutated.write_text(text.replace(needle,'const auto call_stack=std::uint32_t(end)-0x40u,',1))
    old_root,old_file,old_argv=validation.ROOT,validation.__file__,sys.argv
    try:
        validation.ROOT=overlay;validation.__file__=str(overlay/'tools/validate_native_world_integration_tu15.py')
        sys.argv=[validation.__file__,'--output',str(args.output/'negative')]
        result=validation.main()
    finally:validation.ROOT,validation.__file__,sys.argv=old_root,old_file,old_argv
    log=(args.output/'negative/cmake/tests.log').read_text()
    reasons=['Actual constraint builder receiver/retained-packet ABI',
        'Broad-phase pair packets overlap callee frame/arguments']
    observed=[reason for reason in reasons if reason in log]
    rejected=result!=0 and bool(observed)
    report=dict(passed=rejected and sha(original)==before,negative_control_rejected=rejected,observed_rejection_reasons=observed,
        original_source_sha256=before,private_old_layout_sha256=sha(mutated),
        source_unchanged=sha(original)==before,validator_sha256=sha(validator),
        regression_sha256=sha(ROOT/'tests/native_tu15_world_residuals_tests.cpp'),
        mutation='Private call_stack offset200->40 overlaps retained caller builder packet',
        scratch_owner=str(owner),retained_large_artifacts=[])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
