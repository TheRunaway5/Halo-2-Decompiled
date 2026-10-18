#!/usr/bin/env python3
"""Run the native upload comparison with a fresh, retained result directory.

Use only inside the existing tag_match_memory_budget.py validation guard.
Missing owned original data/reference skips; no fixture or result is replaced.
"""
import argparse
import json
import subprocess
import uuid
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output-root', type=Path, required=True)
    parser.add_argument('--gp-context', type=Path)
    parser.add_argument('--return-reference', type=Path)
    args = parser.parse_args()
    if args.return_reference is not None and args.gp_context is None:
        parser.error('--return-reference requires an explicit --gp-context')
    if not args.xbe.is_file() or not (args.reference / 'case.json').is_file():
        return 77
    if args.gp_context is not None and not args.gp_context.is_file():
        return 77
    if args.return_reference is not None and not (args.return_reference / 'case.json').is_file():
        return 77
    args.output_root.mkdir(parents=True, exist_ok=True)
    output = args.output_root / str(uuid.uuid4())
    command = [str(args.native.resolve()), str(args.xbe.resolve()),
               str(output.resolve()), str(args.reference.resolve())]
    if args.gp_context is not None:
        command += ['--complete-gp' if args.return_reference is not None else '--consume-gp',
                    str(args.gp_context.resolve())]
    if args.return_reference is not None:
        command += ['--return-reference', str(args.return_reference.resolve())]
    result = subprocess.run(command)
    if result.returncode:
        return result.returncode
    report = json.loads((output / ('return-report.json' if args.return_reference is not None else 'report.json')).read_text())
    if report['passed'] is not True or report['supplied_request_acknowledgement'] is not False:
        return 1
    if args.gp_context is not None:
        gp = json.loads((output / 'gp-report.json').read_text())
        if gp['passed'] is not True or gp['supplied_request_acknowledgement'] is not False or gp['request_after'] != 0:
            return 1
    if args.return_reference is not None and (report['original_return_compared'] is not True or report['request_word'] != 0):
        return 1
    print('Native upload comparison evidence: ' + str(output), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
