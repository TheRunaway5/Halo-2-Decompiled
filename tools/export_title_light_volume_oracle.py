#!/usr/bin/env python3
"""Capture retail engine-volume records from unchanged XBE instructions.

Only hardware submission is suppressed. Inputs here are controlled fixtures,
not a claim to reproduce a particular captured title frame or object pose.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import random
import math
import subprocess

from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE, STACK, STOP, TABLE = 0x2100000, 0x21fff00, 0x3000000, 0x2300000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
        default=ROOT / 'analysis/frontend/title-light-volume-original.json')
    parser.add_argument('--native-binary', type=Path)
    parser.add_argument('--cases', type=int, default=4)
    args = parser.parse_args()
    image = (ROOT / 'assets/disc/default.xbe').read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    for i in range(count):
        _, address, _, offset, size = struct.unpack_from('<5I', image, table-base+i*56)
        guest.mem_write(address, image[offset:offset+size])
    guest.mem_map(STATE, 0x100000)
    guest.mem_map(TABLE, 0x100000)
    guest.mem_map(STOP, 4096)
    cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    mapped = cache.magic & ~4095
    guest.mem_map(mapped, (cache.magic+len(cache.data)-mapped+4095) & ~4095)
    guest.mem_write(cache.magic, bytes(cache.data))
    guest.mem_write(0x4e3b44, struct.pack('<I', TABLE))
    for tag in cache.tags.values():
        guest.mem_write(TABLE+(tag['datum'] & 65535)*16+8, struct.pack('<I', tag['address']))
    definition = next(t for t in cache.tags.values()
        if t['group'] == 'MGS2' and t['name'].endswith('capitol_ship_engine'))
    captured = []

    def hook(g, address, size, _):
        if address == 0x4f7e0:
            sp = g.reg_read(UC_X86_REG_ESP)
            words = struct.unpack('<9I', g.mem_read(sp, 36))
            records = []
            for i in range(words[2]):
                raw = bytes(g.mem_read(words[1]+i*24, 24))
                records.append({'center': list(struct.unpack_from('<3f', raw)),
                    'radius': list(struct.unpack_from('<2f', raw, 12)),
                    'argb': f'{struct.unpack_from("<I", raw, 20)[0]:08x}',
                    'raw': raw.hex()})
            captured.append({'bitmap': f'{g.reg_read(UC_X86_REG_ECX):08x}',
                'cosine': struct.unpack('<f', struct.pack('<I', words[4]))[0],
                'sine': struct.unpack('<f', struct.pack('<I', words[5]))[0],
                'fuzzy': bool(words[8] & 255), 'records': records})
            g.reg_write(UC_X86_REG_ESP, sp+36)
            g.reg_write(UC_X86_REG_EIP, words[0])
        elif address in (0x16b10, 0x3f5b20, 0x47930, 0x3f5cf0, 0x3fa3d0):
            sp = g.reg_read(UC_X86_REG_ESP)
            ret = struct.unpack('<I', g.mem_read(sp, 4))[0]
            pop={0x3f5b20:4,0x47930:8}.get(address,0)
            g.reg_write(UC_X86_REG_ESP, sp+4+pop)
            g.reg_write(UC_X86_REG_EIP, ret)

    guest.hook_add(UC_HOOK_CODE, hook)
    fixtures = []
    eye = [0., 0., 0.]
    view = [1., 1., 0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0.]
    inputs = list((
        ([100.,30.,-5.], [1.,0.,0.], 1.,1., [1.,1.,1.]),
        ([10.,10.,-20.], [0.,0.,1.], 1.,1., [1.,1.,1.]),
        ([10.,10.,-20.], [1.,0.,0.], .5,.75, [.4,.7,1.]),
        ([10.,10.,-20.], [0.,1.,0.], 2.,.3, [1.,.2,.6]),
    ))
    rng = random.Random(0x4f010)
    def f32(v):
        return struct.unpack('<f', struct.pack('<f', v))[0]
    while len(inputs) < args.cases:
        forward = [rng.uniform(-1,1) for _ in range(3)]
        magnitude = math.sqrt(sum(v*v for v in forward))
        forward = [f32(v/magnitude) for v in forward]
        inputs.append(([f32(rng.uniform(-200,200)) for _ in range(3)],
            forward, f32(rng.uniform(.05,3)), f32(rng.uniform(.01,1.2)),
            [f32(rng.uniform(0,1.3)) for _ in range(3)]))
    native_inputs = bytearray(struct.pack('<I', len(inputs)))
    for index,(position, forward, scale, opacity, color) in enumerate(inputs):
        fixture_eye,fixture_view,projection=eye,view,[2.5,3.]
        if index>=4:
            fixture_eye=[f32(rng.uniform(-100,100))for _ in range(3)]
            yaw,pitch,roll=[rng.uniform(-math.pi,math.pi)for _ in range(3)]
            cy,cp,cr=math.cos(yaw),math.cos(pitch),math.cos(roll)
            sy,sp,sr=math.sin(yaw),math.sin(pitch),math.sin(roll)
            # Three columns of a controlled orthonormal world-to-view rotation.
            basis=[[f32(cy*cp),f32(sy*cp),f32(-sp)],
                [f32(cy*sp*sr-sy*cr),f32(sy*sp*sr+cy*cr),f32(cp*sr)],
                [f32(cy*sp*cr+sy*sr),f32(sy*sp*cr-cy*sr),f32(cp*cr)]]
            translation=[f32(-sum(basis[j][i]*fixture_eye[j]for j in range(3)))for i in range(3)]
            fixture_view=[1.,*basis[0],*basis[1],*basis[2],*translation]
            projection=[f32(rng.uniform(.4,5.)),f32(rng.uniform(.4,5.))]
        guest.mem_write(0x4b9da0, struct.pack('<3f', *fixture_eye))
        guest.mem_write(0x4b9e14, struct.pack('<13f', *fixture_view))
        guest.mem_write(0x4b9ecc, struct.pack('<2f', *projection))
        state = struct.pack('<2I14f', 0, definition['datum'], *position,
            *forward, 0.,0.,1., scale,opacity, *color)
        guest.mem_write(STATE, state)
        guest.mem_write(STACK, struct.pack('<2I', STOP, STATE))
        guest.reg_write(UC_X86_REG_ESP, STACK)
        guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        guest.reg_write(UC_X86_REG_FPCW, 0x027f)
        captured.clear()
        try:
            guest.emu_start(0x4f010, STOP, count=2000000)
        except Exception as e:
            raise RuntimeError(f'retail fault at {guest.reg_read(UC_X86_REG_EIP):08x}') from e
        assert guest.reg_read(UC_X86_REG_EIP) == STOP
        assert len(captured) == 1 and len(captured[0]['records']) == 32
        fixtures.append({'position':position, 'forward':forward, 'scale':scale,
            'opacity':opacity, 'color':color, 'eye':fixture_eye,
            'world_to_view':fixture_view,'projection_scale':projection,'output':captured[0]})
        # Native ABI: marker Transform, opacity/color, eye, view Transform,
        # projection axes. Marker left/up do not change this bounded path.
        native_inputs += struct.pack('<35f', scale,*forward,0.,1.,0.,0.,0.,1.,
            *position,opacity,*color,*fixture_eye,*fixture_view,*projection)
    report = {'xbe_sha256':EXPECTED, 'tag':definition, 'entries':['0004f010','0004ff50'],
        'eye':eye, 'world_to_view':view, 'projection_scale':[2.5,3.],
        'hardware_stubs':['00016b10','0003f5b20','0004f7e0'],
        'fixtures':fixtures, 'native_verified':False, 'framebuffer_parity_verified':False}
    if args.native_binary:
        input_path=args.output.with_suffix('.inputs.bin')
        output_path=args.output.with_suffix('.native.bin')
        input_path.parent.mkdir(parents=True,exist_ok=True)
        input_path.write_bytes(native_inputs)
        subprocess.run([str(args.native_binary.resolve()),str(cache.path.resolve()),
            str(input_path.resolve()),str(output_path.resolve())],check=True)
        raw=output_path.read_bytes();at=0;failures=[];maximum=0.;color_failures=0
        for index,fixture in enumerate(fixtures):
            batches=struct.unpack_from('<I',raw,at)[0];at+=4
            assert batches==1
            bitmap,fuzzy,count,cosine,sine=struct.unpack_from('<IIIff',raw,at);at+=20
            original=fixture['output']
            assert bitmap==int(original['bitmap'],16) and fuzzy==original['fuzzy'] and count==32
            for name,a,b in (('cosine',cosine,original['cosine']),('sine',sine,original['sine'])):
                error=abs(a-b);maximum=max(maximum,error)
                if error>2e-5:failures.append({'case':index,'field':name,'native':a,'original':b})
            for sprite,source in enumerate(original['records']):
                values=struct.unpack_from('<5fI',raw,at);at+=24
                expected=source['center']+source['radius']
                for component,(a,b) in enumerate(zip(values[:5],expected)):
                    error=abs(a-b);maximum=max(maximum,error)
                    if not math.isfinite(a) or error>2e-5:
                        failures.append({'case':index,'sprite':sprite,'component':component,'native':a,'original':b})
                if values[5]!=int(source['argb'],16):
                    color_failures+=1
                    failures.append({'case':index,'sprite':sprite,'field':'argb',
                        'native':f'{values[5]:08x}','original':source['argb']})
        guest.mem_write(0x407488,struct.pack('<I',TABLE+0x50000))
        guest.mem_write(0x46713c,b'\x01')
        guest.mem_write(STACK,struct.pack('<I',STOP))
        guest.reg_write(UC_X86_REG_ESP,STACK)
        guest.emu_start(0x4fe20,STOP,count=10000)
        assert guest.reg_read(UC_X86_REG_EIP)==STOP
        original_program=bytes(guest.mem_read(0x484f68,240))
        assert raw[at:at+240]==original_program,'native volume pixel program differs'
        at+=240
        assert at==len(raw)
        # Original named marker lookup/composition, with the same posed nodes
        # supplied to both implementations. This does not certify animation.
        marker_errors={}
        for suffix,label in (('.markers.bin','local'),('.world.markers.bin','world')):
            markers=output_path.with_suffix(output_path.suffix+suffix).read_bytes()
            model,name,nodes=struct.unpack_from('<3I',markers)
            matrices=markers[12:12+nodes*52]
            expected_count=struct.unpack_from('<I',markers,12+nodes*52)[0]
            expected_markers=markers[16+nodes*52:]
            guest.mem_write(STATE+0x1000,matrices)
            guest.mem_write(STACK,struct.pack('<I',STOP))
            guest.reg_write(UC_X86_REG_ESP,STACK)
            guest.reg_write(UC_X86_REG_ECX,model)
            guest.reg_write(UC_X86_REG_EDI,name)
            guest.emu_start(0x1d8f00,STOP,count=10000)
            group=guest.reg_read(UC_X86_REG_EAX)
            assert group!=0xffffffff,'original marker group absent'
            # 1D8F50 ABI established from its unchanged prologue.
            guest.mem_write(STACK,struct.pack('<7I',STOP,0,0,STATE+0x1000,0,STATE+0x2000,65))
            guest.reg_write(UC_X86_REG_ESP,STACK)
            guest.reg_write(UC_X86_REG_EAX,group)
            guest.reg_write(UC_X86_REG_ECX,model)
            guest.emu_start(0x1d8f50,STOP,count=100000)
            actual_count=guest.reg_read(UC_X86_REG_EAX)&65535
            assert actual_count==expected_count,(actual_count,expected_count)
            marker_error=0.
            for i in range(actual_count):
                original_transform=struct.unpack('<13f',guest.mem_read(STATE+0x2000+i*112+56,52))
                native_transform=struct.unpack_from('<13f',expected_markers,i*52)
                marker_error=max(marker_error,max(abs(a-b)for a,b in zip(original_transform,native_transform)))
            assert marker_error<=2e-5,marker_error
            marker_errors[label]=marker_error
        texture=ROOT/'analysis/frontend/xemu-capture/title-volume-texture-13896.bin'
        original_texture=texture.read_bytes()
        assert hashlib.sha256(original_texture).hexdigest()==\
            '31a3f98d90ce470319a1e8a721f6c33de5b2cda4941a8631ed7aa4ea211ce5b1'
        # Xemu's RG8 resource uses R,R,R,G component swizzles.
        expected_rgba=bytearray()
        for luminance,alpha in struct.iter_unpack('<2B',original_texture):
            expected_rgba.extend((luminance,luminance,luminance,alpha))
        native_texture=output_path.with_suffix(output_path.suffix+'.glow.rgba').read_bytes()
        assert native_texture==expected_rgba,'default glow texture differs from original capture'
        report.update({'native_binary_sha256':hashlib.sha256(args.native_binary.read_bytes()).hexdigest(),
            'native_verified':not failures,'failures':len(failures),'examples':failures[:12],
            'maximum_float_error':maximum,'packed_color_failures':color_failures,
            'compared_sprite_records':len(fixtures)*32,'pixel_program_exact':True,
            'original_pixel_program_words':list(struct.unpack('<60I',original_program)),
            'compared_markers':actual_count,'maximum_marker_error':marker_errors['local'],
            'compared_world_markers':actual_count,'maximum_world_marker_error':marker_errors['world'],
            'default_glow_base_level_texels_exact':True,'compared_glow_texels':4096})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'output':str(args.output), 'fixtures':len(fixtures),
        'records':sum(len(v['output']['records']) for v in fixtures),
        **{k:report[k] for k in ('native_verified','framebuffer_parity_verified','failures',
            'examples','maximum_float_error','packed_color_failures') if k in report}}, indent=2))
    if not report['native_verified'] and args.native_binary:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
