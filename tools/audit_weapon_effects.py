#!/usr/bin/env python3
"""Read original projectile trails, contact and detonation references.

This inventories authored data; it does not certify native visual parity.
All caches are opened read-only and only the requested JSON file is written.
"""
import argparse
import hashlib
import json
from pathlib import Path
from inspect_multiplayer import Cache, ROOT

PROJECTILE_EFFECTS = {
    'detonation_started': 0xec, 'airborne_detonation': 0xf4,
    'ground_detonation': 0xfc, 'detonation_damage': 0x104,
    'attached_detonation_damage': 0x10c, 'super_detonation': 0x114,
    'super_detonation_damage': 0x11c, 'detonation_sound': 0x124,
    'flyby_sound': 0x13c, 'impact_effect': 0x144, 'impact_damage': 0x14c,
}

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def projectile(c, datum):
    tag = c.tags[datum]
    result = dict(tag=c.reference(datum), resident=tag['offset'] is not None)
    if not result['resident']:
        return result
    p = tag['offset']
    result.update(flags=c.u(p + 0xbc), detonation_timer_start=c.h(p + 0xc0),
        arming_seconds=c.f(p + 0xcc), timer=[c.f(p + 0xd4), c.f(p + 0xd8)],
        maximum_range=c.f(p + 0xe0), detonation_radius=c.f(p + 0x134),
        material_effects=c.reference(c.u(p + 0x58)),
        references={name:c.reference(c.u(p + off)) for name,off in PROJECTILE_EFFECTS.items()},
        attachments=[], material_responses=[])
    for at in c.block(p + 0x94, 0x18):
        ref=c.reference(c.u(at + 4))
        row=dict(reference=ref, marker=c.u(at + 8), color=c.h(at + 12),
                 primary_scale=c.u(at + 16), secondary_scale=c.u(at + 20))
        if ref and ref['group']=='cont' and c.tags[c.u(at + 4)]['offset'] is not None:
            q=c.tags[c.u(at + 4)]['offset']
            row['contrail']=dict(raw_hex=bytes(c.data[q:q+0xf0]).hex(),
                flags=c.h(q), scale_flags=c.h(q+2), point_rate=c.f(q+4),
                render_type=c.h(q+0x18), bitmap=c.reference(c.u(q+0x34)),
                blend=c.h(q+0x66), fade=c.h(q+0x68),
                states=[dict(raw_hex=bytes(c.data[s:s+0x40]).hex(),
                    duration=[c.f(s),c.f(s+4)], transition=[c.f(s+8),c.f(s+12)],
                    width=c.f(s+0x18)) for s in c.block(q+0xe8,0x40)])
        result['attachments'].append(row)
    for at in c.block(p + 0x19c, 0x58):
        result['material_responses'].append(dict(raw_hex=bytes(c.data[at:at+0x58]).hex(),
            material=c.h(at+0x10), default_response=c.h(at+2),
            alternate_response=c.h(at+0x14), chance=c.f(at+0x18)))
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--map',type=Path,action='append',dest='maps')
    args=parser.parse_args()
    paths=args.maps
    if paths is None:
        dlc=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c'
        paths=sorted((ROOT/'assets/disc/maps').glob('*.map'))
        paths+=sorted(p for p in dlc.rglob('*.map') if not p.parent.name.endswith('old'))
    report=dict(schema='authored-weapon-effects-v1',tool_sha256=sha(Path(__file__)),maps=[])
    weapons=set();projectiles=set();contrails=set();groups={}
    for path in paths:
        if path.stem in {'mainmenu','shared','single_player_shared'}:continue
        c=Cache(path)
        try:
            rows=[];definitions={}
            for t in c.tags.values():
                if t['group']!='weap' or t['offset'] is None:continue
                barrels=[]
                for index,at in enumerate(c.block(t['offset']+0x2d0,0xec)):
                    datum=c.u(at+0x90)
                    if datum==0xffffffff:continue
                    if datum not in definitions:definitions[datum]=projectile(c,datum)
                    pr=definitions[datum];projectiles.add(pr['tag']['name'])
                    barrels.append(dict(index=index,projectile=pr['tag'],raw_hex=bytes(c.data[at:at+0xec]).hex()))
                    for a in pr.get('attachments',[]):
                        ref=a['reference']
                        if not ref:continue
                        groups.setdefault(ref['group'],set()).add(ref['name'])
                        if ref['group']=='cont':contrails.add(ref['name'])
                weapons.add(t['name']);rows.append(dict(weapon=c.reference(int(t['datum'],16)),barrels=barrels))
            report['maps'].append(dict(path=str(path),sha256=sha(path),weapons=rows,
                projectiles=list(definitions.values())))
        finally:
            c.data.close();c.stream.close()
    report['summary']=dict(maps=len(report['maps']),weapons=len(weapons),projectiles=len(projectiles),
        contrails=len(contrails),attachment_groups={k:len(v)for k,v in sorted(groups.items())})
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report['summary'],sort_keys=True))

if __name__=='__main__':main()
