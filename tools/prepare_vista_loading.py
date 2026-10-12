#!/usr/bin/env python3
"""Prepare the inspected user-owned Vista loading bank, without DirectX/Wine.

The bounded decoder reproduces d3dx9_31's source66B95D allocation/filter call.
All twelve output payloads must match the actual two-pool Microsoft readback.
No retail pixels, executable or DLL are bundled with this tool.
"""
from array import array
import hashlib
import json
import math
from pathlib import Path
import struct

from export_vista_loading_audio import extract as extract_audio, BANK_SHA

BANK_SHA256 = "cd60764ff194b4073895f9c391b6abc22227999463372d2e8b36b46965ba2586"
OFFSETS = (0, 0x3c0038, 0x440070, 0x4600a8, 0x5200e0, 0x5e0118,
           0x5e8150, 0x6348f8, 0x659470, 0x6a0a88, 0x6c0470, 0x6f54e8)
SHAPES = ((1280, 1024, 24), (1024, 128, 32), (512, 64, 32),
          (512, 512, 24), (512, 512, 24), (2048, 4, 32),
          (1044, 75, 32), (1044, 36, 32), (1044, 70, 32),
          (1044, 31, 32), (1044, 52, 32), (512, 512, 24))
PAYLOAD_SHA256 = (
    "5f725d9f487c484e66ab320d382f892bb96e52551e390ce2574abd039cd95375",
    "c51e855d0a595dd3089321ffeff36617e940a773e82da5f834fb25631f4b6b69",
    "ceaf2c60a3fdf7b9e39dfedee538d5bfb0e8ac465a5c8d184faf99eca122ab0a",
    "8369ee4c875f688208387a06823d4dfd0c70f6db58c30ef713675fc2c08028b7",
    "7602d0ed5464b3c4d4386b6847bfee664d0b1afdb3cf95494dea14b31a44b1c8",
    "bfc6b470ef5cc297d124452bba5b9bf434f526967c83fac9733d6822814114db",
    "9d5498cfbbcc993b59a854fb89caafb10362a921d22a47e2c6ef5396a612606d",
    "d95805b088faf1de04fc4ad9b7ff53298828caeeadae261a804025104d9739a1",
    "465205eb830443c2038893a01f7adf4b36317be754477689002b36c54f39968a",
    "054fdc730187ed002fd3eae423bd4e144c030b98d567e081122944109989e280",
    "8299bc3a386de0a3fece7cc67b2b7b58d3b6171fb8e43b4827166838f07f2311",
    "cb75ffa02ff407898e5de1a56cc1fb5baaa9689f7d7ac1d229149630dd3c66a9",
)
# Original ordered thresholds (0x4c5642/0x4c59f6), including reverse traversal
# on odd rows. DITHER_DIFFUSION is a separate flag and is not requested here.
DITHER = ((31, 15, 27, 11), (19, 3, 23, 7),
          (25, 9, 29, 13), (21, 5, 17, 1))
FLOAT = struct.Struct("<f")


def f32(value):
    return FLOAT.unpack(FLOAT.pack(value))[0]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def contributions(source, target):
    """Source-scatter triangle table from original d3dx9_31 RVA 0xd3c50.

    D3D9's 24-bit FPU precision and each store/operation rounding matter.
    Sorting contributions by source preserves the original accumulation order.
    """
    scale = f32(target / source)
    inverse = f32(.5 / scale)
    result = [[] for _ in range(target)]
    for index in range(source):
        grouped = []
        for side in range(2):
            left = index + side - .5
            start = f32(left * scale)
            end = f32(start + scale)
            for dest in range(math.floor(start), math.ceil(end)):
                a, b = max(dest, start), min(dest + 1, end)
                average = f32(f32(f32(a + b) * inverse) - left)
                if side:
                    average = f32(1 - average)
                weight = f32(f32(b - a) * average)
                wrapped = dest % target
                if grouped and grouped[-1][0] == wrapped:
                    grouped[-1] = (wrapped, f32(grouped[-1][1] + weight))
                else:
                    grouped.append((wrapped, weight))
        for dest, weight in grouped:
            if weight > f32(.00001):
                result[dest].append((index, weight))
    return result


def resize(pixels, width, height, target_width, target_height, *, accelerated=True):
    if (width, height) == (target_width, target_height):
        return pixels
    xs, ys = contributions(width, target_width), contributions(height, target_height)
    # NumPy is optional; the standard-library path uses exactly the same f32
    # operations. Neither path delegates resampling to a different image filter.
    if accelerated:
        try:
            import numpy as np
        except ImportError:
            pass
        else:
            source = (np.frombuffer(pixels, dtype=np.uint8).reshape(height, width, 4)
                      .astype(np.float32) * np.float32(f32(1 / 255)))
            output = np.zeros((target_height, target_width, 4), dtype=np.float32)
            columns = []
            for slot in range(max(map(len, xs))):
                columns.append((np.array([x[slot][0] if len(x) > slot else 0 for x in xs]),
                                np.array([x[slot][1] if len(x) > slot else 0 for x in xs], dtype=np.float32)))
            for y, row in enumerate(ys):
                for source_y, cy in row:
                    for indices, cx in columns:
                        output[y] += source[source_y, indices] * (cx * np.float32(cy))[:, None]
            threshold = np.array(DITHER, dtype=np.float64) / 32
            threshold = threshold[np.arange(target_height)[:, None] % 4,
                                  np.arange(target_width)[None, :] % 4]
            return np.clip(np.floor(output.astype(np.float64) * 255 + threshold[:, :, None]),
                           0, 255).astype(np.uint8).tobytes()
    normal = [f32(i * f32(1 / 255)) for i in range(256)]
    output = bytearray(target_width * target_height * 4)
    for y, row in enumerate(ys):
        values = array("f", [0]) * (target_width * 4)
        for source_y, cy in row:
            for x, column in enumerate(xs):
                for source_x, cx in column:
                    weight = f32(cx * cy)
                    at = (source_y * width + source_x) * 4
                    dest = x * 4
                    for channel in range(4):
                        values[dest + channel] += f32(normal[pixels[at + channel]] * weight)
        for x in range(target_width):
            threshold = DITHER[y % 4][x % 4] / 32
            for channel in range(4):
                output[(y * target_width + x) * 4 + channel] = max(0, min(255,
                    math.floor(values[x * 4 + channel] * 255 + threshold)))
    return bytes(output)


def decode(bank, index, *, accelerated=True):
    if digest(bank) != BANK_SHA256:
        raise ValueError("Unverified Vista loading.bin SHA-256")
    offset = OFFSETS[index]
    width, height, depth = SHAPES[index]
    if (bank[offset:offset + 2] != b"BM" or
        struct.unpack_from("<IIiiHHI", bank, offset + 10) !=
            (54, 40, width, height, 1, depth, 0)):
        raise ValueError("Unsupported Vista loading BMP layout")
    stride = (width * (depth // 8) + 3) & ~3
    rows = []
    for y in range(height - 1, -1, -1):
        row = bank[offset + 54 + y * stride:offset + 54 + y * stride + width * (depth // 8)]
        if depth == 24:
            rgba = bytearray(width * 4)
            for channel in range(3):
                rgba[channel::4] = row[channel::3]
            row = bytes(rgba)
        rows.append(row)
    target_width, target_height = (1 << (width - 1).bit_length(), 1 << (height - 1).bit_length())
    pixels = resize(b"".join(rows), width, height, target_width, target_height, accelerated=accelerated)
    if digest(pixels) != PAYLOAD_SHA256[index]:
        raise ValueError(f"Vista loading texture {index} differs from original Microsoft D3DX readback")
    return pixels, target_width, target_height


def prepare(source, output, progress=print):
    source, output = Path(source), Path(output)
    bank = (source / "loading.bin").read_bytes()
    audio, audio_offset = extract_audio((source / "halo2.exe").read_bytes())
    output.mkdir(parents=True, exist_ok=True)
    manifest, textures = ["H2V_LOADING_TEXTURES_V1"], []
    for index, (width, height, depth) in enumerate(SHAPES):
        progress(f"Preparing original Vista loading texture {index + 1}/12…")
        pixels, tw, th = decode(bank, index)
        alpha = depth == 32
        header = [124, 0x100f, th, tw, tw * 4, 0, 0] + [0] * 11
        header += [32, 0x41 if alpha else 0x40, 0, 32, 0xff0000, 0xff00, 0xff,
                   0xff000000 if alpha else 0, 0x1000, 0, 0, 0, 0]
        name = f"loading-{index:02d}.dds"
        data = b"DDS " + struct.pack("<31I", *header) + pixels
        (output / name).write_bytes(data)
        manifest.append("\t".join(map(str, (index, width, height, tw, th, 11 if alpha else 10, name))))
        textures.append(dict(index=index, path=name, sha256=digest(data), payload_sha256=digest(pixels)))
    (output / "loading-textures.tsv").write_text("\n".join(manifest) + "\n")
    (output / "loading-tone.xbox-adpcm").write_bytes(audio)
    report = dict(schema="H2V_LOADING_PREPARATION_V1", loading_bin_sha256=BANK_SHA256,
                  audio_sha256=BANK_SHA, audio_file_offset=audio_offset, textures=textures,
                  generator_sha256=digest(Path(__file__).read_bytes()),
                  scope="Exact twelve original D3DX9_31 texture payloads and unchanged original loading ambience")
    (output / "source.json").write_text(json.dumps(report, indent=2) + "\n")
    return report
