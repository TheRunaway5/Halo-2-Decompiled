#!/usr/bin/env python3
"""Freeze original BC2 decoding, without compiling or using a GPU.

The decoder is the already-built, hash-pinned unchanged Xemu s3tc.c library.
Its source, header, allocation shim, historical build generator and dependency
record must all verify before ctypes loads it. No native decoder is used.

Actual pages are selected by literal direct wgit references, not filenames
alone. The full mainmenu inventory is retained separately; synthetic cube
fixtures do not imply that this cache contains or displays a BC2 cubemap.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
COMMIT = "fc24584ce88f0915ad7f04775bb7712c2e3f49ee"
SOURCE_TREE = ROOT / "analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained"
HISTORY = ROOT / "analysis/multiplayer-shading/weather-point-original-reference-event215873/packet.json"
HISTORY_SHA = "f0b7b6634df523ae29757f2920e722ab4c880d91136e4220adc7352ee0e3c512"
SOURCE_SHA = "bf25e6ffa9cf1c3620b974cf638f9838ea298223d0fa7f1c73910fa761fc9c4a"
HEADER_SHA = "1ec2f492095afd7dcd0b0ad02e74cc54f991960a0bcca279d9b5fde995ab8d1b"
LIBRARY = ROOT / "analysis/multiplayer-shading/weather-point-original-reference-event215873/decoder/original-s3tc.so"
LIBRARY_SHA = "c3dc4f7125545482dbf4cda414da155c5dee8ffdf2c86d58f66861c6c8471114"
MAINMENU_SHA = "1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b"
DEFAULT_OUT = ROOT / "analysis/frontend/title-camera-matrix-audit/world-bc2-original-oracle"
SELECTED = (
    (0xE52C030C, 0, "title-logo", (9,)),
    (0xE5350315, 0, "shell-framing", (5,)),
    (0xE59D037D, 0, "full-dialog-panel", (23, 167, 206)),
    (0xE59D037D, 1, "full-dialog-secondary-page", (185,)),
    (0xE5590339, 0, "appearance-child-panel", (49, 50, 52, 233)),
    (0xE5EE03CE, 0, "create-profile-placeholder", (193,)),
)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def artifact(path):
    raw = path.read_bytes()
    return dict(path=str(path.relative_to(ROOT)), bytes=len(raw), sha256=digest(raw))


def verify_decoder():
    history_raw = HISTORY.read_bytes()
    require(digest(history_raw) == HISTORY_SHA, "historical decoder build record changed")
    history = json.loads(history_raw)
    dependencies = {row["path"]: row for row in history["dependencies"]}
    sources = []
    tree_raw = (SOURCE_TREE / "tree.json").read_bytes()
    tree = json.loads(tree_raw)
    require(tree["sha"] == COMMIT and not tree["truncated"], "wrong original Xemu tree")
    blobs = {row["path"]: row["sha"] for row in tree["tree"] if row["type"] == "blob"}
    for name, expected in (("s3tc.c", SOURCE_SHA), ("s3tc.h", HEADER_SHA)):
        path = SOURCE_TREE / "hw/xbox/nv2a/pgraph" / name
        raw = path.read_bytes()
        relative = str(path.relative_to(SOURCE_TREE))
        blob = hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
        require(digest(raw) == expected and blob == blobs[relative], "original source pin changed")
        row = artifact(path)
        require(row == dependencies[row["path"]], "original source differs from build dependency")
        sources.append(row | dict(git_blob=blob))
    shim = LIBRARY.parent / "qemu/osdep.h"
    expected_shim = (b"#include <assert.h>\n#include <stdbool.h>\n#include <stdint.h>\n"
                     b"#include <stdlib.h>\n#define g_malloc malloc\n"
                     b"#define MIN(a,b) ((a)<(b)?(a):(b))\n")
    require(shim.read_bytes() == expected_shim, "decoder allocation shim changed")
    for path in (shim, LIBRARY):
        row = artifact(path)
        require(row == dependencies[row["path"]], "retained decoder build artifact changed")
        sources.append(row)
    require(digest(LIBRARY.read_bytes()) == LIBRARY_SHA, "unverified decoder library")
    tool = history["tool"]
    require(artifact(ROOT / tool["path"]) == tool, "historical decoder build generator changed")
    header = (SOURCE_TREE / "hw/xbox/nv2a/pgraph/s3tc.h").read_text()
    require("S3TC_DECOMPRESS_FORMAT_DXT1,\n    S3TC_DECOMPRESS_FORMAT_DXT3,\n"
            "    S3TC_DECOMPRESS_FORMAT_DXT5," in header, "original BC2 enum order changed")
    library = ctypes.CDLL(str(LIBRARY))
    function = library.s3tc_decompress_2d
    function.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint]
    function.restype = ctypes.c_void_p
    free = ctypes.CDLL(None).free
    free.argtypes = [ctypes.c_void_p]

    def decode(raw, width, height, original_enum=1):
        require(0 < width <= 4096 and 0 < height <= 4096, "unsupported oracle extent")
        stride = 8 if original_enum == 0 else 16
        require(len(raw) == ((width + 3) // 4) * ((height + 3) // 4) * stride,
                "oracle compressed input size differs")
        storage = ctypes.create_string_buffer(raw)
        result = function(original_enum, storage, width, height)
        require(bool(result), "original decoder returned null")
        try:
            return ctypes.string_at(result, width * height * 4)
        finally:
            free(result)

    provenance = dict(
        commit=COMMIT, source_files=sources, retained_build_record=artifact(HISTORY),
        historical_generator=tool, source_tree=artifact(SOURCE_TREE / "tree.json"),
        historical_compile_argv=["cc", "-std=c11", "-O2", "-shared", "-fPIC", "-I",
                                 str(shim.parent.parent.relative_to(ROOT)), sources[0]["path"],
                                 "-o", str(LIBRARY.relative_to(ROOT))],
        verification="Existing library/dependency/source/header/shim/build-generator hashes checked; no compiler was run by this oracle.",
        invoked_symbol="s3tc_decompress_2d", original_format_enum=1,
        original_format_name="S3TC_DECOMPRESS_FORMAT_DXT3", native_decoder_used=False)
    return decode, provenance


def block(c0, c1, rotation, salt):
    alpha = colors = 0
    for pixel in range(16):
        alpha |= ((pixel + rotation + salt) & 15) << (pixel * 4)
        colors |= ((pixel + rotation + salt) & 3) << (pixel * 2)
    return struct.pack("<QHHI", alpha, c0, c1, colors)


def compressed(width, height, c0, c1, rotation, salt=0):
    return b"".join(block(c0, c1, rotation, salt + i)
                    for i in range(((width + 3) // 4) * ((height + 3) // 4)))


def alpha_words(raw, width, height):
    blocks_x = (width + 3) // 4
    for y in range(height):
        for x in range(width):
            at = ((y // 4) * blocks_x + x // 4) * 16
            alpha = struct.unpack_from("<Q", raw, at)[0]
            yield ((alpha >> (((y % 4) * 4 + x % 4) * 4)) & 15) * 17


def verify_alpha(raw, width, height, rgba):
    require(rgba[3::4] == bytes(alpha_words(raw, width, height)),
            "original output does not follow its DXT3 explicit alpha nibbles")


def direct_references(cache):
    references = {}
    for datum, tag in cache.tags.items():
        if tag["group"] != "wgit" or not tag["address"]:
            continue
        _, source = cache.tag(datum, "wgit")
        screen = cache.u16(source + 4)
        for pane_index, pane in enumerate(cache.block(source + 32, 76)):
            for index, widget in enumerate(cache.block(pane + 36, 56)):
                bitmap = cache.u32(widget + 28)
                references.setdefault(bitmap, []).append(dict(
                    screen=screen, screen_datum=f"{datum:08X}", pane=pane_index,
                    direct_bitmap_ordinal=index, authored_frame=struct.unpack_from("<h", cache.data, widget + 10)[0],
                    definition_offset=widget, definition_raw=cache.data[widget:widget + 56].hex()))
    return references


def source_payload(cache, page):
    pointer, size = page["resource_pointers"][0], page["resource_sizes"][0]
    location, offset = pointer >> 30, pointer & 0x3FFFFFFF
    source = cache.path.parent / [cache.path.name, "mainmenu.map", "shared.map", "single_player_shared.map"][location]
    with source.open("rb") as stream:
        stream.seek(offset)
        raw = stream.read(size)
    require(len(raw) == size, "source BC2 partition is truncated")
    return raw, dict(path=str(source.relative_to(ROOT)), offset=offset, size=size,
                     sha256=digest(raw), resource_pointer=f"{pointer:08X}")


def write_image(output, label, layers, levels, mips, usage):
    require(len(mips) == layers * levels, "image does not cover every face/mip")
    raw, rgba, layout = bytearray(), bytearray(), []
    for index, (width, height, data, expected) in enumerate(mips):
        layout.append(dict(face=index // levels, mip=index % levels,
                           width=width, height=height, compressed_offset=len(raw),
                           compressed_size=len(data), rgba_offset=len(rgba), rgba_size=len(expected)))
        raw.extend(data)
        rgba.extend(expected)
    source_path, result_path, layout_path = (output / (label + extension)
                                            for extension in (".bc2", ".rgba", ".layout.json"))
    source_path.write_bytes(raw)
    result_path.write_bytes(rgba)
    layout_path.write_text(json.dumps(layout, indent=2) + "\n")
    return dict(label=label, width=mips[0][0], height=mips[0][1], layers=layers,
                levels=levels, source_format=15, output_format=11,
                layout_order="face-major", raw=artifact(source_path), rgba=artifact(result_path),
                layout=artifact(layout_path), usage=usage)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    output = args.output.resolve()
    require(output.is_relative_to(ROOT), "oracle output must be inside the workspace")
    require(not output.exists(), "oracle output must be fresh; existing evidence is never replaced")
    decode, decoder = verify_decoder()
    cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
    try:
        require(digest(cache.data) == MAINMENU_SHA, "mainmenu source pin changed")
        references = direct_references(cache)
        inventory = []
        for datum, tag in cache.tags.items():
            if tag["group"] != "bitm" or not tag["address"]:
                continue
            cache.bitmap(datum)
            for page_index, page in enumerate(cache.bitmaps[f"{datum:08x}"]["records"]):
                if page["format"] == 15:
                    inventory.append(dict(datum=f"{datum:08X}", name=tag["name"], page=page_index,
                                          **page, direct_wgit_references=references.get(datum, [])))
        require(len(inventory) == 127 and all(row["type"] == 0 for row in inventory),
                "original BC2 inventory differs from the retained 127 2D pages")
        for datum, page_index, _, screens in SELECTED:
            require({row["screen"] for row in references.get(datum, [])}.issuperset(screens),
                    "selected actual BC2 image has no claimed authored frontend reference")
            record = cache.bitmaps[f"{datum:08x}"]["records"][page_index]
            require(record["format"] == 15 and record["type"] == 0 and record["declared_additional_mips"] == 0,
                    "selected frontend BC2 page format/mip declaration changed")
        placeholder_path = ROOT / "analysis/frontend/profile-row-appearance/profile-preview-placeholder-original.json"
        placeholder_raw = placeholder_path.read_bytes()
        require(digest(placeholder_raw) == "769893a6c82e3726bd2a0d0310d781f527708de3a97bad44bb162ac3aeb4704d",
                "unchanged original placeholder admission proof changed")
        placeholder = json.loads(placeholder_raw)
        require(placeholder["placeholder_runtime_bitmap_index"] == 6 and
                placeholder["placeholder_datum"] == "e5ee03ce" and len(placeholder["cases"]) == 7,
                "actual placeholder use is unproved")
        output.mkdir(parents=True)
        colors = ((0, 0xFFFF), (0xFFFF, 0), (0x8410, 0x8410),
                  (0xF800, 0x07E0), (0x001F, 0x7BEF), (0x451F, 0x3D1F))
        extents = ((1, 1), (1, 4), (3, 2), (4, 4), (5, 7), (7, 5), (8, 8), (9, 1))
        rows, alpha_seen, color_seen, nibble_positions = [], set(), set(), set()
        for color_index, (c0, c1) in enumerate(colors):
            for rotation in range(16):
                for width, height in extents:
                    raw = compressed(width, height, c0, c1, rotation)
                    rgba = decode(raw, width, height)
                    verify_alpha(raw, width, height, rgba)
                    alpha_seen.update(rgba[3::4])
                    for pixel in range(16):
                        nibble_positions.add((pixel, (pixel + rotation) & 15))
                        color_seen.add((pixel + rotation) & 3)
                    label = f"colors{color_index}-rotation{rotation}-{width}x{height}"
                    rows.append(f"{label}\t{width}\t{height}\t{raw.hex()}\t{rgba.hex()}\n")
        require(len(rows) == 768 and len(nibble_positions) == 256 and color_seen == set(range(4))
                and alpha_seen == {17 * x for x in range(16)}, "synthetic BC2 coverage is incomplete")
        synthetic = output / "synthetic.tsv"
        synthetic.write_text("# BC2_ORIGINAL_SYNTHETIC_V1 label width height raw_bc2_hex original_rgba_hex\n" + "".join(rows))

        images = []
        sweep_raw = b"".join(block(word, word ^ 0xFFFF, word & 15, 0) for word in range(65536))
        sweep_rgba = decode(sweep_raw, 1024, 1024)
        verify_alpha(sweep_raw, 1024, 1024, sweep_rgba)
        images.append(write_image(output, "endpoint-sweep", 1, 1,
                                  [(1024, 1024, sweep_raw, sweep_rgba)],
                                  "synthetic: every RGB565 value in both endpoint positions; all four color selectors and all16 alpha nibbles in every block"))
        for label, width, height, layers, levels in (
                ("clipped-2d-chain", 9, 7, 1, 4),
                ("cube-small", 1, 1, 6, 1),
                ("cube-subblock-mips", 8, 8, 6, 4),
                ("cube-three-mips", 16, 16, 6, 3)):
            mips = []
            for face in range(layers):
                for mip in range(levels):
                    w, h = max(1, width >> mip), max(1, height >> mip)
                    c0, c1 = colors[(face + mip) % len(colors)]
                    data = compressed(w, h, c0, c1, (face * 3 + mip) & 15, face * 7 + mip * 13)
                    expected = decode(data, w, h)
                    verify_alpha(data, w, h, expected)
                    mips.append((w, h, data, expected))
            images.append(write_image(output, label, layers, levels, mips,
                                      "synthetic layout/decoder coverage; no original BC2 cube consumer is claimed"))
        for datum, page_index, label, _ in SELECTED:
            tag = cache.tags[datum]
            page = cache.bitmaps[f"{datum:08x}"]["records"][page_index]
            partition, resource = source_payload(cache, page)
            width, height = page["width"], page["height"]
            length = ((width + 3) // 4) * ((height + 3) // 4) * 16
            require(len(partition) >= length, "actual BC2 base mip escapes source partition")
            raw = partition[:length]
            rgba = decode(raw, width, height)
            verify_alpha(raw, width, height, rgba)
            item = write_image(output, label, 1, 1, [(width, height, raw, rgba)],
                               "actual direct-wgit-bound image; authored tag reference is not a synchronized framebuffer or per-page runtime-selection proof")
            item.update(datum=f"{datum:08X}", page=page_index, name=tag["name"],
                        source_resource=resource, unused_partition_bytes=len(partition) - length,
                        direct_wgit_references=references[datum])
            images.append(item)

        # This independently executed format control distinguishes DXT3's
        # unconditional four-color RGB path from DXT1's transparent palette.
        control = block(0, 0xFFFF, 0, 0)
        bc2 = decode(control, 4, 4)
        bc1 = decode(control[8:], 4, 4, 0)
        different_rgb = sum(bc2[i] != bc1[i] for i in range(64) if i % 4 != 3)
        require(different_rgb > 0, "original endpoint-order control was ineffective")
        manifest = output / "manifest.tsv"
        manifest.write_text("# BC2_ORIGINAL_IMAGES_V1 label width height layers levels source_format raw rgba layout usage\n" +
                            "".join("\t".join(map(str, (row["label"], row["width"], row["height"], row["layers"],
                                                          row["levels"], row["source_format"],
                                                          Path(row["raw"]["path"]).name, Path(row["rgba"]["path"]).name,
                                                          Path(row["layout"]["path"]).name, row["usage"]))) + "\n" for row in images))
        result = dict(schema="BC2_ORIGINAL_ORACLE_V1", passed=True, generator=artifact(Path(__file__)),
                      mainmenu=dict(path=str(cache.path.relative_to(ROOT)), sha256=MAINMENU_SHA),
                      decoder=decoder, synthetic=dict(cases=len(rows), color_endpoint_pairs=colors,
                      clipped_extents=extents, alpha_nibble_position_pairs=len(nibble_positions),
                      alpha_values=sorted(alpha_seen), color_selectors=sorted(color_seen), fixture=artifact(synthetic)),
                      endpoint_sweep=dict(blocks=65536, pixels=1048576, both_endpoint_values=65536),
                      image_manifest=artifact(manifest), images=images,
                      BC2_inventory_count=len(inventory), actual_BC2_cube_count=0,
                      inventory_direct_wgit_bound_pages=sum(bool(row["direct_wgit_references"]) for row in inventory),
                      inventory_only_pages=sum(not row["direct_wgit_references"] for row in inventory),
                      inventory=inventory, actual_placeholder_admission_proof=artifact(placeholder_path),
                      format_control=dict(original_DXT1_vs_DXT3_RGB_differences=different_rgb,
                                          compressed_hex=control.hex(), DXT3_RGBA_hex=bc2.hex(), DXT1_RGBA_hex=bc1.hex()),
                      total_image_mip_faces=sum(row["layers"] * row["levels"] for row in images),
                      total_image_rgba_bytes=sum(row["rgba"]["bytes"] for row in images),
                      compiler_operations=0, GPU_operations=0, production_mutations=0,
                      boundary=["The existing hash-pinned original decoder library was executed offline; it was not rebuilt in this run.",
                                "Mainmenu contains127 BC2 pages, all2D. Cube chains are synthetic decoder/layout coverage only.",
                                "Direct wgit references prove authored frontend use of these bitmap tags, not live visibility of every page.",
                                "The separate unchanged193 fixture proves CreateNew admission of E5EE03CE; no other screen constructor is executed here.",
                                "No native BC2 decoder, GPU storage/filtering, mip residency, live framebuffer or whole-menu parity is admitted."])
        (output / "oracle.json").write_text(json.dumps(result, indent=2) + "\n")
        print(f"768 original BC2 cases +65536 endpoint blocks +{len(SELECTED)} actual frontend pages; "
              f"{result['total_image_mip_faces']} image mip/faces; no compiler/GPU/native changes")
        print("oracle.json SHA256", digest((output / "oracle.json").read_bytes()))
    finally:
        cache.close()


if __name__ == "__main__":
    main()
