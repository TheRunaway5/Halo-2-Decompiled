"""Stage complete authored alpha/environment metadata and original banks.

All88 authored rows,68 original counterparts and38 complete native template
variants are exercised. Programs and assets remain unchanged. Raw metadata
stays in the enclosing test owner; only the required compressed fixture lasts.
"""
import argparse
import json
import lzma
import os
from pathlib import Path

from export_mcc_native_env_illum_three_channel_fixture import EnvironmentExporter, digest
from export_mcc_native_world_animated_fixture import AnimatedExporter
from export_mcc_native_world_cooked_fixture import require, sha, tag_json
from mcc_alpha_environment_projection import project


class AlphaEnvironmentExporter(EnvironmentExporter):
    def fixture(self, row, ordinal, kind):
        self.active_native = (row['map'] if row['map'] in self.evidence['native_libraries']
                              else self.profile['native_records'][0]['map'])
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
        require(result['expected_overlays'] == row['source_overlays'] == [], 'animation requires a complete owner')
        if row['native_pair']:
            result['original_banks'] = {key: value['hex'] for key, value in row['native_banks'].items() if key not in ('4', '60')}
        return result

    def export(self):
        rows = self.evidence['materials']
        require(len(rows) == 88 and sum(row['native_pair'] for row in rows) == 68, 'complete source cohort differs')
        cases = [self.fixture(row, index, 'paired-original-main' if row['native_pair'] else 'authored-mcc-owner')
                 for index, row in enumerate(rows)]
        seen = {case['native_payload_name'] for case in cases}
        for record in self.profile['native_records']:
            if record['payload'] not in seen:
                self.forced_native = record['map']
                cases.append(self.fixture(rows[0], len(cases), 'native-template-variant'))
                seen.add(cases[-1]['native_payload_name'])
        self.forced_native = None
        require(len(seen) == 38, 'complete original variants are not exercised')
        pins = dict(self.pins, **self.shared_pins)
        require(all(digest(Path(path)) == pin for path, pin in pins.items()), 'source input changed during fixture export')
        return dict(schema='H2_MCC_NATIVE_ALPHA_ENVIRONMENT_FIXTURE_V1', cases=cases,
            source_map_sha256=pins, authored_rows=88, paired_original_rows=68, native_template_variants=38,
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
    profile = json.loads((root / 'tools/data/mcc-native-alpha-environment-profiles.json').read_bytes())
    packed = args.authority.read_bytes()
    require(sha(packed) == profile['authority']['sha256'], 'complete paired authority changed')
    exporter = AlphaEnvironmentExporter(root, prepared, json.loads(lzma.decompress(packed)), profile)
    try:
        fixture = exporter.export()
    finally:
        exporter.close()
    raw = (json.dumps(fixture, indent=2) + '\n').encode()
    require(len(raw) <= 32 * 1024 * 1024, 'literal fixture exceeds owned metadata budget')
    args.output.write_bytes(raw)
    print(json.dumps(dict(cases=len(fixture['cases']), paired_original_rows=68, native_template_variants=38,
                         literal_fixture_bytes=len(raw))))


if __name__ == '__main__':
    main()
