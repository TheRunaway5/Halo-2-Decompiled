"""Stage literal tiling-specular inputs and unchanged original packing results.

All14 authored materials and both original counterparts/template variants are
retained as metadata. Source assets and executable/shader programs are unchanged.
The raw fixture remains in its enclosing test owner.
"""
import argparse
import json
import lzma
import os
from pathlib import Path
import subprocess
import sys

from export_mcc_native_env_illum_three_channel_fixture import EnvironmentExporter, digest
from export_mcc_native_world_animated_fixture import AnimatedExporter
from export_mcc_native_world_cooked_fixture import require, sha, tag_json
from mcc_tiling_specular_projection import project


class TilingExporter(EnvironmentExporter):
    def fixture(self, row, ordinal, kind):
        self.active_native = row['map'] if row['map'] in self.evidence['native_libraries'] else '05b_deltatowers'
        result = AnimatedExporter.fixture(self, row, ordinal, kind)
        if result['shader']['size'] == 0:
            shader = self.cache.tags[row['datum']]
            owners = [value for value in self.cache.tags.values() if value['address'] == shader['address'] and
                      value['group'] == shader['group'] and value['size']]
            require(len(owners) == 1 and owners[0]['size'] >= 84, 'shader alias lacks its exact typed allocation')
            owner = owners[0]
            result['namespace'].append(tag_json(owner))
            at = self.cache.pointer(owner['address'], self.cache.magic, owner['size'])
            result['regions'].append([owner['address'], bytes(self.cache.data[at:at + owner['size']]).hex()])
            result['source_alias_owner'] = tag_json(owner)
        expected = project(row, self.profile['source_graph']['definitions'][0]['arrays'],
                           self.profile['native_arrays'], self.profile['roles'])
        result['expected_banks'] = {key: value.hex() for key, value in expected.items()}
        require(result['expected_overlays'] == row['source_overlays'] == [], 'authored animation requires additional proof')
        if row['native_pair']:
            result['original_banks'] = {key: value['hex'] for key, value in row['native_banks'].items() if key not in ('4', '60')}
        return result

    def export(self):
        rows = self.evidence['materials']
        require(len(rows) == 14 and sum(row['native_pair'] for row in rows) == 2, 'complete source cohort differs')
        cases = [self.fixture(row, index, 'paired-original-main' if row['native_pair'] else 'authored-mcc-owner')
                 for index, row in enumerate(rows)]
        require(len({case['native_payload_name'] for case in cases}) == 2, 'complete native variants are not exercised')
        pins = dict(self.pins, **self.shared_pins)
        require(all(digest(Path(path)) == pin for path, pin in pins.items()), 'source input changed during fixture export')
        return dict(schema='H2_MCC_NATIVE_TILING_SPECULAR_FIXTURE_V1', cases=cases,
            source_map_sha256=pins, authored_rows=14, paired_original_rows=2, native_template_variants=2,
            native_allocations_explicitly_translated=True, native_projection_admitted=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--prepared-root', type=Path, required=True)
    parser.add_argument('--authority', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, prepared = args.repo_root.resolve(strict=True), args.prepared_root.resolve(strict=True)
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    require(args.output.resolve().is_relative_to(owner), 'literal fixture escaped its enclosing owner')
    profile = json.loads((root / 'tools/data/mcc-native-tiling-specular-profiles.json').read_bytes())
    packed = args.authority.read_bytes()
    require(sha(packed) == profile['authority']['sha256'], 'complete paired authority changed')
    exporter = TilingExporter(root, prepared, json.loads(lzma.decompress(packed)), profile)
    try:
        fixture = exporter.export()
    finally:
        exporter.close()
    packing = owner / 'tiling-packing-cases.json'
    oracle_report = owner / 'tiling-packing-original.json'
    subprocess.run([str(root / '.tools/python/bin/python3'), '-B', str(root / 'tools/audit_shader_color_packing_tu15.py'),
        '--authority-directory', str(args.authority.resolve(strict=True).parent), '--fixture-output', str(packing),
        '--report', str(oracle_report)], check=True)
    original = json.loads(oracle_report.read_bytes())
    require(original['passed'] and original['original_comparisons'] == 14756 and original['cases'] == 3689,
            'complete unchanged original packing cohort differs')
    fixture['packing'] = json.loads(packing.read_bytes())
    fixture['packing_original_report_sha256'] = sha(oracle_report.read_bytes())
    raw = (json.dumps(fixture, indent=2) + '\n').encode()
    require(len(raw) <= 32 * 1024 * 1024, 'literal metadata fixture exceeds owned budget')
    args.output.write_bytes(raw)
    print(json.dumps(dict(cases=14, paired_original_rows=2, native_template_variants=2,
                         original_packing_cases=3689, literal_fixture_bytes=len(raw))))


if __name__ == '__main__':
    main()
