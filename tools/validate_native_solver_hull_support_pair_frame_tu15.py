#!/usr/bin/env python3
"""Private313390 phased parent and genuinely computed1F6610 child transport."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from native_solver_hull_support_pair_replacements import canonical_body
ROOT=Path(__file__).resolve().parents[1]
PIN='84b0e536ea240e5c8aeeaf372cc0db14667185c3bd622418fc78544e59fa70ef'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body():
    value=canonical_body(next(b for sig,b in function_bodies((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())if sig.symbol=='313390'))
    if hashlib.sha256(value.encode()).hexdigest()!=PIN:raise ValueError('Complete313390 source owner drift')
    return value

def reference(scratch):
    original=body();value=original.replace('run_313390(','owner(',1).replace('s.push(','atomic_push(s,')
    for name,count in [('mul',6),('add',5)]:
        value,n=re.subn(r'_mm_'+name+r'_ps\((s.xmm\[[0-7]\]),(s.xmm\[[0-7]\])\)',lambda match:name+'('+match[1]+','+match[2]+')',value)
        if n!=count:raise ValueError('Support-pair authored numerical seam drift '+name)
    primitive='''static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,address,{},{}};if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);s.memory.write(address,bytes);s.gp[4]=address;if(s.observe){event.written.assign(bytes.begin(),bytes.end());s.observe(event);}}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t pc,bool tail){if(tail)throw std::runtime_error("Unexpected support-pair tail");child(s,target,pc);}
'''
    sites=[]
    for line in original.splitlines():
        match=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if match and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line):sites.append(int(match[1],16))
    text=primitive+'static const std::set<std::uint32_t>io_sites{'+','.join(hex(pc)+'u'for pc in sites)+'};\n'+value
    (scratch/'solver_hull_support_pair_reference.inc').write_text(text)
    return dict(body_sha256=PIN,phased_reference_sha256=hashlib.sha256(text.encode()).hexdigest(),scope='Explicit atomic PUSH and original-ordered6MULPS/5ADDPS; canonical-backed parent transport and genuine qualified polygon children. Independent unchanged-i386 parent authority remains required.'),original

def inputs(image):
    if struct.unpack('<I',image.read(0x4620d0+0x30,4))[0]!=0x1f6610:raise ValueError('Actual polygon vtable slot30 identity drift')
    rng=random.Random(0x313390);rows=[];base=0x61000000
    constants=[dict(address=0x44a000,before=image.read(0x44a000,0x1000).hex()),dict(address=0x461000,before=image.read(0x461000,0x2000).hex())]
    edge=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000)
    for k in range(160):
        bank=bytearray(rng.randbytes(0x10000));gp=[rng.getrandbits(32)for _ in range(8)];context=base+0x1000;direction=base+0x2000;pair=(base+0x3000,context,direction,base+0x6020)[k%4];gp[1]=context;gp[4]=base+0xe000+k%16
        def put(address,value):struct.pack_into('<I',bank,address-base,value&0xffffffff)
        put(gp[4],0x63000000);put(gp[4]+4,direction);put(gp[4]+8,pair);put(context+0x40,base+0x5000);put(context+0x44,base+0x6000)
        for n in range(16):put(context+n*4,rng.getrandbits(32)if k>=64 else struct.unpack('<I',struct.pack('<f',rng.uniform(-4,4)))[0])
        for n in range(4):put(direction+n*4,edge[(k+n*3)%len(edge)]if k>=64 else struct.unpack('<I',struct.pack('<f',rng.uniform(-4,4)))[0])
        for shape in(base+0x5000,base+0x6000):
            put(shape,0x4620d0);put(shape+0x14,(1,2,3,4,8)[k%5])
            for n in range(32):put(shape+0x20+n*4,edge[(k+n*5)%len(edge)]if k>=64 else struct.unpack('<I',struct.pack('<f',rng.uniform(-5,5)))[0])
        if 64<=k<128 and k%16==0:
            for n in range(16):put(context+n*4,0x7fc10001+n*0x101)
            for n in range(4):put(direction+n*4,0xffc20001+n*0x101)
        if k>=128:
            parent_bp=gp[4]-4;frame=(parent_bp&~15)-0x20
            pair=frame-0x20
            direction_after=base+0x2100;put(gp[4]+8,pair)
            for n in range(4):put(direction_after+n*4,edge[(k+n)%len(edge)])
            for shape in(base+0x5000,base+0x6000):put(shape+0x14,1)
            for n in range(4):put(base+0x5020+n*4,0x3f800000+n*0x100000);put(base+0x6020+n*4,0x40000000+n*0x100000)
            # Genuine selected point writes overlap the actual polygon's saved
            # BX/BP/return words. Real POPs adopt those DATA values; no child
            # result/ABI is supplied. Keep each original RET word intact.
            for destination,shape,return_pc in [(pair,base+0x5020,0x3133ae),(pair+0x10,base+0x6020,0x313428)]:
                restored_bp=parent_bp
                if k>=144 and destination==pair+0x10:
                    restored_bp=base+0xd100+(k%8)*0x20;put(restored_bp,rng.getrandbits(32));put(restored_bp+4,0x63000000)
                for address,value in [(frame-0x18,direction_after),(frame-0x10,restored_bp),(frame-0x0c,return_pc)]:
                    offset=address-destination
                    if 0<=offset<16 and offset%4==0:put(shape+offset,value)
        cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');top=struct.unpack_from('<3I',bank,gp[4]-base)
        rows.append(dict(id=k,name=f'support-pair-{k}',entry=0x313390,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=top[0],matrix=top[1],source=top[2],before_fp=[cw,0x4100|((k%8)<<11),mxcsr],live_x87=[values[n%4]for n in range((0,1,4,8)[k%4])],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=5000,banks=[dict(address=base,before=bank.hex()),*constants]))
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    from research_mp_sandbox_update_functions import Xbe
    from validate_native_tu15_prt_helpers import PIN as IMAGE_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['include/halo2/native_tu15_solver_hull_support_pair_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp','tests/native_tu15_solver_hull_support_pair_tests.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            path=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if path is None:continue
            relative=str(path.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        if sha(args.xbe)!=IMAGE_PIN:raise ValueError('Wrong original image')
        report['reference'],_=reference(scratch);image=Xbe(args.xbe);rows=inputs(image);input_path=scratch/'inputs.json';input_path.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(input_path)
        command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_support_pair_tests.cpp'),'-o',str(scratch/'probe')]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Support-pair compile failed: '+built.stderr[-9000:])
        with input_path.open('rb')as inp:run=subprocess.run([str(scratch/'probe')],stdin=inp,capture_output=True,text=True)
        if run.returncode:raise ValueError('Support-pair phased comparison failed: '+run.stderr[-12000:])
        report.update(passed=True,source=json.loads(run.stdout.strip().splitlines()[-1]),child_scope='Both real4620D0+30 slots call qualified1F6610 and compute from actual declared shape/vertex DATA. Other owner identities are not supplied outcomes or qualified here; no default route installed.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())and hashlib.sha256(body().encode()).hexdigest()==PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Support-pair source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
