#!/usr/bin/env python3
"""Audit literal BC3 dialog1 texels that overlay original popup text layers.

No fitted color/alpha/offset: the bitmap position is authored(-139,104) plus
animation55(-131,-73), projected to(50,209). At1:1 the pixel-center sample lands
on the corresponding texel center. The comparator oracle separately establishes
the panel is after layer0 text. Standard source-alpha composition predicts the
retained title and shadow samples; broad GPU/phase parity remains excluded.
"""
import hashlib
import io
import json
from pathlib import Path
import struct

from PIL import Image

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
name = "ui\\global_bitmaps\\dialog1"
datum = next(k for k,t in cache.tags.items() if t["group"] == "bitm" and t["name"] == name)
cache.bitmap(datum)
page = cache.bitmaps[f"{datum:08x}"]["records"][0]
assert (page["width"],page["height"],page["format"]) == (256,256,16)
_, screen = cache.tag(next(k for k,t in cache.tags.items() if t["group"]=="wgit"
                          and cache.u16(t["address"]-cache.magic+4)==52),"wgit")
pane = cache.block(screen+32,76)[0]
definition = next(p for p in cache.block(pane+36,56)
                  if cache.u32(p+28)==datum)
authored = list(struct.unpack_from("<2h",cache.data,definition+12))
animation_id = cache.u16(definition+4)
_, wigl = cache.tag(next(k for k,t in cache.tags.items() if t["group"]=="wigl"),"wigl")
animation = cache.block(wigl+0x120,44)[animation_id-1]
keys = cache.block(animation+36,20) or cache.block(animation+8,20)
offset = list(struct.unpack_from("<3f",cache.data,keys[-1]+8))
projected = [320+authored[0]+offset[0],240-authored[1]-offset[1]]
assert authored == [-139,104] and offset == [-131,-73,0]
assert projected == [50,209]
raw = cache.data[page["resource_pointers"][0]:
                 page["resource_pointers"][0]+page["resource_sizes"][0]]
header = ([124,0x81007,256,256,len(raw),0,1] + [0]*11 +
          [32,4,int.from_bytes(b"DXT5","little"),0,0,0,0,0] + [0x1000,0,0,0,0])
texture = Image.open(io.BytesIO(b"DDS "+struct.pack("<31I",*header)+raw))
native_path = ROOT / "analysis/frontend/customization/corrected-preview/model.png"
original_path = ROOT / "analysis/frontend/profile-regression/original-model-player-model-choice.png"
native = Image.open(native_path).convert("RGB")
original = Image.open(original_path).convert("RGB")
samples = []
for label, point, vertex in [("title",(70,228),(172,195,216)),
                              ("title_shadow",(71,231),(0,0,0)),
                              ("description_shadow",(72,243),(0,0,0))]:
    texel = texture.getpixel((int(point[0]-projected[0]),int(point[1]-projected[1])))
    alpha = texel[3]/255
    predicted = [round(texel[i]*alpha+vertex[i]*(1-alpha)) for i in range(3)]
    observed = list(original.getpixel(point))
    assert predicted == observed
    samples.append(dict(name=label,point=list(point),texel_rgba=list(texel),
                        layer0_text_rgba=[*vertex,255],predicted_rgb=predicted,
                        native_before_layer_correction=list(native.getpixel(point)),
                        original_rgb=observed))
out = dict(scope=__doc__,mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
           bitmap_name=name,bitmap_datum=f"{datum:08x}",page=page,
           compressed_page_sha256=hashlib.sha256(raw).hexdigest(),
           bitmap_authored_position=authored,bitmap_animation_id=animation_id,
           bitmap_animation_index=animation_id-1,bitmap_own_animation_offset=offset,
           bitmap_projected_position=projected,samples=samples,
           comparator_fixture_sha256=hashlib.sha256((ROOT/
             "analysis/frontend/customization/popup-widget-order-original-v2.json").read_bytes()).hexdigest(),
           native_sha256=hashlib.sha256(native_path.read_bytes()).hexdigest(),
           original_sha256=hashlib.sha256(original_path.read_bytes()).hexdigest(),
           decoder="Pillow DDS DXT5/BC3 decoder on unchanged actual page bytes")
path = ROOT/"analysis/frontend/customization/popup-text-overlay-source-audit-v2.json"
path.write_text(json.dumps(out,indent=2)+"\n")
print(f"Three literal panel-texel compositions match original RGB exactly; {path}")
