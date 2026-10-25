#!/usr/bin/env python3
"""Replace only Dire Valley's two preview bitmaps in a private COW menu cache.

Use run_test.py and a fresh output inside HALO2_TEST_SCRATCH. Catalog records,
gameplay tags and source caches are preserved. Native rendering is a separate
validation step before selecting the resulting cache.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zlib

from PIL import Image
from author_dire_valley_menu import MENU, MAP, PROFILE, save, sha, digest, scenario_card, AppendTags
from mcc_content import MccCache
from mcc_multiplayer_catalog import bitmap, catalog
from mcc_native_map_owner_publication import (publish_staged_owner_index,
    verify_owner_index, repair_owner_index)


def cow_patch(path, patches=(), appended=b''):
    temporary = path.with_name(path.name + '.preview-new')
    subprocess.run(['cp', '--reflink=always', '--', str(path), str(temporary)], check=True)
    with temporary.open('r+b') as stream:
        if appended:
            stream.seek(0, 2)
            stream.write(bytes((-stream.tell()) % 16))
            stream.write(appended)
        for at, raw in patches:
            stream.seek(at)
            stream.write(raw)
    temporary.replace(path)


def replace(imported, output, preview, report, package=None):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    imported = imported.resolve(strict=True)
    output = output.resolve()
    if not output.is_relative_to(scratch) or output.exists():
        raise ValueError('Use a fresh output inside owned scratch')
    image = Image.open(preview).convert('RGBA')
    if image.size != (220, 207):
        raise ValueError('Dire Valley requires its native 220x207 preview')
    pixels = image.tobytes('raw', 'BGRA')
    compressed = zlib.compress(pixels, 9)
    partition = struct.pack('<ii', 1, len(compressed)) + compressed
    textures = 'halo2/h2_maps_win64_dx11/textures.dat'
    custom_root = 'custom-maps/dire_valley'
    custom_map = custom_root + '/' + MAP
    before = {name: sha(imported / name) for name in (MENU, textures, custom_map)}
    shutil.copytree(imported, output, copy_function=os.link)
    original_provenance = json.loads((output / 'provenance.json').read_text())
    verified_index = verify_owner_index(output, original_provenance)
    if verified_index is None:
        raise ValueError('Existing preview cache requires its authenticated owner index')

    menu = MccCache(imported / MENU)
    globals_tag, at = menu.tag(menu.u32(menu.index + 16), 'matg')
    rows = catalog(menu, globals_tag, at + 0x178)
    catalog_before = [bytes(menu.data[p:p + 0xc64]) for p in rows]
    selected = [p for p in rows if menu.u32(p) == 20261022]
    if len(selected) != 1:
        raise ValueError('Expected one existing Dire Valley menu card')
    _, frame_at, _ = bitmap(menu, menu.u32(selected[0] + 8))
    pixel_at = ((output / textures).stat().st_size + 15) & ~15
    if pixel_at + len(partition) >= 0x40000000:
        raise ValueError('Preview exceeds the MCC bitmap bank address range')
    cow_patch(output / textures, appended=partition)
    cow_patch(output / MENU, [(frame_at + 56, struct.pack('<I', pixel_at)),
                             (frame_at + 80, struct.pack('<I', len(partition)))])
    menu.close()

    previous = MccCache(imported / custom_map)
    _, row = scenario_card(previous)
    card = bytearray(previous.data[row:row + 0xc64])
    image_tag, frame_at, frame = bitmap(previous, previous.u32(row + 8))
    template = bytes(previous.data[image_tag['offset']:image_tag['offset'] + 76]) + frame
    previous.close()
    if package is not None:
        package = package.resolve(strict=True)
        source_map = package / 'content' / MAP
        source_pin = sha(source_map)
        # The caller supplied a separately validated authored map. Replace only
        # the private nested copy; the root catalog and stock owners stay exact.
        shutil.rmtree(output / custom_root)
        shutil.copytree(package / 'content', output / custom_root, copy_function=os.link)
        custom = MccCache(source_map)
    else:
        custom = MccCache(imported / custom_map)
    _, row = scenario_card(custom)
    geometry = {tag['name']: digest(custom.data[tag['offset']:tag['offset'] + tag['size']])
                for tag in custom.tags.values() if tag['group'] in ('sbsp', 'ltmp')}
    if package is not None:
        builder = AppendTags(custom)
        map_bitmap = builder.image(template, partition)
        struct.pack_into('<I', card, 8, map_bitmap)
        custom.data[row:row + len(card)] = card
        builder.finish(output / custom_map)
        if (output / custom_map).stat().st_size >= 0x40000000:
            raise ValueError('Preview exceeds the custom bitmap bank address range')
    else:
        _, frame_at, _ = bitmap(custom, custom.u32(row + 8))
        pixel_at = ((output / custom_map).stat().st_size + 15) & ~15
        if pixel_at + len(partition) >= 0x40000000:
            raise ValueError('Preview exceeds the custom bitmap bank address range')
        cow_patch(output / custom_map, [(8, struct.pack('<I', pixel_at + len(partition))),
                  (frame_at + 56, struct.pack('<I', pixel_at)),
                  (frame_at + 80, struct.pack('<I', len(partition)))], partition)
    custom.close()
    nested = output / custom_root
    for name in ('textures.dat', 'sounds_en.dat', 'sounds_neutral.dat', 'sounds_remastered.dat'):
        alias = nested / 'halo2/h2_maps_win64_dx11' / name
        alias.unlink()
        os.link(output / custom_map, alias)
    provenance = json.loads((nested / 'provenance.json').read_text())
    pin = sha(output / custom_map)
    provenance['source_files'][MAP.removeprefix('halo2/')] = dict(bytes=(output / custom_map).stat().st_size, sha256=pin)
    provenance['native_shader_library'].pop('owner_bindings', None)
    (nested / 'native-shaders/map-owners.json').unlink()
    receipt = json.loads((nested / 'custom-owner-receipt.json').read_text())
    receipt.update(input_bytes=(output / custom_map).stat().st_size,
                   input_sha256=pin, source_sha256=pin, producer='replace_dire_valley_preview.py')
    raw = (json.dumps(receipt, sort_keys=True) + '\n').encode()
    save(nested / 'custom-owner-receipt.json', receipt)
    save(nested / 'provenance.json', publish_staged_owner_index(nested, provenance, custom_receipts=[raw]))
    provenance = json.loads((output / 'provenance.json').read_text())
    for name in (MENU, textures):
        provenance['graphics_corrections']['changes'].append(dict(path=name,
            donor_sha256=before[name], converted_sha256=sha(output / name),
            producer='replace_dire_valley_preview.py', profile=PROFILE))
    provenance['custom_menu']['screenshot_sha256'] = sha(preview)
    if package is not None:
        provenance['custom_menu']['source_inputs'][str(source_map)] = source_pin
    save(output / 'provenance.json', provenance)
    repair_owner_index(output, provenance, verified_beforeimage=verified_index)

    verified = MccCache(output / MENU)
    globals_tag, at = verified.tag(verified.u32(verified.index + 16), 'matg')
    after_rows = catalog(verified, globals_tag, at + 0x178)
    assert [bytes(verified.data[p:p + 0xc64]) for p in after_rows] == catalog_before
    _, frame_at, _ = bitmap(verified, verified.u32(selected[0] + 8))
    with (output / textures).open('rb') as stream:
        stream.seek(verified.u32(frame_at + 56))
        encoded = stream.read(verified.u32(frame_at + 80))
    assert struct.unpack_from('<ii', encoded) == (1, len(encoded) - 8)
    assert zlib.decompress(encoded[8:]) == pixels
    verified.close()
    verified = MccCache(output / custom_map)
    assert all(digest(verified.data[tag['offset']:tag['offset'] + tag['size']]) == geometry[tag['name']]
               for tag in verified.tags.values() if tag['group'] in ('sbsp', 'ltmp'))
    _, row = scenario_card(verified)
    _, frame_at, _ = bitmap(verified, verified.u32(row + 8))
    pixel_at, size = verified.u32(frame_at + 56), verified.u32(frame_at + 80)
    assert zlib.decompress(verified.data[pixel_at + 8:pixel_at + size]) == pixels
    verified.close()
    assert all(sha(imported / name) == pin for name, pin in before.items())
    if package is not None:
        assert sha(source_map) == source_pin
    save(report, dict(passed=True, source=str(imported), output=str(output),
        input_sha256=before, output_sha256={name: sha(output / name) for name in before},
        decoded_bgra_sha256=digest(pixels), all_catalog_records_unchanged=len(catalog_before),
        geometry_and_lightmap_metadata_unchanged=geometry,
        authored_map_input=None if package is None else dict(path=str(source_map),sha256=source_pin),
        scope='Existing catalog records preserved; preview partitions updated in COW copies. '
              + ('Validated authored map replaces the private nested custom owner.' if package is not None else
                 'Custom geometry and lightmaps unchanged.')))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('imported', 'output', 'preview', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--package', type=Path, help='Separately validated authored map package to publish with its new photo')
    replace(**vars(parser.parse_args()))
