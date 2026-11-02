#!/usr/bin/env python3
"""Read literal Workshop/stock sky data and independently retained TU1.5 pairs.

Run through run_test.py. Raw source regions and generated fixture/policy stay
inside its owner. No map, material instance or shader program is changed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from export_mcc_native_world_animated_fixture import AnimatedExporter
from export_mcc_native_world_cooked_fixture import FIELDS, canonical, require, sha, u32
from mcc_content import MccCache
from preserve_native_shaders import read_selected_library

TEMPLATE_FIELDS = ((0, 10), (8, 2), (16, 10), (24, 6), (32, 4))
NATIVE = {'sky_two_alpha_clouds': '03b_newmombasa',
          'sky_one_alpha_env': '03b_newmombasa',
          'sky_one_alpha_env_illum': '06a_sentinelwalls'}
PREFIX = 'shaders\\shader_templates\\transparent\\'


def graph(cache, stem):
    raw = bytearray(cache.data[stem['offset']:stem['offset'] + 96])
    require(len(raw) == 96, 'incomplete literal MCC sky template')
    definitions = cache.block(stem['offset'] + 88, 40)
    require(len(definitions) == 3, 'MCC sky template platform count changed')
    struct.pack_into('<I', raw, 92, 0)
    result = dict(root_hex=raw.hex(), definitions=[])
    for at in definitions:
        root = bytearray(cache.data[at:at + 40]);arrays = []
        for field, stride in TEMPLATE_FIELDS:
            entries = cache.block(at + field, stride)
            values = bytearray(cache.data[entries[0]:entries[0] + len(entries)*stride]) if entries else bytearray()
            refs = []
            if field == 16:
                for i in range(len(entries)):
                    tag = cache.tags[u32(values, i*10 + 4)]
                    require(tag['group'] == 'spas', 'sky template reference is not a shader pass')
                    refs.append([tag['group'], tag['name']])
                    struct.pack_into('<I', values, i*10 + 4, 0)
            arrays.append(dict(field=field, count=len(entries), raw_hex=values.hex(), typed_references=refs))
            struct.pack_into('<I', root, field + 4, 0)
        result['definitions'].append(dict(definition_hex=root.hex(), arrays=arrays))
    return result


class SkyExporter(AnimatedExporter):
    def __init__(self, root, prepared, sources):
        self.sources = sources
        super().__init__(root, prepared, dict(map_sha256={name: sha(path.read_bytes()) for name, path in sources.items()}))
        self.libraries = {}

    def select(self, name):
        if self.cache and self.cache.path == self.sources[name]:
            return self.cache
        if self.cache:
            self.cache.close()
        path = self.sources[name]
        require(sha(path.read_bytes()) == self.evidence['map_sha256'][name], 'literal sky input changed')
        self.cache = MccCache(path);self.pins[name] = self.evidence['map_sha256'][name]
        return self.cache

    def library(self, name):
        if name not in self.libraries:
            directory = self.prepared/'native-shaders'/name
            library, _ = read_selected_library(directory, name)
            require(sha(canonical(library)) == self.authorities[name], 'retained original sky bank identity changed')
            self.libraries[name] = library
        return self.libraries[name]

    def native_template(self, template):
        if template not in self.native:
            name = NATIVE[template.rsplit('\\', 1)[-1]]
            records = [r for r in self.library(name)['shader_tags'] if r['group'] == 'stem' and r['name'] == template]
            require(len(records) == 1, 'ambiguous retained original sky template')
            record = records[0]
            payload = (self.prepared/'native-shaders'/name/'payloads'/record['payload']).read_bytes()
            require(len(payload) == record['bytes'] and sha(payload)+'.bin' == record['payload'], 'original sky payload changed')
            self.native[template] = name, record, payload
        return self.native[template]

    def native_graph(self, name, template):
        records=[r for r in self.library(name)['shader_tags'] if r['group']=='stem' and r['name']==template]
        require(len(records)==1,'original pair lost its native template owner')
        record=records[0];raw=(self.prepared/'native-shaders'/name/'payloads'/record['payload']).read_bytes()
        require(sha(raw)+'.bin'==record['payload'] and u32(raw,88)==1,'complete original template payload differs')
        refs=dict(record['references']);pp=u32(raw,92);arrays=[]
        for field,stride in TEMPLATE_FIELDS:
            count,pointer=u32(raw,pp+field),u32(raw,pp+field+4)
            require(pointer+count*stride<=len(raw),'original template bank escapes its payload')
            bank=bytearray(raw[pointer:pointer+count*stride]);references=[]
            if field==16:
                for i in range(count):
                    references.append(refs[pointer+i*stride+4]);struct.pack_into('<I',bank,i*stride+4,0)
            arrays.append(dict(field=field,count=count,hex=bank.hex(),references=references))
        return record['payload'],arrays

    def row(self, name, shader_name, paired):
        cache = self.select(name)
        shaders = [t for t in cache.tags.values() if t['group'] == 'shad' and t['name'] == shader_name]
        require(len(shaders) == 1, 'literal sky material is absent or ambiguous')
        shader = shaders[0];at = shader['offset'];pp = cache.block(at+32, 124)[0]
        owner, stem = self.resolve(cache, cache.u32(pp))
        require(stem['name'].startswith(PREFIX) and stem['name'][len(PREFIX):] in NATIVE, 'unreviewed sky template family')
        banks = {}
        for field, stride in FIELDS:
            entries = cache.block(pp + field, stride)
            banks[str(field)] = dict(count=len(entries), stride=stride,
                hex=bytes(cache.data[entries[0]:entries[0]+len(entries)*stride]).hex() if entries else '')
        images = []
        for image in cache.block(pp+4, 12):
            _, bitmap = self.resolve(cache, cache.u32(image))
            images.append([[bitmap['group'], bitmap['name']], cache.u32(image+4), cache.u32(image+8)])
        source_graph = graph(owner, stem)
        row = dict(map=name, shader=shader_name, datum=shader['datum'], template=stem['name'],
            source_banks=banks, source_images=images, source_graph=source_graph,
            source_stem_graph_sha256=sha(canonical(source_graph)), native_pair=paired,
            shader_type_flags=cache.u32(at+20))
        if paired:
            records = [r for r in self.library(name)['shader_tags'] if r['group'] == 'shad' and r['name'] == shader_name]
            require(len(records) == 1, 'independent original sky material is absent')
            record = records[0]
            raw = (self.prepared/'native-shaders'/name/'payloads'/record['payload']).read_bytes()
            require(len(raw) == record['bytes'] and sha(raw)+'.bin' == record['payload'], 'original material changed')
            original_pp = u32(raw, 36);refs = dict(record['references']);native_banks = {}
            for field, stride in FIELDS:
                count, pointer = u32(raw, original_pp+field), u32(raw, original_pp+field+4)
                require(pointer+count*stride <= len(raw), 'original sky bank escapes its complete payload')
                native_banks[str(field)] = dict(count=count, hex=raw[pointer:pointer+count*stride].hex())
            native_images = []
            for i in range(native_banks['4']['count']):
                offset = u32(raw, original_pp+8)+i*12
                native_images.append([refs[offset], u32(raw, offset+4), u32(raw, offset+8)])
            pixels = bytes.fromhex(banks['12']['hex'])
            if stem['name'].endswith(('\\sky_one_alpha_env', '\\sky_one_alpha_env_illum')):
                pixels = pixels[:4]+pixels[8:12]+pixels[4:8]+pixels[12:]
            require(native_images == images and pixels.hex() == native_banks['12']['hex'] and
                    banks['20']['hex'] == native_banks['20']['hex'], 'literal sky main-bank mapping differs from original')
            for field in (28, 36, 44, 52, 68, 76, 84, 92, 100, 108):
                require(banks[str(field)]['hex'] == native_banks[str(field)]['hex'], 'original sky role bank differs')
            curves = []
            for overlay in range(native_banks['60']['count']):
                offset = u32(raw, original_pp+64)+overlay*20
                n, pointer = u32(raw, offset+12), u32(raw, offset+16)
                curves.append(dict(input=u32(raw, offset), range=u32(raw, offset+4),
                    period_word=u32(raw, offset+8), curve_hex=raw[pointer:pointer+n].hex()))
            row.update(native_banks=native_banks, native_images=native_images,
                native_shader_payload=record['payload'], original_overlays=curves)
            selected=NATIVE[stem['name'].rsplit('\\',1)[-1]]
            old_payload,original_graph=self.native_graph(name,stem['name'])
            admitted_payload,admitted_graph=self.native_graph(selected,stem['name'])
            require(original_graph==admitted_graph,'selected native sky template changes complete postprocess remap semantics')
            row['native_template_pair']=dict(original_payload=old_payload,admitted_payload=admitted_payload,
                complete_postprocess_equal=True,semantic_sha256=sha(canonical(original_graph)))
        return row

    def fixture(self, row, ordinal, kind):
        result = super().fixture(row, ordinal, kind)
        # Sky material LOD availability is authored zero in both formats;
        # template availability is a distinct bank and must not replace it.
        result['expected_banks']['28']=row['source_banks']['28']['hex']
        if row['template'].endswith(('\\sky_one_alpha_env', '\\sky_one_alpha_env_illum')):
            raw = bytes.fromhex(result['expected_banks']['12'])
            result['expected_banks']['12'] = (raw[:4]+raw[8:12]+raw[4:8]+raw[12:]).hex()
        if row['native_pair']:
            require(result['expected_overlays'] == row['original_overlays'], 'literal sky curves differ from original authority')
        return result


def export(root, prepared, item, curve_proof):
    proof_raw=curve_proof.read_bytes();proof=json.loads(proof_raw)
    require(proof['xbe_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd' and
            proof['cases']==432 and proof['curves']==8 and proof['complete_original_body_correspondence'] and
            proof['encoded_lookup_rows_exact'] and not proof['instruction_patches'] and not proof['intercepted_calls'],
            'authored sky curves lack complete unchanged TU1.5 execution proof')
    curve_records={r['map']:r['overlays'] for r in proof['records']}
    sources = {'03a_oldmombasa': prepared/'halo2/h2_maps_win64_dx11/03a_oldmombasa.map',
               'zanzibar': prepared/'halo2/h2_maps_win64_dx11/zanzibar.map', 'alphamoon': item/'maps/alphamoon.map'}
    exporter = SkyExporter(root, prepared, sources)
    try:
        names = [('03a_oldmombasa', 'scenarios\\skies\\solo\\earthcity\\shaders\\ec_sky_dome2', True),
                 ('03a_oldmombasa', 'scenarios\\skies\\solo\\earthcity\\shaders\\clouds_low2', True),
                 ('03a_oldmombasa', 'scenarios\\skies\\solo\\earthcity\\shaders\\ec_sky_tether_faded', True),
                 ('zanzibar', 'scenarios\\skies\\multi\\zanzibar\\shaders\\moon', True)]
        names += [('alphamoon', 'digsite\\scenarios\\skies\\solo\\alphamoon\\shaders\\'+n, False)
                  for n in ('sky_dome', 'moon_clouds', 'ring_shard')]
        rows = [exporter.row(*x) for x in names]
        cases = [exporter.fixture(row, i, 'paired-original-main' if row['native_pair'] else 'pc-geometry-owner')
                 for i, row in enumerate(rows)]
        require(cases[1]['expected_overlays']==curve_records['old'] and
                cases[5]['expected_overlays']==curve_records['alpha'], 'literal sky curves differ from their original execution inputs')
        profiles = []
        for row, case in zip(rows[:4], cases[:4]):
            native_map, record, raw = exporter.native_template(row['template']);pp = u32(raw, 92)
            overlays = [dict(input=x['input'], range=x['range'], period_words=[x['period_word']], curve_hex=x['curve_hex'])
                        for x in case['expected_overlays']]
            profile = dict(template=row['template'], native_source_map=native_map, native_record=record,
                source_graph=row['source_graph'], source_graph_sha256=row['source_stem_graph_sha256'],
                source_counts={field:b['count'] for field,b in row['source_banks'].items()},
                shader_type_flags=row['shader_type_flags'], runtime_diffuse_slot=-1,
                native_shape={str(f):u32(raw, pp+f) for f,_ in TEMPLATE_FIELDS},
                clock_overlays=overlays, native_role_banks={str(f):row['source_banks'][str(f)]['hex'] for f in (28,36,44,52,68,76,84,92)},
                mapping='swap_environment_pixels' if 'one_alpha_env' in row['template'] else 'copy',
                paired_proof=dict(original_map=row['map'], original_shader=row['shader'],
                    material_payload=row['native_shader_payload'], complete_main_banks=True, complete_roles_and_curves=True,
                    native_template_pair=row['native_template_pair']))
            for actual_row, actual_case in zip(rows[4:], cases[4:]):
                if (actual_row['template'], actual_row['source_banks']['60']['count']) != (row['template'], len(overlays)):
                    continue
                require(actual_row['source_graph'] == row['source_graph'], 'Workshop sky compiled template differs from original-paired MCC graph')
                require(all(actual_row['source_banks'][str(f)]['hex'] == profile['native_role_banks'][str(f)] for f in (28,36,44,52,68,76,84,92)),
                        'Workshop sky animation role requires another owner')
                for expected, actual in zip(overlays, actual_case['expected_overlays']):
                    require(actual['input']==expected['input']==0 and actual['range']==expected['range']==0,
                            'Workshop sky requires an unproved external input/range owner')
                    if actual['curve_hex']!=expected['curve_hex']:
                        expected['curve_hex_variants']=sorted(set([expected['curve_hex'],actual['curve_hex']]))
                    expected['period_words'] = sorted(set(expected['period_words']+[actual['period_word']]))
            profiles.append(profile)
        return dict(schema='H2_MCC_NATIVE_SKY_COOKED_PROFILES_V1', source_map_sha256=exporter.pins, profiles=profiles,
            curve_reference=dict(report_sha256=sha(proof_raw),fixture_sha256=proof['fixture_sha256'],
                xbe_sha256=proof['xbe_sha256'],entry=proof['entry'],cases=proof['cases'])), dict(
            schema='H2_MCC_NATIVE_SKY_COOKED_FIXTURE_V1', scope='Three authored Workshop sky materials and four independently paired TU1.5 controls; component data admission, no GPU parity claim',
            expected_mod_owners=3, expected_profiles=4, cases=cases, map_sha256=exporter.pins)
    finally:
        exporter.close()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument('--prepared-root', type=Path, required=True)
    p.add_argument('--item', type=Path, required=True)
    p.add_argument('--curve-proof', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a=p.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output=a.output.resolve();require(output.is_relative_to(owner), 'raw sky inputs must stay in owned scratch')
    output.mkdir()
    policy, fixture=export(a.repo_root.resolve(strict=True), a.prepared_root.resolve(strict=True), a.item.resolve(strict=True), a.curve_proof.resolve(strict=True))
    for name, value in (('policy.json', policy), ('fixture.json', fixture)):
        raw=(json.dumps(value, indent=2)+'\n').encode();require(len(raw)<=1024*1024, 'sky data exceeds bounded input budget')
        (output/name).write_bytes(raw)
    print(json.dumps(dict(profiles=len(policy['profiles']), cases=len(fixture['cases']), output=str(output))), flush=True)


if __name__ == '__main__':
    main()
