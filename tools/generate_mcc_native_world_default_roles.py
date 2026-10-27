#!/usr/bin/env python3
"""Compile two source-proved default bitmap/UV data roles, never programs."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def canonical(v):
    return json.dumps(v, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def generate(raw):
    p = json.loads(raw)
    if p['schema'] != 'H2_MCC_NATIVE_WORLD_DEFAULT_ROLES_V1' or len(p['profiles']) != 2:
        raise ValueError('Unreviewed default resource role census')
    owners, rows = set(), []
    rules = [('tex_bump_three_detail_blend', '03a_oldmombasa', 3783344756, 'default_detail', 6, 1064697328, 1),
             ('tex_env', 'deltatap', 3796976452, 'color_black_alpha_black', 4, 1069055604, 0)]
    for row, (family, source, datum, name, slot, log, diffuse) in zip(p['profiles'], rules):
        full = 'shaders\\shader_templates\\opaque\\' + family
        default = row['default_bitmap']
        if (row['template'] != full or row['mapping'] != family or row['native_source_map'] != source or
                row['native_record']['group'] != 'stem' or row['native_record']['name'] != full or
                row['shader_type_flags'] != 1 or row['runtime_diffuse_slot'] != diffuse or
                row['default_uv_hex'] != '0000803f0000803f0000000000000000' or
                default['datum'] != datum or default['name'] != 'shaders\\default_bitmaps\\bitmaps\\' + name or
                default['frame'] != 0 or default['log_word'] != log or default['insert_slot'] != slot):
            raise ValueError('Unreviewed original default resource/constant role')
        graph = row['source_graph']
        if hashlib.sha256(canonical(graph)).hexdigest() != row['source_graph_sha256']:
            raise ValueError('Changed complete MCC compiler remap graph')
        if len(bytes.fromhex(graph['root_hex'])) != 96 or len(graph['definitions']) != 3:
            raise ValueError('Changed compiled template root/platform bank')
        for definition in graph['definitions']:
            if len(bytes.fromhex(definition['definition_hex'])) != 40 or len(definition['arrays']) != 5:
                raise ValueError('Changed compiled template definition')
            for bank, (field, stride) in zip(definition['arrays'], ((0, 10), (8, 2), (16, 10), (24, 6), (32, 4))):
                if (bank['field'] != field or len(bytes.fromhex(bank['raw_hex'])) != bank['count'] * stride or
                        len(bank['typed_references']) != (bank['count'] if field == 16 else 0) or
                        any(len(ref) != 2 or ref[0] != 'spas' for ref in bank['typed_references'])):
                    raise ValueError('Changed complete typed remap array')
        if any(row['source_counts'][str(field)] for field in (60, 68, 76, 84)):
            raise ValueError('Unauthenticated animation producer')
        if row['paired_proof']['complete_original_pairs'] != 15 or not row['paired_proof']['complete_main_matches']:
            raise ValueError('Missing complete original paired default-role proof')
        original = default['original_resource']['texture']
        if not default['actual_PC_resources'] or any(x['texture'] != original or x['datum'] != datum for x in default['actual_PC_resources']):
            raise ValueError('Actual source default bitmap is not byte-exact original at every mip')
        for owner in row['needed_PC_owners']:
            key = owner['map'], owner['datum']
            if key in owners or owner['map'] not in ('needle', 'street_sweeper'):
                raise ValueError('Unknown/repeated required default-role owner')
            owners.add(key)
        record = {k: row['native_record'][k] for k in ('payload', 'bytes', 'fnv64')}
        rows.append({k: row[k] for k in ('template', 'mapping', 'source_graph', 'source_counts',
            'shader_type_flags', 'runtime_diffuse_slot', 'native_shape', 'default_uv_hex')})
        rows[-1].update(native_record=record, default_bitmap={k: default[k] for k in ('datum', 'name', 'frame', 'log_word', 'insert_slot')})
        rows[-1]['default_bitmap']['texture'] = original
    if len(owners) != 3:
        raise ValueError('Changed complete required owner census')
    body = canonical({'schema': p['schema'], 'profiles': rows}).decode()
    return ('// Generated source-proved native default role data.\n// Data SHA256: ' + hashlib.sha256(raw).hexdigest() + '\n'
        'constexpr std::string_view native_world_default_roles_profiles=R"H2DEFWORLD(' + body + ')H2DEFWORLD";\n')


def main():
    p = argparse.ArgumentParser(description=__doc__);p.add_argument('--check', action='store_true');a = p.parse_args()
    out = ROOT / 'src/rendering/mcc_native_world_default_roles_profiles.inc'
    text = generate((ROOT / 'tools/data/mcc-native-world-default-roles.json').read_bytes())
    if a.check:
        if out.read_text() != text:
            raise ValueError('Stale default role registry')
    else:
        out.write_text(text)


if __name__ == '__main__':
    main()
