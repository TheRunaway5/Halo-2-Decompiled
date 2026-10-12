#!/usr/bin/env python3
"""Export captured bloom surfaces and original executable program fixtures.

This is a GPU-pass oracle. It does not substitute matching the complete world,
lighting and haze input that the native runtime sends into bloom.
"""
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
p = ROOT / 'analysis/frontend/xemu-capture'
destination = p / 'bloom-fixture'
destination.mkdir(exist_ok=True)
oracle = json.loads((ROOT / 'analysis/frontend/bloom-differential.json').read_text())
assert not oracle['failures']
assert hashlib.sha256((p / 'title-vulkan_capture.rdc').read_bytes()).hexdigest() == '077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431'
programs = oracle['fixtures'][0]['programs']
passes = []
for index, (before, event, radius) in enumerate(((8034,8048,.5), (8048,8062,1),
        (8062,8076,1.25), (8076,8090,1.5625), (8090,8104,1.937500238418579), (8104,8118,0))):
    words = list(programs[0 if index==0 else 2 if index==5 else 1])
    raw = (p / f'pass-{event}-4-uniforms.bin').read_bytes()
    for i in range(18):
        rgba = [round(v*255) for v in struct.unpack_from('<4f',raw,544+i*16)]
        slot = 10+i//2+(i%2)*8 if i<16 else 43+i-16
        words[slot] = rgba[3]<<24 | rgba[0]<<16 | rgba[1]<<8 | rgba[2]
    width,height = (640,480) if index==5 else (160,120)
    uniform = struct.pack('<60I8f',*words,radius,width,height,0,0,0,1,1)
    (destination / f'{event}-uniform.bin').write_bytes(uniform)
    passes.append({'event': event, 'before': before, 'radius': radius,
                   'width': width, 'height': height, 'screen_blend': index==5})

# Follow the authored screen-effect -> shader -> template -> pass chain.
# The effect consumes the completed bloom framebuffer; verify that the actual
# captured binding agrees instead of assuming its name identifies the input.
cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
try:
    effects=[t for t in cache.tags.values()if t['group']=='egor'and t['name']==r'effects\screen_effects\desaturate']
    assert len(effects)==1
    _,at=cache.tag(effects[0]['datum'],'egor')
    assert cache.u32(at+0x40)==0x73686164
    shader=cache.shader(cache.u32(at+0x44))
    records=cache.shaders[shader]['postprocess'];assert len(records)==1
    template=cache.templates[records[0]['template']]['postprocess'];assert len(template)==1
    authored_passes=template[0]['passes'];assert len(authored_passes)==1
    implementations=cache.passes[authored_passes[0]['pass']]['postprocess'][0]['implementations'];assert len(implementations)==1
    words=list(struct.unpack_from('<60I',bytes.fromhex(implementations[0]['original_bytes']),6))
    raw=(p/'pass-8132-4-uniforms.bin').read_bytes()
    for i in range(18):
        rgba=[round(v*255)for v in struct.unpack_from('<4f',raw,544+i*16)]
        slot=10+i//2+(i%2)*8 if i<16 else 43+i-16
        words[slot]=rgba[3]<<24|rgba[0]<<16|rgba[1]<<8|rgba[2]
    assert struct.pack('<I',words[43]).hex()==records[0]['constants']['pixel'][0]
finally:
    cache.close()
filter_capture=json.loads((p/'title-filter-captured-frame.json').read_text())
assert filter_capture['capture_sha256']=='077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431'
filter_input=Image.open(p/'title-filter-input.png').convert('RGBA')
assert filter_input.size==(640,480)
assert filter_input.tobytes()==Image.open(p/'pass-8118.png').convert('RGBA').tobytes()
(destination/'8132-uniform.bin').write_bytes(struct.pack('<60I8f',*words,0,640,480,0,0,0,1,1))
passes.append({'event':8132,'before':8118,'radius':0,'width':640,'height':480,
               'screen_blend':False,'authored_screen_effect':shader,'actual_input_matches_bloom_output':True})
surfaces = {}
for event in (8034,8048,8062,8076,8090,8104,8118,8132):
    image = Image.open(p / f'pass-{event}.png').convert('RGBA')
    raw = image.tobytes()
    (destination / f'{event}.rgba').write_bytes(raw)
    surfaces[str(event)] = {'width': image.width, 'height': image.height,
                            'sha256': hashlib.sha256(raw).hexdigest()}
report = {'capture_sha256': '077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431',
          'passes': passes, 'surfaces': surfaces, 'includes_authored_post_bloom_filter':True,
          'complete_native_frame_parity_verified': False}
(destination / 'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
