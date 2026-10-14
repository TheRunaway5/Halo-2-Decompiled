"""Check the requested Vista menu owners in the private MCC source tree.

The checked-in inventory records hashes and identities, never retail artwork.
Images remain inside MCC's maps/textures.dat at their original paths. The
Vista executable loading bank is outside onboarding scope.
"""
from collections import Counter, defaultdict
import csv
import hashlib
from pathlib import Path

from audit_rendering import FormatError, file_sha
from export_vista_menu_ui import dds_image
from mcc_content import MccCache, texture_resource

INVENTORY = Path(__file__).resolve().parent / "reference/mcc-vista-menu-inventory.tsv"
PRIMARY_FONTS = ("conduit-12", "conduit-13", "conduit-9", "fixedsys-9",
                 "handel_gothic-11", "handel_gothic-13", "handel_gothic-24", "MSLCD-14",
                 "font_table.txt")


def verify(halo2, inventory=INVENTORY, progress=lambda message: None):
    halo2 = Path(halo2)
    with Path(inventory).open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    by_cache, missing, excluded = defaultdict(list), [], []
    for row in rows:
        if row["status"] == "missing":
            if row["vista_name"].startswith("executable\\loading\\"):
                excluded.append(dict(name=row["vista_name"], frame=int(row["vista_frame"]),
                                     reason="Vista executable loading bank is outside MCC-only onboarding scope"))
                continue
            missing.append(dict(name=row["vista_name"], frame=int(row["vista_frame"]),
                                preservation="unavailable; no state substitution"))
            continue
        if row["status"] not in ("dds-exact", "different-pixels"):
            raise FormatError("Unknown MCC menu inventory comparison")
        cache_name = row["mcc_cache"]
        if Path(cache_name).name != cache_name or not cache_name.endswith(".map"):
            raise FormatError("Unsafe MCC menu cache name")
        by_cache[cache_name].append(row)
    pages, banks, decoded = [], set(), {}
    maps = halo2 / "h2_maps_win64_dx11"
    with (maps / "textures.dat").open("rb") as textures:
        for filename, local in sorted(by_cache.items()):
            progress("Verifying copied Vista menu owners: " + filename)
            cache = MccCache(maps / filename)
            try:
                tags = defaultdict(list)
                for tag in cache.tags.values():
                    if tag["group"] == "bitm" and tag["offset"] is not None:
                        tags[tag["name"]].append(tag)
                for row in local:
                    candidates = tags[row["mcc_name"]]
                    if len(candidates) != 1:
                        raise FormatError("Missing or ambiguous copied menu bitmap: " + row["mcc_name"])
                    tag = candidates[0]
                    frames = cache.block(tag["offset"] + 68, 168)
                    ordinal = int(row["mcc_frame"])
                    if not 0 <= ordinal < len(frames):
                        raise FormatError("Missing copied menu frame: " + row["mcc_name"])
                    at = frames[ordinal]
                    if not tag["offset"] <= at <= at + 168 <= tag["offset"] + tag["size"]:
                        raise FormatError("Copied menu frame escapes its tag owner")
                    width, height = cache.u16(at + 4), cache.u16(at + 6)
                    # MCC stores depth in byte8 and creation flags in byte9.
                    if (width, height) != (int(row["width"]), int(row["height"])) or cache.data[at+8] != 1 or cache.u16(at+10) != 0:
                        raise FormatError("Copied menu frame dimensions/type differ")
                    pointers = tuple(cache.u32(at+56+i*4) for i in range(3))
                    sizes = tuple(cache.u32(at+80+i*4) for i in range(3))
                    if not sizes[0] or any(pointers[1:]) or any(sizes[1:]):
                        raise FormatError("Unsupported split copied menu resource")
                    fmt, mips = cache.u16(at+12), cache.u16(at+20)
                    identity = (pointers[0], sizes[0], width, height, fmt, mips)
                    if identity not in decoded:
                        payload = texture_resource(textures, pointers[0], sizes[0])
                        dds, _ = dds_image(width, height, mips, fmt, payload)
                        decoded[identity] = hashlib.sha256(dds).hexdigest()
                    digest = decoded[identity]
                    if digest != row["dds_sha256"]:
                        raise FormatError("Copied MCC menu DDS differs from verified inventory: " + row["mcc_name"])
                    banks.add(row["vista_name"])
                    pages.append(dict(name=row["mcc_name"], frame=ordinal, cache=filename,
                        datum=f'{tag["datum"]:08x}', width=width, height=height,
                        dds_sha256=digest, source_comparison=row["status"],
                        textures_dat_address=pointers[0] & 0x3fffffff))
            finally:
                cache.close()
    fonts = halo2 / "h2_fonts"
    for name in PRIMARY_FONTS:
        if not (fonts / name).is_file() or (fonts / name).is_symlink():
            raise FormatError("Missing copied MCC menu font/table: " + name)
    font_files = {p.relative_to(halo2).as_posix(): dict(bytes=p.stat().st_size, sha256=file_sha(p))
                  for p in sorted(fonts.rglob("*")) if p.is_file()}
    return dict(schema="H2_MCC_VISTA_MENU_INVENTORY_V1", complete=True,
        inventory_sha256=file_sha(inventory), banks=len(banks), frame_owners=len(pages),
        unique_dds_pages=len({p["dds_sha256"] for p in pages}),
        comparisons=dict(Counter(p["source_comparison"] for p in pages)),
        pages=pages, fonts=font_files, unavailable_in_mcc=missing, excluded_reference_assets=excluded,
        storage="Original MCC caches/textures.dat and case-preserved h2_fonts tree",
        runtime_activation=False)
