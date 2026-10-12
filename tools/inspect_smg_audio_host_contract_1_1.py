#!/usr/bin/env python3
"""Retain exact original settings/capability consumers, without executing them."""
import argparse
import json
from pathlib import Path
from oracle_smg_audio_ownership_1_1 import FROZEN, OUT, ROOT, PIN, digest
from oracle_smg_firing_1_1 import Xbe, _Code
from oracle_smg_constructor_equipped_1_1 import _trace

ENTRIES = (0x2DD56C, 0x33874D, 0x379FEA, 0x3832AA, 0x380810, 0x38087C,
           0x381BEE, 0x381C0A, 0x384C7C, 0x38689A, 0x2205C0,
           0x3824AC, 0x387920, 0x38A840, 0x38A9E4, 0x18A200, 0x18A2A0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    for path, pin in FROZEN.items():
        assert digest(ROOT/path) == pin, path
    image = Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03') == PIN
    pool = json.loads((ROOT/'analysis/multiplayer/map-pack/smg-audio-pool-prefix/source-fixture.json').read_text())
    refs = json.loads((OUT/'reference/provenance.json').read_text())
    for row in refs:
        assert digest(ROOT/row['path']) == row['sha256']
    code = {s.name: _Code(image, s.name) for s in image.sections if s.flags & 4}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    directory = args.output.parent/'instructions'
    directory.mkdir(exist_ok=True)
    listings = []
    for entry in ENTRIES:
        section = next(s for s in image.sections if s.va <= entry < s.va+s.vsize)
        decoder = code[section.name]
        function = _trace(decoder, entry, set(ENTRIES))
        instructions = [decoder.at(address) for address in sorted(function.code)]
        for instruction in instructions:
            assert image.read(instruction.address, len(instruction.bytes)) == instruction.bytes
        path = directory/f'{entry:08x}.asm'
        path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
        listings.append({'entry': f'{entry:08x}', 'section': section.name, 'file': path.name,
            'sha256': digest(path), 'instructions': len(instructions), 'source_inspection_only': True,
            'sites': [{'site': f'{a:08x}', 'target': f'{b:08x}', 'kind': k} for a, b, k in function.sites]})
    owner_matches = []
    needle = (0x18A200).to_bytes(4, 'little')
    for section in image.sections:
        raw = image.section_bytes(section)
        offset = raw.find(needle)
        while offset >= 0:
            address = section.va + offset
            owner_matches.append({'address': f'{address:08x}', 'section': section.name,
                'bytes_around_hex': image.read(address-4, 20).hex(),
                'raw_reference_only_no_dispatch_execution_claimed': True})
            offset = raw.find(needle, offset+1)
    imports = []
    for slot, ordinal, symbol in ((0x417C50, 24, 'ExQueryNonVolatileSetting'), (0x417CE4, 356, 'HalBootSMCVideoMode')):
        raw = image.read(slot, 4)
        assert int.from_bytes(raw, 'little') == 0x80000000 | ordinal
        imports.append({'slot': f'{slot:08x}', 'original_raw_hex': raw.hex(),
            'ordinal': ordinal, 'primary_export_symbol': symbol, 'runtime_service_result_supplied': False})
    snapshot = {'effective_flags_38d144': image.read(0x38D144, 4).hex(),
        'override_38d148': image.read(0x38D148, 4).hex(), 'fault_38d14c': image.read(0x38D14C, 4).hex(),
        'free2D_38d130': image.read(0x38D130, 4).hex(), 'free3D_38d134': image.read(0x38D134, 4).hex(),
        'freeSGE_38d138': image.read(0x38D138, 4).hex(),
        'unexecuted_original_image_bytes_are_not_initialized_capabilities': True}
    report = {'inspector_sha256': digest(__file__), 'source_sha256': PIN,
        'preserved_prior_pins': FROZEN, 'primary_compatibility_references': refs,
        'original_import_slots': imports,
        'unchanged_frozen_settings_stop': pool['source_device_prefix']['first_unprovided_dependency'],
        'original_cold_data_snapshot': snapshot,
        'owner_pool_ctor_raw_references': owner_matches, 'instruction_listings': listings,
        'new_original_device_execution_or_settings_result_supplied': False,
        'retail_Xbox_saved_audio_or_AV_pack_state_known': False,
        'SDL_preference_or_output_format_is_not_original_DSCAPS': True,
        'native_gameplay_update_applied': False}
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'output': str(args.output), 'original_inspection_listings': len(listings),
        'source_instruction_rows': sum(row['instructions'] for row in listings), 'prior_pins': len(FROZEN)}))


if __name__ == '__main__':
    main()
