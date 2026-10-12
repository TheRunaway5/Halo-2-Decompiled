#!/usr/bin/env python3
"""Export actual paired cache constructor callback immediates from owned images."""
import argparse
import hashlib
import json
from pathlib import Path
from export_mp_unit_constructor_constants import section


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(authority, images, output):
    expected = '648d78324c1c0863b6fc22116834863583dc3db417861a352cf981ed327462dd'
    if sha(authority) != expected:
        raise ValueError('Paired sound cache callback packet changed')
    packet = json.loads(authority.read_text())
    constructor_addresses = [(0x21a736, 0x21a731), (0x21f036, 0x21f031)]
    rows, inputs = [], []
    for index, path in enumerate(images):
        if sha(path) != packet['image_sha256'][str(path.resolve())]:
            raise ValueError('Owned executable changed')
        data = path.read_bytes()
        key = 'old' if index == 0 else 'u5'
        words = []
        for name, address in zip(['delete_callback', 'busy_callback'], constructor_addresses[index]):
            entry = packet['callbacks'][name]
            instruction = section(data, address, 5)
            if instruction.hex() != entry[key+'_instruction_hex'] or instruction[0] != 0x68:
                raise ValueError('Actual complete-constructor callback immediate differs')
            identity = int.from_bytes(instruction[1:], 'little')
            if identity != int(entry[key+'_identity'], 16) or not entry['complete_target_body_verified']:
                raise ValueError('Paired callback identity/body coverage differs')
            section(data, identity, 1)
            words.append(instruction[1:].hex())
        rows.append(f'C {index+1} ' + ' '.join(words))
        inputs.append(dict(path=str(path), sha256=sha(path)))
    output.mkdir(parents=True, exist_ok=True)
    fixture = output/'constants.tsv'
    fixture.write_text('\n'.join(rows) + '\n')
    report = dict(schema='source-sound-cache-constructor-constant-provider-v1',
                  authority=dict(path=str(authority), sha256=expected), inputs=inputs,
                  fixture_sha256=sha(fixture), tool_sha256=sha(Path(__file__)),
                  xbe_reader_sha256=sha(Path(__file__).with_name('export_mp_unit_constructor_constants.py')),
                  supported_images=2, exact_operand_bytes=16, exact_instruction_bytes=20,
                  runtime_callback_success_or_cache_admission=False)
    (output/'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(fixture_sha256=sha(fixture), report_sha256=sha(output/'report.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority', type=Path, required=True)
    parser.add_argument('--image-11', type=Path, required=True)
    parser.add_argument('--image-15', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    export(args.authority, [args.image_11, args.image_15], args.output)
