#!/usr/bin/env python3
"""Complete unmodified38390 with authored exclusion volumes and explicit points.

All points, random metadata and scalar edge cases are controlled CPU inputs.
Authored Lockout/Backwash volume and plane bytes are read directly from the
immutable caches. They establish no live weather visibility or particle state.
No original instruction, resource, arithmetic operation or service is replaced.
"""
from pathlib import Path
import argparse, collections, hashlib, json, random, struct, subprocess, sys, tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
POINT,VOLUME,PLANES,STACK,STOP=0x2100000,0x2101000,0x2110000,0x21ffe00,0x3000000
def sha(raw):return hashlib.sha256(raw).hexdigest()
def fw(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def fl(value):return struct.unpack('<f',struct.pack('<I',value))[0]
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def packed(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--asan-binary',type=Path)
    parser.add_argument('--write-fixture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
    u.mem_map(POINT,0x100000);u.mem_map(STOP,4096)
    seen,branch_hits={},collections.Counter()
    def code(g,pc,size,data):
        if pc==STOP:g.emu_stop();return
        assert 0x38390<=pc<0x38448,hex(pc)
        assert bytes(g.mem_read(pc,size))==image.read(pc,size),f'changed source instruction {pc:08X}'
        seen[pc]=size
        if pc in [0x383da,0x383e7,0x3842c,0x38440]:branch_hits[f'{pc:08X}']+=1
    def writing(g,access,address,size,value,data):
        assert STACK-64<=address and address+size<=STACK+4,'original wrote nonstack input/code'
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
    rows,labels,assets=[],[],[]
    def add(point,center,radius,count,planes,label):
        rows.append([*point,*center,radius,count&0xffffffff,len(planes),*(v for p in planes for v in p)])
        labels.append(label)
    randomizer=random.Random(0x38390)
    for name in ['lockout','warlock','backwash']:
        path=ROOT/('assets/disc/maps' if name=='lockout' else 'assets/map-pack/maps')/(name+'.map')
        cache=Cache(path)
        try:
            _,scenario=cache.tag(cache.scenario,'scnr');reference=cache.block(scenario+0x210,68)[0]
            offset,size,memory=words(cache.data[reference:reference+12]);magic=memory-offset
            volumes=cache.block(offset+16+0x8c,24,magic);owned=[]
            for ordinal,at in enumerate(volumes):
                center=words(cache.data[at:at+12]);radius=words(cache.data[at+12:at+16])[0]
                addresses=cache.block(at+16,16,magic)
                planes=[words(cache.data[p:p+16])for p in addresses]
                raw=bytes(cache.data[at:at+24]);raw_planes=b''.join(cache.data[p:p+16]for p in addresses)
                owned.append(dict(ordinal=ordinal,center=[f'{v:08X}'for v in center],radius=f'{radius:08X}',
                    plane_count=len(planes),descriptor_sha256=sha(raw),planes_sha256=sha(raw_planes)))
                samples=[center]
                for fraction in [1.,.25]:
                    for axis in range(3):
                        for sign in [-1.,1.]:
                            point=center.copy();point[axis]=fw(fl(center[axis])+sign*fraction*fl(radius));samples.append(point)
                for _ in range(4):
                    samples.append([fw(fl(v)+randomizer.uniform(-1.5,1.5)*fl(radius))for v in center])
                assert len(samples)==17
                for index,point in enumerate(samples):add(point,center,radius,len(planes),planes,f'authored_{name}_{ordinal}_point_{index}')
            assets.append(dict(map=name,cache_sha256=sha(path.read_bytes()),volumes=owned))
        finally:cache.close()
    zero=[0,0,0]
    for radius in [fw(0),fw(1),fw(-1),0x7f800000,0x7fc00000]:
        for point in [zero,[fw(1),0,0],[0x3f7fffff,0,0],[0x7fc00001,0,0]]:
            add(point,zero,radius,0,[],'controlled_sphere_boundary')
    for count in [0,0xffffffff,0x80000000,0x7fffffff]:
        add([fw(2),0,0],zero,fw(1),count,[],'controlled_count_not_consumed_after_sphere_miss')
    epsilon=words(image.read(0x45dbdc,4))[0]
    for distance in [epsilon-1,epsilon,epsilon+1,0x7fc00000,0x7f800000,0xff800000,0,0x80000000]:
        add(zero,zero,fw(1),1,[[0,0,0,distance^0x80000000]],'controlled_plane_epsilon_or_unordered')
    for normals in [[-16777216,16777216,1],[16777216,-16777216,1],[-1,16777216,-16777216],[1,-16777216,16777216]]:
        add([fw(1)]*3,[fw(1)]*3,fw(1),1,[[*map(fw,normals),0]],'controlled_source_plane_sum_order')
    for index in range(4096):
        center=[fw(randomizer.uniform(-8,8))for _ in range(3)]
        radius=fw(randomizer.uniform(.001,10))
        point=center if index%4==0 else [fw(randomizer.uniform(-12,12))for _ in range(3)]
        n=index%17
        planes=[[*(fw(randomizer.uniform(-2,2))for _ in range(3)),
            fw(1000 if index%5==0 else randomizer.uniform(-8,8))]for _ in range(n)]
        count=0xffffffff if index%31==0 else n
        add(point,center,radius,count,planes,'controlled_finite_metadata')
    assert len(rows)==4421
    expected=[]
    for row in rows:
        point,center,radius,count,n=row[:3],row[3:6],row[6],row[7],row[8]
        before_point=packed(point);before_volume=packed([*center,radius,count,PLANES]);before_planes=packed(row[9:])
        u.mem_write(POINT,before_point);u.mem_write(VOLUME,before_volume)
        if n:u.mem_write(PLANES,before_planes)
        for register in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(register,0)
        u.reg_write(UC_X86_REG_ESI,POINT);u.reg_write(UC_X86_REG_ECX,VOLUME);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.mem_write(STACK,packed([STOP]))
        u.emu_start(0x38390,STOP,count=10000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+4
        assert bytes(u.mem_read(POINT,12))==before_point and bytes(u.mem_read(VOLUME,24))==before_volume
        if n:assert bytes(u.mem_read(PLANES,len(before_planes)))==before_planes
        expected.append(u.reg_read(UC_X86_REG_EAX)&255)
    encoded=[' '.join(f'{value:08X}'for value in row)for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',
        text=True,capture_output=True,check=True)
    actual=[int(value)for value in native.stdout.split()]
    assert actual==expected,[(i,x,y)for i,(x,y)in enumerate(zip(actual,expected))if x!=y][:8]
    args.write_fixture.write_text('# Complete unchanged38390. pointXYZ centerXYZ radius signedCount ownedCount planes(nx,ny,nz,d) AL; all words hexadecimal\n'+
        '\n'.join(f'{row} {result}'for row,result in zip(encoded,expected))+'\n')
    normal=subprocess.run([str(args.binary.resolve()),str(args.write_fixture.resolve())],text=True,capture_output=True,check=True)
    sanitizer=None
    if args.asan_binary:
        import os
        environment=os.environ.copy();environment['ASAN_OPTIONS']='detect_leaks=1'
        outcome=subprocess.run([str(args.asan_binary.resolve()),str(args.write_fixture.resolve())],
            text=True,capture_output=True,check=True,env=environment)
        assert outcome.stdout==normal.stdout and not outcome.stderr
        sanitizer=dict(binary_sha256=sha(args.asan_binary.read_bytes()),
            validation=outcome.stdout.strip(),address_sanitizer=True,undefined_behavior_sanitizer=True,leak_check=True)
    rejections=[]
    with tempfile.TemporaryDirectory(prefix='halo2-weather-exclusion-')as directory:
        for label,content in [('missing',None),('empty',''),('truncated','\n'.join(args.write_fixture.read_text().splitlines()[:-1])+'\n')]:
            path=Path(directory)/(label+'.tsv')
            if content is not None:path.write_text(content)
            outcome=subprocess.run([str(args.binary.resolve()),str(path)],text=True,capture_output=True)
            assert outcome.returncode!=0
            rejections.append(dict(kind=label,diagnostic=outcome.stderr.strip()))
    report=dict(boundary=__doc__,source_xbe_sha256=PIN,cases=len(rows),exact_cases=len(actual),
        return_words=len(expected),input_words_preserved=sum(len(row)for row in rows),
        groups=dict(collections.Counter(label.split('_point_')[0]if label.startswith('authored_')else label for label in labels)),
        source_instruction_count=len(seen),instruction_sha256=sha(b''.join(packed([pc])+image.read(pc,size)for pc,size in sorted(seen.items()))),
        original_instruction_bytes=184,original_routine_sha256=sha(image.read(0x38390,184)),source_branch_hits=dict(branch_hits),
        assets=assets,tool_sha256=sha(Path(__file__).read_bytes()),
        helper_sha256=sha((ROOT/'src/rendering/weather/source_weather_exclusion.cpp').read_bytes()),
        header_sha256=sha((ROOT/'include/halo2/source_weather_exclusion.hpp').read_bytes()),
        test_sha256=sha((ROOT/'tests/source_weather_exclusion_tests.cpp').read_bytes()),
        fixture_sha256=sha(args.write_fixture.read_bytes()),native_binary_sha256=sha(args.binary.read_bytes()),
        native_validation=normal.stdout.strip(),sanitizer_validation=sanitizer,incomplete_fixture_rejections=rejections,mxcsr='1F80',
        native_compile_requirements=['-fno-fast-math','-ffp-contract=off'],
        services_replaced=[],source_instruction_patches=[],historical_weather_state_claimed=False,live_activation=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    args.write_fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:value for key,value in report.items()if key not in ['assets','groups']},indent=2))

if __name__=='__main__':main()
