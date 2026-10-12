#!/usr/bin/env python3
"""Freeze pinned Xemu's unchanged BC3 expansion for frontend bitmap regression.

The retained Vulkan texture loader calls this CPU decoder before uploading RGBA.
Only qemu's allocation/include environment is supplied; s3tc.c is unchanged.
Synthetic cases cover both alpha-palette branches/all indices, color ordering,
degenerate/end-point colors, clipped block extents, and the actual dialog1 page.
No native source/build, emulator input, or capture process is modified.
"""
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
RETAINED = ROOT / "analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained"
OUT = ROOT / "analysis/frontend/customization/popup-bc3-audit"
FIXTURES = ROOT / "tests/fixtures"
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "qemu").mkdir(exist_ok=True)
report = json.loads((RETAINED / "report.json").read_text())
assert report["commit"] == "fc24584ce88f0915ad7f04775bb7712c2e3f49ee"
source = RETAINED / "hw/xbox/nv2a/pgraph/s3tc.c"
source_record = next(f for f in report["files"] if f["path"].endswith("/s3tc.c"))
digest = lambda value: hashlib.sha256(value).hexdigest()
assert digest(source.read_bytes()) == source_record["sha256"]
(OUT / "qemu/osdep.h").write_text(
    "#include <assert.h>\n#include <stdbool.h>\n#include <stdint.h>\n"
    "#include <stdlib.h>\n#define g_malloc malloc\n"
    "#define MIN(a,b) ((a)<(b)?(a):(b))\n")
library = OUT / "original-s3tc.so"
subprocess.run(["cc", "-std=c11", "-O2", "-shared", "-fPIC", "-I", str(OUT),
                str(source), "-o", str(library)], check=True)
lib = ctypes.CDLL(str(library))
lib.s3tc_decompress_2d.argtypes = [ctypes.c_int, ctypes.c_void_p,
                                  ctypes.c_uint, ctypes.c_uint]
lib.s3tc_decompress_2d.restype = ctypes.c_void_p
libc = ctypes.CDLL(None)
libc.free.argtypes = [ctypes.c_void_p]

def decode(raw, width, height):
    block = ctypes.create_string_buffer(raw)
    pointer = lib.s3tc_decompress_2d(2, block, width, height)
    result = ctypes.string_at(pointer, width * height * 4)
    libc.free(pointer)
    return result

alphas = [(70,68),(68,70),(68,68),(255,0),(0,255),(0,0),(255,255),
          (1,0),(0,1),(127,128),(128,127),(254,1),(1,254)]
colors = [(0x451f,0x3d1f),(0,0xffff),(0xffff,0),(0x8410,0x8410),
          (0xf800,0x07e0),(0x001f,0x7bef)]
extents = [(1,1),(3,2),(4,4),(5,7)]
rows = []
for a0,a1 in alphas:
    for c0,c1 in colors:
        for width,height in extents:
            raw = bytearray()
            for salt in range(((width+3)//4)*((height+3)//4)):
                alpha = a0 | (a1 << 8)
                color = 0
                for index in range(16):
                    alpha |= ((index+salt)%8) << (16+3*index)
                    color |= ((index+salt)%4) << (2*index)
                raw.extend(struct.pack("<QHHI",alpha,c0,c1,color))
            rows.append((width,height,bytes(raw),decode(bytes(raw),width,height)))
fixture = FIXTURES / "frontend_ui_bc3_original.tsv"
fixture.write_text("# width height raw_bc3_hex original_rgba_hex\n" +
                   "".join(f"{w}\t{h}\t{raw.hex()}\t{rgba.hex()}\n"
                           for w,h,raw,rgba in rows))
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
datum = next(k for k,t in cache.tags.items() if t["group"] == "bitm" and
             t["name"] == "ui\\global_bitmaps\\dialog1")
cache.bitmap(datum)
page = cache.bitmaps[f"{datum:08x}"]["records"][0]
assert (page["width"],page["height"],page["format"]) == (256,256,16)
raw = cache.data[page["resource_pointers"][0]:
                 page["resource_pointers"][0]+page["resource_sizes"][0]]
rgba = decode(raw,256,256)
input_path = FIXTURES / "frontend_ui_dialog1_original.bc3"
output_path = FIXTURES / "frontend_ui_dialog1_original.rgba"
input_path.write_bytes(raw)
output_path.write_bytes(rgba)
samples = []
for point in [(20,19),(21,22),(22,34)]:
    x,y = point
    block_at = ((y//4)*64+x//4)*16
    block = raw[block_at:block_at+16]
    alpha_index = (int.from_bytes(block[:8],"little") >>
                   (16+3*((y%4)*4+x%4))) & 7
    samples.append(dict(texel=list(point),rgba=list(rgba[(y*256+x)*4:(y*256+x+1)*4]),
                        alpha_endpoints=list(block[:2]),alpha_index=alpha_index,
                        block_hex=block.hex()))
result = dict(scope=__doc__,xemu_version=report["version"],xemu_commit=report["commit"],
              original_source_sha256=source_record["sha256"],
              original_source_url=source_record["url"],
              mainmenu_sha256=digest(cache.data),synthetic_cases=len(rows),
              alpha_endpoint_pairs=alphas,color_endpoint_pairs=colors,extents=extents,
              synthetic_fixture_sha256=digest(fixture.read_bytes()),
              bitmap_datum=f"{datum:08x}",original_page_sha256=digest(raw),
              original_rgba_sha256=digest(rgba),samples=samples,
              production_boundary="Only frontend prepare_ui_bitmap is under correction; model/general BC3 is excluded.")
(OUT / "original-bc3-fixtures.json").write_text(json.dumps(result,indent=2)+"\n")
print(f"{len(rows)} unchanged decoder cases +65536 actual dialog1 texels frozen.")
