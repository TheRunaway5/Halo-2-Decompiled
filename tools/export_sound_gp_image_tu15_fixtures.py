#!/usr/bin/env python3
"""Retain unchanged original decoder output and an authenticated live GP upload.

Run serially under tag_match_memory_budget.py. The native image loader reads
only the owned executable; these reference captures are test inputs, never
runtime state or supplied device results.
"""
import argparse
import hashlib
import json
from pathlib import Path

from audit_uploaded_sound_dsp_tu15 import audit, PIN, ROOT


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True,
                        help='Fresh output of oracle_sound_dsp_image_tu15.py')
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--capture-decoder-reference', type=Path, required=True,
                        help='Decoder fixtures pinned by the live observer report')
    parser.add_argument('--observer', type=Path,
                        default=ROOT / 'tools/capture_live_sound_dsp_tu15.py')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    witness = audit(args.capture, args.capture_decoder_reference, args.observer)
    original_report = (args.original / 'report.json').read_bytes()
    original = json.loads(original_report)
    assert original['xbe_sha256'] == PIN
    assert original['instruction_patches'] == original['supplied_external_results'] == []
    assert original['external_targets'] == []
    assert len(original['effects']) == 15 and len(original['cases']) == 18
    resource = (args.original / 'decoded-resource.bin').read_bytes()
    assert sha(resource) == original['decoded_resource_sha256']
    assert original['dsp_section_sha256'] == json.loads(
        (args.capture_decoder_reference / 'report.json').read_text())['dsp_section_sha256']
    for row in original['effects']:
        effect = (args.original / f"effect-{row['index']:02d}.bin").read_bytes()
        assert sha(effect) == row['sha256'] and len(effect) == row['bytes']
        assert resource[row['section_offset']:row['section_offset'] + len(effect)] == effect
        assert effect == (args.capture / f"effect-{row['index']:02d}-program.bin").read_bytes()
    uploaded = (args.capture / 'gp_p_loaded.bin').read_bytes()
    report = dict(scope=__doc__, xbe_sha256=PIN,
                  original_report_sha256=sha(original_report),
                  original_decoder_tool_sha256=sha((ROOT / 'tools/oracle_sound_dsp_image_tu15.py').read_bytes()),
                  decoded_resource_sha256=sha(resource), gp_upload_sha256=sha(uploaded),
                  decoded_resource_bytes=len(resource), gp_upload_bytes=len(uploaded),
                  upload_audit=witness, full_engine_verified=False)
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'decoded-resource.bin').write_bytes(resource)
    (args.output / 'gp-upload.bin').write_bytes(uploaded)
    (args.output / 'reference.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS original GP image references: {len(resource)} decoded resource bytes and {len(uploaded)} live uploaded program bytes')


if __name__ == '__main__':
    main()
