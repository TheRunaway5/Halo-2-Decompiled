#!/usr/bin/env python3
"""Execute original small-font Settings label layout and final glyph quads.

Actual screen19 strings/bounds/RGB, original conduit-12 records and wigl+4C
are inputs. The full-reveal, settled-animation case explicitly supplies alpha1
and canonical projected bounds. Original13F0E0/13EF40/13F700/1F490/1F3A0 run
unchanged. Ready font/pixel cache services and a fixed atlas origin are supplied;
the final graphics call only records vertices. No GPU or framebuffer claim.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP


def identity(path):
    raw = path.read_bytes()
    return dict(path=str(path.relative_to(ROOT)), bytes=len(raw),
                sha256=hashlib.sha256(raw).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT /
                        "analysis/frontend/settings-text-source-audit/glyph-layout-original")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
    assert hashlib.sha256(cache.data).hexdigest() == \
        "1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b"
    _, globals_data = cache.tag(next(d for d,t in cache.tags.items()
                                     if t["group"] == "wigl"), "wigl")
    scale_word = cache.u32(globals_data + 0x4c)
    assert scale_word == 0x3e99999a
    _, screen = cache.tag(next(d for d,t in cache.tags.items()
                              if t["group"] == "wgit" and
                              cache.u16(t["address"]-cache.magic+4) == 19), "wgit")
    pane = cache.block(screen+32,76)[0]
    definitions = cache.block(pane+28,44)[1:]
    assert len(definitions) == 7
    _, matg = cache.tag(next(d for d,t in cache.tags.items()
                            if t["group"] == "matg"), "matg")
    _, strings = cache.tag(cache.u32(screen+28), "unic")
    count, size, indices, data = struct.unpack_from("<4I",cache.data,matg+0x190)
    start, length = struct.unpack_from("<2H",cache.data,strings+16)
    assert start + length <= count
    localized = {sid: cache.string(data+offset,size-offset)
                 for index in range(start,start+length)
                 for sid,offset in [struct.unpack_from("<2I",cache.data,indices+index*8)]}
    font_path = ROOT / "assets/disc/maps/fonts/conduit-12"
    font = font_path.read_bytes()
    characters = list(dict.fromkeys(c for p in definitions
                                    for c in map(ord,localized[cache.u32(p+36)])))
    original = Retail()
    guest = original.g
    guest.mem_write(0x4e2920,font[0x200:0x3c0])
    guest.mem_write(0x4e2920+0x1c8,b"\1")
    original.w(0x4e28f4+4,0)  # Runtime font1 is the declared ready header0.
    pool = original.alloc(bytes(0x48))
    records = original.alloc(bytes(0x38*len(characters)))
    original.w(pool+0x44,records)
    original.w(0x54d574,pool)
    glyph_headers = []
    for index,codepoint in enumerate(characters):
        at = (struct.unpack_from("<I",font,0x400+codepoint*4)[0]+0x4040)*16
        header = font[at:at+16]
        original.w(records+index*0x38+0x10,4)
        guest.mem_write(records+index*0x38+0x1c,header)
        original.w(records+index*0x38+0x34,0)
        glyph_headers.append(dict(codepoint=codepoint, header=header.hex()))
    # Same explicit ready-atlas seam as the retained original text-style probe.
    # UVs belong to this supplied origin, not the native packed font atlas.
    guest.mem_write(0x4b62b0,struct.pack("<I2h",0,8,8))
    glyphs, visits, services = [], {}, {}

    def hook(machine,address,_size,_context):
        if address in (0x13f0e0,0x13ef40,0x13f700,0x1f490,0x1f3a0,0x122570):
            visits[address] = visits.get(address,0)+1
        if address == 0x140b20:
            services[address] = services.get(address,0)+1
            codepoint = machine.reg_read(UC_X86_REG_ECX)
            original.ret(characters.index(codepoint) if codepoint in characters else 0xffffffff)
        elif address == 0x140870:
            services[address] = services.get(address,0)+1
            stack = machine.reg_read(UC_X86_REG_ESP)
            original.ret(1)
            machine.reg_write(UC_X86_REG_ESP,stack+8)
        elif address == 0x1f490:
            stack = machine.reg_read(UC_X86_REG_ESP)
            raw = bytes(machine.mem_read(stack+4,56))
            values = struct.unpack("<5I7f2I",raw)
            glyphs.append(dict(codepoint=values[2],font=values[1],
                               callback_words=list(struct.unpack("<14I",raw)),
                               x=values[5],y=values[6],source_x=values[7],source_y=values[8],
                               source_width=values[9],source_height=values[10],scale=values[11],
                               color=f"{values[3]:08x}",shadow=f"{values[4]:08x}",quads=[]))
        elif address == 0x363a0:
            services[address] = services.get(address,0)+1
            stack = machine.reg_read(UC_X86_REG_ESP)
            raw = bytes(machine.mem_read(original.r(stack+4),80))
            assert glyphs
            glyphs[-1]["quads"].append(dict(raw=raw.hex(),
                vertices=[dict(zip(("x","y","u","v","color"),
                                   struct.unpack_from("<4fI",raw,i*20))) for i in range(4)]))
            original.ret(0)
            machine.reg_write(UC_X86_REG_ESP,stack+8)

    guest.hook_add(UC_HOOK_CODE,hook)
    cases = []
    for index,definition in enumerate(definitions,1):
        assert cache.u32(definition) == 9 and cache.u16(definition+10) == 9
        value = localized[cache.u32(definition+36)]
        top,left,bottom,right = struct.unpack_from("<4h",cache.data,definition+28)
        bounds = [240-top,320+left,240-bottom,320+right]
        color = [1.,*struct.unpack_from("<3f",cache.data,definition+16)]
        guest.mem_write(0x4e73a0,bytes(0x70))
        original.w(0x4e73a0,1)  # Original line wrapping enabled.
        original.w(0x4e73a0+4,1)  # Actual253765 flag8 runtime font.
        original.w(0x4e73a0+12,0)  # Actual flags9 left justification.
        original.run(0x13ec70,registers=[(UC_X86_REG_EAX,original.alloc(struct.pack("<4f",*color)))])
        original.run(0x13ed50,registers=[(UC_X86_REG_EAX,0)])
        box = original.alloc(struct.pack("<4h",*bounds))
        endpoint = original.alloc(bytes(4))
        text = original.alloc(struct.pack(f"<{len(value)+1}I",*map(ord,value),0))
        glyphs.clear()
        original.run(0x13f0e0,argv=[0x1f490,box,endpoint,0,0,scale_word,text])
        assert glyphs and all(g["font"] == 1 and len(g["quads"]) == 1 for g in glyphs)
        cases.append(dict(index=index,string_id=f"{cache.u32(definition+36):08x}",text=value,
                          definition_raw=cache.data[definition:definition+44].hex(),
                          projected_bounds=bounds, runtime_font=1,scale_word=f"{scale_word:08x}",
                          endpoint=original.read(endpoint,"<2h"),glyphs=list(glyphs)))
    report = dict(scope=__doc__.splitlines(),executable_sha256=PIN,
        inputs=[identity(p) for p in [ROOT/"assets/disc/default.xbe",
                ROOT/"assets/disc/maps/mainmenu.map",font_path,Path(__file__).resolve(),
                ROOT/"tools/oracle_multiplayer_hud_positions.py"]],
        supplied_atlas_origin=[8,8],glyph_headers=glyph_headers,
        executed={f"{k:08x}":v for k,v in visits.items()},
        service_calls={f"{k:08x}":v for k,v in services.items()},
        patched_instructions=[],cases=cases,
        boundary="Complete original small-font wrap/clip/glyph/quad execution at explicit canonical projected bounds, full reveal and alpha1. Widget projection, typewriter lifecycle, atlas residency/packing, GPU and whole-frame parity are excluded.")
    target = output/"original-layout.json"
    target.write_text(json.dumps(report,indent=2)+"\n")
    print(target)
    print(json.dumps(dict(cases=len(cases),glyphs=sum(len(c["glyphs"]) for c in cases),
                          executed=report["executed"]),indent=2))


if __name__ == "__main__":
    main()
