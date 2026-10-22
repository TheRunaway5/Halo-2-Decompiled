"""Prepare native logical UI assets from the portable MCC conversion profile.

MCC bitmap pixels keep their imported owners. Geometry uses original logical
units and responsive center slices, so one converted cache serves every aspect.
"""
import hashlib
import json
import os
from pathlib import Path
import uuid

from audit_rendering import FormatError
from mcc_movies import _cancellation, _lock, _owned

SCHEMA = "H2_MCC_NATIVE_UI_LAYOUT_V1"
SOURCE = "resources/frontend/mcc-native-layout.json"
TARGET = "presentation/ui/native-layout.json"
LIMIT = 2 * 1024 * 1024


def _read(path, root):
    path = _owned(path, root)
    if not path.is_file() or not 0 < path.stat().st_size <= LIMIT:
        raise FormatError("MCC native UI conversion profile is missing or too large")
    raw = path.read_bytes()
    def unique(pairs):
        value = {}
        for key, item in pairs:
            if key in value:
                raise ValueError("Duplicate UI metadata key")
            value[key] = item
        return value
    try:
        value = json.loads(raw, object_pairs_hook=unique)
    except (ValueError, UnicodeError) as error:
        raise FormatError("Invalid MCC native UI conversion profile") from error
    return raw, value


def source_profile(root):
    from installation_paths import resource_root
    root = resource_root(root)
    raw, value = _read(Path(root) / SOURCE, root)
    try:
        layout = value["layout"]
        if (value["schema"] != SCHEMA or set(value) != {"schema", "layout"} or
                set(layout) != {"raw", "original", "responsive"} or
                layout["responsive"]["friends_full_surface"] is not True or
                layout["responsive"]["clans_full_surface"] is not True or
                layout["responsive"]["reference_canvas"] != [640, 480] or
                "26" not in layout["original"]["screens"]):
            raise ValueError("Unsupported native UI conversion contract")
    except (KeyError, TypeError, ValueError) as error:
        raise FormatError("Unsupported MCC native UI conversion profile") from error
    return raw, value


def required_bytes(root):
    return len(source_profile(root)[0]) + 512


def _atomic(path, raw):
    pending = path.with_name(".ui-layout-" + uuid.uuid4().hex)
    try:
        with pending.open("xb") as output:
            output.write(raw)
            output.flush()
            os.fsync(output.fileno())
        os.replace(pending, path)
    finally:
        pending.unlink(missing_ok=True)


def _prepare(root, cache, progress, previous=None):
    raw, value = source_profile(root)
    mainmenu = _owned(cache / "halo2/h2_maps_win64_dx11/mainmenu.map", cache)
    with mainmenu.open("rb") as source:
        if source.read(8) != b"daeh\x0d\0\0\0":
            raise FormatError("Native UI conversion requires an imported MCC mainmenu")
        source.seek(0)
        donor = hashlib.file_digest(source, "sha256").hexdigest()
    converted = {**value, "source_mainmenu_sha256": donor,
                 "source_profile_sha256": hashlib.sha256(raw).hexdigest()}
    payload = (json.dumps(converted, separators=(",", ":"), sort_keys=True) + "\n").encode()
    entry = dict(schema=SCHEMA, complete=True, path=TARGET, bytes=len(payload),
                 sha256=hashlib.sha256(payload).hexdigest(), source_mainmenu_sha256=donor,
                 source_profile_sha256=converted["source_profile_sha256"],
                 logical_canvas=[640, 480], responsive_surfaces=["friends", "clans"],
                 bitmap_pixels="unchanged MCC owners", shaders="unchanged")
    target = _owned(cache / TARGET, cache)
    if target.exists():
        existing = target.read_bytes()
        if existing != payload and (not isinstance(previous, dict) or
                previous.get("schema") != SCHEMA or previous.get("path") != TARGET or
                hashlib.sha256(existing).hexdigest() != previous.get("sha256")):
            raise FormatError("Customized MCC native UI asset was retained; repair its conversion selection")
        if existing == payload:
            return entry
    if source_profile(root)[0] != raw:
        raise FormatError("Native UI profile changed during conversion; retry import")
    _owned(cache / "presentation", cache, directory=True).mkdir(exist_ok=True)
    _owned(cache / "presentation/ui", cache, directory=True).mkdir(exist_ok=True)
    progress("Converting MCC menu layout, selectors and responsive Friends/Clans surfaces…")
    _atomic(target, payload)
    return entry


def prepare(root, cache, *, progress=print):
    root, cache = Path(root).resolve(), Path(cache)
    with _cancellation(), _lock(cache):
        return _prepare(root, cache, progress)


def repair(root, cache, *, progress=print):
    root, cache = Path(root).resolve(), Path(cache)
    with _cancellation(), _lock(cache):
        path = _owned(cache / "provenance.json", cache)
        before = path.read_bytes()
        report = json.loads(before)
        if (report.get("schema") != "H2_MCC_TU15_PREPARATION_V1" or
                report.get("external_installation_requirement") != "Halo MCC only"):
            raise FormatError("Native UI repair requires an owned MCC cache")
        entry = _prepare(root, cache, progress, report.get("native_ui_layout"))
        updated = {**report, "native_ui_layout": entry}
        if updated != report:
            if path.read_bytes() != before:
                raise FormatError("MCC provenance changed during UI conversion; retry setup")
            _atomic(path, (json.dumps(updated, indent=2) + "\n").encode())
        return cache, updated
