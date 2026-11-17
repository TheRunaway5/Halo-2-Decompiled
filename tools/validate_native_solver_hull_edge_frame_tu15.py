#!/usr/bin/env python3
"""Private complete313470 edge selection with genuine313390/shape child execution."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
ROOT=Path(__file__).resolve().parents[1]
PIN='3c1f5c38e0c036e8d2b1df5c44703ed4fe403c375e45b1409e20893db138de22'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body():
    value=next(b for sig,b in function_bodies((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())if sig.symbol=='313470')
    from native_solver_hull_edge_replacements import canonical_body
    value=canonical_body(value)
    if hashlib.sha256(value.encode()).hexdigest()!=PIN:raise ValueError('Complete80-site hull-edge owner drift')
    return value
def reference(scratch):
    original=body();text=original.replace('run_313470(','owner(',1).replace('s.push(','atomic_push(s,')
    for pc in (0x313499,0x3134a9):
        needle=f'atomic_push(s,0x{pc:x}u,0x{pc+5:x}u);run_313390(s);'
        if text.count(needle)!=1:raise ValueError('Actual hull-edge direct child transport changed')
        text=text.replace(needle,f'atomic_push(s,0x{pc:x}u,0x{pc+5:x}u);child(s,0x313390u,0x{pc:x}u);',1)
    for opcode,count in (('sub_ps',4),('mul_ps',2),('add_ss',4)):
        name={'sub_ps':'sub','mul_ps':'mul','add_ss':'add_scalar'}[opcode]
        text,n=re.subn(r'_mm_'+opcode+r'\((s.xmm\[[0-7]\]),(s.xmm\[[0-7]\])\)',lambda m:name+'('+m[1]+','+m[2]+')',text)
        if n!=count:raise ValueError('Complete hull-edge numerical instruction seam changed '+opcode)
    sites=[]
    for line in original.splitlines():
        pc=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if pc and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line):sites.append(int(pc[1],16))
    prefix='''static __m128 sub(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,address,{},{}};if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}std::array<std::byte,4>data{};std::memcpy(data.data(),&value,4);s.memory.write(address,data);s.gp[4]=address;if(s.observe){event.written.assign(data.begin(),data.end());s.observe(event);}}
'''
    output=prefix+'static const std::set<std::uint32_t>io_sites{'+','.join(hex(pc)+'u'for pc in sites)+'};\n'+text
    (scratch/'solver_hull_edge_reference.inc').write_text(output)
    return original,sites,dict(whole_body_sha256=PIN,phased_reference_sha256=hashlib.sha256(output.encode()).hexdigest(),atomic_push_transport=True,ordered_simd_operations=10,original_x87_source_unchanged=True)
def inputs(image):
    rng=random.Random(0x313470);base=0x61000000;rows=[];tables=(0x4620d0,0x4613a0,0x462090)
    for table,target in zip(tables,(0x1f6610,0x2e8d20,0x306790),strict=True):
        if struct.unpack('<I',image.read(table+0x30,4))[0]!=target:raise ValueError('Genuine support vtable owner drift')
    constants=[dict(address=0x44a000,before=image.read(0x44a000,0x1000).hex()),dict(address=0x461000,before=image.read(0x461000,0x2000).hex())]
    edges=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000)
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    for k in range(96):
        raw=bytearray(rng.randbytes(0x10000));context=base+0x1000;direction=base+0x2000;origin=base+0x2100;output=(base+0x3000,direction,origin,context)[k%4]
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=context;gp[4]=base+0xe000+k%16
        def put(address,value):struct.pack_into('<I',raw,address-base,value&0xffffffff)
        def value(n):return edges[(k+n*3)%len(edges)]if k>=48 else struct.unpack('<I',struct.pack('<f',rng.uniform(-3,3)))[0]
        for n in range(16):put(context+4*n,value(n))
        put(context+0x40,base+0x5000);put(context+0x44,base+0x6000)
        for address in(direction,origin):
            for n in range(4):put(address+4*n,value(n+address//16))
        for i,shape in enumerate((base+0x5000,base+0x6000)):
            kind=(k//3 if i==0 else k)%3;put(shape,tables[kind]);put(shape+0x14,(1,2,4)[k%3])
            for n in range(32):put(shape+0x20+4*n,value(n+i*5))
            if kind==2:
                for n in range(4):put(shape+0x10+4*n,value(n+4))
        for off,v in enumerate((0x63000000,direction,origin,output)):put(gp[4]+off*4,v)
        cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f)
        rows.append(dict(id=k,name=f'edge-{k}',entry=0x313470,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=0x63000000,matrix=direction,source=origin,before_fp=[cw,0x4100|((k%8)<<11),mxcsr],live_x87=[live[n%4]for n in range((0,1,4,7)[k%4])],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=12000,banks=[dict(address=base,before=raw.hex()),*constants]))
    # Genuine zero/zero support yields translation exactly. The first XY dot
    # remains finite, while its later Z+XY addition alone overflows. Moving that
    # addition before3134DF becomes observable in the actual FP input packet.
    row=copy.deepcopy(rows[0]);row['id']=len(rows);row['name']='edge-late-first-sum-overflow';data=bytearray.fromhex(row['banks'][0]['before'])
    def put(address,*values):struct.pack_into('<'+'I'*len(values),data,address-base,*values)
    put(base+0x1000,0x3f800000,0,0,0);put(base+0x1010,0,0x3f800000,0,0);put(base+0x1020,0,0,0x3f800000,0);put(base+0x1030,0xfe7fffff,0xfe7fffff,0xff7fffff,0);put(base+0x2000,0x3f800000,0x3f800000,0x3f800000,0);put(base+0x2100,0,0,0,0);put(base+0x5000,0x4613a0);put(base+0x6000,0x4613a0)
    row['banks'][0]['before']=data.hex();row['before_fp']=[0x37f,0x4100,0x1f80];row['live_x87']=[];rows.append(row)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--hardware',action='store_true');p.add_argument('--mutations',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    from research_mp_sandbox_update_functions import Xbe
    from validate_native_tu15_prt_helpers import PIN as IMAGE_PIN,physical_original
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['include/halo2/native_tu15_solver_hull_edge_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_edge_frame.hpp','tests/native_tu15_solver_hull_edge_frame_tests.cpp','tools/native_solver_hull_edge_frame_mutants.py','tools/native_solver_hull_edge_replacements.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is not None:
                relative=str(target.relative_to(ROOT))
                if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=IMAGE_PIN:raise ValueError('Wrong original image')
        image=Xbe(a.xbe);original,sites,report['reference']=reference(scratch);rows=inputs(image);path=scratch/'inputs.json';path.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(path)
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_edge_frame_tests.cpp'),'-o',str(scratch/'edge')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull-edge compile failed: '+built.stderr[-10000:])
        with path.open('rb')as inp:run=subprocess.run([str(scratch/'edge')],stdin=inp,capture_output=True,text=True)
        if run.returncode:raise ValueError('Private hull-edge phased comparison failed: '+run.stderr[-10000:])
        report['phased']=json.loads(run.stdout.strip().splitlines()[-1])
        with path.open('rb')as inp:prefixes=subprocess.run([str(scratch/'edge'),'--prefixes'],stdin=inp,capture_output=True,text=True)
        if prefixes.returncode:raise ValueError('Private hull-edge parent prefix protocol failed: '+prefixes.stderr[-10000:])
        report['atomic_provider_prefixes']=json.loads(prefixes.stdout.strip().splitlines()[-1])
        if a.mutations:
            from native_solver_hull_edge_frame_mutants import run_mutants
            report['compiled_defects']=run_mutants(ROOT,scratch,command,path)
        if a.hardware:
            from oracle_native_campaign_screen_lifecycle_tu15 import _Code
            code=_Code(image,'.text')
            if code.at(0x3134f4).mnemonic!='fld'or code.at(0x313525).mnemonic!='fcomp':raise ValueError('Actual transient x87 lifetime instructions changed')
            traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=int(0x3134f8<=pc<=0x313525))for pc in sites]+[dict(entry=entry,arguments=2,gp=True,xmm=True)for entry in(0x313390,0x1f6610,0x2e8d20,0x306790)]
            observed,proof=physical_original(image,rows,scratch,entries=(0x313470,0x313390,0x1f6610,0x2e8d20,0x306790),regions=((0x61000000,0x10000),(0x44a000,0x1000),(0x461000,0x2000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            expected=[];executed=set();parents=children=0
            for case in observed['cases']:
                if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Genuine whole edge did not return')
                packet=[];entries=[]
                for event in case['calls']:
                    saved={key:value for key,value in event.items()if key!='arguments'}
                    if event['entry']in sites:packet.append(saved)
                    else:
                        pc=(0x313499,0x3133ab,0x313425,0x3134a9,0x3133ab,0x313425)[len(entries)];width=5 if event['entry']==0x313390 else 3
                        entries.append(dict(target=event['entry'],site=pc,return_pc=pc+width,arguments=event['arguments'],abi=saved))
                stores=[]
                for event in case['writes']:
                    raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before'])
                    if event['instruction']in(0x313494,0x313539,0x31353c,0x313540,0x31341d,0x313461,0x2e8d2c,0x30679d,0x3067bf):
                        if len(raw)!=16:raise ValueError('Original MOVAPS vector write shape changed')
                        stores.extend(dict(instruction=event['instruction'],address=event['address']+n,before=before[n:n+8].hex(),written=raw[n:n+8].hex())for n in(0,8))
                    else:stores.append(event)
                stop=0x31354d if 0x31354d in case['executed_sites']else 0x31355d;row=rows[case['id']]
                expected.append(dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],fp=case['after_fp'],live_x87=row['live_x87']),packets=packet,children=entries,stopped=False,banks=sorted(case['banks'],key=lambda b:b['address']),writes=stores));executed.update(case['executed_sites']);parents+=len(packet);children+=len(entries)
            output=scratch/'outputs.json'
            with path.open('rb')as inp,output.open('wb')as out:run=subprocess.run([str(scratch/'edge'),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
            if run.returncode:raise ValueError('Original-input native edge replay failed: '+run.stderr.decode()[-8000:])
            actual=json.loads(output.read_text())
            for n,(want,got)in enumerate(zip(expected,actual,strict=True)):
                if want!=got:
                    field=next(key for key in want if want[key]!=got.get(key));report['first_difference']=dict(case=rows[n]['name'],field=field,expected=want[field],actual=got.get(field));raise ValueError('Whole original edge differs '+rows[n]['name']+' '+field)
            parent_sites={int(m[1],16)for m in re.finditer(r'services.instruction\(0x([0-9a-f]+)u\)',original)}
            report['hardware']=dict(cases=len(rows),parent_sites=len(parent_sites),executed_parent_sites=len(parent_sites&executed),complete_parent_pre_io_packets=parents,genuine_pair_and_shape_entry_packets=children,original_text_unmodified=True,supplied_child_outcomes=False,comparison_sha256=hashlib.sha256(json.dumps(actual,sort_keys=True).encode()).hexdigest(),original=proof)
        report.update(passed=True,scope='Private dense masked/aligned backed caller with one free x87 slot, genuine313390 plus polygon/zero/box children. No source/default/provider feedback or physical page-fault/vector atomicity qualification.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())and hashlib.sha256(body().encode()).hexdigest()==PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull-edge source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
