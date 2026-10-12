#!/usr/bin/env python3
"""Identify captured title particle textures using unchanged Xemu S3TC decoding.

The private shared library is built directly from the pinned reference source.
Only its allocator/header environment is adapted. A pixel match proves bitmap
identity; it does not establish emitter identity, simulation, or scheduling.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path

from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,default=ROOT/'analysis/frontend/xemu-capture')
    parser.add_argument('--decoder',type=Path,default=ROOT/'analysis/frontend/xemu-capture/original-s3tc.so')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-billboard-asset-matches.json')
    args=parser.parse_args()
    library=ctypes.CDLL(str(args.decoder.resolve()))
    decoder=library.s3tc_decompress_2d
    decoder.argtypes=[ctypes.c_int,ctypes.c_void_p,ctypes.c_uint,ctypes.c_uint]
    decoder.restype=ctypes.c_void_p
    libc=ctypes.CDLL(None);libc.free.argtypes=[ctypes.c_void_p]
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
    captured=json.loads((args.capture/'title-billboard-capture.json').read_text())
    candidates=[]
    for datum,tag in cache.tags.items():
        if tag['group']!='bitm' or not tag['address']:continue
        key=cache.bitmap(datum)
        for index,record in enumerate(cache.bitmaps[key]['records']):
            if record['width']!=128 or record['height']!=128 or record['type']!=0 or record['format']not in(14,15,16)or record['declared_additional_mips']<2:continue
            pointer,size=record['resource_pointers'][0],record['resource_sizes'][0]
            location=pointer>>30;offset=pointer&0x3fffffff
            path=ROOT/'assets/disc/maps'/('', 'mainmenu.map','shared.map','single_player_shared.map')[location]if location else cache.path
            with path.open('rb')as stream:stream.seek(offset);raw=stream.read(size)
            assert len(raw)==size
            levels=[];at=0
            for mip in range(3):
                width,height=record['width']>>mip,record['height']>>mip
                length=((width+3)//4)*((height+3)//4)*(8 if record['format']==14 else 16)
                assert at+length<=len(raw)
                data=ctypes.create_string_buffer(raw[at:at+length]);at+=length
                output=decoder(record['format']-14,data,width,height)
                assert output
                decoded=ctypes.string_at(output,width*height*4);libc.free(output)
                levels.append(hashlib.sha256(decoded).hexdigest())
            candidates.append({'datum':f'{datum:08x}','name':tag['name'],'index':index,
                'format':record['format'],'declared_additional_mips':record['declared_additional_mips'],
                'decoded_level_sha256':levels})
    matches={}
    for resource,texture in captured['textures'].items():
        hashes=[v['sha256']for v in texture['levels']]
        matches[resource]=[c for c in candidates if c['decoded_level_sha256']==hashes]
        assert len(matches[resource])==1,(resource,matches[resource])
    report={'original_capture':captured['capture'],'decoder_source':str(ROOT/'tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/s3tc.c'),
        'decoder_source_sha256':hashlib.sha256((ROOT/'tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/s3tc.c').read_bytes()).hexdigest(),
        'decoder_library_sha256':hashlib.sha256(args.decoder.read_bytes()).hexdigest(),
        'candidate_bitmaps':len(candidates),'compared_mip_levels':3,'matches':matches,
        'bitmap_identity_verified':True,'emitter_identity_verified':False,
        'native_effect_implemented':False,'whole_frame_parity_verified':False}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
