"""MCC-only loading asset preparation, retaining independently authored art.

Original Vista executable/loading.bin pixels and ambience are not inputs.
The MCC banks retain authored tag/frame identities; this does not invent a
Vista slot mapping or admit an unfinished custom texture to presentation.
"""
import json
from pathlib import Path
import shutil

from audit_rendering import FormatError, file_sha
from export_vista_menu_ui import dds_image, safe_asset_path

CUSTOM = Path("assets/native-loading-custom/authored-rings-20261013")


def preserve_custom(root, destination):
    source = Path(root) / CUSTOM
    if not source.exists():
        return dict(files={}, required=False, runtime_admitted=False)
    profile = source / "profile.json"
    if profile.is_symlink() or not profile.is_file():
        raise FormatError("Missing authored loading asset profile")
    value = json.loads(profile.read_text())
    if value.get("schema") != "H2_AUTHORED_LOADING_ASSETS_V1" or not isinstance(value.get("files"), dict):
        raise FormatError("Invalid authored loading asset profile")
    checked = {}
    for name, record in value["files"].items():
        path = source / name
        if Path(name).name != name or record.get("origin") not in ("authored-vector", "rasterized-authored-vector"):
            raise FormatError("Loading custom assets must be independently authored local files")
        if path.is_symlink() or not path.is_file() or path.stat().st_size != record["bytes"] or file_sha(path) != record["sha256"]:
            raise FormatError("Authored loading asset changed: " + name)
        checked[name] = record
    destination.mkdir(parents=True, exist_ok=True)
    for name in (*checked, "profile.json"):
        target = destination / name
        if target.exists() and file_sha(target) != file_sha(source / name):
            raise FormatError("Existing custom loading copy differs: " + name)
        if not target.exists():
            shutil.copyfile(source / name, target)
        if file_sha(target) != file_sha(source / name):
            raise FormatError("Copied authored loading asset differs: " + name)
    return dict(files=checked, profile_sha256=file_sha(profile), required=False,
                runtime_admitted=value.get("runtime_admitted", False),
                state=value.get("state", "authored-needs-review"))


def prepare(root, pack):
    pack = Path(pack)
    inventory = json.loads((pack / "bitmap-resources.json").read_text())
    banks = [b for b in inventory["banks"] if b["map"] == "mainmenu.map" and
             (b["tag"] == "globals\\loading_screen" or b["tag"].startswith((
                 "globals\\loading_screen\\", "ui\\screens\\misc\\loading_screen\\")))]
    if not banks:
        raise FormatError("MCC mainmenu has no loading bitmap owners")
    exported = []
    for bank in banks:
        frames = []
        for ordinal, frame in enumerate(bank["frames"]):
            if frame["dimensions"][2] != 1 or frame["type"] != 0 or not frame["partitions"][0] or any(frame["partitions"][1:]):
                raise FormatError("Unsupported MCC loading bitmap storage")
            resource = inventory["resources"][frame["partitions"][0]]
            relative = Path(resource["path"])
            if relative.is_absolute() or ".." in relative.parts or relative.parts[0] != "bitmap-resources":
                raise FormatError("Unsafe MCC loading resource path")
            payload = pack / relative
            if payload.is_symlink() or file_sha(payload) != resource["sha256"]:
                raise FormatError("MCC loading resource differs from its copied owner")
            width, height, _ = frame["dimensions"]
            dds, _ = dds_image(width, height, frame["additional_mips"], frame["format"], payload.read_bytes())
            target = pack / "loading/mcc/bitmaps" / safe_asset_path(bank["tag"], require_ui=False) / f"{ordinal:03d}.dds"
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists() and target.read_bytes() != dds:
                raise FormatError("Existing MCC loading DDS differs")
            if not target.exists():
                target.write_bytes(dds)
            frames.append(dict(frame=ordinal, path=target.relative_to(pack).as_posix(),
                dds_sha256=file_sha(target), source_record_hex=frame["record_hex"],
                width=width, height=height, format=frame["format"], additional_mips=frame["additional_mips"]))
        exported.append(dict(tag=bank["tag"], datum=bank["datum"], cache="mainmenu.map", frames=frames))
    custom = preserve_custom(root, pack / "loading/custom")
    result = dict(schema="H2_MCC_LOADING_ASSETS_V1", complete=True,
        source="MCC mainmenu.map/textures.dat plus authored port textures",
        banks=exported, frames=sum(len(b["frames"]) for b in exported), custom=custom,
        vista_executable_bank_imported=False, external_vista_or_xbox_required=False,
        native_presentation_mapping_complete=False)
    target = pack / "loading/mcc-loading-assets.json"
    target.write_text(json.dumps(result, indent=2) + "\n")
    return result
