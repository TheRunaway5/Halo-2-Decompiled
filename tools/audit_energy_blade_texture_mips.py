#!/usr/bin/env python3
"""Compare literal authored BC mip chains with unchanged original8787 images.

This identifies the source bound mip subset. It does not infer the original
streaming/residency producer or change the native texture selection.
"""
import ctypes,hashlib,json,struct
from pathlib import Path
from audit_rendering import Cache
ROOT=Path(__file__).resolve().parents[1]
def digest(data):return hashlib.sha256(data).hexdigest()
def main():
    c=Cache(ROOT/'assets/disc/maps/zanzibar.map')
    source=ROOT/'analysis/multiplayer-shading/appearance-residual-replay/source'
    pipeline=json.loads((source/'8787-pipeline.json').read_text())
    identity=json.loads((ROOT/'analysis/multiplayer-shading/original-energy-blade-draw8787.json').read_text())
    library_path=ROOT/'analysis/frontend/xemu-capture/original-s3tc.so'
    library=ctypes.CDLL(str(library_path));decoder=library.s3tc_decompress_2d
    decoder.argtypes=[ctypes.c_int,ctypes.c_void_p,ctypes.c_uint,ctypes.c_uint];decoder.restype=ctypes.c_void_p
    libc=ctypes.CDLL(None);libc.free.argtypes=[ctypes.c_void_p]
    result=[];pixels=0
    for stage,(bitmap,texture)in enumerate(zip(identity['authored_bitmaps'],pipeline['textures'])):
        record=bitmap['records'][0];pointer,size=record['resource_pointers'][0],record['resource_sizes'][0]
        path=c.path.parent/[c.path.name,'mainmenu.map','shared.map','single_player_shared.map'][pointer>>30]
        with path.open('rb')as stream:stream.seek(pointer&0x3fffffff);raw=stream.read(size)
        assert len(raw)==size
        levels=[];at=0;width,height=record['width'],record['height']
        while at<len(raw):
            length=((width+3)//4)*((height+3)//4)*(8 if record['format']==14 else 16)
            assert at+length<=len(raw)
            compressed=raw[at:at+length];data=ctypes.create_string_buffer(compressed);at+=length
            out=decoder(record['format']-14,data,width,height);assert out
            decoded=ctypes.string_at(out,width*height*4);libc.free(out)
            levels.append(dict(width=width,height=height,compressed_sha256=digest(compressed),decoded=decoded))
            width,height=max(1,width>>1),max(1,height>>1)
        captured=texture['texture'];base=[i for i,l in enumerate(levels)if l['width']==captured['width']and l['height']==captured['height']]
        assert len(base)==1;base=base[0];matches=[]
        for sub in texture['subresources']:
            assert sub['slice']==0
            literal=levels[base+sub['mip']];actual=(source/sub['file']).read_bytes()
            assert literal['decoded']==actual,(stage,sub['mip'])
            pixels+=len(actual)//4
            matches.append(dict(captured_mip=sub['mip'],authored_mip=base+sub['mip'],width=literal['width'],height=literal['height'],compressed_sha256=literal['compressed_sha256'],decoded_sha256=digest(actual),bytes=len(actual)))
        result.append(dict(stage=stage,bitmap=bitmap['datum'],name=bitmap['name'],authored_width=record['width'],authored_height=record['height'],captured_base_mip=base,mips=matches,sampler=texture['binding']['sampler']))
    report=dict(passed=True,event=8787,capture_sha256=identity['capture_sha256'],cache_sha256=digest(c.path.read_bytes()),decoder_source='tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/s3tc.c',decoder_source_sha256=digest((ROOT/'tools/reference/xemu-0.8.136/hw/xbox/nv2a/pgraph/s3tc.c').read_bytes()),decoder_library_sha256=digest(library_path.read_bytes()),compared_mips=sum(len(r['mips'])for r in result),compared_rgba8_pixels=pixels,all_bytes_exact=True,stages=result,boundary='Original8787 stage base authored mip offsets are0,1,2,2. The captured images exactly equal literal authored BC-chain decoding with the pinned original replay decoder. Native highest resident mip selection and Vulkan BC filtering remain separate; no image resized or mip/clock policy fitted. Original1B0A0/143C0 coverage-driven residency is not implemented by this audit.')
    path=ROOT/'analysis/multiplayer-shading/energy-blade-texture-mips-original.json';path.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{report["compared_mips"]} original8787 bound mips / {pixels} RGBA8 pixels exact; base authored offsets0,1,2,2.')
if __name__=='__main__':main()
