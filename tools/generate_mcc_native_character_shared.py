#!/usr/bin/env python3
"""Compile finite source-authenticated shared character/overlay metadata setups."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Explicit shader authority is fixed before reading authored parameter values.
PAIRED_SHADER_PROVIDERS = {'objects\\gear\\covenant\\military\\shaders\\cov_storage_metal': '03a_oldmombasa',
 'objects\\gear\\human\\industrial\\shaders\\panel_crate': '01b_spacestation',
 'objects\\gear\\human\\industrial\\shaders\\panel_crate_multi_single': '01b_spacestation',
 'objects\\gear\\human\\military\\shaders\\military_decals_1': '01b_spacestation',
 'objects\\gear\\human\\military\\shaders\\military_decals_2': '01b_spacestation',
 'objects\\gear\\human\\military\\shaders\\military_decals_3': '01b_spacestation',
 'objects\\gear\\human\\military\\shaders\\military_decals_4': '01b_spacestation',
 'objects\\gear\\human\\military\\shaders\\military_decals_5': '01b_spacestation',
 'objects\\vehicles\\civilian\\shaders\\panel_truck_chassis': '03a_oldmombasa',
 'objects\\vehicles\\civilian\\shaders\\panel_truck_destroyed': '03a_oldmombasa',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_body': '03a_oldmombasa',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_body_dead': '03a_oldmombasa',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_glass': '03a_oldmombasa',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_glass_dead': '03a_oldmombasa',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_wheels': '03a_oldmombasa',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_wheels_dead': '03a_oldmombasa',
 'objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_bolt_luminous': 'coagulation',
 'scenarios\\objects\\special\\shaders\\invisible': '01b_spacestation',
 'ui\\hud\\shaders\\widget_flash_red_pulse': 'coagulation',
 'ui\\hud\\shaders\\widget_simple_flash_green': 'coagulation',
 'ui\\hud\\shaders\\widget_simple_flash_red': 'coagulation'}
PAIRED_SOURCE_LIBRARIES = {'coagulation': 'f523c80325d5fe3f8bff7d4f3768a3b97fb2d9904a9fa455f307757e7436917e',
 '01b_spacestation': 'db415f7973f0cc7a966148db98fd2837c8dfd3c4c320e534eec00ae7e11beb41',
 '03a_oldmombasa': 'a249dafbf4bf99413cc4470c65e83e66ba1c1c94d8fc5bf4deffb4870e53f519'}

AUTHENTICATED_HUD_AMMO_BINDING = {'kind': 'source-owned-binding-descriptor',
 'source_setup_sha256': 'd816052e2228bf828f0e01ed528acaf3fc054e52b227acd0b63e8cab23ae66a1',
 'source_stem': {'group': 'stem',
                 'name': 'shaders\\shader_templates\\hud\\ammo_meter',
                 'datum': 3979890219,
                 'address': 14214828,
                 'size': 228,
                 'sha256': '26de7b8a159fb45598625ab2726b051ee98e240159794c9d906422f815f695d5'},
 'source_pass': {'group': 'spas',
                 'name': 'shaders\\shader_passes\\hud\\ammo_meter',
                 'datum': 3979955756,
                 'address': 14215056,
                 'size': 712,
                 'sha256': 'f41d9fd8bab9c0f9b308130ca383bf040bfe394c17b90b8e5d13ba38e6fc201a'},
 'existing_fragment_kernel': '3e6fb316fa50af5b3953aa345dbcaa98adc15e2bb1d83047266c88f167ba4d33',
 'derived_from_original': {'scope': 'Source-owned Ammo descriptors from authentic Original legacy '
                                    'code and authored MIP2; not an Original Ammo record',
                           'authority_sha256': 'd60db91932e676e6efd927093425e720ceec008c72475b27ce41fd2443deb500',
                           'kernel_authority_sha256': '0f5b2344bc00df4ea5a74408ee284e9620590a246d1c0ec28f8d398e01e5e7b9',
                           'original_stem_record': {'group': 'stem',
                                                    'name': 'shaders\\shader_templates\\hud\\widget_legacy_meter',
                                                    'bytes': 236,
                                                    'payload': 'd59d437df56e11a020c7b02d72e040ad83f86ca770adf144abac0fe35e3ec2b9.bin',
                                                    'fnv64': '3a6d683b23616ae1',
                                                    'pointers': [[92, 96],
                                                                 [100, 136],
                                                                 [108, 148],
                                                                 [116, 200],
                                                                 [124, 212],
                                                                 [132, 220]],
                                                    'references': [[204,
                                                                    ['spas',
                                                                     'shaders\\shader_passes\\hud\\widget_legacy_meter']]],
                                                    'strings': []},
                           'original_pass_record': {'group': 'spas',
                                                    'name': 'shaders\\shader_passes\\hud\\widget_legacy_meter',
                                                    'bytes': 512,
                                                    'payload': '6faafadb85bed5049cc53d9e62ca50b8a733cb6336e25344022058864aa2ac0e.bin',
                                                    'fnv64': '50d25530880b976c',
                                                    'pointers': [[32, 36],
                                                                 [40, 124],
                                                                 [48, 432],
                                                                 [56, 436],
                                                                 [64, 484],
                                                                 [96, 508]],
                                                    'references': [[380,
                                                                    ['vrtx',
                                                                     'rasterizer\\vertex_shaders_xbox\\screen']]],
                                                    'strings': []},
                           'original_pass_metadata_hex': '000000000000000000000000000000000100000000000000000000000100000024000000010000007c00000001000000b001000009000000b401000001000000e401000000000000000000000000000000000000000000000000000001000000fc0100000000000000000000000000000000000000000000000000000002000e00020000200820a8201100000000201220110a192018000011180000000000000000090000008018000000ff000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffff00000000000000000000000000000000d0000000000c000000000000004900000008000080000000000000000000000000000801000000003d011d02200c200100000000000000000000000000000000000000000000000000000000a020000000000000000c000000490000000000000000000000000000000000000611010001000000000000000000000000000000000000000000000000000000000078747276b7012ce300000000000000000000000000010000000000000000010007040000000000020100000804000008040000000000cfcfff10000043010101013b010000003e020300003f030300004a068000008f0000000093000000008f020000009300090000c1c1c120020000030303000000000000000000000000000000000040010328',
                           'changed_pass_bytes': [[485, 2, 34]],
                           'source_authored_MIP': 2,
                           'original_sampler_hex': '200200000303030000000000000000000000000000000000',
                           'derived_sampler_hex': '202200000303030000000000000000000000000000000000'},
 'native_stem_record': {'group': 'stem',
                        'name': 'shaders\\shader_templates\\hud\\ammo_meter',
                        'bytes': 236,
                        'payload': 'd59d437df56e11a020c7b02d72e040ad83f86ca770adf144abac0fe35e3ec2b9.bin',
                        'fnv64': '3a6d683b23616ae1',
                        'pointers': [[92, 96],
                                     [100, 136],
                                     [108, 148],
                                     [116, 200],
                                     [124, 212],
                                     [132, 220]],
                        'references': [[204, ['spas', 'shaders\\shader_passes\\hud\\ammo_meter']]],
                        'strings': []},
 'native_pass_record': {'group': 'spas',
                        'name': 'shaders\\shader_passes\\hud\\ammo_meter',
                        'bytes': 512,
                        'payload': '5a6d323ddd92c29012b7274d719717d427a2217fa18115f6868683c490977786.bin',
                        'fnv64': 'a0d85707e88fd9cc',
                        'pointers': [[32, 36],
                                     [40, 124],
                                     [48, 432],
                                     [56, 436],
                                     [64, 484],
                                     [96, 508]],
                        'references': [[380, ['vrtx', 'rasterizer\\vertex_shaders_xbox\\screen']]],
                        'strings': []}}

def generate(raw):
    value = json.loads(raw)
    if (value['schema'] != 'H2_MCC_NATIVE_CHARACTER_SHARED_V1' or
            [p['native_shader_record']['name'] for p in value['profiles']] != ['objects\\characters\\masterchief\\shaders\\masterchief',
 'objects\\characters\\masterchief\\shaders\\mc_emblem',
 'objects\\characters\\elite\\shaders\\elite_emblem',
 'objects\\vehicles\\warthog\\turrets\\gauss\\shaders\\gauss_turret_sensor',
 'objects\\vehicles\\scorpion\\shaders\\icon',
 'objects\\vehicles\\scorpion\\shaders\\number_decal',
 'objects\\vehicles\\scorpion\\shaders\\marine',
 'objects\\vehicles\\scorpion\\shaders\\no_step',
 'objects\\vehicles\\scorpion\\shaders\\scorpion_decal',
 'objects\\weapons\\multiplayer\\assault_bomb\\shaders\\assault_bomb_scrawl',
 'objects\\weapons\\rifle\\plasma_rifle\\shaders\\plasma_rifle_gauge',
 'objects\\weapons\\rifle\\plasma_rifle\\shaders\\plasma_rifle_current',
 'objects\\weapons\\support_low\\sentinel_gun\\shaders\\sentinel_gun',
 'objects\\weapons\\support_low\\sentinel_gun\\shaders\\sentinel_gun_illum',
 'objects\\characters\\elite\\fp_arms\\shaders\\elite_fp_glow',
 'objects\\characters\\elite\\shaders\\elite_honor_inset_lights',
 'objects\\characters\\elite\\shaders\\inset_lights',
 'objects\\weapons\\pistol\\magnum\\shaders\\magnum_lod',
 'objects\\weapons\\pistol\\needler\\shaders\\needler_core',
 'objects\\weapons\\pistol\\needler\\shaders\\needler_luminous_3p',
 'objects\\weapons\\pistol\\plasma_pistol\\shaders\\plasma_pistol_current',
 'objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lod',
 'objects\\weapons\\rifle\\beam_rifle\\shaders\\beam_battery_luminous',
 'objects\\weapons\\rifle\\brute_plasma_rifle\\shaders\\plasma_rifle_current',
 'objects\\weapons\\rifle\\brute_plasma_rifle\\shaders\\plasma_rifle_gauge',
 'objects\\weapons\\rifle\\smg\\shaders\\smg_lod',
 'objects\\weapons\\rifle\\sniper_rifle\\shaders\\sniper_rifle_lod',
 'objects\\characters\\elite\\fp_arms\\shaders\\elite_fp_armor',
 'objects\\characters\\elite\\fp_arms\\shaders\\elite_fp_leather',
 'objects\\characters\\elite\\shaders\\arms',
 'objects\\characters\\elite\\shaders\\elite_honor_armor',
 'objects\\characters\\elite\\shaders\\head',
 'objects\\characters\\elite\\shaders\\helmet',
 'objects\\characters\\elite\\shaders\\helmet_ultra',
 'objects\\characters\\elite\\shaders\\legs',
 'objects\\characters\\elite\\shaders\\soc_eyes',
 'objects\\characters\\elite\\shaders\\torso',
 'objects\\characters\\masterchief\\fp\\shaders\\fp_arms',
 'objects\\weapons\\melee\\energy_blade\\shaders\\energy_blade_bottom',
 'objects\\weapons\\melee\\energy_blade\\shaders\\energy_blade_top',
 'objects\\weapons\\pistol\\needler\\shaders\\needler_luminous',
 'objects\\weapons\\pistol\\plasma_pistol\\shaders\\plasma_pistol_gauge',
 'objects\\weapons\\rifle\\battle_rifle\\shaders\\ones',
 'objects\\weapons\\rifle\\battle_rifle\\shaders\\tens',
 'objects\\weapons\\rifle\\beam_rifle\\shaders\\beam_rifle_ammo',
 'objects\\weapons\\rifle\\covenant_carbine\\shaders\\carbine_display',
 'objects\\weapons\\rifle\\covenant_carbine\\shaders\\carbine_switch',
 'objects\\weapons\\rifle\\sniper_rifle\\shaders\\sniper_rifle_screen',
 'objects\\weapons\\support_high\\rocket_launcher\\shaders\\rocket_launcher_decal',
 'objects\\weapons\\support_low\\sentinel_gun\\shaders\\sentinel_gun_two',
 'objects\\characters\\masterchief\\shaders\\materchief_plasma_shield',
 'objects\\characters\\masterchief\\shaders\\materchief_plasma_shield_fp',
 'objects\\characters\\masterchief\\shaders\\materchief_overshield',
 'objects\\characters\\masterchief\\shaders\\materchief_overshield_fp',
 'objects\\characters\\elite\\shaders\\elite_plasma_shield',
 'objects\\characters\\elite\\shaders\\elite_plasma_shield_fp',
 'objects\\characters\\elite\\shaders\\elite_overshield',
 'objects\\characters\\elite\\shaders\\elite_overshield_fp',
 'objects\\gear\\covenant\\military\\shaders\\cov_storage_metal',
 'objects\\gear\\human\\industrial\\shaders\\panel_crate',
 'objects\\gear\\human\\industrial\\shaders\\panel_crate_multi_single',
 'objects\\gear\\human\\military\\shaders\\military_decals_1',
 'objects\\gear\\human\\military\\shaders\\military_decals_2',
 'objects\\gear\\human\\military\\shaders\\military_decals_3',
 'objects\\gear\\human\\military\\shaders\\military_decals_4',
 'objects\\gear\\human\\military\\shaders\\military_decals_5',
 'objects\\vehicles\\civilian\\shaders\\panel_truck_chassis',
 'objects\\vehicles\\civilian\\shaders\\panel_truck_destroyed',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_body',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_body_dead',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_glass',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_glass_dead',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_wheels',
 'objects\\vehicles\\civilian\\uberchassis\\shaders\\uberchassis_wheels_dead',
 'objects\\weapons\\support_high\\flak_cannon\\shaders\\flak_bolt_luminous',
 'scenarios\\objects\\special\\shaders\\invisible',
 'ui\\hud\\shaders\\bomb_defusal_meter',
 'ui\\hud\\shaders\\camo_meter',
 'ui\\hud\\shaders\\dim_text_flash',
 'ui\\hud\\shaders\\plasma_meter',
 'ui\\hud\\shaders\\scope_masks',
 'ui\\hud\\shaders\\shield_test',
 'ui\\hud\\shaders\\text_flash',
 'ui\\hud\\shaders\\waypoint_emblem',
 'ui\\hud\\shaders\\widget_flash_red_pulse',
 'ui\\hud\\shaders\\widget_gradient_flash',
 'ui\\hud\\shaders\\widget_gradient_flash_pulse',
 'ui\\hud\\shaders\\widget_meter_custom',
 'ui\\hud\\shaders\\widget_simple_custom',
 'ui\\hud\\shaders\\widget_simple_flash_green',
 'ui\\hud\\shaders\\widget_simple_flash_red',
 'ui\\hud\\shaders\\ammo_meter',
 'ui\\hud\\shaders\\waypoint_icon_meter',
 'multiplayer\\hill\\hill',
 'objects\\powerups\\over_shield\\shaders\\over_shield']):
        raise ValueError('Unreviewed finite shared character source census')
    canonical = lambda x: json.dumps(x, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()
    for p in value['profiles']:
        if (len(p['prediction_roles']) > 96 or
                int.from_bytes(bytes.fromhex(p['source_root_hex'])[44:48], 'little') != len(p['prediction_roles'])):
            raise ValueError('Unreviewed complete shared predicted-resource census')
        if hashlib.sha256(canonical(p['source_graph'])).hexdigest() != p['source_graph_sha256']:
            raise ValueError('Complete shared character source graph changed')
        expected_source = ('01b_spacestation' if p['template'] in
                           ('shaders\\shader_templates\\opaque\\overlay', 'shaders\\shader_templates\\hud\\widget_simple_flash') else '03a_oldmombasa' if p['template'] == 'shaders\\shader_templates\\transparent\\one_alpha_env' else 'coagulation')
        if p['native_source_map'] != expected_source:
            raise ValueError('Unreviewed original character source owner')
        name = p['native_shader_record']['name']
        if name in PAIRED_SHADER_PROVIDERS:
            source = PAIRED_SHADER_PROVIDERS[name]
            if (p.get('native_shader_source_map') != source or
                    p.get('native_shader_source_canonical_sha256') != PAIRED_SOURCE_LIBRARIES[source] or
                    p.get('native_template_source_map') != p['native_source_map'] or
                    p.get('provider_shader_record') != p['native_shader_record'] or
                    p.get('retained_programme_stem_record') != p['native_record']):
                raise ValueError('Unreviewed paired shader source provenance')
        if name == r'ui\hud\shaders\scope_masks':
            if (p.get('source_authored_pixel_pair') != {'index': 3, 'parameter': 'meter_empty_color', 'component': 'rgb', 'source_word': 0, 'original_word': 11590655, 'original_pixel_bank_hex': '00000000000000004846d900ffdbb000', 'projected_pixel_bank_hex': '00000000000000004846d90000000000', 'named_role_proof_sha256': 'bf11083c9bc895adf32161a210402dc7233f69dbf390f0a13c4ae64e57b83231', 'packing_proof_sha256': 'd5ed9e16c8c8fc54eb218a00e0f52e483a9c59d76ad5071caecb8dc263622b52'} or
                    p['source_banks']['12'] != '00000000000000004846d90000000000' or
                    p['native_banks']['12'] != '00000000000000004846d90000000000'):
                raise ValueError('Unreviewed finite HUD source-authored pixel pair')


        if name == r'ui\hud\shaders\ammo_meter':
            if (p.get('resident_binding_descriptor') != AUTHENTICATED_HUD_AMMO_BINDING or
                    p['native_record'] != AUTHENTICATED_HUD_AMMO_BINDING['native_stem_record']):
                raise ValueError('Unreviewed source-owned Ammo binding metadata')
    encoded = canonical(value).decode()
    return ('// Generated by tools/generate_mcc_native_character_shared.py; do not edit.\n'
            '// Source data SHA256: ' + hashlib.sha256(raw).hexdigest() + '\n'
            'constexpr char native_character_shared_storage[]=R"H2CHAR(' + encoded + ')H2CHAR";\n'
            'constexpr std::string_view native_character_shared_profiles{native_character_shared_storage,sizeof(native_character_shared_storage)-1};\n')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    text = generate((ROOT/'tools/data/mcc-native-character-shared.json').read_bytes())
    path = ROOT/'src/rendering/mcc_native_character_shared_profiles.inc'
    if args.check:
        if not path.is_file() or path.read_text() != text:
            raise ValueError('Stale finite shared character source registry')
    else:
        path.write_text(text)

if __name__ == '__main__':
    main()
