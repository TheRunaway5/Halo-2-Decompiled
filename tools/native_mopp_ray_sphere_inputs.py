"""Authored MOPP/filter/list/sphere DATA; no supplied callee responses."""
import struct
from validate_native_sphere_ray_cast_tu15 import cases as sphere_cases

def cases(image,smoke):
    assert int.from_bytes(image.read(0x4613a0+0x20,4),'little')==0x2e8dd0
    assert int.from_bytes(image.read(0x461f98+0x34,4),'little')==0x306220
    assert int.from_bytes(image.read(0x461f98+0x38,4),'little')==0x306230
    rows=sphere_cases(image,smoke)
    for n,row in enumerate(rows):
        # Add the exact authored callable table page to the existing sparse DATA.
        row['banks'].append(dict(address=0x461000,before=image.read(0x461000,0x1000).hex()))
        def access(at,size):return next(b for b in row['banks']if b['address']<=at and at+size<=b['address']+len(b['before'])//2)
        def read(at,size):
            b=access(at,size);return bytes.fromhex(b['before'])[at-b['address']:at-b['address']+size]
        def put(at,data):
            b=access(at,len(data));v=bytearray.fromhex(b['before']);v[at-b['address']:at-b['address']+len(data)]=data;b['before']=v.hex()
        def word(at,value):put(at,struct.pack('<I',value&0xffffffff))
        sp=row['gp'][4];shape=row['gp'][1];out=int.from_bytes(read(sp+12,4),'little')
        # The parent has a larger live frame. Keep the original numerical/IEEE
        # aliases except private sphere-local aliases, which refer to a different
        # child frame here. The parent output is explicit authored caller DATA.
        if '_out_locals' in row['name']:out=0x61000600
        cache=0x61000a00;collection=0x61000b00;rows_at=0x61000b80;filter_at=0x61000900
        program=0x61000d00;variables=0x61000e00;bounds=0x61000f00;key=1+n%3
        word(shape,0x4613a0);word(collection,0x461f98);word(collection+12,rows_at);word(collection+8,8)
        for lane in range(8):word(rows_at+lane*8,shape);word(rows_at+lane*8+4,0)
        word(program,0x30+key);put(variables,bytes(32));word(variables+0x18,0)
        ray=read(0x61000400,32);put(cache+0x20,ray);put(bounds,ray)
        word(cache+0x40,n%32);word(cache+0x44,0 if n%3==0 else filter_at)
        word(cache+0x50,0);word(cache+0x54,0x3f800000);word(cache+0x58,out);word(cache+0x64,collection)
        word(filter_at,0x41c654)
        for lane in range(32):word(filter_at+12+lane*4,0 if n%7==0 else 0xffffffff)
        for i,value in enumerate((0x63000000,variables,program,bounds)):word(sp+i*4,value)
        row['gp'][1]=cache;row['live_x87']=row['live_x87'][:5]
        row.update(entry=0x30aa70,matrix=variables,source=program,name=row['name']+'_genuine_mopp_sphere',max_steps=4000)
    return rows
