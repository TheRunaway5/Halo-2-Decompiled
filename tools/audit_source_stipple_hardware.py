#!/usr/bin/env python3
"""Pinned Xbox hardware framebuffer stipple orientation and global-phase fixture.

Remote PNGs are decoded only in memory for numerical measurement. This tool
writes scalar fixtures/provenance, never downloads media for display.
"""
from pathlib import Path
from urllib.request import urlopen
from io import BytesIO
import hashlib,json
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
REFS={'abaire/nxdk_pgraph_tests':'7f40a15958e837e0cc4209e58e365bc8f0a4e9dc',
      'abaire/nxdk_pgraph_tests_golden_results':'4acbb6ac3dcdc4d02cf4bf2d70ddf89b6e1618cb',
      'abaire/pbkitplusplus':'27a09692ccee84965ec4bb1438419096c9c75eb0'}
PATTERNS={'Asymmetric_On':[0xc0c0ffff]*8+[0x3030ffff]*8+[0x8cef]*8+[0x0f000f0f]*8,
 'Alternating_On':[0xaaaaaaaa]*32,'Checkered_On':([0x0f0f0f0f]*4+[0xf0f0f0f0]*4)*4,
 'Border_On':[0]+[0x7ffffffe]*30+[0],'Zero_On':[0]*32,
 'Checkered_Off':[0xffffffff]*32} # Disabled control represented by all-visible mask.
def digest(raw):return hashlib.sha256(raw).hexdigest()
def url(repo,path):return 'https://raw.githubusercontent.com/'+repo+'/'+REFS[repo]+'/'+path
def main():
 sources=[]
 for repo,path in [('abaire/nxdk_pgraph_tests','src/tests/stipple_tests.cpp'),
                   ('abaire/nxdk_pgraph_tests','src/test_host.cpp'),
                   ('abaire/pbkitplusplus','src/shaders/passthrough.vsh')]:
  link=url(repo,path);raw=urlopen(link).read();sources.append(dict(url=link,sha256=digest(raw)))
 images={};rows=[];cases=[];measurements=[];common=None
 bg=np.array([32,34,36,255]);white=np.array([255,255,255,255])
 for pid,(name,pattern)in enumerate(PATTERNS.items()):
  link=url('abaire/nxdk_pgraph_tests_golden_results','results/Stipple_tests/'+name+'.png')
  raw=urlopen(link).read();image=np.array(Image.open(BytesIO(raw)).convert('RGBA'));assert image.shape==(480,640,4);images[name]=image
  # Current source's diagnostic white quad uses literal absolute(288,384)..
  #(352,448), independently of Draw()'s translated geometry origin.
  interior=image[385:447,289:351]
  assert np.all(np.all(interior==bg,axis=2)|np.all(interior==white,axis=2)),name
  sample=[(x,y,int(np.array_equal(image[y,x],white)))for y in range(385,447)for x in range(289,351)]
  rows.append(' '.join(map(str,['P',pid,*pattern])))
  cases.extend(' '.join(map(str,['S',pid,x,y,bit]))for x,y,bit in sample)
  x=np.array([p[0]for p in sample]);y=np.array([p[1]for p in sample]);bits=np.array([p[2]for p in sample]);bank=np.array(pattern,dtype=np.uint32);candidates=[]
  for ys in [-1,1]:
   for xs in [-1,1]:
    for yo in range(32):
     r=bank[(ys*y+yo)&31]
     for xo in range(32):
      for byte_reverse in [False,True]:
       cols=(xs*x+xo)&31
       if byte_reverse:cols=(cols&24)|(7-(cols&7))
       if np.array_equal((r>>cols)&1,bits):candidates.append((xs,ys,xo,yo,byte_reverse))
  common=set(candidates)if common is None else common.intersection(candidates)
  measurements.append(dict(name=name,url=link,png_sha256=digest(raw),pixels=len(sample),white=int(bits.sum()),exact_candidate_count=len(candidates)))
 assert common=={(1,-1,0,31,True)},common
 baseline=images['Checkered_Off'];translated=[]
 for pid,name in enumerate(list(PATTERNS)[:4]):
  actual=images[name];pattern=PATTERNS[name];count=0
  for y in range(60,160):
   for x in range(380,480):
    if np.array_equal(baseline[y,x],bg):continue
    if any(np.array_equal(baseline[yy,xx],bg)for yy in [y-1,y,y+1]for xx in [x-1,x,x+1]):continue
    visible=int(not np.array_equal(actual[y,x],bg));expected=baseline[y,x]if visible else bg
    assert np.array_equal(actual[y,x],expected),(name,x,y)
    bit=(x&24)|(7-(x&7));assert visible==((pattern[(31-y)&31]>>bit)&1),(name,x,y)
    cases.append(' '.join(map(str,['S',pid,x,y,visible])));count+=1
  translated.append(dict(name=name,pixels=count,source_geometry_translation=[160,48],roi=[380,60,480,160]))
 fixture=ROOT/'tests/fixtures/xbox_hardware_stipple.tsv';fixture.write_text('\n'.join([*rows,*cases])+'\n')
 report=dict(commits=REFS,sources=sources,hardware='Publisher identifies output as original Xbox 1.0 hardware',
  white_quads=measurements,translated_triangles=translated,unique_coordinate_rule=dict(xsign=1,ysign=-1,xoffset=0,yoffset=31,reverse_bits_within_byte=True),
  samples=len(cases),fixture_sha256=digest(fixture.read_bytes()),oracle='tools/audit_source_stipple_hardware.py',oracle_sha256=digest(Path(__file__).read_bytes()),
  scope='Exact observed 640x480 top-left framebuffer pixel mapping, all 32 phases of the asymmetric pattern, and translated primitive global phase. SaveBackBuffer copies framebuffer byte order directly with a color-channel swizzle only. Does not prove physical Xbox odd framebuffer height, nonzero SurfaceClip origin, or MSAA sample scheduling. Preserving the observed phase at higher native resolutions is explicit port policy; no height-dependent origin is inferred.')
 fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
 out=ROOT/'analysis/multiplayer-shading/stipple-coordinate-audit/hardware-fixture.json';out.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2));assert len(cases)==29400
if __name__=='__main__':main()
