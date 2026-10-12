#!/usr/bin/env python3
"""Check authored carrier markers against sampled original title glow quads.

The original capture supplies camera axes/eye and unquantized quad inputs.
Source-backed projection constants and authored placement/idle-pose inputs
produce native candidates. Record residuals rather than tune any parameters.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native-binary',type=Path,required=True)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-light-volume-markers.json')
    args=p.parse_args()
    capture=ROOT/'analysis/frontend/xemu-capture'
    uniform=(capture/'title-volume-6242-stage-0-uniform.bin').read_bytes()
    constants=struct.unpack('<1336f',uniform)
    camera=[1.,*constants[16:19],*constants[20:23],*constants[24:27],*constants[28:31]]
    image=Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(0x2100000,0x100000);g.mem_map(0x3000000,4096)
    g.mem_write(0x2100000,struct.pack('<13f',*camera))
    g.mem_write(0x21fff00,struct.pack('<I',0x3000000))
    g.reg_write(UC_X86_REG_ECX,0x2100000);g.reg_write(UC_X86_REG_EAX,0x2101000)
    g.reg_write(UC_X86_REG_ESP,0x21fff00)
    g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    g.emu_start(0x141590,0x3000000,count=10000)
    assert g.reg_read(UC_X86_REG_EIP)==0x3000000
    view=list(struct.unpack('<13f',g.mem_read(0x2101000,52)))
    lens=json.loads((capture/'title-original-projection.json').read_text())['rows'][1]['scale_xy']
    f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
    depth=f32(1024./(1024.-.015625))
    projection=[lens[0],0,0,0,0,lens[1],0,0,0,0,-depth,-1,0,0,f32(-depth*.015625),0]
    inputs=struct.pack('<37f',*view,*projection,0,0,640,480,1024,*camera[10:13])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    input_path,output_path=args.output.with_suffix('.inputs.bin'),args.output.with_suffix('.native.bin')
    input_path.write_bytes(inputs)
    subprocess.run([str(args.native_binary.resolve()),str(ROOT/'assets/disc/maps/mainmenu.map'),
        '--title-quads',str(input_path),str(output_path)],check=True)
    raw=output_path.read_bytes();count=struct.unpack_from('<I',raw)[0];at=4;candidates=[]
    for _ in range(count):
        placement,marker,sprite=struct.unpack_from('<3I',raw,at);at+=12
        center_radius=struct.unpack_from('<5f',raw,at);argb=struct.unpack_from('<I',raw,at+20)[0];at+=24
        cosine,sine=struct.unpack_from('<2f',raw,at);at+=8
        vertices=[struct.unpack_from('<10f',raw,at+i*40)for i in range(4)];at+=160
        candidates.append({'placement':placement,'marker':marker,'sprite':sprite,
            'center':list(center_radius[:3]),'radius':list(center_radius[3:]),
            'argb':f'{argb:08x}','cosine':cosine,'sine':sine,'vertices':vertices})
    assert at==len(raw)
    source=json.loads((capture/'title-light-volume-capture.json').read_text())
    checks=[]
    for draw in source['passes']:
        event=draw['event']
        original=list(struct.iter_unpack('<4f',(capture/f'title-volume-{event}-input-0.bin').read_bytes()[:64]))
        rgba=draw['postvs']['float_rows'][0][8:12]
        color=[round(v*255)for v in rgba]
        argb=f'{color[3]:02x}{color[0]:02x}{color[1]:02x}{color[2]:02x}'
        eligible=[c for c in candidates if c['argb']==argb]
        assert eligible,(event,argb)
        def xy_error(c):
            return max(abs(a-b)for row,reference in zip(c['vertices'],original)for a,b in zip(row[:2],reference[:2]))
        eligible.sort(key=xy_error)
        best=eligible[0]
        z_error=max(abs(row[2]-reference[2])for row,reference in zip(best['vertices'],original))
        w_error=max(abs(row[3]-reference[3])for row,reference in zip(best['vertices'],original))
        color_error=max(abs(a-b)for row in best['vertices']for a,b in zip(row[4:8],rgba))
        checks.append({'event':event,'placement':best['placement'],'marker':best['marker'],'sprite':best['sprite'],
            'argb':argb,'packed_color_exact':True,'maximum_xy_pixel_error':xy_error(best),
            'maximum_z_depth_unit_error':z_error,'maximum_w_depth_unit_error':w_error,
            'maximum_vertex_color_error':color_error,
            'second_candidate_xy_error':xy_error(eligible[1])if len(eligible)>1 else None})
    passed=all(c['maximum_xy_pixel_error']<=.0005 and c['maximum_w_depth_unit_error']<=4. and
        not c['maximum_vertex_color_error']for c in checks)
    report={'original_capture_sha256':'077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431',
        'native_binary_sha256':hashlib.sha256(args.native_binary.read_bytes()).hexdigest(),
        'camera_fixture_sha256':hashlib.sha256(inputs).hexdigest(),'source_projection':projection,
        'candidate_quads':count,'checks':checks,'sampled_marker_identity_verified':passed,
        'geometry_byte_exact':all(not c['maximum_xy_pixel_error'] and not c['maximum_w_depth_unit_error'] and
            not c['maximum_z_depth_unit_error']for c in checks),'whole_frame_parity_verified':False}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    raise SystemExit(not passed)


if __name__=='__main__':main()
