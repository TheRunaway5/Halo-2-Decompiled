#!/usr/bin/env python3
"""Publish Dire Valley's authored map card into a private MCC menu cache.

Run through run_test.py. Original imports and the walkaround package are inputs;
only the owned output tree is written. The existing menu consumes the C64 map
record and ordinary 220x207 bitmap without changing the menu layout.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zlib

from PIL import Image
from mcc_content import MccCache
from mcc_multiplayer_catalog import bitmap, catalog, summary
from mcc_native_map_owners import bank_identity
from mcc_native_map_owner_publication import publish_staged_owner_index


PROFILE = 'H2_DIRE_VALLEY_MENU_V1'
BITMAP_NAME = r'ui\global_bitmaps\map_shots\map_mp_dire_valley'
MAP = 'halo2/Mods/dire_valley/dire_valley.map'
MENU = 'halo2/h2_maps_win64_dx11/mainmenu.map'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def save(path, value):
    # Output files initially share read-only input inodes. Always replace.
    temporary = path.with_name(path.name + '.new')
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
    temporary.replace(path)


def write_cache(cache, output):
    struct.pack_into('<I', cache.data, 8, len(cache.data))
    struct.pack_into('<H', cache.data, 28, cache.u16(28) & ~1)
    struct.pack_into('<4I', cache.data, 776, 0, 0, 0, 0)
    temporary = output.with_name(output.name + '.new')
    temporary.write_bytes(cache.data)
    temporary.replace(output)


class AppendTags:
    def __init__(self, cache):
        self.cache = cache
        self.data = cache.data
        self.count = cache.u32(32)
        self.table_magic = cache.u32(cache.index) - cache.index - 32
        at = cache.pointer(cache.u32(cache.index + 8), self.table_magic, self.count * 16)
        self.table = bytearray(self.data[at:at + self.count * 16])
        at, size = cache.u32(36), cache.u32(40)
        self.names = bytearray(self.data[at:at + size])
        at = cache.u32(44)
        self.offsets = bytearray(self.data[at:at + self.count * 4])

    def append(self, value):
        self.data.extend(bytes((-len(self.data)) % 16))
        at = len(self.data)
        self.data.extend(value)
        return at

    def image(self, template, partition):
        datum = 0xf17e0000 | self.count
        assert self.count < 65535 and datum not in self.cache.tags
        pixels = self.append(partition)
        # One standalone preview; sequence/sprite and mip blocks are absent.
        payload = bytearray(template)
        struct.pack_into('<4I', payload, 60, 0, 0, 1, 0)
        struct.pack_into('<I', payload, 76 + 28, datum)
        struct.pack_into('<2I', payload, 76 + 32, 0, 0)
        struct.pack_into('<3I', payload, 76 + 56, pixels, 0, 0)
        struct.pack_into('<3I', payload, 76 + 80, len(partition), 0, 0)
        self.data.extend(bytes((-len(self.data)) % 16))
        at = len(self.data)
        address = at + self.cache.magic
        struct.pack_into('<I', payload, 72, address + 76)
        self.data.extend(payload)
        self.table.extend(struct.pack('<4I', int.from_bytes(b'bitm', 'big'), datum,
                                      address, len(payload)))
        self.offsets.extend(struct.pack('<I', len(self.names)))
        self.names.extend(BITMAP_NAME.encode('ascii') + b'\0')
        self.count += 1
        return datum

    def finish(self, output):
        names = self.append(self.names)
        offsets = self.append(self.offsets)
        table = self.append(self.table)
        for at, value in ((8, len(self.data)), (32, self.count), (36, names),
                          (40, len(self.names)), (44, offsets),
                          (self.cache.index + 8, table + self.table_magic),
                          (self.cache.index + 24, self.count)):
            struct.pack_into('<I', self.data, at, value)
        write_cache(self.cache, output)


def record_text(record, profile):
    for field, width, text in ((12, 64, profile['name']),
                               (0x24c, 256, profile['description'])):
        raw = text.encode('utf-16le')
        if len(raw) > width - 2:
            raise ValueError('Map card text exceeds its authored UTF16 field')
        for language in range(9):
            at = field + language * width
            record[at:at + width] = raw.ljust(width, b'\0')


def scenario_card(cache):
    tag, at = cache.tag(cache.u32(cache.index + 12), 'scnr')
    rows = catalog(cache, tag, at + 0x398)
    assert len(rows) == 1 and cache.u32(rows[0]) == 20261022
    return tag, rows[0]


def build(imported, package, output, profile_path, report):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    imported = imported.resolve(strict=True)
    package = package.resolve(strict=True)
    output = output.resolve()
    if not output.is_relative_to(scratch) or output.exists():
        raise ValueError('Use a fresh output inside owned test scratch')
    profile = json.loads(profile_path.read_text())
    image_path = profile_path.parent / profile['preview']
    im = Image.open(image_path).convert('RGBA')
    assert im.size == (220, 207)
    pixels = im.tobytes('raw', 'BGRA')
    compressed = zlib.compress(pixels, 9)
    partition = struct.pack('<ii', 1, len(compressed)) + compressed
    before = {str(path): sha(path) for path in
              (imported / MENU, imported / 'halo2/h2_maps_win64_dx11/textures.dat',
               package / 'content/halo2/Mods/dire_valley/dire_valley.map')}
    shutil.copytree(imported, output, copy_function=os.link, symlinks=True)
    menu_path = output / MENU
    menu = MccCache(menu_path)
    globals_tag, globals_at = menu.tag(menu.u32(menu.index + 16), 'matg')
    rows = catalog(menu, globals_tag, globals_at + 0x178)
    assert not any(menu.u32(at) == profile['map_id'] for at in rows)
    stock = [bytes(menu.data[at:at + 0xc64]) for at in rows]
    # Original1A5590 populates an unused C64 row and its reserved preview bank.
    # Blank rows retain their full-salt bitmap owners even while map_id is NONE.
    free_index = next(i for i, at in enumerate(rows) if menu.u32(at) == 0xffffffff)
    free_at = rows[free_index]
    menu_bitmap = menu.u32(free_at + 8)
    slot, frame_at, frame = bitmap(menu, menu_bitmap)
    assert slot['name'].startswith('ui\\code_global_bitmaps\\multiplayer')
    image_at = slot['offset']
    template = bytes(menu.data[image_at:image_at + 76]) + frame
    assert len(template) == 244

    source = package / 'content/halo2/Mods/dire_valley/dire_valley.map'
    custom_relative = 'custom-maps/dire_valley'
    custom_root = output / custom_relative
    shutil.copytree(package / 'content', custom_root, copy_function=os.link)
    custom_path = custom_root / MAP
    custom = MccCache(source)
    scenario, row_at = scenario_card(custom)
    old_record = bytes(custom.data[row_at:row_at + 0xc64])
    card = bytearray(old_record)
    record_text(card, profile)
    assert summary(card)['scenario'] == profile['scenario_path']
    map_builder = AppendTags(custom)
    map_bitmap = map_builder.image(template, partition)
    struct.pack_into('<I', card, 8, map_bitmap)
    custom.data[row_at:row_at + len(card)] = card
    map_builder.finish(custom_path)
    bsp_hashes = {tag['name']: digest(custom.data[tag['offset']:tag['offset'] + tag['size']])
                  for tag in custom.tags.values() if tag['group'] in ('sbsp', 'ltmp')}
    custom.close()

    # MCC bitmaps read their declared shared textures.dat, including bank0.
    # Retain the stock bank byte-for-byte and append the new card in a COW copy.
    textures = output / 'halo2/h2_maps_win64_dx11/textures.dat'
    temporary = textures.with_name('textures.dat.new')
    subprocess.run(['cp', '--reflink=always', '--', str(textures), str(temporary)], check=True)
    with temporary.open('ab') as stream:
        stream.write(bytes((-stream.tell()) % 16))
        pixel_at = stream.tell()
        assert pixel_at + len(partition) < 0x40000000
        stream.write(partition)
    temporary.replace(textures)
    struct.pack_into('<I', menu.data, frame_at + 56, pixel_at)
    struct.pack_into('<I', menu.data, frame_at + 80, len(partition))
    struct.pack_into('<I', card, 8, menu_bitmap)
    card[4:12] = stock[free_index][4:12]
    menu.data[free_at:free_at + 0xc64] = card
    write_cache(menu, menu_path)
    menu.close()

    verified = MccCache(menu_path)
    tag, at = verified.tag(verified.u32(verified.index + 16), 'matg')
    rows = catalog(verified, tag, at + 0x178)
    assert len(rows) == len(stock)
    assert all(bytes(verified.data[p:p + 0xc64]) == stock[i]
               for i, p in enumerate(rows) if i != free_index)
    assert summary(bytes(verified.data[rows[free_index]:rows[free_index] + 0xc64])) == summary(card)
    bitmap(verified, menu_bitmap)
    verified.close()
    verified = MccCache(custom_path)
    tag, at = scenario_card(verified)
    assert verified.u32(at + 8) == map_bitmap
    assert all(digest(verified.data[tag['offset']:tag['offset'] + tag['size']]) == bsp_hashes[tag['name']]
               for tag in verified.tags.values() if tag['group'] in ('sbsp', 'ltmp'))
    verified.close()

    # Each custom cache keeps its own exact resource bank, including audio.
    # Sharing the stock bank would reinterpret the custom map's resource offsets.
    custom_maps = custom_root / 'halo2/h2_maps_win64_dx11'
    for name in ('textures.dat', 'sounds_en.dat', 'sounds_neutral.dat', 'sounds_remastered.dat'):
        (custom_maps / name).unlink()
        os.link(custom_path, custom_maps / name)
    bank_directory = 'native-shaders/maps/dire_valley'
    bank = bank_identity(bank_directory, (custom_root / bank_directory / 'library.json').read_bytes())
    custom_provenance = json.loads((custom_root / 'provenance.json').read_text())
    custom_provenance['source_files'][MAP.removeprefix('halo2/')] = dict(
        bytes=custom_path.stat().st_size, sha256=sha(custom_path))
    custom_provenance['native_shader_library'].pop('owner_bindings', None)
    (custom_root / 'native-shaders/map-owners.json').unlink()
    custom_provenance['menu_authoring_input_sha256'] = before[str(source)]
    # Follow the importer's existing ordered output chain. The stock menu and
    # its original program bank remain the same menu owner; this appends UI data.
    receipt = dict(map=MAP, input_bytes=custom_path.stat().st_size,
        input_sha256=sha(custom_path), source_sha256=sha(custom_path), cache_role='scenario',
        shared_directory='halo2/h2_maps_win64_dx11', native_mode='owned-bank', bank_owner_map=MAP,
        producer='author_dire_valley_menu.py', profile=PROFILE,
        **{key: bank[key] for key in ('bank_directory', 'bank_namespace', 'bank_library_sha256')})
    raw = (json.dumps(receipt, sort_keys=True) + '\n').encode()
    save(custom_root / 'custom-owner-receipt.json', receipt)
    custom_provenance = publish_staged_owner_index(custom_root, custom_provenance, custom_receipts=[raw])
    save(custom_root / 'provenance.json', custom_provenance)
    provenance = json.loads((output / 'provenance.json').read_text())
    for name in (MENU, 'halo2/h2_maps_win64_dx11/textures.dat'):
        provenance.setdefault('graphics_corrections', {}).setdefault('changes', []).append(dict(
            path=name, donor_sha256=before[str(imported / name)], converted_sha256=sha(output / name),
            producer='author_dire_valley_menu.py', profile=PROFILE))
    provenance['custom_menu'] = dict(profile=PROFILE, map_id=profile['map_id'],
        cache_root=custom_relative, screenshot_sha256=sha(image_path), source_inputs=before)
    provenance = publish_staged_owner_index(output, provenance)
    save(output / 'provenance.json', provenance)
    save(output / 'frontend-map-bindings.json', dict(schema='H2_FRONTEND_MAP_BINDINGS_V1', maps=[dict(
        map_id=profile['map_id'], scenario_path=profile['scenario_path'], cache_root=custom_relative,
        cache_file=custom_relative + '/' + MAP,
        shared_maps=custom_relative + '/halo2/h2_maps_win64_dx11')]))
    assert all(sha(Path(path)) == pin for path, pin in before.items())
    result = dict(profile=PROFILE, passed=True, output=str(output), original_inputs=before,
        original_inputs_unchanged=True, original_catalog_rows_preserved=len(stock) - 1,
        original_named_catalog_rows_preserved=sum(struct.unpack_from('<I', row)[0] != 0xffffffff for row in stock),
        map_card=summary(card), bitmap=dict(name=BITMAP_NAME, width=220, height=207,
            decoded_bgra_sha256=digest(pixels), menu_datum=hex(menu_bitmap),
            menu_slot=slot['name'], map_datum=hex(map_bitmap)),
        bsp_and_lightmap_metadata_unchanged=bsp_hashes,
        mainmenu_sha256=sha(menu_path), custom_map_sha256=sha(custom_path),
        unique_authored_bytes=menu_path.stat().st_size + custom_path.stat().st_size,
        scope='Authored menu/scenario metadata and screenshot. Existing programs, stock rows, geometry and lightmaps preserved. Native menu capture is a separate gate.')
    save(report, result)
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('imported', 'package', 'output', 'profile-path', 'report'):
        parser.add_argument('--' + option, type=Path, required=True)
    build(**vars(parser.parse_args()))
