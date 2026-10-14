"""MCC-only launcher source selection and verified private-cache reuse.

Selecting a prepared cache does not declare its unfinished conversion playable.
Original Xbox/Vista source registries are never read by this onboarding path.
"""
import json
import os
from pathlib import Path, PurePosixPath
import uuid

from audit_rendering import FormatError, file_sha
from mcc_content import directory, discover
from prepare_mcc_gameplay import PROFILE, prepare, source_files, source_directories

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
        output_hashes = {"h2_maps_win64_dx11/" + row["map"]: row["output_sha256"]
                         for row in conversion["maps"]}
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


def setup(root, mcc_source=None, *, progress=print):
    root = Path(root).resolve()
    source = directory(mcc_source) if mcc_source is not None else discover()
    progress("Found Halo MCC Halo 2: " + str(source))
    imports = root / "assets/imported"
    candidates = sorted(imports.glob("mcc-*/provenance.json"),
                        key=lambda p: p.stat().st_mtime_ns, reverse=True) if imports.is_dir() else []
    for provenance in candidates:
        try:
            report = json.loads(provenance.read_text())
            if Path(report.get("source", "")).resolve() != source:
                continue
            # This validates the entire private copy. Reuse never makes the
            # live MCC installation a normal runtime data source.
            cache, report = private_cache(root, provenance.parent, verify=True, source=source, progress=progress)
        except (OSError, ValueError, KeyError) as error:
            progress("Existing MCC cache needs a fresh import: " + str(error))
            continue
        return select(root, cache, verify=False, progress=progress)
    cache, report = prepare(root, source, progress=progress)
    return select(root, cache, verify=False, progress=progress)
