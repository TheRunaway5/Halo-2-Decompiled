#!/usr/bin/env python3
"""Inventory real Banshee authored phase, range and chunk boundaries by path."""
import hashlib,json,os,struct
from pathlib import Path
from inspect_multiplayer import Cache
from validate_mcc_weapon_vehicle_sound_resources import Bank,merge
ROOT=Path(__file__).resolve().parents[1]
def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
    local=Bank(maps/'coagulation.map');shared=Bank(maps/'shared.map');original=Cache(ROOT/'assets/disc/maps/coagulation.map')
    rows=[]
    try:
        tags=merge(local,shared);original_names={t['name']:t for t in original.tags.values()}
        def sound(bank,tag,mcc):
            s=tag['offset'];gestalts=[t for t in bank.tags.values() if t['group']=='ugh!' and t['offset'] is not None]
            assert len(gestalts)==1;g=gestalts[0]['offset']
            u16=lambda at:struct.unpack_from('<H',bank.data,at)[0]
            ranges=bank.block(g+(0x28 if mcc else 0x20),12);perms=bank.block(g+(0x30 if mcc else 0x28),16)
            codecs=bank.block(g,3) if mcc else []
            rate=bank.data[codecs[bank.data[s+3]]] if mcc else bank.data[s+3]
            codec=bank.data[codecs[bank.data[s+3]]+2] if mcc else bank.data[s+5]
            if mcc:
                languages=bank.block(g+0x60,8);chunks=bank.block(languages[0],16)
            else:chunks=bank.block(g+0x40,12)
            pitch_rows=[]
            for index in range(bank.data[s+10]):
                r=ranges[u16(s+8)+index];items=[]
                for n in range(u16(r+10)):
                    p=perms[u16(r+8)+n]
                    info=bank.block(p+8,8)[0] if mcc else p+8
                    first,count=u16(info+4),u16(info+6)
                    items.append(dict(stored_extent=bank.u(info),chunks=[dict(flags=bank.u(c+4),
                        encoded_bytes=bank.u(c+4)&(0x1fffffff if mcc else 0x3fffffff),
                        boundary_bit31=bool(bank.u(c+4)&0x80000000),
                        decoded_bytes=bank.u(c+8) if mcc else None)
                        for c in chunks[first:first+count]]))
                pitch_rows.append(dict(index=index,permutations=items))
            return dict(flags=u16(s),rate=(22050,44100,32000,48000)[rate],codec=codec,pitch_ranges=pitch_rows)
        visited=set()
        for _,(bank,tag) in tags.items():
            if tag['group']!='lsnd' or not tag['name'].startswith('sound\\vehicles\\banshee\\'):continue
            tracks=bank.block(tag['offset']+0x24,0x58)
            for track in tracks:
                for phase,offset in enumerate((0x18,0x20,0x28,0x44,0x30,0x4c,0x38)):
                    datum=bank.u(track+offset)
                    if datum==0xffffffff or datum in visited:continue
                    visited.add(datum);sb,st=tags[datum]
                    row=dict(name=st['name'],lsnd=tag['name'],phase=phase,track_flags=bank.u(track+4),mcc=sound(sb,st,True))
                    if st['name'] in original_names:row['xbox']=sound(original,original_names[st['name']],False)
                    rows.append(row)
        report=dict(scope=__doc__,source_sha256={str(p):sha(p) for p in (local.path,shared.path,original.path)},
            rows=rows,metadata_only=True,native_playback_verified=False)
        out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/banshee-loop-packets.json'
        assert not out.exists();out.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(dict(sounds=len(rows),summary=[dict(name=r['name'],phase=r['phase'],track_flags=r['track_flags'],
            ranges=len(r['mcc']['pitch_ranges']),codec=r['mcc']['codec'],
            chunks=[len(p['chunks']) for x in r['mcc']['pitch_ranges'] for p in x['permutations']]) for r in rows])),flush=True)
    finally:local.close();shared.close();original.data.close();original.stream.close()
if __name__=='__main__':main()
