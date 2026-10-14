#!/usr/bin/env python3
"""Guard one real tag-backed campaign world with the shared 64 MiB budget.

The native test starts authored scripts and submits ordinary player Controls.
Rendering, audio playback and original-console gameplay parity are separate.
Use --require-completion to reject a run that ends at its step budget.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time
from tag_match_memory_budget import enter_memory_budget


def digest(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'map', 'shared', 'xbe', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--ticks', type=int, default=300)
    parser.add_argument('--timeout', type=float, default=120)
    parser.add_argument('--require-completion', action='store_true')
    parser.add_argument('--no-skip', action='store_true')
    parser.add_argument('--route', help='Comma-separated authored trigger volume names')
    parser.add_argument('--route-after', help='Wait for this authored device to open before following the route')
    args = parser.parse_args()
    if not 1 <= args.ticks <= 100000 or not 10 <= args.timeout <= 600:
        parser.error('Use 1..100000 steps and a 10..600 second timeout')
    if args.route_after and not args.route:
        parser.error('--route-after requires --route')
    for path in (args.native, args.map, args.xbe):
        if not path.is_file():
            parser.error('Missing input file: ' + str(path))
    if not args.shared.is_dir():
        parser.error('Missing shared cache directory')
    contained = enter_memory_budget(args.output, args.timeout)
    if contained is not None:
        return contained
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    command = [str(args.native.resolve()), str(args.map.resolve()),
               str(args.shared.resolve()), str(args.xbe.resolve()), str(args.ticks), str(output)]
    for option in ('require_completion', 'no_skip'):
        if getattr(args, option):
            command.append('--' + option.replace('_', '-'))
    for option in ('route', 'route_after'):
        if getattr(args, option):
            command.append('--' + option.replace('_', '-') + '=' + getattr(args, option))
    inputs = {name: digest(getattr(args, name)) for name in ('native', 'map', 'xbe')}
    started = time.monotonic()
    errors = []
    status = None
    with (output / 'campaign.log').open('w') as log:
        try:
            status = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                    timeout=args.timeout).returncode
        except (OSError, subprocess.TimeoutExpired) as error:
            errors.append(str(error))
    # Original compiled print strings can contain legacy Windows bytes.
    # Keep the raw log and replace undecodable bytes only in JSON diagnostics.
    text = (output / 'campaign.log').read_text(encoding='utf-8', errors='replace')
    passed = re.search(r'PASS bounded campaign simulation: (.*)', text)
    measurements = dict((key, int(value)) for key, value in
                        re.findall(r'(?:^|\s)(\w+)=(\d+)(?=\s|$)', passed.group(1))) if passed else {}
    if status != 0:
        errors.append('Campaign simulation exited with status ' + str(status))
    if not passed:
        errors.append('Strict campaign simulation checks did not pass')
    if args.require_completion and measurements.get('level_finished') != 1:
        errors.append('Authored level completion was not reached')
    if any(digest(getattr(args, name)) != value for name, value in inputs.items()):
        errors.append('A campaign input changed during the run')
    report = dict(passed=not errors, scope=__doc__, command=command,
                  input_sha256=inputs, measurements=measurements, errors=errors,
                  native_status=passed.group(1) if passed else None,
                  script_faults=[line for line in text.splitlines() if line.startswith('Script fault:')],
                  elapsed_seconds=round(time.monotonic() - started, 3),
                  completion_required=args.require_completion, cinematic_skip_allowed=not args.no_skip,
                  rendering_tested=False, audio_playback_tested=False,
                  original_gameplay_parity_tested=False)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2), flush=True)
    return int(bool(errors))


if __name__ == '__main__':
    raise SystemExit(main())
