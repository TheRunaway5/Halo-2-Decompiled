#!/usr/bin/env python3
"""Original CPU-only mainmenu resident-plan selection fixture.

No native helper/library, graphics API, build or emulator process is used.
Original coverage/quantizer/alpha/1BEB0/part metric/1BF50 instructions run
unchanged. All-owner admission is an explicit test input, not live visibility.
The bias6 cases alter one private shader input and only the coverage-camera
distance word; they are controls, not claimed authored mainmenu states.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_rendering import Cache

XBE_SHA = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
MAP_SHA = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
BASE, STACK, STOP = 0x2100000, 0x22fef00, 0x3000000
CONTEXT, POST, POINTERS, RESULT, AUX = [BASE+i for i in (0, 0x1000, 0x2000, 0x3000, 0x4000)]

def packed(values):
    return struct.pack('<'+'I'*len(values), *values)

def words(raw):
    return list(struct.unpack('<'+'I'*(len(raw)//4), raw))

def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]

def real(value):
    return struct.unpack('<f', struct.pack('<I', value))[0]

def main():
    c = Cache(ROOT/'assets/disc/maps/mainmenu.map')
    image = (ROOT/'assets/disc/default.xbe').read_bytes()
    assert hashlib.sha256(image).hexdigest() == XBE_SHA
    assert hashlib.sha256(c.data).hexdigest() == MAP_SHA
    u = Uc(UC_ARCH_X86, UC_MODE_32); u.mem_map(0x10000, 0x600000)
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    for i in range(count):
        _, address, _, offset, size = struct.unpack_from('<5I', image, table-base+i*56)
        u.mem_write(address, image[offset:offset+size])
    u.mem_map(BASE, 0x200000); u.mem_map(0x2300000, 0x100000); u.mem_map(STOP, 4096)
    mapped = c.magic & ~4095
    u.mem_map(mapped, (c.magic+len(c.data)-mapped+4095)&~4095)
    u.mem_write(c.magic, bytes(c.data))
    u.mem_write(0x4e3b44, packed([0x2300000]))
    for tag in c.tags.values():
        u.mem_write(0x2300000+(tag['datum']&65535)*16,
                    packed([int.from_bytes(tag['group'].encode(), 'big'), tag['datum'], tag['address'], tag['size']]))
    _, scenario = c.tag(c.scenario, 'scnr')
    bsp = c.block(scenario+0x210, 68)[0]
    file, size, address = (c.u32(bsp+i) for i in (0, 4, 8))
    u.mem_write(address, bytes(c.data[file:file+size]))
    magic = address-file
    materials = c.block(file+16+164, 32, magic)
    clusters = c.block(file+16+156, 176, magic)
    definitions = c.block(file+16+312, 200, magic)
    instances = c.block(file+16+320, 88, magic)
    def section(at):
        resources = c.block(at+56, 16, magic)
        if not resources: return []
        resource = resources[0]
        start = (c.u32(at+40)&0x3fffffff)+c.u32(at+48)+8+c.u32(resource+12)
        length = c.u32(resource+8); assert length%72 == 0
        result = []
        for ordinal in range(length//72):
            p = start+ordinal*72
            material = struct.unpack_from('<h', c.data, p+4)[0]
            assert 0 <= material < len(materials)
            shader = c.u32(materials[material]+12)
            result.append((ordinal, shader, c.u32(p+0x2c)))
        return result
    owners = [(i, i, p, section(p)) for i,p in enumerate(clusters)]
    owners += [(-2-i, c.u16(p+52), p, section(definitions[c.u16(p+52)])) for i,p in enumerate(instances)]
    stop_target=STOP
    def stop(_u, at, _size, _data):
        if at==stop_target: _u.emu_stop()
    u.hook_add(UC_HOOK_CODE,stop)
    def run(at, args=(), end=STOP):
        nonlocal stop_target
        stop_target=end
        u.mem_write(STACK, packed([STOP, *args])); u.reg_write(UC_X86_REG_ESP, STACK)
        u.reg_write(UC_X86_REG_MXCSR, 0x1f80); u.reg_write(UC_X86_REG_FPCW, 0x027f)
        u.reg_write(UC_X86_REG_FPTAG, 0xffff); u.reg_write(UC_X86_REG_FPSW, 0)
        try: u.emu_start(at, end, count=200000)
        except Exception as error:
            raise RuntimeError(f'original{at:X} failed at{u.reg_read(UC_X86_REG_EIP):X} '
                               f'EAX{u.reg_read(UC_X86_REG_EAX):X} ECX{u.reg_read(UC_X86_REG_ECX):X} '
                               f'EDX{u.reg_read(UC_X86_REG_EDX):X} ESI{u.reg_read(UC_X86_REG_ESI):X}') from error
        assert u.reg_read(UC_X86_REG_EIP) == end, hex(u.reg_read(UC_X86_REG_EIP))
    def setup(bank, present, cinematic):
        camera = [bank[i] for i in (3, 6, 9, 12, 47)]
        for at, value in zip((0x4b9e20,0x4b9e2c,0x4b9e38,0x4b9e44,0x4b9ed0), camera):
            u.mem_write(at, packed([value]))
        u.mem_write(0x4b9dd0, struct.pack('<H', 0)); u.mem_write(0x4b9dd4, struct.pack('<H', 480))
        u.mem_write(0x510c50, packed([AUX if present else 0])); u.mem_write(AUX+5, bytes([cinematic]))
        run(0x1beb0)
        return words(u.mem_read(0x485b28,28))
    def coverage(owner, p):
        if owner >= 0:
            u.reg_write(UC_X86_REG_ECX, p+magic); u.reg_write(UC_X86_REG_EDX, 0)
            u.reg_write(UC_X86_REG_EBX, CONTEXT); u.reg_write(UC_X86_REG_XMM0, bits(.5))
            run(0x1333da, end=0x1334e7)
            value = u.reg_read(UC_X86_REG_XMM1)&0xffffffff
        else:
            u.mem_write(STACK+0x30, packed([CONTEXT])); u.mem_write(CONTEXT+0x18, packed([POST]))
            u.mem_write(POST+0x14, packed([POINTERS])); u.mem_write(POST+8, packed([POINTERS]))
            u.reg_write(UC_X86_REG_ESI, p+magic)
            u.reg_write(UC_X86_REG_EBX, 0); run(0x133582, end=0x13362d)
            value = words(u.mem_read(STACK+0x18,4))[0]
        u.mem_write(STACK+0x38, packed([value])); u.reg_write(UC_X86_REG_EBX, 0)
        run(0x2cded, end=0x2ce7b); byte = u.mem_read(0x4c0b84,1)[0]
        alpha = 255
        if owner < 0:
            u.reg_write(UC_X86_REG_XMM0, value); u.reg_write(UC_X86_REG_XMM2, 0)
            run(0x13362d, end=0x13368f)
            alpha = u.reg_read(UC_X86_REG_EAX)&255
        return byte, alpha
    def choice(shader, scale, byte, override):
        _, p = c.tag(shader, 'shad'); bias = override if override is not None else c.u16(p+0x3c)
        resolved = scale if real(scale)>0 else bits(1)
        u.mem_write(CONTEXT+0x28, packed([resolved])); u.mem_write(0x4670e4, packed([bits(.85)]))
        u.reg_write(UC_X86_REG_EAX, byte); u.reg_write(UC_X86_REG_ECX, CONTEXT)
        run(0x410f8, end=0x410fc); run(0x41101, end=0x41116)
        metric = u.reg_read(UC_X86_REG_XMM0)&0xffffffff
        u.reg_write(UC_X86_REG_EAX, bias); u.mem_write(0x4ba019, b'\0')
        run(0x1bf50, [shader, metric, RESULT])
        return metric, words(u.mem_read(RESULT,4))[0], u.reg_read(UC_X86_REG_EAX)&255
    source_camera = ROOT/'tests/fixtures/menu_camera_bank_original.tsv'
    cameras = {}
    for line in source_camera.read_text().splitlines():
        f = line.split()
        if f and f[0]=='C' and int(f[2])==bits(70) and f[3]=='0':
            cameras[int(f[1])] = list(map(int, f[14:62]))
    candidates=[]
    for owner in owners:
        if owner[0]<0: continue
        for part in owner[3]:
            _, root = c.tag(part[1], 'shad')
            post = c.block(root+0x20, 124)[0]
            _, stem = c.tag(c.u32(post), 'stem')
            tp = c.block(stem+0x58, 40)[0]
            thresholds = c.block(tp,10)
            if len(thresholds)>1 and real(c.u32(thresholds[0]+6))>0:
                candidates.append((owner,part))
    assert candidates
    control_owner, control_part = candidates[0]
    control_shader = control_part[1]
    _, control_root = c.tag(control_shader, 'shad')
    selected_control = None
    for distance in range(0, 200001, 250):
        bank = cameras[181].copy(); bank[12] = bits(real(bank[12])+distance)
        variants = []
        for cinematic in (0,1):
            setup(bank, 1, cinematic)
            byte, alpha = coverage(control_owner[0], control_owner[2])
            variants.append(choice(control_shader, control_part[2], byte, 6)[1:])
        if variants[0] != variants[1]: selected_control = (bank, distance); break
    assert selected_control is not None, 'bias6 control did not distinguish auxiliary forwarding'
    cases = [(tick, cameras[tick], 0, cinematic, 0xffffffff) for tick in (0,181,840,2990) for cinematic in (0,1)]
    cases += [(-1, selected_control[0], present, cinematic, control_shader) for present,cinematic in ((0,0),(0,1),(1,0),(1,1))]
    lines = ['# V case tick present byte5 private_bias6_shader private_bias6_file_offset bank48 original_view7',
             '# P case owner section part shader coverage_byte alpha metric_bits original_lod original_accepted']
    lower = alpha_zero = records = 0
    for case, (tick, bank, present, cinematic, controlled) in enumerate(cases):
        view = setup(bank, present, cinematic)
        # Actual title cases have auxiliary pointer present, including byte5=0.
        if controlled==0xffffffff:
            present = 1; view = setup(bank, present, cinematic)
        lines.append('V '+' '.join(str(v) for v in [case,tick,present,cinematic,controlled,
                     control_root+0x3c if controlled!=0xffffffff else 0,*bank,*view]))
        for owner, section_id, p, parts in owners:
            byte, alpha = coverage(owner, p)
            for ordinal, shader, scale in parts:
                metric, lod, accepted = choice(shader, scale, byte, 6 if shader==controlled else None)
                if not alpha: lod, accepted = 0xffffffff, 0; alpha_zero += 1
                lower += accepted and lod>0; records += 1
                lines.append('P '+' '.join(str(v)for v in [case,owner,section_id,ordinal,shader,byte,alpha,metric,lod,accepted]))
    output = ROOT/'tests/fixtures/menu_bsp_resident_original.tsv'
    output.write_text('\n'.join(lines)+'\n')
    report = {'xbe_sha256':XBE_SHA,'map_sha256':MAP_SHA,'camera_fixture_sha256':hashlib.sha256(source_camera.read_bytes()).hexdigest(),
              'fixture_sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'views':len(cases),'part_records':records,
              'lower_lod_records':lower,'zero_alpha_records':alpha_zero,'private_bias6_shader':f'{control_shader:08X}',
              'private_bias6_file_offset':control_root+0x3c,'controlled_camera_distance_word_add':selected_control[1],
              'source_routines':['1BEB0 complete','1333DA..1334E7 cluster coverage','133582..13362D instance coverage',
                 '2CDED..2CE7B collector byte','13362D..13368F instance alpha','410F8+41101..41116 part metric','1BF50 complete ordinary shad'],
              'explicit_inputs':'All70clusters/104instances; resolved45CE0scale fallback; original retained render-camera words. Controls privately supplybias6 anddistance offset; no visibility, residency, original caller chronology or framebuffer parity claim.',
              'no_native_build_library_or_graphics':True}
    output.with_suffix('.provenance.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__=='__main__': main()
