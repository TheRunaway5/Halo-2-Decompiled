"""Compile the finite MCC meter metadata contract; no executable shader bank."""
import argparse
import hashlib
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]


def render(raw):
    value = json.loads(raw)
    assert value['schema'] == 'H2_MCC_RESIDENT_METER_METADATA_V1'
    setups = [value['setup'], *value['additional_setups']]
    assert [s['existing_fragment_kernel'] for s in setups] == [
        '5dbdb0d883c442a24c4130b408a748fc0e83ae39929cbe954faaa9ff5d9b7988',
        '0632caf4684c3014c70612473e728b81d1290f96a03b8d51dcd2c60f74d7afd5',
        '3e6fb316fa50af5b3953aa345dbcaa98adc15e2bb1d83047266c88f167ba4d33']
    assert setups[2] == {'source_stem': {'group': 'stem', 'name': 'shaders\\shader_templates\\hud\\ammo_meter', 'datum': 3979890219, 'address': 14214828, 'size': 228, 'sha256': '26de7b8a159fb45598625ab2726b051ee98e240159794c9d906422f815f695d5'}, 'source_pass': {'group': 'spas', 'name': 'shaders\\shader_passes\\hud\\ammo_meter', 'datum': 3979955756, 'address': 14215056, 'size': 712, 'sha256': 'f41d9fd8bab9c0f9b308130ca383bf040bfe394c17b90b8e5d13ba38e6fc201a'}, 'pc_vertices': [{'group': 'vrtx', 'name': 'rasterizer\\vertex_shaders_dx9\\screen', 'datum': 3820831920, 'address': 7137892, 'size': 1600, 'sha256': '16448737252e5afe77c3c968233fb968ad1211e5b67d1e2ddfe8d5aae6d2c6f0'}], 'external_regions': [{'address': 446456, 'bytes': 128, 'sha256': '38723a2e5e8a17aa7950dc008209944e898f69a7bd10a23c839d341e935fd5ca'}, {'address': 7159472, 'bytes': 1876, 'sha256': '2a8e146e5a2b5a6b02314ac7d2ea0e310e654ba4f35fcd001458be57481877b5'}, {'address': 7161348, 'bytes': 1868, 'sha256': '3abc285eb3db258b8d3cf70ded7ea72084870eb3408fe8068f26796210859543'}, {'address': 7163216, 'bytes': 2136, 'sha256': '8506295546f8d483317b8877d90f51095d48654818f0daaed4c9945e6b8f198e'}], 'native_stem': {'record': {'group': 'stem', 'name': 'shaders\\shader_templates\\hud\\ammo_meter', 'bytes': 236, 'payload': 'd59d437df56e11a020c7b02d72e040ad83f86ca770adf144abac0fe35e3ec2b9.bin', 'fnv64': '3a6d683b23616ae1', 'pointers': [[92, 96], [100, 136], [108, 148], [116, 200], [124, 212], [132, 220]], 'references': [[204, ['spas', 'shaders\\shader_passes\\hud\\ammo_meter']]], 'strings': []}, 'metadata_hex': '000000000000000000000000010000000000000000000000000000000000000074696c73ffffffff00000000000000000000000000000000000000000000000064616873ffffffff0000000064616873ffffffff0000000001000000600000000100000088000000190000009400000001000000c800000001000000d400000004000000dc00000000320200000000000000ffff0000000201000100010001000100010001000100010001000100010001000100010001000100010001000100010001000100c2c273617073b6012be30002c2c2000000080400cece400003008000040180040402c0000403'}, 'derived_from_original': {'scope': 'Source-owned Ammo descriptors from authentic Original legacy code and authored MIP2; not an Original Ammo record', 'authority_sha256': 'd60db91932e676e6efd927093425e720ceec008c72475b27ce41fd2443deb500', 'kernel_authority_sha256': '0f5b2344bc00df4ea5a74408ee284e9620590a246d1c0ec28f8d398e01e5e7b9', 'original_stem_record': {'group': 'stem', 'name': 'shaders\\shader_templates\\hud\\widget_legacy_meter', 'bytes': 236, 'payload': 'd59d437df56e11a020c7b02d72e040ad83f86ca770adf144abac0fe35e3ec2b9.bin', 'fnv64': '3a6d683b23616ae1', 'pointers': [[92, 96], [100, 136], [108, 148], [116, 200], [124, 212], [132, 220]], 'references': [[204, ['spas', 'shaders\\shader_passes\\hud\\widget_legacy_meter']]], 'strings': []}, 'original_pass_record': {'group': 'spas', 'name': 'shaders\\shader_passes\\hud\\widget_legacy_meter', 'bytes': 512, 'payload': '6faafadb85bed5049cc53d9e62ca50b8a733cb6336e25344022058864aa2ac0e.bin', 'fnv64': '50d25530880b976c', 'pointers': [[32, 36], [40, 124], [48, 432], [56, 436], [64, 484], [96, 508]], 'references': [[380, ['vrtx', 'rasterizer\\vertex_shaders_xbox\\screen']]], 'strings': []}, 'original_pass_metadata_hex': '000000000000000000000000000000000100000000000000000000000100000024000000010000007c00000001000000b001000009000000b401000001000000e401000000000000000000000000000000000000000000000000000001000000fc0100000000000000000000000000000000000000000000000000000002000e00020000200820a8201100000000201220110a192018000011180000000000000000090000008018000000ff000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffff00000000000000000000000000000000d0000000000c000000000000004900000008000080000000000000000000000000000801000000003d011d02200c200100000000000000000000000000000000000000000000000000000000a020000000000000000c000000490000000000000000000000000000000000000611010001000000000000000000000000000000000000000000000000000000000078747276b7012ce300000000000000000000000000010000000000000000010007040000000000020100000804000008040000000000cfcfff10000043010101013b010000003e020300003f030300004a068000008f0000000093000000008f020000009300090000c1c1c120020000030303000000000000000000000000000000000040010328', 'changed_pass_bytes': [[485, 2, 34]], 'source_authored_MIP': 2, 'original_sampler_hex': '200200000303030000000000000000000000000000000000', 'derived_sampler_hex': '202200000303030000000000000000000000000000000000'}, 'native_pass': {'record': {'group': 'spas', 'name': 'shaders\\shader_passes\\hud\\ammo_meter', 'bytes': 512, 'payload': '5a6d323ddd92c29012b7274d719717d427a2217fa18115f6868683c490977786.bin', 'fnv64': 'a0d85707e88fd9cc', 'pointers': [[32, 36], [40, 124], [48, 432], [56, 436], [64, 484], [96, 508]], 'references': [[380, ['vrtx', 'rasterizer\\vertex_shaders_xbox\\screen']]], 'strings': []}, 'metadata_hex': '000000000000000000000000000000000100000000000000000000000100000024000000010000007c00000001000000b001000009000000b401000001000000e401000000000000000000000000000000000000000000000000000001000000fc0100000000000000000000000000000000000000000000000000000002000e00020000200820a8201100000000201220110a192018000011180000000000000000090000008018000000ff000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffff00000000000000000000000000000000d0000000000c000000000000004900000008000080000000000000000000000000000801000000003d011d02200c200100000000000000000000000000000000000000000000000000000000a020000000000000000c000000490000000000000000000000000000000000000611010001000000000000000000000000000000000000000000000000000000000078747276b7012ce300000000000000000000000000010000000000000000010007040000000000020100000804000008040000000000cfcfff10000043010101013b010000003e020300003f030300004a068000008f0000000093000000008f020000009300090000c1c1c120220000030303000000000000000000000000000000000040010328'}, 'existing_fragment_kernel': '3e6fb316fa50af5b3953aa345dbcaa98adc15e2bb1d83047266c88f167ba4d33', 'resident_vertex_records': [{'group': 'vrtx', 'name': 'rasterizer\\vertex_shaders_xbox\\screen', 'bytes': 156, 'payload': 'd330e767959f5df680b5eb50f4c17dc490349f8e321753f05adf73f8354e316e.bin', 'fnv64': 'ec7b3f769368d970', 'pointers': [[8, 12], [28, 40]], 'references': [], 'strings': []}]}
    for setup, expected_bytes in zip(setups, (568, 592, 512)):
        for label in ('native_pass', 'native_stem'):
            record = setup[label]['record']
            data = bytes.fromhex(setup[label]['metadata_hex'])
            assert len(data) == record['bytes'] and hashlib.sha256(data).hexdigest() + '.bin' == record['payload']
        assert setup['native_pass']['record']['bytes'] == expected_bytes
    encoded = json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False)
    return ('// Generated by tools/generate_mcc_native_meter.py; no executable bank.\n'
            '// Source data SHA256: ' + hashlib.sha256(raw).hexdigest() + '\n'
            'constexpr std::string_view resident_meter_metadata=R"H2METER(' + encoded + ')H2METER";\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    text = render((ROOT / 'tools/data/mcc-native-meter.json').read_bytes())
    path = ROOT / 'src/rendering/mcc_native_meter_profiles.inc'
    if args.check:
        assert path.read_text() == text
    else:
        path.write_text(text)


if __name__ == '__main__':
    main()
