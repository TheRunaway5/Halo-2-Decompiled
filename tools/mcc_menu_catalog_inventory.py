"""Strict inventory admission for the two authored MCC catalog preview pages."""
import struct

from mcc_multiplayer_catalog import PROFILE, TARGETS, bitmap, catalog, digest, require, summary

# Independently decoded MCC map-shot DDS hashes. Their raw BGRA payloads also
# match the owned Xbox v3 catalog pixels offline (work/.../v533.json). The stock
# menu inventory remains unchanged and its blank-page comparison is retained.
STOCK_DDS = '5b3bfa1a1e53adf1320cf49d111e59b2fc3fa181e60cd93430d30c49692e3f24'
BLANK_RESOURCE = dict(pointer=0x02399A80, stored_bytes=1986, decoded_bytes=182160,
    stored_sha256='607ae2946f19f452701378ddea49891064f0175da25a9e7828f516bf091e7768',
    sha256='eb0d2150edff9063576d2f37d04d2b2786889362318cbc9cc86cb3a489ca2db8')
PAGES = (
    dict(path=r'scenarios\multi\halo\highplains\highplains', map_id=1402, old_id=1239,
         datum='e89e05a8', name=r'ui\code_global_bitmaps\multiplayer23',
         source_datum='e9530113', source_name=r'ui\global_bitmaps\map_shots\map_mp_highplains',
         source_cache_sha256='d6ea847433ba564e20100194c027cacc69f1f1ef85697fffe8463c58c6a0f02f',
         record_sha256='cf4d8b45be0bc623b942b7c1fb3123b6edfe2c239a58b46c3719e3fdd05e244a',
         pointer=0x1FCC8600, stored_bytes=158459,
         stored_sha256='610943a8e8468b5c8ac33b468b812831a1f28c076fdb5e19425bc4e774f151a5',
         pixel_sha256='f79b27ea8935a4e1e1b0f42f553e0e657fcf23f1dcc5466cd8057a1068031ca6',
         dds_sha256='6d96f8067ed34ab2dfc581c0bcb746c29d7e8c5715e79a13d91a0a59ef8fd060'),
    dict(path=r'scenarios\multi\halo\derelict\derelict', map_id=1400, old_id=1240,
         datum='e89f05a9', name=r'ui\code_global_bitmaps\multiplayer24',
         source_datum='e8f70140', source_name=r'ui\global_bitmaps\map_shots\map_mp_derelict',
         source_cache_sha256='45efa1376a59c19b49945b12235e3d8fa33811582151452098affd0609aae6cc',
         record_sha256='ba8fefaf1e04e842b22352b548cf7ee8093d9a0ad4aedbda3c2de96bacb9e53c',
         pointer=0x1F301C00, stored_bytes=153929,
         stored_sha256='ff586c62b8cb8f8dba4417e4776f6d16be0e789ab8695f6e4851ec177df56a88',
         pixel_sha256='04a26aee21323e227bf67f12121cfb1e81ca04139cac34c3de26de67d3ff7177',
         dds_sha256='5c05c9d6f9ffd565342bbc9f7d90c6844cb75b467d04d2391de0f70b52e468a8'),
)


def qualified_pages(cache, projection):
    """Recheck complete current C64/frame owners, not arbitrary report DDS data."""
    require(isinstance(projection, dict) and projection.get('profile') == PROFILE,
            'inventory requires the exact authored catalog projection profile')
    require(projection.get('serialized_decoded_sha256') == digest(cache.data),
            'inventory projection does not own the current serialized mainmenu')
    records = projection.get('records')
    require(isinstance(records, list) and len(records) == 2, 'inventory requires both complete projected records')
    require(all(isinstance(row, dict) and isinstance(row.get('after'), dict) for row in records),
            'inventory projected records are malformed')
    globals_tag, at = cache.tag(cache.u32(cache.index + 16), 'matg')
    entries = catalog(cache, globals_tag, at + 0x178)
    output = {}
    for spec in PAGES:
        selected = [p for p in entries if cache.string(p + 0xB4C, 256) == spec['path']]
        require(len(selected) == 1, 'inventory projected scenario identity is missing or ambiguous')
        current = bytes(cache.data[selected[0]:selected[0] + 0xC64])
        require(digest(current) == spec['record_sha256'], 'inventory current whole projected C64 definition differs')
        actual = summary(current)
        matching = [r for r in records if isinstance(r, dict) and r.get('after', {}).get('scenario') == spec['path']]
        require(len(matching) == 1, 'inventory projection record identity is missing or ambiguous')
        row = matching[0]
        pins = TARGETS[spec['path']]
        require(row.get('after') == actual and row.get('source_record_sha256') == pins[1] and
                row.get('projected_record_sha256') == spec['record_sha256'] and
                row.get('source_cache_sha256') == spec['source_cache_sha256'],
                'inventory authored source record/provenance differs')
        require(row.get('state') in ('stock-placeholder', 'already-converted'), 'inventory projection state is unqualified')
        if row['state'] == 'stock-placeholder':
            old = dict(actual, map_id=spec['old_id'], titles=[''] * 9,
                       descriptions=[''] * 9, sort_order=0, flags=0, team_counts=[0, 2] + [8] * 14)
            require(row.get('before') == old and row.get('before_record_sha256') == pins[0] and
                    row.get('prior_resource') == BLANK_RESOURCE,
                    'inventory original stock owner/provenance differs')
        else:
            require(row.get('before') == actual and row.get('before_record_sha256') == spec['record_sha256'],
                    'inventory completed projection before-image differs')
        datum = cache.u32(selected[0] + 8)
        require(f'{datum:08x}' == spec['datum'], 'inventory projected bitmap namespace differs')
        tag, frame_at, frame = bitmap(cache, datum)
        require(tag['name'] == spec['name'] and
                row.get('destination_bitmap') == dict(datum=spec['datum'], tag=spec['name'], frame_offset=frame_at),
                'inventory destination bitmap owner differs')
        source_bitmap = row.get('source_bitmap', {})
        require(isinstance(source_bitmap, dict) and source_bitmap.get('datum') == spec['source_datum'] and source_bitmap.get('tag') == spec['source_name'],
                'inventory source bitmap namespace differs')
        original = bytearray(frame)
        require(struct.unpack_from('<I', frame, 56)[0] == spec['pointer'] and
                struct.unpack_from('<I', frame, 80)[0] == spec['stored_bytes'],
                'inventory projected preview partition differs')
        struct.pack_into('<I', original, 56, BLANK_RESOURCE['pointer'])
        struct.pack_into('<I', original, 80, BLANK_RESOURCE['stored_bytes'])
        require(digest(original) == pins[2], 'inventory whole destination frame/registration differs')
        require(row.get('registration_point') == dict(destination=[110, 103], source=[110, 104], preserved=True),
                'inventory projection changed registration ownership')
        expected_resource = dict(pointer=spec['pointer'], stored_bytes=spec['stored_bytes'],
            stored_sha256=spec['stored_sha256'], decoded_bytes=182160, sha256=spec['pixel_sha256'])
        require(row.get('authored_resource') == expected_resource, 'inventory authored preview resource provenance differs')
        output[(spec['name'], 0)] = spec
    return output


def projected_page(spec, row, actual_dds_sha256, actual_pixel_sha256):
    require(row['mcc_cache'] == 'mainmenu.map' and row['mcc_name'] == spec['name'] and
            int(row['mcc_frame']) == 0 and (int(row['width']), int(row['height'])) == (220, 207) and
            row['dds_sha256'] == STOCK_DDS and row['status'] == 'dds-exact',
            'inventory original stock page identity differs')
    require(actual_dds_sha256 == spec['dds_sha256'] and actual_pixel_sha256 == spec['pixel_sha256'],
            'inventory converted DDS/pixels differ from authenticated authored MCC data')
    return dict(source_comparison='mcc-authored-catalog-projection',
                original_stock_dds_sha256=row['dds_sha256'], original_source_comparison=row['status'],
                authored_pixel_sha256=actual_pixel_sha256,
                catalog_projection=dict(profile=PROFILE, map_id=spec['map_id'], scenario=spec['path'],
                    source_bitmap=spec['source_name'], source_bitmap_datum=spec['source_datum'],
                    source_cache_sha256=spec['source_cache_sha256'], source_record_sha256=TARGETS[spec['path']][1],
                    projected_record_sha256=spec['record_sha256']))
