"""MCC-only launcher source selection and verified private-cache reuse.

Selecting a prepared cache does not declare its unfinished conversion playable.
Original Xbox/Vista source registries are never read by this onboarding path.
"""
import json
import os
from pathlib import Path, PurePosixPath
import uuid
import shutil

from audit_rendering import FormatError, file_sha
from mcc_content import directory, discover
from mcc_conversion_coverage import validate as validate_conversion_coverage
from prepare_mcc_gameplay import PROFILE, prepare, source_files, source_directories
from mcc_movies import _cancellation

SCHEMA = "H2_MCC_ASSET_SOURCE_V1"
REGISTRY = "assets/mcc-source.tsv"
REQUIRED = (
    "halo2/h2_maps_win64_dx11/mainmenu.map", "halo2/h2_maps_win64_dx11/shared.map",
    "halo2/h2_maps_win64_dx11/single_player_shared.map", "halo2/h2_maps_win64_dx11/lockout.map",
    "halo2/h2_maps_win64_dx11/textures.dat", "halo2/h2_fonts/font_table.txt",
    "bitmap-resources.json", "tag-conversion.json", "loading/mcc-loading-assets.json",
    "provenance.json",
)
NOT_PLAYABLE = "MCC content is cached. Native gameplay loading and complete Xbox TU1.5 conversion are still being completed."


def private_cache(root, cache, *, verify=False, source=None, progress=print):
    root, cache = Path(root).resolve(), Path(cache)
    if cache.is_symlink():
        raise FormatError("The MCC cache must be an owned directory, not a symlink")
    cache = cache.resolve()
    if cache.parent != root / "assets/imported" or cache.name.startswith("."):
        raise FormatError("Select a completed private MCC cache under assets/imported")
    for name in REQUIRED:
        path = cache / name
        if not path.is_file() or path.is_symlink() or not path.resolve().is_relative_to(cache):
            raise FormatError("MCC cache is incomplete: " + name)
    report = json.loads((cache / "provenance.json").read_text())
    if not isinstance(report, dict) or report.get("schema") != PROFILE or report.get("external_installation_requirement") != "Halo MCC only":
        raise FormatError("Selected cache is not an MCC-only preparation")
    if verify:
        records = report.get("source_files")
        if not isinstance(records, dict) or not records:
            raise FormatError("MCC cache has no verified source-file inventory")
        copied = cache / "halo2"
        cached_files, cached_omitted = source_files(copied)
        if {p.as_posix() for p, _ in cached_files} != set(records) or cached_omitted:
            raise FormatError("The private MCC file inventory has changed")
        if [p.as_posix() for p in source_directories(copied)] != report.get("source_directories", []):
            raise FormatError("The private MCC directory tree has changed")
        if source is not None:
            files, omitted = source_files(source)
            if {p.as_posix() for p, _ in files} != set(records) or omitted != report.get("excluded_files", []):
                raise FormatError("The installed MCC asset set has changed")
            if [p.as_posix() for p in source_directories(source)] != report.get("source_directories", []):
                raise FormatError("The installed MCC directory tree has changed")
        conversion = json.loads((cache / "tag-conversion.json").read_text())
        output_hashes, coverage = validate_conversion_coverage(records, conversion)
        if "conversion_coverage" in report and report["conversion_coverage"] != coverage:
            raise FormatError("MCC conversion coverage differs from its copied-source owners")
        changes = report.get("output_changes", []) + report.get("graphics_corrections", {}).get("changes", [])
        for change in changes:
            relative = PurePosixPath(change["path"])
            name = relative.relative_to("halo2").as_posix()
            if name not in records or change["donor_sha256"] != output_hashes.get(name, records[name]["sha256"]):
                raise FormatError("MCC graphics conversion does not match the copied donor")
            output_hashes[name] = change["converted_sha256"]
        progress(f"Checking the existing MCC cache ({len(records)} copied files)…")
        for index, (name, expected) in enumerate(records.items()):
            relative = PurePosixPath(name)
            if relative.is_absolute() or any(p in ("", ".", "..") for p in relative.parts) or "\\" in name:
                raise FormatError("Invalid MCC cached relative filename")
            path = cache / "halo2" / relative
            if path.is_symlink() or not path.is_file() or not path.resolve().is_relative_to(cache):
                raise FormatError("MCC cached file lost its private owner: " + name)
            digest = output_hashes.get(name, expected["sha256"])
            if file_sha(path) != digest:
                raise FormatError("MCC cached file changed: " + name)
            if name not in output_hashes and path.stat().st_size != expected["bytes"]:
                raise FormatError("MCC cached file extent changed: " + name)
            if source is not None and file_sha(source / relative) != expected["sha256"]:
                raise FormatError("The installed MCC file has changed: " + name)
            if index and index % 100 == 0:
                progress(f"Checked {index}/{len(records)} MCC files…")
        if source is not None:
            # Do not publish a reused cache if an update changed the installation
            # during verification, including files already hashed above.
            after, final_omitted = source_files(source)
            stamp = lambda rows: [(p, s.st_size, s.st_mtime_ns, s.st_ino) for p, s in rows]
            if stamp(files) != stamp(after) or omitted != final_omitted or (
                    [p.as_posix() for p in source_directories(source)] != report["source_directories"]):
                raise FormatError("The installed MCC content changed during verification")
        from mcc_native_map_owner_publication import verify_owner_index
        verify_owner_index(cache, report)
        if 'custom_ui_assets' in report:
            from custom_ui_assets import verify as verify_custom_ui
            verify_custom_ui(cache, report['custom_ui_assets'])
        if 'bundled_custom_maps' in report:
            from custom_map_assets import verify as verify_custom_maps
            verify_custom_maps(cache, report)
        if report.get('h2ek_loading_ambience', {}).get('complete') is True:
            from h2ek_loading_ambience import verify as verify_h2ek_ambience
            verify_h2ek_ambience(cache, report['h2ek_loading_ambience'])
    return cache, report


def selected(root):
    root = Path(root).resolve()
    registry = root / REGISTRY
    if not registry.is_file():
        return None
    lines = registry.read_text().splitlines()
    if len(lines) != 2 or lines[0] != SCHEMA or not lines[1].startswith("cache\t"):
        raise FormatError("Invalid MCC cache selection")
    value = lines[1][6:]
    if not value or any(c in value for c in "\t\r\n\0"):
        raise FormatError("Invalid MCC cache selection path")
    path = Path(value)
    return private_cache(root, root / path if not path.is_absolute() else path)


def select(root, cache, *, verify=True, progress=print):
    root = Path(root).resolve()
    cache, report = private_cache(root, cache, verify=verify, progress=progress)
    from custom_map_assets import compatible as custom_maps_compatible
    if not custom_maps_compatible(root, cache, report):
        raise FormatError("The selected cache needs the bundled custom maps; run MCC setup to prepare a fresh cache. Existing files were retained.")
    from custom_ui_assets import repair as repair_custom_ui
    cache, report = repair_custom_ui(root, cache, progress=progress)
    # Selecting an already prepared cache is also an onboarding entry point.
    # Import/repair the authenticated kit bank before publishing the selection,
    # including when the kit was installed after the original MCC import.
    from h2ek_loading_ambience import repair as repair_h2ek_ambience
    cache, report = repair_h2ek_ambience(root, cache, progress=progress)
    from native_variant_assets import install as install_variants
    install_variants(root, cache, progress=progress)
    install_variants(root, root, progress=progress)
    relative = cache.relative_to(root).as_posix()
    registry = root / REGISTRY
    registry.parent.mkdir(parents=True, exist_ok=True)
    pending = registry.with_name(registry.name + "." + uuid.uuid4().hex + ".tmp")
    try:
        pending.write_text(SCHEMA + "\ncache\t" + relative + "\n")
        os.replace(pending, registry)
    finally:
        pending.unlink(missing_ok=True)
    progress("Selected private MCC cache: " + str(cache))
    return cache, report


def runtime_status(root):
    try:
        value = selected(root)
    except (OSError, ValueError, KeyError):
        return "The MCC cache needs preparation. Open setup to scan the installed game."
    if value is None:
        return "MCC has not been imported yet. Open setup to scan the installed game."
    report = value[1]
    if report.get("runtime_active") is not True or report.get("full_tu15_parity") is not True:
        return NOT_PLAYABLE
    return None


def preview_maps(root):
    """Owned imported map choices for the native asset viewer, not gameplay admission."""
    try:
        value = selected(root)
    except (OSError, ValueError, KeyError):
        return {}
    if value is None:
        return {}
    owner = value[0] / "halo2"
    stock_maps = owner / "h2_maps_win64_dx11"
    result = {}
    for path in sorted(owner.rglob("*"), key=lambda p: p.as_posix().casefold()):
        if path.suffix.casefold() != ".map":
            continue
        if path.parent == stock_maps and path.stem.casefold() in (
                "shared", "single_player_shared", "mainmenu"):
            continue
        if path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(owner):
            result[path.relative_to(owner).as_posix()] = path
    return result


def missing_shader_scalar_tables(destination, catalog):
    """Read an authenticated existing LUT for an older cache's common owner."""
    from native_shader_bundle import TABLE_SHA, digest, identity

    def read_tables(path):
        if path.is_symlink() or not path.is_file():
            raise FormatError('Retained scalar function tables require a private file owner')
        if path.stat().st_size != 28 * 1024:
            raise FormatError('Retained scalar function tables have changed; existing files were retained')
        tables = path.read_bytes()
        if digest(tables) != TABLE_SHA:
            raise FormatError('Retained scalar function tables have changed; existing files were retained')
        return tables

    common = destination / 'function-tables.bin'
    if common.exists() or common.is_symlink():
        read_tables(common)
        return None
    rows = catalog.get('maps')
    if not isinstance(rows, list):
        raise FormatError('Invalid cached native shader catalog records')
    for row in rows:
        if not isinstance(row, dict):
            raise FormatError('Invalid cached native shader catalog record')
        identity(row.get('map'))
    for row in sorted(rows, key=lambda row: row['map']):
        if row.get('complete') is not True:
            continue
        owner = destination / row['map']
        if owner.is_symlink():
            raise FormatError('Retained scalar function tables require a private directory owner')
        candidate = owner / 'function-tables.bin'
        if candidate.exists() or candidate.is_symlink():
            return read_tables(candidate)
    raise FormatError('Retained scalar function tables are unavailable; repair this port installation')


def publish_shader_scalar_tables(destination, tables):
    """Publish one complete absent common file without replacing any bank."""
    from mcc_native_map_owner_publication import _publish_absent
    temporary = destination / ('.function-tables-' + uuid.uuid4().hex)
    try:
        temporary.write_bytes(tables)
        _publish_absent(temporary, destination / 'function-tables.bin')
    except FileExistsError as error:
        raise FormatError('Common scalar function tables changed during repair; retry setup') from error
    finally:
        temporary.unlink(missing_ok=True)


def _shader_beforeimage(path, expected):
    if expected is None:
        if path.exists() or path.is_symlink():
            raise FormatError('Native repair destination changed; existing files retained: ' + str(path))
    elif path.is_symlink() or not path.is_file() or path.read_bytes() != expected:
        raise FormatError('Native repair metadata beforeimage changed: ' + str(path))


def _publish_shader_change(staged, destination, previous, temporary, journal):
    """Journal before rename, including SIGTERM between syscall and return."""
    from mcc_native_map_owner_publication import _publish_absent
    _shader_beforeimage(destination, previous)
    backup = None
    if previous is not None:
        backup = temporary / ('metadata-' + uuid.uuid4().hex + '.previous')
        backup.write_bytes(previous)
    value = staged.stat()
    journal.append((staged, destination, backup, (value.st_dev, value.st_ino)))
    if previous is None:
        _publish_absent(staged, destination)
    else:
        os.replace(staged, destination)


def _rollback_shader_changes(journal):
    errors = []
    for staged, destination, backup, identity in reversed(journal):
        # An unconsumed staging name means its rename did not take effect.
        if staged.exists():
            continue
        try:
            value = destination.lstat()
            if destination.is_symlink() or (value.st_dev, value.st_ino) != identity:
                raise FormatError('Published repair file changed before rollback: ' + str(destination))
            if backup is None:
                destination.rename(staged)
            else:
                os.replace(backup, destination)
        except BaseException as error:
            errors.append(str(error))
    return errors


@_cancellation()
def repair_cache_shaders(root, cache, report, *, progress=print):
    """Repair native metadata in one exception/cancellation rollback boundary.

    Cairo, catalog, an absent common LUT, owner index and provenance share the
    same beforeimage. These sequential renames are not a cross-file crash-atomic
    or concurrent-writer guarantee. Existing MCC/program/custom bytes and all
    gameplay admission flags remain unchanged.
    """
    from mcc_native_map_owner_publication import verify_owner_index, prepare_owner_index_refresh, _read
    from preserve_native_shaders import copy_selected
    root, cache = Path(root), Path(cache)
    # Authenticate declared bindings before changing any closure input. A
    # no-index shader-only legacy fixture remains compatible, but prepared MCC
    # reports with stock banks must derive the real final-input owner metadata.
    beforeimage = verify_owner_index(cache, report)
    provenance = cache / 'provenance.json'
    old_provenance, _ = _read(cache, 'provenance.json', [])
    if json.loads(old_provenance) != report:
        raise FormatError('Native repair provenance changed before preparation')
    destination = cache / 'native-shaders'
    if destination.is_symlink():
        raise FormatError('The native shader library must be an owned directory')
    temporary = cache / ('.native-shaders-' + uuid.uuid4().hex)
    temporary.mkdir()
    journal = []
    committed = False
    recovery_required = False
    try:
        updated, next_catalog, staged_source, source = report, None, None, None
        if destination.exists():
            catalog_path = destination / 'catalog.json'
            if catalog_path.is_symlink():
                raise FormatError('The native shader catalog must have a private owner')
            catalog_bytes = catalog_path.read_bytes()
            catalog = json.loads(catalog_bytes)
            if catalog.get('schema') != 'H2_PRESERVED_NATIVE_SHADER_CATALOG_V1':
                raise FormatError('Invalid cached native shader catalog')
            tables = missing_shader_scalar_tables(destination, catalog)
            updated, next_catalog, staged_source, source = _stage_shader_closure(
                root, cache, report, catalog, temporary, progress=progress)
            _shader_beforeimage(provenance, old_provenance)
            _shader_beforeimage(catalog_path, catalog_bytes)
            if staged_source is not None:
                _publish_shader_change(staged_source, source, None, temporary, journal)
            if tables is not None:
                staged_tables = temporary / 'function-tables.next'
                staged_tables.write_bytes(tables)
                _publish_shader_change(staged_tables, destination / 'function-tables.bin', None, temporary, journal)
            if next_catalog is not None:
                replacement = temporary / 'catalog.next'
                replacement.write_bytes(next_catalog)
                _publish_shader_change(replacement, catalog_path, catalog_bytes, temporary, journal)
        else:
            progress('Preparing the retained native material library for this MCC cache…')
            shaders = copy_selected(root, temporary)
            if not shaders.get('maps'):
                raise FormatError(shaders.get('blocker', 'Native material library is unavailable'))
            updated = {**report, 'native_shader_library': {**report.get('native_shader_library', {}), **shaders}}
            _shader_beforeimage(provenance, old_provenance)
            _publish_shader_change(temporary / 'native-shaders', destination, None, temporary, journal)
        plan = None
        if report.get('schema') == PROFILE and updated.get('native_shader_library', {}).get('maps'):
            plan = prepare_owner_index_refresh(cache, report, updated, verified_beforeimage=beforeimage)
            plan.check_beforeimage()
            pending_index = temporary / 'map-owners.next'
            pending_index.write_bytes(plan.index_bytes)
            if plan.index_bytes != plan.previous_index:
                _publish_shader_change(pending_index, destination / 'map-owners.json',
                                       plan.previous_index, temporary, journal)
            updated = plan.updated_provenance
            next_provenance = plan.provenance_bytes
        else:
            next_provenance = (json.dumps(updated, indent=2) + '\n').encode()
        if updated != report:
            pending = temporary / 'provenance.next'
            pending.write_bytes(next_provenance)
            _publish_shader_change(pending, provenance, old_provenance, temporary, journal)
        if plan is not None:
            plan.check_inputs()
        committed = True
        progress('Retained native materials are ready. Existing native programs and MCC files were retained.')
        return cache, updated
    except BaseException as error:
        if not committed:
            errors = _rollback_shader_changes(journal)
            if errors:
                recovery_required = True
                raise FormatError('Native repair rollback incomplete; recovery files retained at ' + str(temporary) +
                                  ': ' + '; '.join(errors)) from error
        raise
    finally:
        if not recovery_required:
            shutil.rmtree(temporary)


def _stage_shader_closure(root, cache, report, catalog, temporary, *, progress):
    """Validate and stage an absent common bank without committing metadata."""
    from native_shader_bundle import canonical_digest, closure_policy, require_closure_source
    from preserve_native_shaders import copy_selected, read_selected_library
    destination = cache / 'native-shaders'
    policy = closure_policy()
    source_name = policy['source_map']
    expected = {row['map']: row for row in policy['libraries'] if row['missing']}
    rows = catalog.get('maps')
    if not isinstance(rows, list):
        raise FormatError('Invalid cached native shader catalog records')
    by_name = {}
    for row in rows:
        if not isinstance(row, dict) or not isinstance(row.get('map'), str) or row['map'] in by_name:
            raise FormatError('Ambiguous cached native shader catalog')
        by_name[row['map']] = row
    dependencies = []
    for name, row in by_name.items():
        if name not in expected or row.get('complete') is not True:
            continue
        bank = destination / name
        manifest_path = bank / 'library.json'
        if bank.is_symlink() or manifest_path.is_symlink():
            raise FormatError('Cached native shader dependency must have a private owner')
        try:
            manifest = json.loads(manifest_path.read_bytes())
        except (OSError, ValueError) as error:
            raise FormatError('Cached native bank is incomplete; automatic closure repair cannot replace it: ' + name) from error
        # Unknown/custom banks retain their authored namespace and selection.
        if canonical_digest(manifest) == expected[name]['canonical_sha256']:
            dependencies.append(name)
    if not dependencies:
        return report, None, None, None
    source = destination / source_name
    if source.is_symlink():
        raise FormatError('Canonical retained Cairo native library must have a private owner')
    source_exists = source.exists()
    if source_exists:
        try:
            source_manifest, _ = read_selected_library(source, source_name)
            require_closure_source(source_manifest)
        except (OSError, ValueError, KeyError) as error:
            raise FormatError('Existing Cairo native library is customized or incomplete; automatic repair retained it unchanged') from error
        if (source_name in by_name and by_name[source_name].get('complete') is True and
                report.get('native_shader_library', {}).get('maps', {}).get(source_name) == file_sha(source / 'library.json')):
            return report, None, None, None
    # A canonical recipient with changed payload files is not an authenticated
    # old import. Leave it intact rather than making its report look repaired.
    for name in dependencies:
        try:
            read_selected_library(destination / name, name)
        except (OSError, ValueError, KeyError) as error:
            raise FormatError('Cached native bank has changed payloads; automatic closure repair retained it unchanged: ' + name) from error
    progress('Preparing the missing retained Cairo native library; the MCC data import is reused…')
    if not source_exists:
        shaders = copy_selected(root, temporary, maps={source_name})
        if source_name not in shaders.get('maps', {}):
            raise FormatError(shaders.get('blocker', 'Canonical retained Cairo native library is unavailable'))
        staged_source = temporary / 'native-shaders' / source_name
        source_manifest, _ = read_selected_library(staged_source, source_name)
        require_closure_source(source_manifest)
    else:
        staged_source = source
    new_row = dict(map=source_name, complete=True, tags=len(source_manifest['shader_tags']))
    if source_name in by_name:
        previous = by_name[source_name]
        if previous.get('complete') is not True or previous.get('tags', new_row['tags']) != new_row['tags']:
            raise FormatError('Existing Cairo native catalog entry is customized; automatic repair retained it unchanged')
        next_catalog = None
    else:
        next_catalog = (json.dumps({**catalog, 'maps': [*rows, new_row]}, indent=2) + '\n').encode()
    previous_shaders = report.get('native_shader_library', {})
    updated_shaders = {**previous_shaders,
        'maps': {**previous_shaders.get('maps', {}), source_name: file_sha(staged_source / 'library.json')},
        'closure_repair': dict(complete=True, source_map=source_name, dependent_maps=dependencies,
            source_canonical_sha256=policy['source_canonical_sha256'], existing_programs_replaced=False)}
    return {**report, 'native_shader_library': updated_shaders}, next_catalog, (
        staged_source if not source_exists else None), source


def repair_shader_closure(root, cache, report, catalog, catalog_bytes, *, progress=print):
    """Compatibility entry point; closure, LUT and owner metadata share commit."""
    path = Path(cache) / 'native-shaders/catalog.json'
    _shader_beforeimage(path, catalog_bytes)
    if json.loads(catalog_bytes) != catalog:
        raise FormatError('Native closure catalog differs from supplied beforeimage')
    return repair_cache_shaders(root, cache, report, progress=progress)


def repair_shaders(root, *, progress=print):
    value = selected(root)
    if value is None:
        raise FormatError('Import MCC before preparing native materials')
    return repair_cache_shaders(Path(root), *value, progress=progress)


def repair_movies(root, *, cache=None, progress=print):
    """Explicit supplied-movie reference repair; normal MCC setup omits it."""
    """Resume only the bundled movie handoff; never reimport the MCC tree."""
    value = selected(root) if cache is None else private_cache(root, cache)
    if value is None:
        raise FormatError('Import MCC before preparing bundled movies')
    from mcc_movies import repair
    return repair(root, value[0], progress=progress)


def repair_prompts(root, *, cache=None, progress=print):
    """Explicit reference/custom repair, outside normal native MCC setup."""
    value = selected(root) if cache is None else private_cache(root, cache)
    if value is None:
        raise FormatError('Import MCC before preparing authored button prompts')
    from mcc_prompts import repair
    return repair(root, value[0], progress=progress)


def _needs_graphics_reimport(root, report):
    """Old completed copies remain usable; setup upgrades their correction set."""
    from installation_paths import resource_root
    root = resource_root(root)
    from mcc_bitmap_correction_bundle import BUNDLE_V2, PROFILES_V2
    from mcc_bitmap_correction_bundle_v3 import BUNDLE_V3, MAP_NAME, PROFILES_V3
    from mcc_bitmap_correction_bundle_v4 import BUNDLE_V4, MAP_NAME as SACRED_ICON_MAP, PROFILES_V4
    from mcc_bitmap_correction_bundle_v5 import BUNDLE_V5, MAP_NAME as ELONGATION_MAP, PROFILES_V5
    from mcc_bitmap_correction_bundle_v6 import BUNDLE_V6, OWNERS_V6, PROFILES_V6
    from mcc_bitmap_correction_bundle_v7 import BUNDLE_V7, read_v7, profiles_for_map_v7
    from mcc_decal_bitmap_corrections import BUNDLE as DECAL_BUNDLE, read_bundle as read_decals, profiles_for_map as decal_profiles
    if not all((root / path).is_file() for path in (BUNDLE_V2, BUNDLE_V3)):
        return False
    graphics = report.get("graphics_corrections", {})
    if not isinstance(graphics, dict) or graphics.get("complete") is not True:
        return True
    sources = [("zanzibar.map", PROFILES_V2), (MAP_NAME, PROFILES_V3)]
    has_v4 = (root / BUNDLE_V4).is_file()
    has_v7 = (root / BUNDLE_V7).is_file()
    minimum = 7 if has_v7 else 6 if (root / BUNDLE_V6).is_file() else 5 if (root / BUNDLE_V5).is_file() else 4 if has_v4 else 3
    decal_sources = []
    if (root / DECAL_BUNDLE).is_file():
        path, manifest, _ = read_decals(root)
        decal_sources = [(name, decal_profiles(root, name))
                         for name in dict.fromkeys(o["map"] for o in manifest["owners"])]
        expected_decals = [dict(map=name, tag=p["tag"], restored_mips=p["mip_count"],
                               format=p["format"], target_sha256=p["target_sha256"])
                           for name, profiles in decal_sources for p in profiles]
        receipt = graphics.get("decal_bitmap_corrections", {})
        if (not isinstance(receipt, dict) or receipt.get("schema") != manifest["schema"] or
                receipt.get("manifest_sha256") != file_sha(path.with_suffix("")) or
                receipt.get("profiles") != expected_decals or
                receipt.get("restored_mips") != sum(p["restored_mips"] for p in expected_decals)):
            return True
    for version in (3, 4, 5, 6, 7):
        if version == 4 and (has_v4 or minimum < 5):
            sources.append((SACRED_ICON_MAP, PROFILES_V4))
        if version == 5:
            sources.append((ELONGATION_MAP, PROFILES_V5))
        if version == 6:
            sources.extend((owner["map"], PROFILES_V6) for owner in OWNERS_V6)
        if version == 7 and has_v7:
            _, manifest, _ = read_v7(root)
            sources.extend((name, profiles_for_map_v7(root, name))
                           for name in dict.fromkeys(owner["map"] for owner in manifest["owners"]))
        if version < minimum:
            continue
        expected = [dict(map=name, tag=p["tag"], restored_mips=p["mip_count"],
                         format=p["format"], target_sha256=p["target_sha256"])
                    for name, profiles in sources for p in profiles]
        if (graphics.get("schema") == f"H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V{version}" and
                graphics.get("restored_mips") == sum(p["restored_mips"] for p in expected) and
                graphics.get("profiles") == expected):
            # Reusing an already verified current copy needs no reconstruction
            # delta. A partial/older tool set must not downgrade its assets.
            return False
    return True


def setup(root, mcc_source=None, *, progress=print):
    root = Path(root).resolve()
    import mcc_presentation
    presentation = mcc_presentation.requirements(root)
    source = directory(mcc_source) if mcc_source is not None else discover()
    progress("Found Halo MCC Halo 2: " + str(source))
    if presentation['family'] == mcc_presentation.AUTHORED:
        mcc_presentation.preflight(root, source, presentation)
    imports = root / "assets/imported"
    candidates = sorted(imports.glob("mcc-*/provenance.json"),
                        key=lambda p: p.stat().st_mtime_ns, reverse=True) if imports.is_dir() else []
    for provenance in candidates:
        try:
            report = json.loads(provenance.read_text())
            if Path(report.get("source", "")).resolve() != source:
                continue
            if _needs_graphics_reimport(root, report):
                progress("Existing MCC cache predates the current graphics corrections; preparing a fresh private import. The previous cache is preserved.")
                continue
            # This validates the entire private copy. Reuse never makes the
            # live MCC installation a normal runtime data source.
            cache, report = private_cache(root, provenance.parent, verify=True, source=source, progress=progress)
        except (OSError, ValueError, KeyError) as error:
            progress("Existing MCC cache needs a fresh import: " + str(error))
            continue
        if not mcc_presentation.compatible(report, presentation):
            progress("Existing MCC loading presentation belongs to a different runtime family; preserving it and preparing a separate cache.")
            continue
        from mcc_loading_assets import compatible as loading_sources_compatible
        if not loading_sources_compatible(root, cache, report):
            progress("Existing MCC cache needs the complete loading-image inventory; preparing a fresh private import. The previous cache is preserved.")
            continue
        from custom_map_assets import compatible as custom_maps_compatible
        if not custom_maps_compatible(root, cache, report):
            progress("Existing MCC cache predates the bundled custom maps; preparing a fresh private import. The previous cache is preserved.")
            continue
        # Older imports did not have a portable material library. Repair those
        # when this installation includes one; no MCC bytes need re-copying.
        from native_shader_bundle import BUNDLE
        from installation_paths import resource_root
        if (resource_root(root) / BUNDLE).is_file() or (root / 'assets/native-shaders-source.tsv').is_file():
            cache, report = repair_cache_shaders(root, cache, report, progress=progress)
        from mcc_ui_layout import repair as repair_ui_layout
        cache, report = repair_ui_layout(root, cache, progress=progress)
        cache, report = mcc_presentation.repair(root, cache, source, presentation, progress=progress)
        from controller_prompt_assets import repair as repair_controller_prompts
        cache, report = repair_controller_prompts(root,cache,progress=progress)
        mcc_presentation.recheck(presentation)
        return select(root, cache, verify=False, progress=progress)
    cache, report = prepare(root, source, progress=progress, presentation=presentation)
    mcc_presentation.recheck(presentation)
    return select(root, cache, verify=False, progress=progress)
