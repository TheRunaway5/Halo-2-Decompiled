#!/usr/bin/env python3
"""Trace captured particle bitmaps through original cache references.

Reports structural asset relationships. These references do not establish
which live emitter produced a particular captured quad or its random state.
"""
import hashlib
import json
from pathlib import Path
import struct

from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]


def main():
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
    matches=json.loads((ROOT/'analysis/frontend/title-billboard-asset-matches.json').read_text())
    bitmaps={int(rows[0]['datum'],16):resource for resource,rows in matches['matches'].items()}
    particles={}
    for datum,tag in cache.tags.items():
        if tag['group']!='prt3' or not tag['address']:continue
        _,root=cache.tag(datum,'prt3')
        for parameter in cache.block(root+0x14,0x28):
            bitmap=cache.u32(parameter+12)
            if cache.u32(parameter+8)!=0x6269746d or bitmap not in bitmaps:continue
            particles[datum]={'particle':f'{datum:08x}','name':tag['name'],
                'bitmap':f'{bitmap:08x}','capture_resource':bitmaps[bitmap],
                'flags':cache.u32(root),'billboard_style':cache.u16(root+4)}
    effects=[]
    for datum,tag in cache.tags.items():
        if tag['group']!='effe' or not tag['address']:continue
        _,root=cache.tag(datum,'effe')
        for event_index,event in enumerate(cache.block(root+0x14,0x38)):
            for system_index,system in enumerate(cache.block(event+0x30,0x38)):
                particle=cache.u32(system+4)
                if particle not in particles:continue
                emitters=[]
                for index,at in enumerate(cache.block(system+0x30,0xb8)):
                    properties=[]
                    for offset in (8,0x18,0x28,0x38,0x48,0x58,0x68,0x7c,0x8c):
                        size,pointer=struct.unpack_from('<2I',cache.data,at+offset+8)
                        start=cache.pointer(pointer,cache.magic,size)if size else 0
                        raw=bytes(cache.data[start:start+size])if size else b''
                        properties.append({'offset':offset,'inputs_modifier':list(struct.unpack_from('<4h',cache.data,at+offset)),
                            'function_bytes':size,'function_sha256':hashlib.sha256(raw).hexdigest(),
                            'function_type':raw[0]if raw else None,'function_flags':raw[1]if raw else None})
                    emitters.append({'index':index,'physics':f'{cache.u32(at+4):08x}',
                        'properties':properties,'emission_shape_raw':cache.u32(at+0x78),
                        'translation':list(struct.unpack_from('<3f',cache.data,at+0x9c)),
                        'direction':list(struct.unpack_from('<2f',cache.data,at+0xa8)),
                        'runtime_words':list(struct.unpack_from('<2I',cache.data,at+0xb0))})
                effects.append({'effect':f'{datum:08x}','name':tag['name'],'event':event_index,
                    'system':system_index,'particle':particles[particle],
                    'event_delay':list(struct.unpack_from('<2f',cache.data,event+8)),
                    'event_duration':list(struct.unpack_from('<2f',cache.data,event+0x10)),
                    'location':cache.u16(system+8),'coordinates_raw':cache.u16(system+0xc),
                    'sort_bias':struct.unpack_from('<h',cache.data,system+0x14)[0],
                    'flags':cache.u16(system+0x16),'emitters':emitters})
    death_effects={int(effect['effect'],16)for effect in effects};parents=[]
    for datum,tag in cache.tags.items():
        if tag['group']!='prt3' or not tag['address']:continue
        _,root=cache.tag(datum,'prt3')
        if cache.u32(root+0x74)==0x65666665 and cache.u32(root+0x78)in death_effects:
            parents.append({'particle':f'{datum:08x}','name':tag['name'],
                'death_effect':f'{cache.u32(root+0x78):08x}','offset':'0x74'})
    primary={int(parent['particle'],16)for parent in parents};root_effects=[]
    for datum,tag in cache.tags.items():
        if tag['group']!='effe' or not tag['address']:continue
        _,root=cache.tag(datum,'effe')
        for i,event in enumerate(cache.block(root+0x14,0x38)):
            for j,system in enumerate(cache.block(event+0x30,0x38)):
                if cache.u32(system+4)in primary:root_effects.append({'effect':f'{datum:08x}',
                    'name':tag['name'],'event':i,'system':j,'particle':f'{cache.u32(system+4):08x}'})
    root_ids={int(effect['effect'],16)for effect in root_effects};objects=[]
    for datum,tag in cache.tags.items():
        if tag['group']!='scen' or not tag['address']:continue
        _,root=cache.tag(datum,'scen')
        for i,attachment in enumerate(cache.block(root+0x94,0x18)):
            if cache.u32(attachment)==0x65666665 and cache.u32(attachment+4)in root_ids:
                objects.append({'object':f'{datum:08x}','name':tag['name'],'attachment':i,
                    'effect':f'{cache.u32(attachment+4):08x}','marker':f'{cache.u32(attachment+8):08x}',
                    'attachment_words':list(struct.unpack_from('<6I',cache.data,attachment))})
    objects_ids={int(obj['object'],16)for obj in objects}
    _,scenario=cache.tag(cache.scenario,'scnr');palette=cache.block(scenario+0x58,0x28);placements=[]
    for i,placed in enumerate(cache.block(scenario+0x50,0x5c)):
        choice=struct.unpack_from('<h',cache.data,placed)[0]
        if choice<0 or choice>=len(palette):continue
        datum=cache.u32(palette[choice]+4)
        if datum not in objects_ids:continue
        placements.append({'scenery_index':i,'object':f'{datum:08x}','flags':cache.u32(placed+4),
            'position':list(struct.unpack_from('<3f',cache.data,placed+8)),
            'rotation':list(struct.unpack_from('<3f',cache.data,placed+0x14)),
            'authored_scale_raw':struct.unpack_from('<f',cache.data,placed+0x20)[0]})
    report={'original_map':str(cache.path),'original_map_sha256':hashlib.sha256(cache.data).hexdigest(),
        'bitmap_identity_evidence':'title-billboard-asset-matches.json',
        'particle_sprite_evidence':'title-particle-sprite-differential.json',
        'particle_systems':effects,'parent_particles':parents,'parent_effects':root_effects,
        'scenery_attachments':objects,'scenario_placements':placements,
        'asset_dependency_chain_verified':True,'captured_live_emitter_identity_verified':False,
        'original_emission_simulation_verified':False,'native_particle_runtime_implemented':False,
        'whole_frame_parity_verified':False}
    output=ROOT/'analysis/frontend/title-particle-source-chain.json'
    output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'report':str(output),'particle_systems':len(effects),'parent_particles':parents,
        'parent_effects':root_effects,'scenery_attachments':objects,'scenario_placements':placements},indent=2))


if __name__=='__main__':main()
