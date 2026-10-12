#!/usr/bin/env python3
"""Compare native glow quads with unchanged original 48B00/480A0 instructions.

Hardware submission is suppressed; the original writes its actual XYZW and UV
commands into a controlled push buffer. This is a calculation comparison, not
a framebuffer or title-marker identity claim.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW,
    UC_X86_REG_MXCSR, UC_X86_REG_ESI, UC_X86_REG_EBX, UC_X86_REG_EDX)

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000


def f32(v):
    return struct.unpack('<f', struct.pack('<f', v))[0]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native-binary', type=Path, required=True)
    p.add_argument('--cases', type=int, default=512)
    p.add_argument('--output', type=Path, default=ROOT/'analysis/frontend/light-volume-projection.json')
    args = p.parse_args()
    image = Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest() == PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x600000)
    for section in image.sections:
        g.mem_write(section.va, image.section_bytes(section))
    g.mem_map(STATE, 0x100000)
    g.mem_map(STOP, 4096)

    def hook(guest, address, size, _):
        if address in (0x1cf50, 0x400fa0, 0x3fb3e0, 0x3fb480):
            sp = guest.reg_read(UC_X86_REG_ESP)
            ret = struct.unpack('<I', guest.mem_read(sp, 4))[0]
            guest.reg_write(UC_X86_REG_ESP, sp+4+(8 if address == 0x3fb3e0 else 0))
            guest.reg_write(UC_X86_REG_EIP, ret)
    g.hook_add(UC_HOOK_CODE, hook)
    # Execute the complete pure 2FD90/2FB70/141590 chain. The native helper
    # receives the resulting 2FB70 XY scales; the lens has separate fixtures.
    camera_rng=random.Random(0x2fd90)
    camera_inputs=bytearray(struct.pack('<I',args.cases));camera_expected=[]
    g.mem_write(0x4e6948,bytes(4));g.mem_write(0x510c50,bytes(4));g.mem_write(0x485ac2,bytes(1))
    for i in range(args.cases):
        eye=[f32(camera_rng.uniform(-300,300))for _ in range(3)]
        forward=[f32(camera_rng.uniform(-1,1))for _ in range(3)]
        up=[f32(camera_rng.uniform(-1,1))for _ in range(3)]
        viewport=(0,0,640,480)if i%2==0 else (160,0,960,720)
        left,top,width,height=viewport
        g.mem_write(STATE,bytes(0x74))
        g.mem_write(STATE,struct.pack('<9f',*eye,*forward,*up))
        g.mem_write(STATE+0x28,struct.pack('<2f',.7592033743858337,1))
        g.mem_write(STATE+0x30,struct.pack('<4h',top,left,top+height,left+width))
        g.mem_write(STATE+0x40,struct.pack('<2f',.015625,1024))
        g.mem_write(STATE+0x1000,struct.pack('<4f',-1.1764706373214722,1.1764706373214722,
            -1.1764706373214722,1.1764706373214722))
        g.mem_write(STACK,struct.pack('<I',STOP))
        g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_ESI,STATE)
        g.reg_write(UC_X86_REG_EDX,STATE+0x1000);g.reg_write(UC_X86_REG_EBX,STATE+0x2000)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(0x2fd90,STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP
        view=bytes(g.mem_read(STATE+0x2000,52))
        projection=bytes(g.mem_read(STATE+0x2078,64))
        values=struct.unpack('<16f',projection)
        camera_expected.append(view+projection)
        camera_inputs.extend(struct.pack('<15f',*eye,*forward,*up,values[0],values[5],*viewport))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    camera_input=args.output.with_suffix('.cameras.inputs.bin')
    camera_output=args.output.with_suffix('.cameras.native.bin')
    camera_input.write_bytes(camera_inputs)
    subprocess.run([str(args.native_binary.resolve()),'--cameras',str(camera_input),str(camera_output)],check=True)
    raw=camera_output.read_bytes()
    assert len(raw)==args.cases*116
    camera_maximum=max(abs(a-b)for i,expected in enumerate(camera_expected)
        for a,b in zip(struct.unpack_from('<29f',raw,i*116),struct.unpack('<29f',expected)))
    assert camera_maximum==0.,camera_maximum
    rng = random.Random(0x480a0)
    fixtures, inputs = [], bytearray(struct.pack('<I', args.cases))
    aspects = [(0,0,640,480),(160,0,960,720),(107,0,1066,800),
               (360,0,960,720),(100,0,800,600),(0,50,800,600)]
    for i in range(args.cases):
        yaw, pitch, roll = [rng.uniform(-math.pi, math.pi) for _ in range(3)]
        cy, sy, cp, sp, cr, sr = math.cos(yaw), math.sin(yaw), math.cos(pitch), math.sin(pitch), math.cos(roll), math.sin(roll)
        view = list(map(f32, [1,cy*cp,sy*cp,-sp,cy*sp*sr-sy*cr,sy*sp*sr+cy*cr,cp*sr,
            cy*sp*cr+sy*sr,sy*sp*cr-cy*sr,cp*cr,*[rng.uniform(-20,20) for _ in range(3)]]))
        near, far = .015625, 1024.
        depth = f32(far/(far-near))
        projection = [f32(rng.uniform(.4,4)),0,0,0,0,f32(rng.uniform(.4,4)),0,0,
                      f32(rng.uniform(-.15,.15)),f32(rng.uniform(-.15,.15)),-depth,-1,
                      0,0,f32(-depth*near),0]
        viewport = aspects[i % len(aspects)]
        point = [f32(rng.uniform(-200,200)) for _ in range(3)]
        radii = [f32(rng.uniform(.1,12)),f32(rng.uniform(.1,12))]
        if i % 23 == 0: radii[0] = radii[1] = -1.
        elif i % 17 == 0: radii[1] = -1.
        angle = rng.uniform(-math.pi,math.pi)
        cosine, sine = f32(math.cos(angle)), f32(math.sin(angle))
        color = rng.getrandbits(32)
        row = struct.pack('<39fI2f', *view,*projection,*viewport,far,*point,*radii,color,cosine,sine)
        inputs.extend(row)
        g.mem_write(0x48568c, struct.pack('<13f', *view))
        g.mem_write(0x485704, struct.pack('<16f', *projection))
        left, top, width, height = viewport
        g.mem_write(0x485648, struct.pack('<4h', top,left,top+height,left+width))
        g.mem_write(0x48565c, struct.pack('<f', far))
        g.mem_write(STATE, struct.pack('<3f', *point))
        g.mem_write(0x407488, struct.pack('<I', STATE+0x3000))
        g.mem_write(STATE+0x3000, struct.pack('<3I', STATE+0x4000,STATE+0x5000,0))
        g.mem_write(STACK, struct.pack('<2I4fI', STOP,STATE,max(radii),radii[1],cosine,sine,color))
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_FPCW, 0x027f)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        g.emu_start(0x480a0, STOP, count=10000)
        assert g.reg_read(UC_X86_REG_EIP) == STOP
        end = struct.unpack('<I', g.mem_read(STATE+0x3000, 4))[0]
        commands = bytes(g.mem_read(STATE+0x4000, end-STATE-0x4000))
        vertices, at, uv = [], 0, None
        while at < len(commands):
            opcode = struct.unpack_from('<I', commands, at)[0]
            if opcode == 0x417fc:
                at += 8
            elif opcode == 0x4190c:
                value = struct.unpack_from('<I', commands, at+4)[0]
                uv = [float(value & 65535), float(value >> 16)]
                at += 8
            else:
                assert opcode == 0x101a00, hex(opcode)
                vertices.append({'screen':list(struct.unpack_from('<4f', commands, at+4)), 'uv':uv})
                at += 20
        assert len(vertices) in (0,4)
        fixtures.append({'input':list(struct.unpack('<39fI2f', row)), 'vertices':vertices})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    input_path, output_path = args.output.with_suffix('.inputs.bin'), args.output.with_suffix('.native.bin')
    input_path.write_bytes(inputs)
    subprocess.run([str(args.native_binary.resolve()),'--quads',str(input_path),str(output_path)],check=True)
    raw, at, maximum, failures, vertices = output_path.read_bytes(), 0, 0., [], 0
    for i, fixture in enumerate(fixtures):
        visible = struct.unpack_from('<I', raw, at)[0]; at += 4
        expected = fixture['vertices']
        assert visible == bool(expected), (i, visible, bool(expected))
        for v, source in enumerate(expected):
            row = struct.unpack_from('<10f',raw,at); at += 40
            vertices += 1
            errors = [abs(a-b) for a,b in zip(row[:4]+row[8:], source['screen']+source['uv'])]
            maximum = max(maximum, *errors)
            if any(errors): failures.append({'case':i,'vertex':v,'errors':errors})
    assert at == len(raw)
    report = {'xbe_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.native_binary.read_bytes()).hexdigest(),
        'entries':['00048b00','000480a0'], 'hardware_stubs':['0001cf50','00400fa0','003fb3e0','003fb480'],
        'cases':args.cases,'visible_quads':vertices//4,'compared_vertices':vertices,'maximum_float_error':maximum,
        'projection_camera_cases':args.cases,'maximum_projection_camera_error':camera_maximum,
        'failures':len(failures),'examples':failures[:8],'passed':not failures,'fixtures':fixtures,
        'whole_frame_parity_verified':False,'matched_title_marker_verified':False}
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('fixtures','examples')},indent=2))
    raise SystemExit(bool(failures))


if __name__ == '__main__':
    main()
