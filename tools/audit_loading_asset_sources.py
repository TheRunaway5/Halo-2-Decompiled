#!/usr/bin/env python3
"""Compare permitted MCC/Mod Kit loading inputs to offline Xbox references.

The editable-tag reader is pinned and cloned inside run_test's owned scratch.
No installed tag/cache is modified, no PE is executed, and no pixel payload is
retained. Raw-byte identity and metadata are evidence, never a filename claim.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

from audit_rendering import Cache
from generate_original_loading_assets import PIN,ROOT,SOURCES
from import_mcc_hud import MccCache,texture_resource
from export_vista_menu_ui import mip_shapes

PARSER_PIN='7b01076b485cb851277c14defb71c8f88e214de4'


def sha(data):return hashlib.sha256(data).hexdigest()


def resident_pixels(pixels,w,h,fmt,flags,maximum):
    # Same source359E0/12D6D0 resident stop as native retail_mip_count.
    largest=max(w//4,h//4,1)if 14<=fmt<=16 else max(w,h)
    mips=min(maximum,largest.bit_length()-1)if not flags&16 else 0
    data=bytearray();at=0
    for level in mip_shapes(w,h,mips,fmt):
        width,height=level['width'],level['height'];size=level['bytes']
        part=pixels[at:at+size];assert len(part)==size;at+=size
        if flags&8:
            assert fmt==1
            linear=bytearray(size)
            for y in range(height):
                for x in range(width):
                    source=0;destination_bit=1;bit=1
                    while bit<width or bit<height:
                        if bit<width:
                            if x&bit:source|=destination_bit
                            destination_bit<<=1
                        if bit<height:
                            if y&bit:source|=destination_bit
                            destination_bit<<=1
                        bit<<=1
                    linear[y*width+x]=part[source]
            part=linear
        data.extend(part)
    return bytes(data),mips


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--kit',type=Path,required=True)
    p.add_argument('--mcc',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    interface=scratch/'tag-interface'
    subprocess.run(['git','clone','--quiet','https://github.com/General-101/Halo-Tag-Interface.git',str(interface)],check=True)
    subprocess.run(['git','-C',str(interface),'checkout','--quiet',PARSER_PIN],check=True)
    assert subprocess.check_output(['git','-C',str(interface),'rev-parse','HEAD'],text=True).strip()==PARSER_PIN
    sys.path.insert(0,str(interface/'tag_interface'));import tag_interface as ti
    game,engine,definitions,groups,extensions,_,_=ti.get_version_args('halo2',0)
    definitions=game.generate_defs(definitions,str(scratch/'definitions'),groups,extensions)
    dvd=Cache(ROOT/'assets/disc/maps/mainmenu.map');assert sha(dvd.data)==PIN
    mcc=MccCache(a.mcc/'mainmenu.map');rows=[]
    try:
        with (a.mcc/'textures.dat').open('rb')as textures:
            for suffix,count,_ in SOURCES:
                name='globals\\loading_screen\\'+suffix
                original,at=next((t,at)for t in dvd.tags.values()if t['name']==name for _,at in[dvd.tag(t['datum'],'bitm')])
                original_pages=dvd.block(at+68,116);assert len(original_pages)==count
                kit_path=a.kit/'tags/globals/loading_screen'/(suffix+'.bitmap')
                source=kit_path.read_bytes()
                tag=ti.read_file(definitions,str(kit_path.parent),str(kit_path),0,engine_tag=engine)['Data']
                blob=base64.b64decode(tag['processed pixel data']['encoded']);assert len(blob)==tag['processed pixel data']['length']
                kit_pages=tag['bitmaps'];assert len(kit_pages)==count
                candidate=[t for t in mcc.tags.values()if t['name']==name and t['group']=='bitm' and t['offset']is not None]
                assert len(candidate)==1
                mt=candidate[0];mp=mcc.block(mt['offset']+68,168)
                bank=dict(name=name,original_datum=original['datum'],kit_path=str(kit_path),
                    kit_bytes=len(source),kit_sha256=sha(source),kit_processed_bytes=len(blob),
                    mcc_datum=mt['datum'],mcc_pages=len(mp),pages=[])
                for index,(kp,dp)in enumerate(zip(kit_pages,original_pages)):
                    w,h,fmt=kp['width'],kp['height'],kp['format']['value']
                    offset=kp['pixels offset'];end=kit_pages[index+1]['pixels offset']if index+1<len(kit_pages)else len(blob)
                    assert 0<=offset<end<=len(blob);pixels=blob[offset:end]
                    pointer,size=dvd.u32(dp+28),dvd.u32(dp+52);assert pointer>>30 in(0,1)
                    op=bytes(dvd.data[pointer&0x3fffffff:(pointer&0x3fffffff)+size])
                    top=mip_shapes(w,h,0,fmt)[0]['bytes']
                    original_resident,original_mips=resident_pixels(op,dvd.u16(dp+4),dvd.u16(dp+6),
                        dvd.u16(dp+12),dvd.u16(dp+14),dvd.u16(dp+20))
                    kit_resident,kit_mips=resident_pixels(pixels,w,h,fmt,kp['flags'],kp['mipmap count'])
                    row=dict(index=index,width=w,height=h,format=fmt,flags=kp['flags'],additional_mips=kp['mipmap count'],
                        processed_bytes=len(pixels),processed_sha256=sha(pixels),original_bytes=len(op),original_sha256=sha(op),
                        original_width=dvd.u16(dp+4),original_height=dvd.u16(dp+6),original_format=dvd.u16(dp+12),
                        entire_encoded_payload_equal=pixels==op,top_encoded_mip_equal=pixels[:top]==op[:top],top_mip_bytes=top,
                        original_resident_mips=original_mips,kit_resident_mips=kit_mips,
                        original_resident_bytes=len(original_resident),original_resident_sha256=sha(original_resident),
                        kit_resident_sha256=sha(kit_resident),kit_resident_pixels_equal=kit_resident==original_resident)
                    if index<len(mp):
                        q=mp[index];mw,mh,mf,mm=mcc.u16(q+4),mcc.u16(q+6),mcc.u16(q+12),mcc.u16(q+20)
                        expected=sum(x['bytes']for x in mip_shapes(mw,mh,mm,mf))
                        payload,resource=texture_resource(textures,mcc,q,expected)
                        mcc_resident,mcc_mips=resident_pixels(payload,mw,mh,mf,mcc.u16(q+14),mm)
                        row['mcc']=dict(width=mw,height=mh,format=mf,additional_mips=mm,
                            bytes=len(payload),sha256=sha(payload),resource=resource,
                            top_encoded_mip_equal=payload[:top]==op[:top],kit_encoded_payload_equal=payload==pixels,
                            resident_mips=mcc_mips,resident_sha256=sha(mcc_resident),
                            resident_pixels_equal=mcc_resident==original_resident)
                    bank['pages'].append(row)
                rows.append(bank)
    finally:dvd.close()
    report=dict(scope=__doc__,parser_pin=PARSER_PIN,offline_original_mainmenu_sha256=PIN,
        mcc_mainmenu=mcc.provenance,rows=rows,installed_inputs_unchanged=True,
        pixel_payload_retained=False,scratch=str(scratch),scratch_retained=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(banks=len(rows),pages=sum(len(r['pages'])for r in rows),report=str(a.report))))


if __name__=='__main__':main()
