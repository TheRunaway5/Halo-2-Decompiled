#!/usr/bin/env python3
"""Freeze SCEN selected-collector parts directly from original cache resources.

This preserves untouched-executable collector outputs and literal 72-byte part
records. It does not use the native scene or native material preparation.
"""
import hashlib
import json
import struct
from pathlib import Path
from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer-shading'
NAMES='ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar containment warlock deltatap turf backwash dune elongation gemini triplicate'.split()
geom=json.loads((BASE/'object-geometry-lod-original.json').read_text())
contact=json.loads((BASE/'object-contact-inventory.json').read_text())
owners={(p['map'],p['placement_type'],p['placement']):p for p in contact['placements']}
sections={(s['map'],s['render_model'],s['section']):s for s in geom['sections']}
lines=['# Original SCEN: O map placement definition mode implementation; C map placement geometryLOD count sections; P map mode section part 18 literal72byte words shader']
report=[]
for index,name in enumerate(NAMES):
    path=ROOT/('assets/disc/maps' if index<12 else 'assets/map-pack/maps')/(name+'.map')
    c=Cache(path)
    models={}
    seen=set()
    row={'map':name,'owners':0,'collectors':0,'parts':0,'source_raw_hashes':{},'cache_sha256':hashlib.sha256(c.data).hexdigest()}
    for g in geom['placements']:
        if g['map']!=name or g['type']!=6:continue
        p=owners[(name,6,g['placement'])]
        if not p['slayer_automatic'] or g['render_model']==0xffffffff:continue
        if name=='zanzibar' and g['placement'] in (19,20,21,22):continue
        mode=g['render_model']
        if mode not in models:
            _,root=c.tag(mode,'mode')
            models[mode]=[c.u32(m+12) for m in c.block(root+0x60,32)]
        ids=models[mode]
        impl=1 if p['baked'] else 2
        lines.append(f"O {name} {g['placement']:x} {g['definition']:x} {mode:x} {impl:x}")
        row['owners']+=1
        for lod,collector in enumerate(g['collectors']):
            lines.append(f"C {name} {g['placement']:x} {lod:x} {len(collector):x}"+''.join(f' {s:x}' for s in collector))
            row['collectors']+=1
            for section in collector:
                if (mode,section) in seen:continue
                seen.add((mode,section))
                s=sections[(name,mode,section)]
                resource=s['resources'][0]
                assert resource['type']==0 and resource['channel']==72 and resource['size']%72==0
                with (ROOT/s['raw_file']).open('rb') as f:
                    f.seek(resource['file_offset']);data=f.read(resource['size'])
                assert len(data)==resource['size']
                digest=hashlib.sha256(data).hexdigest()
                assert digest==resource['sha256']
                row['source_raw_hashes'][f'{mode:08X}:{section}']=digest
                for part,off in enumerate(range(0,len(data),72)):
                    words=struct.unpack_from('<18I',data,off)
                    material=words[1]&0xffff
                    assert material<len(ids)
                    lines.append(f'P {name} {mode:x} {section:x} {part:x}'+''.join(f' {w:x}' for w in (*words,ids[material])))
                    row['parts']+=1
    c.close();report.append(row)
assert sum(m['owners'] for m in report)==274
fixture=ROOT/'tests/fixtures/retail_scenery_shader_parts.tsv'
fixture.write_text('\n'.join(lines)+'\n')
proof={'authority':'Original Xbox caches; retained untouched4C2B0 collector outputs and validated automatic Slayer gate','original_xbe_sha256':geom['xbe_sha256'],'maps':report,'source_input_hashes':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [BASE/'object-geometry-lod-original.json',BASE/'object-contact-inventory.json']},'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'scope':['274 resolved admitted SCEN owners; four unresolved Zanzibar pillar selections excluded.','Every selected geometryLOD collector retains its actual original section/part identity.','No live creation/RNG history, visibility traversal, clock or animation phase is synthesized.']}
(BASE/'scenery-shader-scene-literal.json').write_text(json.dumps(proof,indent=2)+'\n')
print({k:sum(m[k] for m in report) for k in ('owners','collectors','parts')})
