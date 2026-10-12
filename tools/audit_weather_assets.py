#!/usr/bin/env python3
"""Inventory authored Xbox weather assets; no live GPU/admission claim."""
import hashlib
import json
from pathlib import Path
import struct
from audit_rendering import Cache,file_sha

ROOT=Path(__file__).resolve().parents[1]
OUTPUT=ROOT/'analysis/multiplayer-shading/weather-gpu-assets'

def words(c,p,n):return list(struct.unpack_from('<'+'I'*n,c.data,p))
def floats(c,p,n):return list(struct.unpack_from('<'+'f'*n,c.data,p))
def main():
    OUTPUT.mkdir(parents=True,exist_ok=True);report=[]
    for group,names in [('disc',['ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron','foundation','headlong','lockout','midship','waterworks','zanzibar']),
                        ('map-pack',['containment','warlock','deltatap','turf','backwash','dune','elongation','gemini','triplicate'])]:
        for name in names:
            path=ROOT/'assets'/group/'maps'/(name+'.map');c=Cache(path)
            _,scnr=c.tag(c.scenario,'scnr');ref=c.block(scnr+0x210,68)[0]
            at,size,memory=words(c,ref,3);b=at+16;magic=memory-at
            palettes=c.block(b+0x84,0x88,magic);clusters=c.block(b+0x9c,0xb0,magic)
            palette_indices=[c.u16(p+0x76) for p in clusters]
            record={'map':name,'cache_sha256':file_sha(path),'clusters':len(clusters),
                'palette':[{'ordinal':i,'name':c.string(p,32),'datum':f'{c.u32(p+0x24):08x}',
                            'admitted_clusters':[j for j,v in enumerate(palette_indices) if v==i]} for i,p in enumerate(palettes)],
                'weather_polyhedra':len(c.block(b+0x8c,24,magic)),'definitions':[]}
            for datum in sorted({c.u32(p+0x24) for p in palettes}-{0xffffffff}):
                tag,p=c.tag(datum,'weat');definition={'datum':f'{datum:08x}','name':tag['name'],'particle_systems':[],'background_plates':[]}
                for ordinal,r in enumerate(c.block(p,0x8c)):
                    ptr,size,header=words(c,r+0x40,3);location=ptr>>30
                    files=['','mainmenu.map','shared.map','single_player_shared.map'];resource=path.parent/files[location] if location else path
                    with resource.open('rb') as stream:stream.seek(ptr&0x3fffffff);raw=stream.read(size)
                    descriptors=[]
                    for d in c.block(r+0x50,16):
                        kind,unused,unused2,locator,stride,size_,offset=struct.unpack_from('<BBHHHII',c.data,d)
                        start=header+8+offset;payload=raw[start:start+size_]
                        descriptors.append({'kind':kind,'locator':locator,'stride':stride,'size':size_,'offset':offset,'sha256':hashlib.sha256(payload).hexdigest()})
                        (OUTPUT/f'{name}-system{ordinal}-locator{locator}.bin').write_bytes(payload)
                    sprite=c.u32(r+4)
                    definition['particle_systems'].append({'ordinal':ordinal,'type':c.u16(r+0x6c),'sprites':f'{sprite:08x}',
                        'sprite_name':c.tags[sprite]['name'] if sprite!=0xffffffff else None,'maximum_particles':c.u32(r+0x2c),
                        'view_box':floats(c,r+8,3),'exclusion_radius':floats(c,r+0x14,1)[0],'maximum_velocity':floats(c,r+0x18,1)[0],
                        'mass':floats(c,r+0x1c,2),'size':floats(c,r+0x24,2),'initial_velocity':floats(c,r+0x30,3),
                        'animation_speed':floats(c,r+0x3c,1)[0],'opacity':floats(c,r+0x70,2),'streak_scale':floats(c,r+0x78,1)[0],
                        'line_width':floats(c,r+0x7c,1)[0],'raw_size':len(raw),'descriptors':descriptors})
                for ordinal,r in enumerate(c.block(p+8,0x3a8)):
                    textures=[c.u32(r+j*8+4) for j in range(3)]
                    definition['background_plates'].append({'ordinal':ordinal,'textures':[{'datum':f'{d:08x}','name':c.tags[d]['name']} for d in textures],
                        'flags':c.u32(r+0x94),'positions':floats(c,r+0x18,3),'velocities':floats(c,r+0x24,9),
                        'texture_scale':floats(c,r+0x48,3),'jitter':floats(c,r+0x54,9),'near_far':floats(c,r+0x78,2),
                        'depth_blend_near_far':floats(c,r+0x80,2),'opacity':floats(c,r+0x88,3),'tints':floats(c,r+0x98,9),
                        'masses':floats(c,r+0xbc,3)})
                matg=next(t for t in c.tags.values() if t['group']=='matg');_,g=c.tag(matg['datum'],'matg')
                raster=c.block(g+0x108,0x108)[0];shaders=c.block(raster+0x58,8);definition['global_vertex_shaders']=[]
                for ordinal in [21,22]:
                    datum_=c.u32(shaders[ordinal]+4);tag_,root=c.tag(datum_,'vrtx');cls=c.block(root+4,28)[0]
                    code=c.pointer(c.u32(cls+16),c.magic,c.u32(cls+12));payload=c.data[code:code+c.u32(cls+12)]
                    (OUTPUT/f'{name}-global{ordinal}-class0.vp').write_bytes(payload)
                    tokens=[struct.unpack_from('<4I',payload,4+i*16) for i in range(struct.unpack_from('<H',payload,2)[0])]
                    definition['global_vertex_shaders'].append({'ordinal':ordinal,'datum':f'{datum_:08x}','name':tag_['name'],
                        'class':0,'size':len(payload),'sha256':hashlib.sha256(payload).hexdigest(),
                        'instructions':[{'mac':(t[1]>>21)&15,'ilu':(t[1]>>25)&7,'attribute':(t[1]>>9)&15,'constant':(t[1]>>13)&255,
                            'output':(t[3]>>3)&15,'output_mask':(t[3]>>12)&15,'words':[f'{v:08x}' for v in t]} for t in tokens]})
                record['definitions'].append(definition)
            report.append(record);c.close()
    (OUTPUT/'inventory.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'{len(report)} maps; {sum(len(r["definitions"]) for r in report)} authored weather definitions')
if __name__=='__main__':main()
