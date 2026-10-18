"""Assemble verified MCC menu resources from an already copied private cache.

This is an owned image/font catalogue, not a widget/locale metadata conversion
or a sampling pack. No original Xbox/Vista installation or executable is read.
"""
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import tempfile

from audit_rendering import FormatError, file_sha
from export_vista_menu_ui import dds_image


def _owned_file(pack, name):
    relative = PurePosixPath(name)
    if (not name or "\\" in name or relative.is_absolute() or
            any(part in ("", ".", "..") for part in name.split("/"))):
        raise FormatError("Invalid relative MCC menu resource path")
    path = pack / relative
    if (path.is_symlink() or not path.is_file() or
            not path.resolve().is_relative_to(pack)):
        raise FormatError("MCC menu resource lost its private owner: " + name)
    # Reject intermediate symlinks too, including ones pointing back into pack.
    if any(parent.is_symlink() for parent in path.parents if parent != pack and pack in parent.parents):
        raise FormatError("MCC menu resource has a symlink parent: " + name)
    return path


def prepare(pack, menu_inventory=None):
    """Publish absent ui/ atomically; refuse to replace any existing UI owner.

    The existing copied-pixel inventory supplies exact tag/frame/hash identity.
    Bitmap partitions and fonts are checked again before publication. Retained
    authored loading textures have their separate profile and are not UI art.
    """
    pack = Path(pack)
    if pack.is_symlink() or not pack.is_dir():
        raise FormatError("MCC menu cache requires an owned private directory")
    pack = pack.resolve()
    destination = pack / "ui"
    if destination.exists() or destination.is_symlink():
        raise FormatError("Existing MCC UI resources were retained; use a fresh preparation")
    inventory_path = _owned_file(pack, "vista-menu-inventory.json")
    inventory = json.loads(inventory_path.read_text())
    if menu_inventory is not None and inventory != menu_inventory:
        raise FormatError("MCC menu inventory changed before assembly")
    if inventory.get("schema") != "H2_MCC_VISTA_MENU_INVENTORY_V1" or inventory.get("complete") is not True:
        raise FormatError("MCC menu resources require the complete copied-pixel inventory")
    bitmap_path = _owned_file(pack, "bitmap-resources.json")
    bitmap = json.loads(bitmap_path.read_text())
    if bitmap.get("schema") != "H2_MCC_BITMAP_RESOURCES_V1":
        raise FormatError("Unsupported MCC bitmap resource inventory")
    banks = {}
    for bank in bitmap["banks"]:
        key = (bank["map"], bank["tag"], bank["datum"])
        if key in banks:
            raise FormatError("Duplicate MCC menu bitmap directory owner")
        banks[key] = bank
    stage = Path(tempfile.mkdtemp(prefix=".mcc-menu-resources-", dir=pack))
    try:
        pages, seen, payloads = [], set(), {}
        for page in inventory["pages"]:
            key = (page["cache"], page["name"], page["datum"])
            ordinal = page["frame"]
            if (not page["name"].startswith("ui\\") or not isinstance(ordinal, int) or
                    isinstance(ordinal, bool) or ordinal < 0 or (key, ordinal) in seen):
                raise FormatError("Invalid or duplicate MCC menu page owner")
            seen.add((key, ordinal))
            bank = banks.get(key)
            if bank is None or ordinal >= len(bank["frames"]):
                raise FormatError("Missing MCC menu bitmap frame owner")
            frame = bank["frames"][ordinal]
            dimensions, partitions = frame["dimensions"], frame["partitions"]
            if (len(dimensions) != 3 or dimensions != [page["width"], page["height"], 1] or
                    frame["type"] != 0 or len(partitions) != 3 or
                    not partitions[0] or any(partitions[1:])):
                raise FormatError("Unsupported MCC menu bitmap storage")
            resource = bitmap["resources"][partitions[0]]
            path = _owned_file(pack, resource["path"])
            if not resource["path"].startswith("bitmap-resources/"):
                raise FormatError("MCC menu pixels require their copied bitmap partition")
            if path.stat().st_size != resource["decoded_bytes"] or file_sha(path) != resource["sha256"]:
                raise FormatError("MCC menu pixels differ from their copied partition")
            identity = (resource["sha256"], tuple(dimensions), frame["format"], frame["additional_mips"])
            if identity not in payloads:
                dds, _ = dds_image(*dimensions[:2], frame["additional_mips"], frame["format"], path.read_bytes())
                digest = hashlib.sha256(dds).hexdigest()
                relative = Path("bitmaps") / (digest + ".dds")
                target = stage / relative
                target.parent.mkdir(exist_ok=True)
                if not target.exists():
                    target.write_bytes(dds)
                if file_sha(target) != digest:
                    raise FormatError("MCC menu DDS readback differs")
                payloads[identity] = (digest, relative.as_posix(), len(dds))
            digest, relative, size = payloads[identity]
            if digest != page["dds_sha256"]:
                raise FormatError("MCC menu DDS differs from its verified source page")
            pages.append(dict(**page, path="ui/" + relative, bytes=size,
                source_record_hex=frame["record_hex"], source_partition=partitions[0]))
        fonts = {}
        for name, record in inventory["fonts"].items():
            if not name.startswith("h2_fonts/"):
                raise FormatError("MCC menu font is outside its original font tree")
            path = _owned_file(pack, "halo2/" + name)
            if path.stat().st_size != record["bytes"] or file_sha(path) != record["sha256"]:
                raise FormatError("MCC menu font differs from its copied owner")
            fonts["halo2/" + name] = record
        report = dict(schema="H2_MCC_MENU_RESOURCE_CACHE_V1", complete=True,
            source="Copied MCC Classic bitmap partitions and case-preserved h2_fonts",
            menu_inventory_sha256=file_sha(inventory_path), bitmap_inventory_sha256=file_sha(bitmap_path),
            pages=pages, fonts=fonts, frame_owners=len(pages),
            unique_dds_pages=len({page["dds_sha256"] for page in pages}),
            unavailable_in_mcc=inventory.get("unavailable_in_mcc", []),
            excluded_reference_assets=inventory.get("excluded_reference_assets", []),
            runtime_activation=False, sampling_pack=False,
            frontend_metadata="Widget/locale/menu-source admission remains separate; no Vista source sidecar is emitted",
            pixels="Original format, dimensions and complete authored mip chain; no resampling",
            external_vista_or_xbox_required=False)
        (stage / "mcc-menu-assets.json").write_text(json.dumps(report, indent=2) + "\n")
        if destination.exists() or destination.is_symlink():
            raise FormatError("MCC UI resources changed during assembly")
        stage.rename(destination)
        return report
    finally:
        if stage.exists():
            shutil.rmtree(stage)
