#!/usr/bin/env python3
"""Export measured D3DX loading textures without resampling or recompression.

Requires the real Microsoft d3dx9_31 texture readback report and both actual
memory-pool outputs from d3dx_decode_probe.cpp. This does not synthesize a BMP
decoder, loading progress, loading lifetime, shader or pixel-parity verdict.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("readback", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root, output = args.readback.resolve(), args.output.resolve()
    report = json.loads((root / "report.json").read_text())
    if not report["pools_identical"]:
        raise RuntimeError("Original D3DX memory-pool outputs differ")
    rows = [row for row in report["textures"] if "index" in row]
    if len(rows) != 24 or {(r["index"], r["pool"]) for r in rows} != {
            (i, p) for i in range(12) for p in range(2)}:
        raise RuntimeError("Incomplete actual D3DX readback")
    output.mkdir(parents=True, exist_ok=True)
    manifest = ["H2V_LOADING_TEXTURES_V1"]
    exported = []
    for index in range(12):
        pair = [next(r for r in rows if r["index"] == index and r["pool"] == p) for p in range(2)]
        raw = []
        for row in pair:
            if any(row[name] for name in ("hresult", "level_result", "lock_result")) or row["levels"] != 1:
                raise RuntimeError("Original D3DX texture creation/readback failed")
            data = (root / f"texture-{index:02d}-pool{row['pool']}.bgra").read_bytes()
            if digest(data) != row["sha256"] or len(data) != row["width"] * row["height"] * 4:
                raise RuntimeError("Original D3DX readback changed")
            raw.append(data)
        row = pair[0]
        if raw[0] != raw[1] or row["format"] not in (21, 22):
            raise RuntimeError("Unsupported or inconsistent actual D3DX output")
        alpha = row["format"] == 21
        # DDS retains the exact BGRA/XRGB image. XRGB's unused byte stays
        # untouched; the native texture reader supplies sampled alpha1.
        header = [124, 0x100f, row["height"], row["width"], row["width"] * 4, 0, 0]
        header += [0] * 11
        header += [32, 0x41 if alpha else 0x40, 0, 32, 0xff0000, 0xff00, 0xff,
                   0xff000000 if alpha else 0, 0x1000, 0, 0, 0, 0]
        assert len(header) == 31
        dds = b"DDS " + struct.pack("<31I", *header) + raw[0]
        name = f"loading-{index:02d}.dds"
        path = output / name
        if not path.exists() or path.read_bytes() != dds:
            path.write_bytes(dds)
        if path.read_bytes()[128:] != raw[0]:
            raise RuntimeError("Exported loading texture payload differs")
        manifest.append("\t".join(map(str, (index, row["source_width"], row["source_height"],
            row["width"], row["height"], 11 if alpha else 10, name))))
        exported.append(dict(index=index, path=name, sha256=digest(dds),
            payload_sha256=digest(raw[0]), source_size=[row["source_width"], row["source_height"]],
            texture_size=[row["width"], row["height"]], alpha="original A8" if alpha else "original X8 sampled as1"))
    text = "\n".join(manifest) + "\n"
    (output / "loading-textures.tsv").write_text(text)
    (output / "source.json").write_text(json.dumps(dict(schema="H2V_LOADING_TEXTURE_EXPORT_V1",
        readback_report_sha256=digest((root / "report.json").read_bytes()),
        generator_sha256=digest(Path(__file__).read_bytes()), source_pins=report["pins"],
        manifest_sha256=digest(text.encode()), textures=exported,
        scope="Actual Microsoft D3DX9_31 decoding under WineGE/DXVK, exact source66B95D parameters; no native loading lifetime or framebuffer parity claim"), indent=2) + "\n")
    print(f"PASS exported12 measured original D3DX textures: {output}")


if __name__ == "__main__":
    main()
