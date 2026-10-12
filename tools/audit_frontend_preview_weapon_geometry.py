#!/usr/bin/env python3
"""Identify the actual held BR section through the original collector format.

The unchanged4C2B0 section selector reads literal mainmenu metadata. Resource
readiness is an explicit true seam, because the captured original collector
already contains the loaded BR section. This is section selection authority,
not a reconstruction of geometry-distance timing or resource residency.
"""
import hashlib,json,struct
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail,ROOT,UC_X86_REG_EAX
from unicorn import UC_HOOK_CODE
base=ROOT/'analysis/frontend/customization'
path=base/'original-preview-collector-prior-spartan.json'
capture=json.loads(path.read_text())
row=next(v for v in capture['rows']if v['pc']=='0002ce7b'and v['collector_index']==4)
descriptor=bytes.fromhex(row['collector_descriptor_hex'])
flags,render=struct.unpack_from('<2I',descriptor);section_flags=struct.unpack_from('<I',descriptor,12)[0]
# Original460D0's literal `shr9; and1FF`, also decompiled without field inference.
actual_section=(section_flags>>9)&0x1ff
c=Cache(ROOT/'assets/disc/maps/mainmenu.map');tag,p=c.tag(render,'mode');r=Retail()
r.g.mem_map(0x80000000,0x4000000)
r.g.mem_write(tag['address'],bytes(c.data[p:p+tag['size']]))
r.g.mem_write(r.table+(render&65535)*16,struct.pack('<4I',int.from_bytes(b'mode','big'),render,tag['address'],tag['size']))
checks=[]
def ready(uc,address,size,data):
 if address==0x12de70:r.ret(1)
r.g.hook_add(UC_HOOK_CODE,ready)
wanted=r.alloc(b'\0'*16);current=r.alloc(b'\0'*16);sections=r.alloc(bytes(16));fallback=r.alloc(bytes(4))
for lod in range(6):
 result=r.run(0x4c2b0,argv=[wanted,current,lod,0,sections,fallback],registers=[(UC_X86_REG_EAX,render)])
 selected=list(r.g.mem_read(sections,c.u32(p+0x1c)))
 checks.append(dict(level=lod,regions=selected,admitted=bool(result&255),fallback=bool(r.g.mem_read(fallback,1)[0])))
matching=[v['level']for v in checks if v['regions']==[actual_section]]
assert actual_section==0 and matching==[0]
report=dict(method=__doc__.splitlines()[2:],capture_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),mainmenu_sha256=hashlib.sha256(c.data).hexdigest(),original_xbe_sha256=hashlib.sha256((ROOT/'assets/disc/default.xbe').read_bytes()).hexdigest(),actual_collector_index=row['collector_index'],actual_descriptor_hex=descriptor.hex(),render_model=f'{render:08x}',collector_classification_before_layer_overrides=flags>>29,actual_section=actual_section,source_4c2b0_cases=checks,only_matching_geometry_levels=matching,boundary='Actual collector BR section uniquely implies geometryLOD0 for this literal active permutation. Source geometry-distance producer and later lighting classification are separate evidence; no fitted LOD threshold or per-object capture constant is introduced.')
(base/'preview-weapon-geometry-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
