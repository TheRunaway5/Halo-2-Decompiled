#!/usr/bin/env python3
"""Export literal actual PC/default-bitmap roles and original paired controls."""
import argparse
import json
import lzma
import os
from pathlib import Path
import struct
from export_mcc_native_world_cooked_fixture import Exporter, NATIVE, require, tag_json

NATIVE.update(tex_bump_three_detail_blend='03a_oldmombasa', tex_env='deltatap')
RULES = {'tex_bump_three_detail_blend': (3783344756, 6, 1064697328),
         'tex_env': (3796976452, 4, 1069055604)}
UV = bytes.fromhex('0000803f0000803f0000000000000000')


def mapped(row):
    family = row['template'].rsplit('\\', 1)[-1]
    datum, slot, log = RULES[family]
    images = list(row['source_images'])
    name = 'default_detail' if family == 'tex_bump_three_detail_blend' else 'color_black_alpha_black'
    images.insert(slot, [['bitm', 'shaders\\default_bitmaps\\bitmaps\\' + name], 0, log])
    pixels = bytes.fromhex(row['source_banks']['12']['hex'])
    raw = bytes.fromhex(row['source_banks']['20']['hex'])
    vectors = [raw[i:i + 16] for i in range(0, len(raw), 16)]
    if family == 'tex_env':
        pixels = pixels[:20]
    else:
        vectors = [vectors[i] for i in (1, 2, 3, 4, 0, 7, 8, 5, 6)]
    return images, pixels, b''.join(vectors) + UV


class DefaultRolesExporter(Exporter):
    def fixture(self, row, ordinal, kind):
        out = super().fixture(row, ordinal, kind)
        family = row['template'].rsplit('\\', 1)[-1]
        _, slot, log = RULES[family]
        cache = self.select(row['map'])
        name = 'shaders\\default_bitmaps\\bitmaps\\' + ('default_detail' if family == 'tex_bump_three_detail_blend' else 'color_black_alpha_black')
        selected = [x for x in cache.tags.values() if x['group'] == 'bitm' and x['name'] == name]
        require(len(selected) == 1, 'source default role has another typed namespace')
        datum = selected[0]['datum'];owner, tag = self.resolve(cache, datum)
        require(tag['offset'] is not None and tag['group'] == 'bitm' and tag['size'] >= 76,
                'actual shared default bitmap is nonresident')
        count = len(owner.block(tag['offset'] + 68, 168))
        require(count == 1, 'actual default bitmap is not its single-frame owner')
        names = {t['datum']: t for t in out['namespace']}
        require(datum not in names or names[datum] == tag_json(tag), 'shared default bitmap identity conflicts')
        names[datum] = tag_json(tag)
        out['namespace'] = list(names.values());out['bitmap_frame_counts'][str(datum)] = count
        images = bytes.fromhex(out['expected_banks']['4'])
        out['expected_banks']['4'] = (images[:slot * 12] + struct.pack('<III', datum, 0, log) + images[slot * 12:]).hex()
        _, pixels, vectors = mapped(row)
        out['expected_banks']['12'] = pixels.hex();out['expected_banks']['20'] = vectors.hex()
        out['default_bitmap'] = tag_json(tag)
        resources = json.loads((self.root / 'analysis/native-renderer-all-20261025/pc-default-role-bitmap-authority.json').read_bytes())
        resource = next(x for x in resources if x['mcc'] and x['map'] == row['map'] and x['name'] == name)
        require(resource['datum'] == datum and 'error' not in resource, 'actual prepared default bitmap resource owner differs')
        out['default_bitmap_resource'] = resource['texture']
        return out

    def export(self):
        rows = [r for r in self.evidence['materials'] if r['template'].rsplit('\\', 1)[-1] in RULES]
        pc = [r for r in rows if r['map'] in ('needle', 'street_sweeper')]
        require(len(pc) == 3, 'complete default-role owner census changed')
        cases = [self.fixture(row, i, 'pc-geometry-owner') for i, row in enumerate(pc)]
        for family in RULES:
            candidates = []
            for row in rows:
                if not row['native_pair'] or not row['template'].endswith('\\' + family):
                    continue
                images, pixels, vectors = mapped(row)
                if (images != row['native_images'] or pixels.hex() != row['native_banks']['12']['hex'] or
                        vectors.hex() != row['native_banks']['20']['hex']):
                    self.exclusions.append({'map': row['map'], 'shader': row['shader'],
                        'reason': 'Literal authored main bank differs from original; no source data fitted'})
                    continue
                candidates.append(row)
            candidates.sort(key=lambda x: (x['map'] != NATIVE[family], x['map'], x['shader']))
            require(len(candidates) >= 2, 'two complete original controls unavailable')
            for row in candidates[:2]:
                cases.append(self.fixture(row, len(cases), 'paired-original-main'))
        return dict(schema='H2_MCC_NATIVE_WORLD_DEFAULT_ROLES_FIXTURE_V1',
            scope='Literal original default role data component; actual shared bitmap owners; no live/GPU claim',
            cases=cases, map_sha256=self.pins, paired_exclusions=self.exclusions, expected_pc_geometry_owners=3)


def main():
    p = argparse.ArgumentParser(description=__doc__);p.add_argument('--repo-root', type=Path, required=True)
    p.add_argument('--prepared-root', type=Path, required=True);p.add_argument('--evidence', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True);a = p.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);output = a.output.resolve()
    require(output.is_relative_to(owner), 'raw fixture must remain inside owned scratch')
    e = DefaultRolesExporter(a.repo_root.resolve(strict=True), a.prepared_root.resolve(strict=True),
        json.loads(lzma.decompress(a.evidence.read_bytes())))
    try:
        result = e.export()
    finally:
        e.close()
    output.write_text(json.dumps(result, indent=2) + '\n')
    print('Literal default-role fixture:3PC owners,4original paired controls')


if __name__ == '__main__':
    main()
