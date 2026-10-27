"""MCC-only loading asset preparation, retaining independently authored art.

Original Vista executable/loading.bin pixels and ambience are not inputs.
The MCC banks retain authored tag/frame identities; this does not invent a
Vista slot mapping or admit an unfinished custom texture to presentation.
"""
import json
from pathlib import Path
import shutil
import hashlib
import os
import struct
import uuid

from audit_rendering import FormatError, file_sha
from export_vista_menu_ui import dds_image, safe_asset_path

CUSTOM = Path("assets/native-loading-custom/authored-rings-20261013")
MANIFEST = "loading/mcc-loading-assets.json"

# Accepted MCC mainmenu resource audit, 2026-10-25. These are the MCC resident
# byte streams (including retained mips), not an Xbox/H2EK onboarding dependency.
# Tickers retain eight additional resident mips; the authoring tags also contain
# two smaller offline mips which the original resident loading banks do not use.
ORIGINAL_PAGES = (
    (0, 'loading_screen', 0, 'background', 1024, 512, 14, 0, '13da1aa08df4db2e30c50da926b4fa95aba58f88f73124369813c8c3ba248800'),
    (1, 'loading_screen_sweep', 0, 'sweep', 2048, 4, 1, 0, 'd436aeee198e80d59b29d51de33ae73e48f7a0c8cd496c15bf496d186bf325b3'),
    (2, 'loading_screen_spinners', 0, 'spinner_outer', 512, 512, 14, 7, '05c371d5d6426534c70ef2a1aaec89107205f2cdfbb2d56bec3fecb35e0e5d2c'),
    (2, 'loading_screen_spinners', 1, 'spinner_inner', 512, 512, 14, 7, '58955c8fc2608db5c5f9c2d21fb6072ffdd0698cf54797bc7c32a3740b223000'),
    (3, 'loading_screen_logos', 0, 'logo_primary', 512, 64, 15, 0, '997ec7d4a33cc76250cc756fdc4b0e42edad01879562c09ca83d000a1911d624'),
    (3, 'loading_screen_logos', 1, 'logo_alternate', 512, 64, 15, 0, 'c25a4080283d8f458ec571c23d212fb3aacde8a345c5f60e6dd5a01c39d3da78'),
    (4, 'loading_screen_tickers', 0, 'ticker_0', 1024, 8, 15, 8, '746d305bd8dcd219620c9fdbe1e083c88f4a00b6aad3df1a436f17d3aaa7ab1a'),
    (4, 'loading_screen_tickers', 1, 'ticker_1', 1024, 4, 15, 8, 'd45cbd4e01c40b78c945f7f7facdac9500d33e9d5710a46ea9ee86136c7e41d7'),
    (4, 'loading_screen_tickers', 2, 'ticker_2', 1024, 8, 15, 8, '201f97a13939bd0ebbbc5f908fcd35c0ff88706b472a91eee93e14381cf2daa0'),
    (4, 'loading_screen_tickers', 3, 'ticker_3', 1024, 8, 15, 8, '04ce4544cab28e39cdbf13e34a48815512aaaddcfce830752c81a279db971399'),
    (4, 'loading_screen_tickers', 4, 'ticker_4', 1024, 8, 15, 8, 'b11884e7e6d5ffd9a8d24aa432c2ccc2340b4cfc424d3eaaa138dfce40c313b5'),
)


def original_bank_inputs(banks):
    """Authenticate all standard loading pages independently of legacy slots."""
    owners = {}
    for bank in banks:
        key = bank['cache'], bank['tag']
        if key in owners:
            raise FormatError('Duplicate MCC loading bitmap owner')
        owners[key] = bank
    pages = []
    counts = {}
    for _, suffix, *_ in ORIGINAL_PAGES:
        counts[suffix] = counts.get(suffix, 0) + 1
    for bank_index, suffix, ordinal, role, width, height, fmt, mips, payload_sha in ORIGINAL_PAGES:
        name = 'globals\\loading_screen\\' + suffix
        owner = owners.get(('mainmenu.map', name))
        if owner is None or len(owner['frames']) != counts[suffix]:
            raise FormatError('MCC original loading bank is missing required pages: ' + name)
        matches = [frame for frame in owner['frames'] if frame['frame'] == ordinal]
        if len(matches) != 1:
            raise FormatError('MCC original loading bank has an ambiguous frame: ' + name)
        frame = matches[0]
        identity = frame.get('source_payload', {})
        if ((frame['width'], frame['height'], frame['format'], frame['additional_mips']) !=
                (width, height, fmt, mips) or identity.get('sha256') != payload_sha):
            raise FormatError('MCC original loading page differs from its accepted source identity: ' + name)
        try:
            datum = int(owner['datum'], 16)
            record = bytes.fromhex(frame['source_record_hex'])
            source_width, source_height = struct.unpack_from('<2H', record, 4)
            source_format, source_flags = struct.unpack_from('<2H', record, 12)
            source_mips = struct.unpack_from('<H', record, 20)[0]
            source_datum = struct.unpack_from('<I', record, 28)[0]
        except (ValueError, TypeError, struct.error) as error:
            raise FormatError('Invalid complete MCC loading source frame identity') from error
        if (len(record) != 168 or not 0 <= datum < 2**32 or source_datum != datum or
                (source_width, source_height, source_format, source_flags, source_mips) !=
                (width, height, fmt, frame['flags'], mips)):
            raise FormatError('MCC loading frame differs from its full-salt source record')
        pages.append(dict(bank=bank_index, page=ordinal, role=role,
            cache=owner['cache'], tag=name, datum=owner['datum'], datum_u32=datum,
            resident_mips=mips, resident_bytes=identity['bytes'],
            resident_sha256=payload_sha, **frame))
    return dict(schema='H2_MCC_ORIGINAL_LOADING_BANK_INPUTS_V1', complete=True,
        bank_count=len(counts), page_count=len(pages), pages=pages,
        source='MCC copied bitmap resources; exact resident page bytes and mips',
        external_vista_or_xbox_required=False, runtime_compositor_admitted=False)


def compatible(root, pack, report=None):
    """Read-only reuse check; missing legacy pages require a fresh preparation.

    Changed DDS/custom files are retained. The enclosing onboarding transaction
    may prepare a separate cache rather than repairing selected files in place.
    """
    from mcc_movies import _owned
    pack = Path(pack)
    path = _owned(pack / MANIFEST, pack)
    if not path.is_file():
        return False
    if path.stat().st_size > 1024 * 1024:
        raise FormatError('Oversized MCC loading bitmap manifest')
    metadata = json.loads(path.read_bytes())
    if metadata.get('schema') != 'H2_MCC_LOADING_ASSETS_V1' or 'original_bank_inputs' not in metadata:
        return False
    expected = original_bank_inputs(metadata['banks'])
    if metadata['original_bank_inputs'] != expected:
        raise FormatError('MCC original loading bank manifest differs from its source owners')
    for page in expected['pages']:
        def contained(name):
            relative = Path(name)
            if (relative.is_absolute() or not relative.parts or
                    any(part in ('', '.', '..') for part in name.split('/')) or '\\' in name):
                raise FormatError('Unsafe MCC loading asset path')
            return _owned(pack / relative, pack)
        target = contained(page['path'])
        if not target.is_file():
            return False
        if file_sha(target) != page['dds_sha256']:
            raise FormatError('Existing MCC loading DDS differs; retained unchanged')
        source = contained(page['source_payload']['path'])
        if not source.is_file() or file_sha(source) != page['source_payload']['sha256']:
            raise FormatError('MCC loading resource differs from its copied owner')
        dds, _ = dds_image(page['width'], page['height'], page['additional_mips'], page['format'], source.read_bytes())
        if hashlib.sha256(dds).hexdigest() != page['dds_sha256']:
            raise FormatError('MCC loading DDS does not retain its exact resident source bytes')
    return True

# Source-owner roles are distinct from a complete runtime presentation. The
# existing normal-mode renderer also samples a separate logo/vignette mask
# (slot 11); neither a same-name logo nor an unfinished ring is that image.
DRAW_INPUTS = (
    (0, "background", "globals\\loading_screen\\loading_screen", 0),
    (1, "logo", "globals\\loading_screen\\loading_screen_logos", 0),
    (3, "spinner_outer", "globals\\loading_screen\\loading_screen_spinners", 0),
    (4, "spinner_inner", "globals\\loading_screen\\loading_screen_spinners", 1),
    (5, "sweep", "globals\\loading_screen\\loading_screen_sweep", 0),
)


def presentation_inputs(banks):
    owners = {}
    for bank in banks:
        key = (bank["cache"], bank["tag"])
        if key in owners:
            raise FormatError("Duplicate MCC loading bitmap owner")
        owners[key] = bank
    present, missing = [], []
    for slot, role, name, ordinal in DRAW_INPUTS:
        owner = owners.get(("mainmenu.map", name))
        frames = owner["frames"] if owner else []
        matches = [frame for frame in frames if frame["frame"] == ordinal]
        if len(matches) > 1:
            raise FormatError("Duplicate MCC loading bitmap frame")
        if not matches:
            missing.append(dict(role=role, texture_slot=slot, tag=name, frame=ordinal))
            continue
        frame = matches[0]
        present.append(dict(role=role, texture_slot=slot, tag=name, frame=ordinal,
            datum=owner["datum"], cache=owner["cache"], path=frame["path"],
            width=frame["width"], height=frame["height"],
            dds_sha256=frame["dds_sha256"], source_record_hex=frame["source_record_hex"]))
    return dict(schema="H2_MCC_LOADING_BITMAP_INPUTS_V1", inputs=present,
        missing_bitmap_inputs=missing,
        separate_consumers=[dict(role="logo_vignette_mask", texture_slot=11,
            state="producer-unresolved"), dict(role="loading_ambience", state="producer-unresolved")],
        runtime_factory="halo2::load_mcc_loading_bitmap_inputs",
        readiness_signal=False, exact_vista_bank=False)


def preserve_custom(root, destination):
    from installation_paths import resource_root
    source = resource_root(root) / CUSTOM
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
                width=width, height=height, format=frame["format"], flags=frame['flags'],
                additional_mips=frame["additional_mips"],
                source_payload=dict(path=relative.as_posix(), bytes=payload.stat().st_size,
                                    sha256=resource['sha256'])))
        exported.append(dict(tag=bank["tag"], datum=bank["datum"], cache="mainmenu.map", frames=frames))
    original_inputs = original_bank_inputs(exported)
    custom = preserve_custom(root, pack / "loading/custom")
    result = dict(schema="H2_MCC_LOADING_ASSETS_V1", complete=True,
        source="MCC mainmenu.map/textures.dat plus authored port textures",
        banks=exported, frames=sum(len(b["frames"]) for b in exported), custom=custom,
        vista_executable_bank_imported=False, external_vista_or_xbox_required=False,
        native_presentation_mapping_complete=False,
        original_bank_inputs=original_inputs,
        presentation_inputs=presentation_inputs(exported))
    target = pack / MANIFEST
    temporary = target.with_name('.mcc-loading-' + uuid.uuid4().hex)
    try:
        temporary.write_text(json.dumps(result, indent=2) + "\n")
        os.replace(temporary, target)
    finally:
        temporary.unlink(missing_ok=True)
    return result
