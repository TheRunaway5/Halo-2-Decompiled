#!/usr/bin/env python3
"""Compare literal Xbox and prepared MCC object-loop metadata by exact path."""
import hashlib,json,os
from pathlib import Path
from inspect_multiplayer import Cache
from original_xbox_pose_case import BASE
from validate_mcc_weapon_vehicle_sound_resources import Bank,merge,schema
ROOT=Path(__file__).resolve().parents[1]
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=json.loads((BASE/'authored-scope.json').read_text())
    item=next(row for row in source['maps'] if Path(row['path']).stem=='coagulation')
    original=Path(item['path']);assert sha(original)==item['sha256']
    maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
    local=Bank(maps/'coagulation.map');shared=Bank(maps/'shared.map');xbox=Cache(original)
    try:
        tags=merge(local,shared);native={row['name']:row for row in xbox.tags.values() if row['group']=='lsnd'}
        rows=[]
        for _,(bank,tag) in tags.items():
            name=tag['name']
            if tag['group']!='lsnd' or not tag['offset'] or not any(s in name for s in ('\\teleport_loop\\','\\banshee\\','\\sound_scenery\\')):continue
            def fields(owner,t,prefix):
                at=t['offset'];tracks=owner.block(at+prefix+0x1c,0x58)
                return dict(flags=owner.u(at+prefix),details=owner.u(at+prefix+0x24),
                    tracks=[dict(flags=owner.u(t+4),fade_bits=[owner.u(t+0xc),owner.u(t+0x10)],
                        sounds=[owner.tags.get(owner.u(t+o),{}).get('name') for o in (0x18,0x20,0x28,0x44,0x30,0x4c,0x38)]) for t in tracks])
            row=dict(name=name,mcc=fields(bank,tag,8))
            if name in native:row['xbox']=fields(xbox,native[name],0)
            rows.append(row)
        result=dict(scope=__doc__,source_images={str(original):item['sha256'],str(local.path):sha(local.path),str(shared.path):sha(shared.path)},
            rows=rows,mcc_flag_schema=[dict(node.attrib,children=[dict(child.attrib) for child in node]) for node in schema('lsnd') if node.attrib.get('name','').lower()=='flags'],
            runtime_playback_verified=False)
        out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/object-loop-flag-comparison.json'
        assert not out.exists();out.write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps(dict(rows=len(rows),matched=sum('xbox' in r for r in rows),
            differences=[r for r in rows if 'xbox' in r and r['mcc']!=r['xbox']],schema=result['mcc_flag_schema'])),flush=True)
    finally:
        local.close();shared.close();xbox.data.close();xbox.stream.close()
if __name__=='__main__':main()
