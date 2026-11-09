"""One required receipt for the port's custom maps, modes, buttons and ranks.

The source catalog and its authenticated importers own the payloads. This
receipt binds their installed results together; it grants no engine capability.
"""
import hashlib
import json
from pathlib import Path
import os
import uuid

from audit_rendering import FormatError
from installation_paths import resource_root
from mcc_movies import _owned

SCHEMA = 'H2_PORT_CUSTOM_ASSET_CACHE_V1'
MANIFEST = 'custom-asset-cache.json'
CONTRACT = 'resources/custom-asset-bundle.json'
POLICY = dict(button_pixels='port-authored-only', mcc_button_fallback=False,
              celestial_ranks='port-authored-only', numeric_ranks='MCC-original')


def contract(root):
    """The required source cohort is independent of the map delivery catalog."""
    from custom_map_assets import source_maps
    from native_variant_assets import DEFINITIONS
    source = resource_root(root)
    path = _owned(source/CONTRACT, source)
    if not path.is_file() or path.stat().st_size > 65536:
        raise FormatError('Required custom asset bundle contract is missing')
    value = json.loads(path.read_bytes())
    if (not isinstance(value, dict) or value.get('schema') != 'H2_PORT_CUSTOM_ASSET_BUNDLE_V1' or
            not isinstance(value.get('version'), str) or not value['version'] or
            value.get('maps') != [row['id'] for row in source_maps(root)] or
            value.get('game_modes') != list(DEFINITIONS) or
            value.get('controller_families') != ['xbox','playstation'] or
            value.get('celestial_slots') != list(range(43,50)) or value.get('policy') != POLICY):
        raise FormatError('Custom asset bundle contract differs from required maps, modes or artwork policy')
    return value


def _pin(path):
    before = path.stat()
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    after = path.stat()
    if (before.st_size, before.st_mtime_ns, before.st_ino) != (
            after.st_size, after.st_mtime_ns, after.st_ino):
        raise FormatError('Custom bundle changed during verification')
    return dict(bytes=after.st_size, sha256=digest)


def expected(root, report):
    contract(root)
    from controller_prompt_assets import runtime_sources as prompts
    from custom_ui_assets import runtime_sources as ranks
    from custom_map_assets import source_maps, RECEIPT, BINDINGS
    from native_variant_assets import source_bundle, DEFINITIONS, MANIFEST as VARIANTS, SCHEMA as VARIANT_SCHEMA
    prompt_entry = prompts(root)[0]
    rank_entry = ranks(root)[0]
    if report.get('controller_prompts') != prompt_entry or report.get('custom_ui_assets') != rank_entry:
        raise FormatError('Custom buttons or celestial ranks need Rebuild Cache')
    map_rows = source_maps(root)
    maps = [row['id'] for row in map_rows]
    declaration = report.get('bundled_custom_maps', {})
    if maps and (declaration.get('complete') is not True or declaration.get('maps') != maps):
        raise FormatError('Custom maps need Rebuild Cache')
    variants = {name: dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
                for name, raw in source_bundle(root).items()}
    variant_receipt = dict(schema=VARIANT_SCHEMA, files=variants,
                          provenance='Newly authored native variants; no extracted game assets')
    encoded = (json.dumps(variant_receipt, indent=2, sort_keys=True) + '\n').encode()
    files = {**prompt_entry['files'], **rank_entry['files'], **variants,
             VARIANTS: dict(bytes=len(encoded), sha256=hashlib.sha256(encoded).hexdigest())}
    if maps:
        files.update({RECEIPT: declaration['receipt'], BINDINGS: declaration['bindings']})
    from custom_map_assets import MANIFEST as CATALOG
    return dict(schema=SCHEMA, complete=True, policy=POLICY, maps=maps,
                bundle_contract=_pin(_owned(resource_root(root)/CONTRACT, resource_root(root))),
                game_modes=list(DEFINITIONS), controller_families=['xbox', 'playstation'],
                celestial_slots=list(range(43, 50)), files=files,
                map_sources={row['id']: {name: _pin(_owned(resource_root(root) / row['source'] / name,
                                                         resource_root(root)))
                                        for name in (row['profile'], 'content/custom-owner-receipt.json',
                                                     *([row['source_receipt']] if 'source_receipt' in row else []))}
                             for row in map_rows},
                map_catalog=_pin(_owned(resource_root(root) / CATALOG, resource_root(root))))


def _verify_files(cache, entry):
    for name, record in entry['files'].items():
        path = _owned(Path(cache) / name, cache)
        if not path.is_file() or _pin(path) != record:
            raise FormatError('Custom asset missing or changed; select Rebuild Cache: ' + name)


def _verify_maps(cache, report, entry):
    if entry['maps']:
        from custom_map_assets import verify as verify_maps
        verify_maps(cache, report)


def publish(root, cache, report):
    """Called only after all importers succeed, before selecting this cache."""
    cache = Path(cache)
    entry = expected(root, report)
    _verify_files(cache, entry)
    _verify_maps(cache, report, entry)
    path = _owned(cache / MANIFEST, cache)
    raw = (json.dumps(entry, indent=2, sort_keys=True) + '\n').encode()
    if path.exists() and path.read_bytes() == raw:
        return entry
    pending = path.with_name('.custom-cache-' + uuid.uuid4().hex)
    try:
        pending.write_bytes(raw)
        os.replace(pending, path)
    finally:
        pending.unlink(missing_ok=True)
    return entry


def verify(root, cache, report):
    """Read-only startup check: no missing group may silently use MCC art."""
    path = _owned(Path(cache) / MANIFEST, cache)
    if not path.is_file() or path.stat().st_size > 1024**2:
        raise FormatError('Custom asset bundle is missing; select Rebuild Cache')
    entry = json.loads(path.read_bytes())
    if entry != expected(root, report):
        raise FormatError('Custom asset bundle is outdated; select Rebuild Cache')
    _verify_files(cache, entry)
    _verify_maps(cache, report, entry)
    return entry
