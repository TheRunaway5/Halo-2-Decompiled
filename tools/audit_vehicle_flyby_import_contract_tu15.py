#!/usr/bin/env python3
"""Recover the complete original AP flyby import contract across authored maps.

Read-only data work under run_test.py. Literal native metadata, normalized
import names and every encoded resource extent are kept separate. This does
not create runtime sound tags, aliases, variation state or readiness.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from inspect_multiplayer import Cache,ROOT

NAME=r'sound\vehicles\phantom\phantom_turr_flyby_c_ap'


def require(value,message):
    if not value:raise ValueError(message)


def sha(path):
    with Path(path).open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()


def symbol(cache,value):
    if not value:return ''
    index=value&65535
    require(index<cache.u(356),'Original StringID escapes its bank')
    return cache.text(cache.u(368)+cache.u(cache.u(364)+index*4))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--bindings',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    inventory=json.loads(args.inventory.read_text());bindings=json.loads(args.bindings.read_text())
    needed={r['map'] for r in bindings['edges'] if r['reference']==NAME}
    rows=[];contracts={};resource_pins={}
    for item in inventory['source_files']:
        path=Path(item['xbox'])
        if path.name not in needed:continue
        require(sha(path)==item['xbox_sha256'],'Original map source changed: '+str(path))
        cache=Cache(path)
        try:
            sound=[t for t in cache.tags.values() if t['group']=='snd!' and t['name']==NAME and t['offset'] is not None]
            require(len(sound)==1,'Expected one exact original AP sound owner')
            sound=sound[0];s=sound['offset']
            gestalts=[t for t in cache.tags.values() if t['group']=='ugh!' and t['offset'] is not None]
            classes=[t for t in cache.tags.values() if t['group']=='sncl' and t['offset'] is not None]
            require(len(gestalts)==len(classes)==1,'Ambiguous original sound metadata ownership')
            g=gestalts[0]['offset'];root=bytes(cache.data[s:s+20]);cls=cache.data[s+2]
            playback=cache.block(g,56)[cache.h(s+6)];scale=cache.block(g+8,20)[cache.data[s+11]]
            class_row=cache.block(classes[0]['offset'],92)[cls]
            require(root[12:14]==b'\xff\xff' and cache.h(s+14)==-1,
                    'AP resource acquired nontrivial promotion/custom/extra-info owners')
            shape=dict(flags=int.from_bytes(root[:2],'little'),sound_class=cls,codec=list(root[3:6]),
                       maximum_play_time=cache.u(s+16),playback_hex=bytes(cache.data[playback:playback+56]).hex(),
                       scale_hex=bytes(cache.data[scale:scale+20]).hex(),
                       sound_class_hex=bytes(cache.data[class_row:class_row+92]).hex(),pitch_ranges=[])
            ranges=cache.block(g+32,12);permutations=cache.block(g+40,16)
            imports=cache.block(g+16,4);intervals=cache.block(g+24,10);chunks=cache.block(g+64,12)
            literal=dict(sound_hex=root.hex(),pitch_ranges=[])
            for index in range(root[10]):
                r=ranges[cache.h(s+8)+index];first,count=cache.h(r+8),cache.h(r+10)
                interval=intervals[cache.h(r+2)]
                # 21FFC4..21FFDA use byte-addressed storage selected by the
                # original bit offset. Retain its authored initial contents;
                # a normalized import must not invent an empty used bank.
                flag_count=(count+7)//8
                flag_offset=int.from_bytes(cache.data[r+6:r+8],'little')>>3
                flag_bank=cache.block(g+56,1)
                require(flag_offset+flag_count<=len(flag_bank),'Original used flags escape their owning byte bank')
                used=bytes(cache.data[flag_bank[flag_offset]:flag_bank[flag_offset]+flag_count])
                pitch=dict(name=symbol(cache,cache.u(imports[cache.h(r)])),
                           interval_hex=bytes(cache.data[interval:interval+10]).hex(),
                           used_flags_hex=used.hex(),permutations=[])
                literal_pitch=dict(raw_hex=bytes(cache.data[r:r+12]).hex(),permutations=[])
                for ordinal,p in enumerate(permutations[first:first+count]):
                    first_chunk,chunk_count=cache.h(p+12),cache.h(p+14)
                    parts=[];encoded=hashlib.sha256();encoded_bytes=0
                    for c in chunks[first_chunk:first_chunk+chunk_count]:
                        pointer,flags=cache.u(c),cache.u(c+4);location=pointer>>30;offset=pointer&0x3fffffff
                        resource=path if not location else ROOT/'assets/disc/maps'/(
                            '', 'mainmenu.map','shared.map','single_player_shared.map')[location]
                        if str(resource) not in resource_pins:resource_pins[str(resource)]=sha(resource)
                        size=flags&0x3fffffff
                        with resource.open('rb') as stream:stream.seek(offset);raw=stream.read(size)
                        require(size and len(raw)==size,'Original chunk extent is invalid')
                        encoded.update(raw);encoded_bytes+=size
                        parts.append(dict(path=str(resource),offset=offset,bytes=size,flags=flags>>30,
                                          sha256=hashlib.sha256(raw).hexdigest()))
                    pitch['permutations'].append(dict(ordinal=ordinal,
                        name=symbol(cache,cache.u(imports[cache.h(p)])),
                        chance_and_copy_hex=bytes(cache.data[p+2:p+8]).hex(),
                        original_sample_size=cache.u(p+8),encoded_bytes=encoded_bytes,
                        encoded_sha256=encoded.hexdigest()))
                    literal_pitch['permutations'].append(dict(raw_hex=bytes(cache.data[p:p+16]).hex(),parts=parts))
                shape['pitch_ranges'].append(pitch);literal['pitch_ranges'].append(literal_pitch)
            encoded=json.dumps(shape,sort_keys=True,separators=(',',':')).encode()
            digest=hashlib.sha256(encoded).hexdigest();contracts[digest]=shape
            rows.append(dict(map=path.name,path=str(path),sha256=item['xbox_sha256'],sound_datum=sound['datum'],
                             gestalt_datum=gestalts[0]['datum'],normalized_contract_sha256=digest,literal=literal))
        finally:cache.data.close();cache.stream.close()
    covered={r['map'] for r in rows}
    require(covered==needed,'Import contract omitted authored AP maps')
    report=dict(passed=True,schema='H2_ORIGINAL_VEHICLE_FLYBY_IMPORT_CONTRACT_V1',scope=__doc__,
                owner=str(owner),inventory_sha256=sha(args.inventory),bindings_sha256=sha(args.bindings),
                tool_sha256=sha(Path(__file__)),reader_sha256=sha(ROOT/'tools/inspect_multiplayer.py'),
                name=NAME,contexts=rows,contracts=contracts,resource_files=resource_pins,
                distinct_original_contracts=len(contracts),runtime_import_verified=False,
                original_XBE_executed=False,full_vehicle_parity=False,large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,maps=len(covered),contracts=len(contracts),report=str(args.output))),flush=True)


if __name__=='__main__':main()
