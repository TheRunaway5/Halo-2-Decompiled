"""Install the authored multiplayer definitions with the standard native assets."""
import hashlib
import json
import os
from pathlib import Path
import uuid

from audit_rendering import FormatError
from installation_paths import resource_root
from mcc_movies import _owned

DIRECTORY = "data/tags/variants"
LAYOUTS = "data/tags/bomb-defusal-sites-v1.json"
MANIFEST = "data/tags/bundled-variants.json"
SCHEMA = "H2_BUNDLED_NATIVE_VARIANTS_V1"
DEFINITIONS = {
    "fiesta-slayer-v1": ("slayer", 1),
    "infection-slayer-v1": ("slayer", 2),
    "bomb-defusal-v1": ("assault", 1),
}


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def source_bundle(root):
    root = resource_root(root)
    files = {}
    for identifier, (engine, ruleset) in DEFINITIONS.items():
        name = DIRECTORY + "/" + identifier + ".json"
        path = _owned(root / name, root)
        if not path.is_file() or not 0 < path.stat().st_size <= 16384:
            raise FormatError("Missing or oversized bundled multiplayer definition: " + name)
        raw = path.read_bytes()
        row = json.loads(raw)
        if (not isinstance(row, dict) or row.get("id") != identifier or
                row.get("schema") != "halo2-native-" + engine + "-variant-v1" or
                row.get("engine") != engine or row.get("native_ruleset") != ruleset or
                not isinstance(row.get("settings"), dict)):
            raise FormatError("Invalid bundled multiplayer definition: " + name)
        files[name] = raw
    path = _owned(root / LAYOUTS, root)
    if not path.is_file() or not 0 < path.stat().st_size <= 65536:
        raise FormatError("Missing or oversized Bomb Defusal map layouts")
    raw = path.read_bytes()
    layouts = json.loads(raw)
    if (not isinstance(layouts, dict) or layouts.get("schema") != "halo2-native-bomb-defusal-sites-v1" or
            not isinstance(layouts.get("maps"), list) or not 0 < len(layouts["maps"]) <= 32):
        raise FormatError("Invalid Bomb Defusal map layouts")
    files[LAYOUTS] = raw
    return files


def install(root, destination, *, progress=lambda _: None):
    """Copy standard definitions; retain unrelated files and customized definitions.

    A receipt permits updates to our previously installed bytes. Interrupted
    installs resume from exact matching files without reimporting MCC content.
    """
    files = source_bundle(root)
    entry = dict(schema=SCHEMA, files={name: dict(bytes=len(raw), sha256=digest(raw))
                                      for name, raw in files.items()},
                 provenance="Newly authored native variants; no extracted game assets")
    destination = Path(destination).absolute()
    if destination == resource_root(root):
        return entry
    _owned(destination, destination, directory=True)
    receipt = _owned(destination / MANIFEST, destination)
    if receipt.exists() and receipt.stat().st_size > 65536:
        raise FormatError("Oversized bundled multiplayer asset receipt")
    previous = json.loads(receipt.read_bytes()) if receipt.is_file() else {}
    if previous and (not isinstance(previous, dict) or previous.get("schema") != SCHEMA or
                     not isinstance(previous.get("files"), dict)):
        raise FormatError("Invalid bundled multiplayer asset receipt")
    pending_files = {}
    for name, raw in files.items():
        path = _owned(destination / name, destination)
        if path.exists():
            existing = path.read_bytes()
            if existing == raw:
                continue
            record = previous.get("files", {}).get(name, {})
            if not isinstance(record, dict) or digest(existing) != record.get("sha256"):
                raise FormatError("Customized multiplayer definition retained: " + name)
        pending_files[name] = raw
    if source_bundle(root) != files:
        raise FormatError("Bundled multiplayer definitions changed during setup; retry")
    encoded = (json.dumps(entry, indent=2, sort_keys=True) + "\n").encode()
    if not receipt.exists() or receipt.read_bytes() != encoded:
        pending_files[MANIFEST] = encoded
    if pending_files:
        progress("Installing Bomb Defusal, Fiesta Slayer and Infection Slayer…")
    for name, raw in pending_files.items():
        path = destination / name
        for parent in reversed(path.parents):
            if parent == destination or parent.is_relative_to(destination):
                _owned(parent, destination, directory=True).mkdir(exist_ok=True)
        pending = path.with_name(".native-variants-" + uuid.uuid4().hex)
        try:
            pending.write_bytes(raw)
            os.replace(pending, path)
        finally:
            pending.unlink(missing_ok=True)
    return entry
