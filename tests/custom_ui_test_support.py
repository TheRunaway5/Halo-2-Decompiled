"""Small authored-art contract fixtures, without claiming rendered visual proof."""
import hashlib
import json
from pathlib import Path

from custom_ui_assets import SOURCE, TAGS, SLOTS, runtime_sources, prepare as prepare_custom_ui
from export_vista_menu_ui import dds_image


def custom_ui_fixture(root):
    bank = Path(root) / SOURCE
    pages = {}
    for tag in TAGS:
        for frame in sorted(SLOTS):
            key = hashlib.sha256((tag + str(frame)).encode()).hexdigest()
            vector = b'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 2 2"><path d="M0 0H2V2H0Z"/></svg>'
            dds = dds_image(4, 4, 0, 11, bytes((frame, 64, 128, 255)) * 16)[0]
            page = dict(family='celestial-rank-illustration', logical=[2, 2], physical=[4, 4],
                vector='svg/' + key + '.svg', dds='bitmaps/' + key + '.dds',
                vector_sha256=hashlib.sha256(vector).hexdigest(), dds_sha256=hashlib.sha256(dds).hexdigest(),
                bindings=[dict(cache='mainmenu.map', name=tag, frame=frame,
                    dds_sha256=hashlib.sha256(('original ' + key).encode()).hexdigest(), registration=[1, 1])])
            for field, data in (('vector', vector), ('dds', dds)):
                path = bank / page[field]
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            pages[key] = page
    (bank / 'masters.json').write_text(json.dumps(dict(schema='halo2-vista-menu-masters-v1', complete=False, pages=pages)))


def custom_ui_menu_fixture(cache, root):
    metadata = json.loads(runtime_sources(root)[1]['presentation/custom-ranks/manifest.json'])
    pages = [dict(cache='mainmenu.map', name=page['name'], frame=page['frame'],
        width=page['logical'][0], height=page['logical'][1], dds_sha256=page['source_dds_sha256'])
        for page in metadata['pages']]
    path = Path(cache) / 'ui/mcc-menu-assets.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(dict(schema='H2_MCC_MENU_RESOURCE_CACHE_V1', complete=True, pages=pages)))


def custom_ui_prepare_fixture(root, cache, *, progress=print):
    """Replace only the unrelated map/menu producer, then run real rank setup."""
    custom_ui_menu_fixture(cache, root)
    return prepare_custom_ui(root, cache, progress=progress)


def empty_custom_map_fixture(root):
    path = Path(root) / 'resources/custom-maps.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(dict(schema='H2_BUNDLED_CUSTOM_MAPS_V1', maps=[])))
