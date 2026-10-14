#!/usr/bin/env python3
"""Find installed MCC counterparts for the Vista menu redraw inventory.

Read-only source audit. DDS/PNG exports go to a fresh reference directory,
never to a runtime pack or a source-art bundle. Exact RGBA hashes establish
pixel identity independently of filenames, frame order, or bank ownership.
Same-name/frame candidates are reported separately until their pixels match.
Emblems and HUD/button-prompt banks are outside this audit.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import io
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image

from audit_rendering import file_sha
from export_mcc_rank_reference import inventory
from export_vista_menu_ui import dds_image, mip_shapes, preview, sequences
from import_mcc_hud import MccCache, discover, mcc_directory, texture_resource
from setup_assets import banks

ROOT = Path(__file__).resolve().parents[1]
EMBLEMS = "ui\\global_bitmaps\\emblems\\"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def rgba_key(image):
    w, h = image.size
    return sha(struct.pack("<II", w, h) + image.tobytes())


def excluded(name):
    return name.startswith((EMBLEMS, "ui\\hud\\"))


def save_json(path, value):
    path.write_text(json.dumps(value, indent=2, default=json_bytes) + "\n")


def json_bytes(value):
    if isinstance(value, bytes):
        return {"bytes_hex": value.hex()}
    raise TypeError(type(value).__name__)


def frontend_image(record, payload):
    """Formats/offsets follow the retained MCCTexturePackDumper source."""
    w, h, fmt = (record[k] for k in ("width", "height", "format"))
    if record["depth"] != 1 or not 0 < w <= 16384 or not 0 < h <= 16384:
        raise ValueError("Unsupported frontend image extent")
    if fmt == 0:
        needed = w * h * 4
        if len(payload) < needed:
            raise ValueError("Truncated frontend RGBA image")
        return Image.frombytes("RGBA", (w, h), payload[:needed])
    if fmt not in (1, 2, 3):
        raise ValueError("Unsupported frontend format")
    needed = ((w + 3) // 4) * ((h + 3) // 4) * (8 if fmt == 1 else 16)
    if len(payload) < needed:
        raise ValueError("Truncated frontend BC image")
    words = [124, 0x81007, h, w, needed, 0, 1] + [0] * 11
    words += [32, 4, int.from_bytes({1: b"DXT1", 2: b"DXT3", 3: b"DXT5"}[fmt], "little"),
              0, 0, 0, 0, 0, 0x1000, 0, 0, 0, 0]
    dds = b"DDS " + struct.pack("<31I", *words) + payload[:needed]
    return Image.open(io.BytesIO(dds)).convert("RGBA")


def inline_data(path):
    data = path.read_bytes()
    at, result = 0, []
    while at < len(data):
        if len(data) - at < 16:
            raise ValueError("Truncated inline chunk")
        uid, length, _, _ = struct.unpack_from("<4I", data, at)
        end = at + 16 + length
        if not at < end <= len(data):
            raise ValueError("Invalid inline chunk extent")
        if uid == 0x5e73cdd7:
            result.append((at + 16, data[at + 16:end]))
        at = end
    return result


def audit(mcc, reference, output):
    output.mkdir(parents=True, exist_ok=False)
    original = json.loads(reference.read_text())
    targets = {k: v for k, v in original["pages"].items()
               if any(not excluded(u["name"]) for u in v["uses"])}
    originals, dds_keys = {}, defaultdict(set)
    for key, page in targets.items():
        path = reference.parent / page["png"]
        if file_sha(path) != page["png_sha256"]:
            raise ValueError("Changed Vista reference: " + str(path))
        image = Image.open(path).convert("RGBA")
        if rgba_key(image) != key:
            raise ValueError("Vista RGBA identity disagrees with its inventory")
        originals[key] = image
    for local in original["owners"].values():
        for bank in local:
            if excluded(bank["name"]):
                continue
            for frame in bank["frames"]:
                if frame["page"] in targets:
                    dds_keys[frame["dds_sha256"]].add(frame["page"])

    report = dict(schema="halo2-mcc-vista-menu-audit-v1", complete=False,
        scope=dict(vista_reference=str(reference.resolve()), reference_sha256=file_sha(reference),
            excluded=["emblem customization banks", "HUD and button prompt banks"],
            included="Menu keyboard keys, controller illustrations and status icons remain in scope"),
        source=dict(mcc_halo2=str(mcc.resolve())), classic_maps=[], classic_banks=[],
        classic_images={}, frontend_packs=[], frontend_images=[], errors=[], pages={})
    maps = mcc / "h2_maps_win64_dx11"
    textures = maps / "textures.dat"
    report["source"]["textures"] = dict(path=str(textures), bytes=textures.stat().st_size,
        sha256=file_sha(textures))
    resource_seen, bank_seen = {}, {}
    image_dir = output / "reference"
    image_dir.mkdir()
    with textures.open("rb") as stream:
        paths = sorted(maps.glob("*.map"), key=lambda p: (p.name != "mainmenu.map", p.name))
        for path in paths:
            print("Classic cache: " + path.name, flush=True)
            cache = MccCache(path)
            local = {n: b for n, b in banks(cache).items() if not excluded(n)}
            report["classic_maps"].append(dict(file=path.name, sha256=cache.provenance["sha256"],
                decoded_sha256=cache.provenance["decoded_sha256"], banks=len(local)))
            for name, (frames, effective) in sorted(local.items()):
                records = []
                for ordinal, at in enumerate(frames):
                    fields = struct.unpack_from("<6H", cache.data, at + 4)
                    w, h, depth, kind, fmt, flags = fields
                    mips = cache.u16(at + 20)
                    identity = (*fields, mips, bytes(cache.data[at + 56:at + 68]),
                                bytes(cache.data[at + 80:at + 92]))
                    if identity not in resource_seen:
                        try:
                            if depth != 1 or kind != 0 or flags & 8:
                                raise ValueError("Unsupported Classic image storage")
                            levels = mip_shapes(w, h, mips, fmt)
                            raw, resource = texture_resource(stream, cache, at, sum(l["bytes"] for l in levels))
                            dds, _ = dds_image(w, h, mips, fmt, raw)
                            image = preview(w, h, fmt, raw, dds)
                            key = rgba_key(image)
                            dds_sha = sha(dds)
                            dds_path = image_dir / (dds_sha + ".dds")
                            if not dds_path.exists():
                                dds_path.write_bytes(dds)
                            png_path = image_dir / (key + ".png")
                            if not png_path.exists():
                                image.save(png_path)
                            item = dict(width=w, height=h, format=fmt, additional_mips=mips,
                                rgba_key=key, dds_sha256=dds_sha, resource=resource,
                                dds=dds_path.relative_to(output).as_posix(),
                                png=png_path.relative_to(output).as_posix(), png_sha256=file_sha(png_path))
                            resource_seen[identity] = item
                            report["classic_images"][dds_sha] = item
                        except Exception as exc:
                            resource_seen[identity] = dict(error=str(exc), width=w, height=h, format=fmt)
                            report["errors"].append(dict(source=path.name, name=name, frame=ordinal, error=str(exc)))
                    records.append(dict(frame=ordinal, registration=[cache.u16(at + i) for i in (16, 18)],
                        **resource_seen[identity]))
                tag = next(t for t in cache.tags.values() if t["name"] == name and t["group"] == "bitm")
                seq = sequences(cache, tag["offset"], len(frames))
                sig = sha(json.dumps([name, records, effective, seq], sort_keys=True, default=json_bytes).encode())
                if sig in bank_seen:
                    bank_seen[sig]["caches"].append(path.name)
                else:
                    bank = dict(name=name, caches=[path.name], frames=records,
                        effective_sequences=effective, sequences=seq)
                    bank_seen[sig] = bank
                    report["classic_banks"].append(bank)
            del cache
            save_json(output / "classic-progress.json", report)

    packs = mcc.parent / "Data/UI/texturepacks"
    for perm in sorted(packs.glob("*.perm.bin")):
        print("Frontend pack: " + perm.name, flush=True)
        records, perm_sha = inventory(perm)
        pack = dict(file=perm.name, sha256=perm_sha, definitions=len(records))
        report["frontend_packs"].append(pack)
        if perm.name.startswith(("emblems", "controller")):
            pack["excluded"] = "Emblems/button prompts are outside the requested scope"
            continue
        temp = perm.with_name(perm.name.replace(".perm.bin", ".temp.bin"))
        inline = None if temp.exists() else inline_data(perm)
        if inline is not None and len(inline) != len(records):
            raise ValueError("Inline frontend texture/data counts differ")
        if temp.exists():
            pack["payload_file"] = temp.name
            pack["payload_sha256"] = file_sha(temp)
        with (temp if temp.exists() else perm).open("rb") as stream:
            for i, record in enumerate(records):
                item = dict(pack=perm.name, **record)
                try:
                    if inline is None:
                        offset = record["offset"]
                        if not 0 <= offset <= offset + record["size"] <= temp.stat().st_size:
                            raise ValueError("Frontend payload outside temp file")
                        stream.seek(offset)
                        payload = stream.read(record["size"])
                    else:
                        offset, payload = inline[i]
                        if len(payload) != record["size"]:
                            raise ValueError("Inline texture/data sizes differ")
                    item["payload_offset"] = offset
                    item["payload_sha256"] = sha(payload)
                    image = frontend_image(record, payload)
                    item["rgba_key"] = rgba_key(image)
                    item["visible_bbox"] = image.getchannel("A").getbbox()
                    # Small reference thumbnails are for visual discovery only.
                    thumb = image.copy()
                    thumb.thumbnail((256, 256), Image.Resampling.LANCZOS)
                    thumb_path = image_dir / (item["rgba_key"] + "-thumb.png")
                    if not thumb_path.exists():
                        thumb.save(thumb_path)
                    item["thumbnail"] = thumb_path.relative_to(output).as_posix()
                except Exception as exc:
                    item["error"] = str(exc)
                    report["errors"].append(dict(source=perm.name, name=record["name"], error=str(exc)))
                report["frontend_images"].append(item)

    classic_by_key, classic_by_owner = defaultdict(list), defaultdict(list)
    for bank in report["classic_banks"]:
        for frame in bank["frames"]:
            if "rgba_key" not in frame:
                continue
            entry = dict(name=bank["name"], caches=bank["caches"], **frame)
            classic_by_key[frame["rgba_key"]].append(entry)
            classic_by_owner[bank["name"], frame["frame"]].append(entry)
    frontend_by_key = defaultdict(list)
    for item in report["frontend_images"]:
        if "rgba_key" in item:
            frontend_by_key[item["rgba_key"]].append(item)

    groups_path = reference.parent.parent / "menu-branding-20261013/remaining-inventory.json"
    groups = json.loads(groups_path.read_text())["groups"] if groups_path.exists() else {}
    page_groups = {r["page"]: group for group, rows in groups.items() for r in rows}
    portrait_path = reference.parent.parent / "menu-illustrations-pass10-20261013/portrait-surface-plan.json"
    portraits = {r["page"] for r in json.loads(portrait_path.read_text())["pages"]} if portrait_path.exists() else set()
    for key, page in targets.items():
        candidates, seen = [], set()
        for use in page["uses"]:
            for match in classic_by_owner[use["name"], use["frame"]]:
                identity = match["name"], match["frame"], match["dds_sha256"]
                if identity in seen:
                    continue
                seen.add(identity)
                match = dict(match)
                match["same_dimensions"] = [match["width"], match["height"]] == [page["width"], page["height"]]
                match["larger_in_both_axes"] = match["width"] > page["width"] and match["height"] > page["height"]
                match["rgba_exact"] = match["rgba_key"] == key
                match["dds_exact"] = key in dds_keys[match["dds_sha256"]]
                if match["same_dimensions"] and not match["rgba_exact"]:
                    decoded = Image.open(output / match["png"]).convert("RGBA")
                    a = np.asarray(originals[key], dtype=np.int16)
                    b = np.asarray(decoded, dtype=np.int16)
                    delta = np.abs(a - b)
                    match["difference"] = dict(max_rgba=[int(x) for x in delta.max(axis=(0, 1))],
                        changed_pixels=int(np.any(delta, axis=2).sum()),
                        rmse_rgba=float(np.sqrt(np.mean(delta.astype(np.float64) ** 2))))
                candidates.append(match)
        exact = classic_by_key[key]
        report["pages"][key] = dict(width=page["width"], height=page["height"], uses=page["uses"],
            unfinished_group=page_groups.get(key), portrait_study=key in portraits,
            classification="exact-pixels" if exact or frontend_by_key[key] else
                "same-owner-different-pixels" if candidates else "no-classic-owner-or-exact-pixel-match",
            exact_classic=exact, exact_frontend=frontend_by_key[key], same_owner_candidates=candidates)

    by_name = defaultdict(list)
    for bank in report["classic_banks"]:
        by_name[bank["name"]].append(bank)
    report["bank_compatibility"] = []
    seen_banks = {}
    for cache_name, local in original["owners"].items():
        if cache_name == "loading.bin":
            continue
        for bank in local:
            if excluded(bank["name"]):
                continue
            signature = sha(json.dumps(bank, sort_keys=True).encode())
            if signature in seen_banks:
                seen_banks[signature]["vista_caches"].append(cache_name)
                continue
            matches = []
            for donor in by_name[bank["name"]]:
                matches.append(dict(caches=donor["caches"],
                    page_count_equal=len(bank["frames"]) == len(donor["frames"]),
                    vista_pages=len(bank["frames"]), mcc_pages=len(donor["frames"]),
                    sequence_metadata_equal=bank["sequences"] == donor["sequences"],
                    registration_equal_on_shared_frames=all(
                        a.get("registration") == b["registration"]
                        for a, b in zip(bank["frames"], donor["frames"]))))
            entry = dict(name=bank["name"], vista_caches=[cache_name], mcc=matches)
            seen_banks[signature] = entry
            report["bank_compatibility"].append(entry)

    rows = ["vista_page\tvista_name\tvista_frame\tstatus\tmcc_cache\tmcc_name\tmcc_frame\twidth\theight\tdds_sha256\ttextures_dat_address"]
    for key, page in report["pages"].items():
        uses = sorted({(u["name"], u["frame"]) for u in page["uses"] if not excluded(u["name"])})
        for name, ordinal in uses:
            candidates = [c for c in page["same_owner_candidates"] if c["name"] == name and c["frame"] == ordinal]
            match = next((c for c in candidates if c["rgba_exact"]), candidates[0] if candidates else None)
            fields = [key, name, ordinal, "dds-exact" if match and match["dds_exact"] else
                "rgba-exact" if match and match["rgba_exact"] else "different-pixels" if match else "missing"]
            fields += [match["caches"][0], match["name"], match["frame"], match["width"], match["height"],
                match["dds_sha256"], hex(match["resource"]["address"])] if match else [""] * 7
            rows.append("\t".join(map(str, fields)))
    (output / "importer-map.tsv").write_text("\n".join(rows) + "\n")
    report["summary"] = dict(vista_unique_pages=len(targets),
        excluded_vista_unique_pages=len(original["pages"]) - len(targets),
        classifications=dict(Counter(p["classification"] for p in report["pages"].values())),
        classic_maps=len(report["classic_maps"]), classic_bank_variants=len(report["classic_banks"]),
        classic_unique_dds=len(report["classic_images"]), frontend_packs=len(report["frontend_packs"]),
        frontend_images=len(report["frontend_images"]), errors=len(report["errors"]))
    report["complete"] = not report["errors"]
    report["policy"] = dict(purpose="Local reference and importer audit only; no runtime activation",
        pixel_identity="SHA256 of width, height and full decoded RGBA bytes; no resampling",
        owner_candidates="Names and ordinals alone never establish artwork or sprite ownership equality",
        higher_resolution="Larger extents alone do not establish faithful higher-resolution artwork",
        source_art="No redraw assets, bundled masters, cache selections or original files modified")
    save_json(output / "report.json", report)
    print(json.dumps(report["summary"], indent=2), flush=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mcc-root", type=Path, default=None, help="Installed MCC halo2 directory")
    parser.add_argument("--reference", type=Path,
        default=ROOT / "analysis/frontend/vista-menu-vectors/pack/reference.json")
    parser.add_argument("--output", type=Path, required=True, help="Fresh reference-only directory")
    args = parser.parse_args()
    audit(mcc_directory(args.mcc_root) if args.mcc_root else discover(), args.reference, args.output)


if __name__ == "__main__":
    main()
