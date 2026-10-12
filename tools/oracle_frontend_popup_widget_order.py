#!/usr/bin/env python3
"""Freeze retail2356D8 on actual appearance-child text/bitmap metadata.

Constructed collector entries supply original type6/8, copied authored signed
layers, and steady depths from literal animation keys. No qsort tie order or
GPU text/bitmap composition is asserted; every pair executes the original.
"""
import functools
import hashlib
import json
import struct

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN

cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
_, wigl = cache.tag(next(k for k,t in cache.tags.items() if t["group"]=="wigl"),"wigl")
animations = cache.block(wigl+0x120,44)
original = Retail()
cases = []
for screen_id in (49,50,52,233):
    _, screen = cache.tag(next(k for k,t in cache.tags.items() if t["group"]=="wgit"
                       and cache.u16(t["address"]-cache.magic+4)==screen_id),"wgit")
    pane = cache.block(screen+32,76)[0]
    widgets, entries = [], []
    for kind, block_offset, stride, layer_offset in ((6,28,44,40),(8,36,56,32)):
        for index, p in enumerate(cache.block(pane+block_offset,stride)):
            animation_id = cache.u16(p+4)
            # 253765/2B01EB copy authored animation minus1 to widget+68.
            assert animation_id > 0
            animation = animations[animation_id-1]
            ambient = cache.block(animation+36,20)
            intro = cache.block(animation+8,20)
            steady = ambient[-1] if ambient else intro[-1]
            depth = struct.unpack_from("<f",cache.data,steady+16)[0]
            layer = struct.unpack_from("<h",cache.data,p+layer_offset)[0]
            node = original.alloc(bytes(0x100))
            original.w(node+4,kind)
            entry = original.alloc(struct.pack("<Ifh2x",node,depth,layer))
            entries.append(entry)
            widgets.append(dict(kind=kind,index=index,layer=layer,depth=depth,
                                animation=animation_id,animation_index=animation_id-1,
                                bitmap=cache.tags[cache.u32(p+28)]["name"] if kind==8 else None))
    comparisons = []

    def compare(a,b):
        value = original.run(0x2356D8,argv=[entries[a],entries[b]])
        result = struct.unpack("<i",struct.pack("<I",value))[0]
        comparisons.append(dict(left=a,right=b,result=result))
        return result

    for a in range(len(entries)):
        for b in range(len(entries)):
            compare(a,b)
    order = sorted(range(len(entries)),key=functools.cmp_to_key(compare))
    assert all(w["depth"]==0. for w in widgets)
    assert widgets[order[0]]["bitmap"].endswith("appearance_detail")
    groups = []
    for index in order:
        widget = widgets[index]
        key = (widget["layer"],widget["kind"],widget["depth"])
        if not groups or groups[-1]["key"] != list(key):
            groups.append(dict(key=list(key),widgets=[]))
        groups[-1]["widgets"].append(index)
    assert [g["key"][:2] for g in groups] == [[0,8],[0,6],[1,8],[2,8],[5,6]]
    cases.append(dict(screen=screen_id,widgets=widgets,groups=groups,comparisons=comparisons))
out = dict(xbe_sha256=PIN,mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
           scope=__doc__,cases=cases)
path = ROOT/"analysis/frontend/customization/popup-widget-order-original-v2.json"
path.write_text(json.dumps(out,indent=2)+"\n")
print(f"Four original popup cases; {sum(len(c['comparisons']) for c in cases)} comparator calls; {path}")
