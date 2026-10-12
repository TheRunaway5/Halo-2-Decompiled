#!/usr/bin/env python3
"""Export only the literal paired sound-startup operands from owned images."""
import argparse
import hashlib
import json
from pathlib import Path
from export_mp_unit_constructor_constants import section


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(authority, images, output):
    expected = 'dcfdb4ae7172b4ef3f746f4b186d0bbd363158235cb202a32ab2be8a93765aec'
    if sha(authority) != expected:
        raise ValueError('Paired sound-startup operand packet changed')
    packet = json.loads(authority.read_text())
    rows = []
    inputs = []
    for index, path in enumerate(images, 1):
        if sha(path) != packet['image_sha256'][str(path.resolve())]:
            raise ValueError('Owned executable changed')
        data = path.read_bytes()
        key = 'old_va' if index == 1 else 'u5_va'
        ambient = section(data, int(packet['ambient_transition_bits'][key], 16), 4)
        policy = section(data, int(packet['voice_policy'][key], 16), 16)
        if ambient.hex() != packet['ambient_transition_bits']['raw4_hex'] or policy.hex() != packet['voice_policy']['raw16_hex']:
            raise ValueError('Actual paired startup operands differ')
        rows.append(f'C {index} {ambient.hex()} {policy.hex()}')
        inputs.append(dict(path=str(path), sha256=sha(path)))
    output.mkdir(parents=True, exist_ok=True)
    fixture = output / 'constants.tsv'
    fixture.write_text('\n'.join(rows) + '\n')
    report = dict(schema='source-sound-startup-constant-provider-v1',
                  authority=dict(path=str(authority), sha256=expected), inputs=inputs,
                  fixture_sha256=sha(fixture), tool_sha256=sha(Path(__file__)),
                  xbe_reader_sha256=sha(Path(__file__).with_name('export_mp_unit_constructor_constants.py')),
                  supported_images=2, exact_bytes=40, mutable_state_or_platform_admission=False)
    (output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(fixture_sha256=sha(fixture), report_sha256=sha(output / 'report.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority', type=Path, required=True)
    parser.add_argument('--image-11', type=Path, required=True)
    parser.add_argument('--image-15', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    export(args.authority, [args.image_11, args.image_15], args.output)
