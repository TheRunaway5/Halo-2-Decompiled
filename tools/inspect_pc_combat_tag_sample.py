#!/usr/bin/env python3
"""Read a bounded Vista/MCC tag sample against measured Xbox TU1.5 stores.

Original game files are read only. This compares stored PC metadata, not the
results of executing either PC engine or its map-load postprocessors.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zlib
from inspect_multiplayer import Cache as XboxCache

ROOT = Path(__file__).resolve().parents[1]
SCHEMAS = ROOT / 'tools/reference/assembly/src/Assembly/Plugins/Halo2'
SOURCE = ROOT / 'analysis/multiplayer/title-update-1.5/combat-postprocess/measured/report.json'
RECLAIMER = ROOT / 'tools/reference/reclaimer/Reclaimer.Blam/Blam/Halo2'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


class PcCache:
    def __init__(self, path):
        self.path = path.resolve()
        with self.path.open('rb') as stream:
            raw = mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ)
            try:
                version = struct.unpack_from('<I', raw, 4)[0]
                assert raw[:4] == b'daeh' and version in (8, 13)
                self.mcc = version == 13
                chunks = []
                if self.mcc:
                    chunk_size, _, table, count = struct.unpack_from('<4I', raw, 776)
                    assert 0 < count < 65536 and table + count * 8 <= len(raw)
                    data = bytearray(raw[:896])
                    for i in range(count):
                        size, address = struct.unpack_from('<ii', raw, table + i * 8)
                        assert address >= 0
                        if size >= 0:
                            assert address + size <= len(raw)
                            block = zlib.decompress(raw[address:address + size])
                        else:
                            assert address + 2 - size <= len(raw)
                            block = raw[address + 2:address + 2 - size]
                        assert 0 < len(block) <= chunk_size
                        chunks.append(dict(address=address, stored_size=size, decoded_size=len(block)))
                        data.extend(block)
                    self.data = data
                else:
                    self.data = bytes(raw)
            finally:
                raw.close()
        assert self.u(8) == len(self.data)
        self.index = self.u(16)
        self.magic = self.u(720 if self.mcc else 32) - self.index
        table = self.u(self.index + 8) - (self.u(self.index) - self.index - 32)
        names = self.u(36 if self.mcc else 720)
        offsets = self.u(44 if self.mcc else 728)
        count = self.u(self.index + 24)
        assert table + count * 16 <= len(self.data)
        assert offsets + count * 4 <= len(self.data)
        self.tags = {}
        for i in range(count):
            at = table + i * 16
            datum, address, size = struct.unpack_from('<III', self.data, at + 4)
            if datum == 0xffffffff:
                continue
            p = names + self.u(offsets + i * 4)
            assert 0 <= p < len(self.data)
            end = self.data.find(b'\0', p)
            assert end >= p
            name = bytes(self.data[p:end]).decode('utf-8')
            group = bytes(self.data[at:at + 4][::-1]).decode('ascii')
            offset = address - self.magic if address else None
            if address:
                assert 0 <= offset <= offset + size <= len(self.data)
            self.tags[i] = dict(index=i, datum=f'{datum:08x}', group=group,
                                name=name, offset=offset, size=size)
        self.provenance = dict(path=str(self.path), sha256=sha(self.path),
                               version=version, decoded_size=len(self.data),
                               decoded_sha256=hashlib.sha256(self.data).hexdigest(),
                               chunks=chunks)

    def u(self, at):
        return struct.unpack_from('<I', self.data, at)[0]

    def named(self, group, name):
        matches = [t for t in self.tags.values() if t['group'] == group and t['name'] == name]
        assert len(matches) == 1, (self.path, group, name, len(matches))
        return matches[0]

    def field(self, tag, annotation):
        base = tag['offset']
        assert base is not None
        node = ET.parse(SCHEMAS / (tag['group'] + '.xml')).getroot()
        for part in annotation['name'].split('/'):
            match = re.fullmatch(r'(.*)\[(\d+)\]', part)
            if match:
                label, index = match.group(1), int(match.group(2))
                element = next(e for e in node if e.tag == 'tagblock' and e.get('name') == label)
                at = base + int(element.get('offset'), 0)
                assert index < self.u(at)
                base = self.u(at + 4) - self.magic + index * int(element.get('elementSize'), 0)
                node = element
            else:
                element = next(e for e in node if e.get('name') == part and
                               int(e.get('offset'), 0) == annotation['schema_leaf_offset'])
                at = base + int(element.get('offset'), 0) + annotation['field_relative_offset']
                assert tag['offset'] <= at <= at + 4 <= tag['offset'] + tag['size']
                return at, bytes(self.data[at:at + 4])
        raise AssertionError(annotation)


def locate_original_leaf(cache, tag, annotation):
    """Disambiguate repeated field names using the original measured address."""
    base = tag['offset']
    node = ET.parse(SCHEMAS / (tag['group'] + '.xml')).getroot()
    for part in annotation['name'].split('/'):
        match = re.fullmatch(r'(.*)\[(\d+)\]', part)
        if match:
            label, index = match.group(1), int(match.group(2))
            element = next(e for e in node if e.tag == 'tagblock' and e.get('name') == label)
            at = base + int(element.get('offset'), 0)
            assert index < cache.u(at)
            base = cache.u(at + 4) - cache.magic + index * int(element.get('elementSize'), 0)
            node = element
        else:
            matches = [e for e in node if e.get('name') == part and
                       base + int(e.get('offset'), 0) == annotation['start']]
            assert len(matches) == 1, (tag, annotation)
            return annotation | {'schema_leaf_offset': int(matches[0].get('offset'), 0)}
    raise AssertionError(annotation)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vista-maps', type=Path, default=ROOT / 'Halo 2 Vista installer/maps')
    parser.add_argument('--mcc-maps', type=Path, default=Path('/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/h2_maps_win64_dx11'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    original = json.loads(SOURCE.read_text())
    image = next(r for r in original['images'] if r['image_label'] == 'owned_1_5')
    stores = [r for r in image['ordered_stores'] if r['region'] == 'original_cache_metadata']
    assert len(stores) == 41 and all(r['size'] == 4 for r in stores)
    xbox = XboxCache(ROOT / 'assets/disc/maps/lockout.map')
    try:
        assert sha(xbox.path) == original['cache_sha256']
        annotations = [locate_original_leaf(xbox, s['tag'], s['schema_annotations'][0]) for s in stores]
    finally:
        xbox.data.close()
        xbox.stream.close()
    result = dict(
        scope='41 scalar destinations of the measured Xbox TU1.5 combat postprocessor; raw Lockout-visible PC tags resolved through the actual shared tag indices. Not a full tag-set comparison or execution of either PC engine.',
        limitations=['The supplied Vista archive includes Cartographer components; pristine retail provenance is unverified.',
                     'PC executable postprocessing and gameplay behavior were not executed.',
                     'These targeted fields do not rank overall tag compatibility, artwork or complete balance parity.'],
        tool_sha256=sha(Path(__file__)), xbox_report_sha256=sha(SOURCE),
        xbox_executable_sha256=image['xbe_sha256'], schemas={}, reader_sources={}, platforms={})
    for path in [RECLAIMER / 'Config/CacheFile.config.cs', RECLAIMER / 'MccCompressedCacheStream.cs',
                 RECLAIMER / 'TagAddressTranslator.cs', RECLAIMER / 'HeaderAddressTranslator.cs',
                 RECLAIMER / 'CacheFile.cs',
                 ROOT / 'tools/reference/assembly/src/Blamite/Formats/Engines.xml']:
        result['reader_sources'][str(path.relative_to(ROOT))] = sha(path)
    for label, root in [('supplied_vista', args.vista_maps), ('installed_mcc', args.mcc_maps)]:
        level, shared = PcCache(root / 'lockout.map'), PcCache(root / 'shared.map')
        rows = []
        for store, annotation in zip(stores, annotations):
            group, name = store['tag']['group'], store['tag']['name']
            entry = level.named(group, name)
            owner, tag = level, entry
            if entry['offset'] is None:
                owner, tag = shared, shared.tags[entry['index']]
                assert (tag['group'], tag['name'], tag['datum']) == (group, name, entry['datum'])
            assert len(store['schema_annotations']) == 1
            offset, value = owner.field(tag, annotation)
            schema_path = SCHEMAS / (group + '.xml')
            result['schemas'][str(schema_path.relative_to(ROOT))] = sha(schema_path)
            rows.append(dict(tag_group=group, tag_name=name, field=annotation['name'],
                             component_offset=annotation['field_relative_offset'],
                             schema_leaf_offset=annotation['schema_leaf_offset'],
                             level_entry=entry, owner_path=str(owner.path), owner_tag=tag,
                             decoded_file_offset=offset, pc_raw_hex=value.hex(),
                             pc_raw_float=struct.unpack('<f', value)[0],
                             xbox_raw_hex=store['before_hex'], xbox_raw_float=store['before_float'],
                             xbox_processed_hex=store['after_hex'], xbox_processed_float=store['after_float'],
                             equal_to_xbox_processed=value.hex() == store['after_hex']))
        result['platforms'][label] = dict(inputs=[level.provenance, shared.provenance],
                                          compared_fields=len(rows),
                                          matching_fields=sum(r['equal_to_xbox_processed'] for r in rows),
                                          rows=rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + '\n')
    print(json.dumps({k: dict(compared=v['compared_fields'], matching=v['matching_fields'])
                      for k, v in result['platforms'].items()}))


if __name__ == '__main__':
    main()
