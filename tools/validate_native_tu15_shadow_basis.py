#!/usr/bin/env python3
"""Compare typed shadow arithmetic with unchanged physical i386 TU1.5 blocks.

Run only under run_test.py / CTest. All original ELF/observer/banks remain in
HALO2_TEST_SCRATCH. Only the bounded report is retained.
"""
import argparse
import hashlib
import gzip
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
STACK,SP,DATA,SHADOW=0x62000000,0x62000800,0x1001000,0x48c000


def real(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def cases():
    def row(name,direction,axial=real(.2),transverse=real(.3),center=None,cw=0x27f,mxcsr=0x1f80,status=0):
        return dict(name=name,center=center or [real(1.25),real(-2.5),real(3.75)],direction=list(direction),
            axial=axial,transverse=transverse,cw=cw,mxcsr=mxcsr,status=status)
    for xyz in itertools.product((-1.,0.,1.),repeat=3):yield row('axis-'+str(xyz),map(real,xyz))
    for y in range(0x3f666663,0x3f66666a):
        for sign in (0,0x80000000):yield row(f'basis-threshold-{y:x}-{sign:x}',[real(.3),y^sign,real(.3162278)])
    for n,xyz in enumerate(itertools.permutations((0x38d1b716,0x330b05d9,0))):yield row('unrounded-epsilon-'+str(n),xyz)
    for x in range(0x38d1b713,0x38d1b71c):yield row('epsilon-'+str(x),(x,0,0))
    for center in itertools.product((0,0x80000000),repeat=3):
        yield row('zero-depth-'+str(center),(0,0,0),axial=0,transverse=0,center=list(center))
    special=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0xff7fffff,
        0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff854321)
    for axis in range(3):
        for v in special:
            d=[real(.13),real(.61),real(-.37)];d[axis]=v;yield row(f'special-{axis}-{v:x}',d)
    for a,b in itertools.product(special[-4:],repeat=2):yield row(f'paired-nan-{a:x}-{b:x}',(a,b,real(.25)))
    for v in special:
        yield row('all-'+hex(v),(v,v,v));yield row('axial-'+hex(v),(real(.1),real(.2),real(.3)),axial=v)
        yield row('transverse-'+hex(v),(real(.1),real(.2),real(.3)),transverse=v)
        yield row('center-'+hex(v),(real(.1),real(.2),real(.3)),center=[v,v,v])
    rng=random.Random(0x32340)
    for n in range(64):yield row('raw-'+str(n),[rng.getrandbits(32) for _ in range(3)],rng.getrandbits(32),rng.getrandbits(32))
    for cw,mxcsr in itertools.product((0x27f,0x67f,0xa7f,0xe7f,0x7f,0x37f),(0x1f80,0x1fc0,0x9fc0)):
        for n,d in enumerate(((0x38d1b716,0x330b05d9,1),(real(.1),real(.2),real(.3)),(0x7f812345,0xffc54321,real(.9)))):
            yield row(f'controls-{cw:x}-{mxcsr:x}-{n}',d,cw=cw,mxcsr=mxcsr,status=0x20)
    for field in ('owner_fade','history_fade','cached_opacity','radius'):
        for v in (*special,0x3db851eb,0x3db851ec,0x3db851ed):
            item=row(f'gate-{field}-{v:x}',(real(.1),real(.2),real(.3)));item[field]=v;yield item


def prepare(image,scratch):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(0x32340,0x30bd0);metadata=common.metadata(image);metadata['stop']=0x3263e
    sites={s['instruction']:s for s in metadata['sites']};metadata['sites']=list(sites.values())
    directory=scratch/'physical-original';directory.mkdir()
    section=image.section('.text');(directory/'text.bin').write_bytes(image.section_bytes(section))
    (directory/'metadata.json').write_text(json.dumps(metadata))
    sections={'basis':(0x44a000,4096),'constants':(0x46a000,4096),'pointers':(0x474000,4096),
        'shadow':(SHADOW,4096),'data':(0x1000000,8192),'stack':(STACK,8192)}
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{directory/"text.bin"}"\n'
    for name,(_,size) in sections.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n'
    (directory/'reference.s').write_text(assembly)
    linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    linker+=' '.join(f'.{name} 0x{address:x}:{{*(.{name})}}'for name,(address,_)in sections.items())
    linker+=' .bridge 0x70000000:{*(.bridge)}}\n';(directory/'link.ld').write_text(linker)
    for command in (['as','--32',str(directory/'reference.s'),'-o',str(directory/'reference.o')],
        ['ld','-melf_i386','-T',str(directory/'link.ld'),str(directory/'reference.o'),'-o',str(directory/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(directory/'observer')]):
        r=subprocess.run(command,capture_output=True,text=True)
        if r.returncode:raise ValueError('Physical reference build failed: '+r.stderr[-3000:])
    return directory,metadata


def physical(directory,rows,label):
    inputs=directory/(label+'-inputs.json');output=directory/(label+'-observed.json')
    inputs.write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(directory/'observer'),str(directory/'reference'),str(directory/'metadata.json'),str(inputs),str(output)],capture_output=True,text=True,timeout=150)
    if r.returncode:raise ValueError('Physical reference execution failed: '+(r.stdout+r.stderr)[-4000:])
    proof=json.loads(output.read_text())
    if proof['instruction_patches'] or proof['supplied_original_outcomes']:raise ValueError('Reference supplied instructions or outcomes')
    if not all(c['completed']for c in proof['cases']):raise ValueError('Original did not reach declared arithmetic boundary')
    return proof


def bank(row,address):return next(bytes.fromhex(b['after'])for b in row['banks']if b['address']==address)
def u(data,offset):return struct.unpack_from('<I',data,offset)[0]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--fixture-output',type=Path);a=p.parse_args()
    if not a.xbe.is_file():print('SKIP missing owned TU1.5 executable');return 77
    if sha(a.xbe)!=PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);directory,metadata=prepare(image,scratch)
    constants=[(address,int.from_bytes(image.read(address,4),'little'))for address in (0x46a0ec,0x46a0d0,0x46a11c,0x46a194,0x46a0cc,0x46a45c,0x46a0e8,0x46a4c0,0x474d0c,0x474d14)]
    for pointer in (0x474d0c,0x474d14):
        base=dict(constants)[pointer];constants +=[(base+4*n,int.from_bytes(image.read(base+4*n,4),'little'))for n in range(3)]
    rows=list(cases());inputs=[]
    for n,row in enumerate(rows):
        for key,value in dict(owner_fade=real(.75),history_fade=real(.8),cached_opacity=real(.5),radius=real(.1)).items():row.setdefault(key,value)
        row['constants']=constants;data=bytearray(4096);struct.pack_into('<6I',data,0,*row['center'],*row['direction'])
        stack=bytearray(8192);struct.pack_into('<2I',stack,0x820,row['axial'],row['transverse'])
        inputs.append(dict(id=n,name=row['name'],entry=0x32459,stop_pc=0x3263e,gp=[0,0,0,DATA,SP,0,DATA+12,DATA],matrix=0,source=0,
            before_fp=[row['cw'],row['status'],row['mxcsr']],live_x87=[],banks=[
                *[dict(address=address,before=image.read(address,4096).hex())for address in (0x44a000,0x46a000,0x474000)],
                dict(address=SHADOW,before=bytes(4096).hex()),dict(address=DATA,before=data.hex()),dict(address=STACK,before=stack.hex())]))
    prefix=physical(directory,inputs,'prefix');digest=hashlib.sha256();events=0;branches=set();fixture_cases=[]
    # Four disjoint original gate blocks. Stop before each branch/call, so no
    # callee outcome or branch result is supplied to the original execution.
    # The second opacity block is compared only when the first admits it.
    fade_inputs=[]
    for n,row in enumerate(rows):
        c=dict(inputs[n]);stack=bytearray(8192);struct.pack_into('<I',stack,0x80c,row['owner_fade']);struct.pack_into('<I',stack,0x1350,row['history_fade'])
        c.update(entry=0x32386,stop_pc=0x3239b,banks=[dict(b, before=stack.hex())if b['address']==STACK else b for b in inputs[n]['banks']]);fade_inputs.append(c)
    fades=physical(directory,fade_inputs,'fade')
    minimum_inputs=[]
    for n,(row,original)in enumerate(zip(rows,fades['cases'],strict=True)):
        data=bytearray(bank(original,DATA));struct.pack_into('<I',data,0x3c,row['radius'])
        c=dict(inputs[n]);c.update(entry=0x323d9,stop_pc=0x323ee,before_fp=original['after_fp'],banks=[dict(address=b['address'],before=data.hex()if b['address']==DATA else b['after'])for b in original['banks']]);minimum_inputs.append(c)
    minima=physical(directory,minimum_inputs,'minimum')
    first_inputs=[]
    for n,(row,original,fade)in enumerate(zip(rows,minima['cases'],fades['cases'],strict=True)):
        data=bytearray(bank(original,DATA));struct.pack_into('<I',data,0x1c,row['cached_opacity'])
        stack=bytearray(bank(original,STACK));struct.pack_into('<I',stack,0x814,u(bank(fade,STACK),0x80c))
        c=dict(inputs[n]);gp=list(c['gp']);gp[0]=DATA
        c.update(entry=0x32416,stop_pc=0x32422,gp=gp,before_fp=original['after_fp'],banks=[dict(address=b['address'],before=data.hex()if b['address']==DATA else stack.hex()if b['address']==STACK else b['after'])for b in original['banks']]);first_inputs.append(c)
    firsts=physical(directory,first_inputs,'opacity-first')
    second_inputs=[]
    for n,original in enumerate(firsts['cases']):
        c=dict(first_inputs[n]);c.update(entry=0x32428,stop_pc=0x3243b,before_fp=original['after_fp'],xmm=original['boundary_xmm'],banks=[dict(address=b['address'],before=b['after'])for b in original['banks']]);second_inputs.append(c)
    seconds=physical(directory,second_inputs,'opacity-second')
    # Run the disjoint tail with arbitrary completed projection words. This
    # deliberately varies aspect/eye/distance to reject width/tan substitutes.
    tail_inputs=[]
    for n,(row,original)in enumerate(zip(rows,prefix['cases'],strict=True)):
        projection=[0]*31;projection[10]=real(12.5);projection[11:14]=[real(-4.5),0x80000000,real(.25)]
        projection[22]=row['transverse'];projection[23]=row['axial'];row['projection']=projection
        stack=bytearray(bank(original,STACK));struct.pack_into('<31I',stack,0x86c,*projection)
        c=dict(inputs[n]);c.update(entry=0x32656,stop_pc=0x327d5,before_fp=original['after_fp'],banks=[
            dict(address=b['address'],before=(stack.hex()if b['address']==STACK else b['after']))for b in original['banks']]);tail_inputs.append(c)
    tails=physical(directory,tail_inputs,'tail')
    radius_inputs=[]
    for n,original in enumerate(tails['cases']):
        c=dict(inputs[n]);c.update(entry=0x32817,stop_pc=0x32855,before_fp=original['after_fp'],banks=[dict(address=b['address'],before=b['after'])for b in original['banks']]);radius_inputs.append(c)
    radii=physical(directory,radius_inputs,'radius')
    with (scratch/'native.stderr').open('w+')as errors:
        process=subprocess.Popen([str(a.native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,original,tail,radius,fade,minimum,first,second in zip(rows,prefix['cases'],tails['cases'],radii['cases'],fades['cases'],minima['cases'],firsts['cases'],seconds['cases'],strict=True):
                process.stdin.write(json.dumps(row,separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
                if not line:errors.seek(0);raise ValueError(row['name']+': native stopped: '+errors.read()[-3000:])
                actual=json.loads(line);shadow=bank(original,SHADOW);stack=bank(tail,STACK)
                sites=set(original['executed_sites']);direction_normalized=any(w['instruction']==0x30c1b and w['address']==0x48c558 for w in original['writes'])
                up_normalized=any(w['instruction']==0x30c1b and w['address']==0x48c564 for w in original['writes'])
                expected_stores=[[w['instruction'],w['address'],int.from_bytes(bytes.fromhex(w['written']),'little')]for w in original['writes']if SHADOW<=w['address']<SHADOW+4096]
                cached_positive=not(first['boundary_eflags']&0x41)
                gates=dict(fade=u(bank(fade,STACK),0x80c),minimum_radius=minimum['result_bits'],cached_positive=cached_positive,
                    opacity=second['result_bits']if cached_positive else row['cached_opacity'],admitted=cached_positive and not(second['boundary_eflags']&0x41),fp=(second if cached_positive else first)['after_fp'])
                expected=dict(direction=[u(shadow,0x558+4*n)for n in range(3)],up=[u(shadow,0x564+4*n)for n in range(3)],near=u(shadow,0x5e4),far=u(shadow,0x5e8),width=u(shadow,0x5d8),
                    direction_normalized=direction_normalized,up_normalized=up_normalized,second_basis=0x325ce in sites,stores=expected_stores,
                    fp=original['after_fp'],gates=gates,tail_fp=radius['after_fp'],host_fp_and_stack_restored=True,tail=dict(left=[u(stack,0x83c+4*n)for n in range(3)],volume_center=[u(stack,0x854+4*n)for n in range(3)],
                        half_extent=[u(stack,0x860+4*n)for n in range(3)],virtual_eye=row['projection'][11:14],virtual_distance=row['projection'][10],query_radius=u(bank(radius,STACK),0x7f0)))
                for field,value in expected.items():
                    if actual.get(field)!=value:raise ValueError(row['name']+': '+field+' differs; native='+str(actual.get(field))+' original='+str(value))
                fixture_cases.append(dict(input=row,expected=expected))
                branches.add((direction_normalized,up_normalized,expected['second_basis']));events+=len(expected_stores);digest.update(json.dumps(expected,sort_keys=True).encode())
            process.stdin.close()
            if process.wait(timeout=10):raise ValueError('Native process failed')
        finally:
            if process.poll()is None:process.terminate();process.wait(timeout=5)
    names=['include/halo2/native_tu15_shadow_basis_32340.hpp','src/rendering/shadows/native_tu15_shadow_basis_32340.cpp','src/rendering/shadows/source_live_shadow_roots_tu15.cpp',
        'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp','include/halo2/detail/native_tu15_arithmetic_environment.hpp',
        'tests/native_tu15_shadow_basis_tests.cpp','tools/validate_native_tu15_shadow_basis.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp']
    report=dict(passed=True,cases=len(rows),physical_blocks=len(rows)*7,ordered_publications=events,comparison_sha256=digest.hexdigest(),
        branch_combinations=sorted(branches),original_xbe_sha256=PIN,owners=metadata['owners'],
        original_instructions=sum(proof['original_instructions']for proof in (prefix,tails,radii,fades,minima,firsts,seconds)),instruction_patches=[],supplied_original_outcomes=False,
        source_sha256={name:sha(ROOT/name)for name in names},owned_scratch=str(scratch),retained_large_artifacts=[],
        scope='Numeric32459..32635 publications/FP,32656..327D4 tail and32817..32852 radius values/FP, ordered fade/radius/opacity gates; typed ordinary stable inputs, not complete32340 ABI or visibility/GPU parity. Virtual eye/distance passthrough is an API check; actualC3AB0production remains separately owned.',whole_renderer_visual_parity_verified=False)
    if a.fixture_output:
        payload=json.dumps(dict(schema='TU15_NATIVE_SHADOW_ARITHMETIC_V1',original_xbe_sha256=PIN,
            instruction_patches=[],supplied_original_outcomes=False,cases=fixture_cases),separators=(',',':')).encode()
        packed=gzip.compress(payload,mtime=0)
        if len(packed)>1024*1024:raise ValueError('Compact reference fixture exceeds its declared1MiB bound')
        if gzip.decompress(packed)!=payload:raise ValueError('Compact reference fixture changed bytes')
        a.fixture_output.parent.mkdir(parents=True,exist_ok=True);a.fixture_output.write_bytes(packed)
        report['required_reference_fixture']=dict(path=str(a.fixture_output),bytes=len(packed),sha256=sha(a.fixture_output),
            purpose='Typed input/output regression reference when the owned XBE is unavailable; no raw banks or original executable bytes.')
    a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'physical original shadow arithmetic cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
