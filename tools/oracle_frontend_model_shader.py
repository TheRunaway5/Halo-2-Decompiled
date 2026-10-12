#!/usr/bin/env python3
"""Freeze the original enclosing UI camera, distinct from model-widget camera.

Execute unchanged3763B0→2232A0→2F800 up to its2C8A0 submission, then unchanged
2FD90 with that submitted camera and the original null projection bounds.
Only the already independently verified video setup supplies NTSC video flags;
mainmenu layout and cinematic auxiliary fields are explicit environment inputs.
Actual hardware-captured collector/metric outputs and source choices are retained
separately, rather than replacing the expectations with a native calculation.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_multiplayer_hud_positions import (Retail,ROOT,UC_X86_REG_ESI,
    UC_X86_REG_EDX,UC_X86_REG_EBX)
base=ROOT/'analysis/frontend/customization'
def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))
rows=[];fixture=[]
for flags in [0,1,8,9]:
 for cinematic in [False,True]:
  r=Retail();video=r.video(flags)
  layout=r.alloc(bytes(0x1124));r.g.mem_write(layout+1,b'\1');r.g.mem_write(layout+2,b'\0\0');r.w(layout+8,3);r.w(0x4e6948,layout)
  aux=r.alloc(bytes(32));r.w(0x510c50,aux);r.g.mem_write(aux+5,bytes([cinematic]))
  r.run(0x3763b0);r.run(0x2232a0,stop=0x2c8a0)
  camera=bytes(r.g.mem_read(0x47f104,0x74));projection=r.alloc(bytes(0xc0))
  r.run(0x2fd90,registers=[(UC_X86_REG_ESI,0x47f104),(UC_X86_REG_EDX,0),(UC_X86_REG_EBX,projection)])
  output=[r.r(projection+n)for n in [12,24,36,48,188]]
  rows.append(dict(video=video,cinematic=cinematic,configured_angle_word=f'{r.r(0x5234dc):08x}',submitted_camera_hex=camera.hex(),coverage_camera_words=[f'{v:08x}'for v in output]))
  fixture.append('0 '+' '.join(f'{v:08x}'for v in output))
capture=base/'original-preview-collector-prior-spartan.json';live=json.loads(capture.read_text())['rows']
owner=next(r for r in live if r['pc']=='001337df');sphere=words(bytes.fromhex(owner['cached_sphere_hex']))
stores=[r for r in live if r['pc']=='0002ce7b'];assert len(stores)==5
for r in stores:fixture.append('1 '+' '.join(f'{v:08x}'for v in [*sphere,int(owner['object_multiplier_word'],16),int(r['actual_input_coverage_word'],16),r['collector_byte']]))
audit=base/'preview-shader-lod-choice-audit.json';choices=json.loads(audit.read_text())['rows']
for row in choices:
 fixture.append('2 '+' '.join(f'{v:08x}'for v in [int(row['shader'],16),int(row['metric_bits'],16),*map(lambda x:int(x,16),row['view_bits']),row['modifier'],*row['original_choice']]))
target=base/'frontend-model-shader-original.tsv';target.write_text('\n'.join(fixture)+'\n')
report=dict(method=__doc__.splitlines()[2:],original_xbe_sha256=hashlib.sha256((ROOT/'assets/disc/default.xbe').read_bytes()).hexdigest(),producer_rows=rows,actual_collector_capture_sha256=hashlib.sha256(capture.read_bytes()).hexdigest(),original_shader_choice_audit_sha256=hashlib.sha256(audit.read_bytes()).hexdigest(),actual_collector_stores=len(stores),actual_shader_choices=len(choices),fixture_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),boundary='Literal producer output and captured coverage/choices only; implementation/pass routing and full preview framebuffer are separate proofs.')
(base/'frontend-model-shader-original.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
