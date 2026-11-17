#!/usr/bin/env python3
"""Private actual77-site hull-line reduction with genuine Pair/Append/shape children."""
import argparse,copy,hashlib,json,os,platform,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_solver_hull_edge_frame_tu15 import inputs as edge_inputs
ROOT=Path(__file__).resolve().parents[1]
BODY_PIN='be94c9aeae3665bdf27dcb043494f38b36637e7d7456fc19341eb4c3c064e664'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body():
    source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
    from native_solver_hull_line_replacements import canonical_body
    value=canonical_body(next(text for sig,text in function_bodies(source)if sig.symbol=='3136a0'))
    if hashlib.sha256(value.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete77-site3136A0 owner drift')
    return value

def reference(scratch):
    original=body();text=original.replace('run_3136a0(','owner(',1).replace('s.push(','atomic_push(s,')
    for pc,target in ((0x3136b6,0x313390),(0x313713,0x313390),(0x313768,0x313290),(0x31377a,0x313290)):
        old=f'atomic_push(s,0x{pc:x}u,0x{pc+5:x}u);run_{target:x}(s);'
        if text.count(old)!=1:raise ValueError('Actual line child seam changed')
        text=text.replace(old,f'atomic_push(s,0x{pc:x}u,0x{pc+5:x}u);child(s,0x{target:x}u,0x{pc:x}u);',1)
    for opcode,count,name in [('sub_ps',4,'sub'),('mul_ps',2,'mul'),('add_ss',4,'add_scalar')]:
        text,n=re.subn(r'_mm_'+opcode+r'\((s.xmm\[[0-7]\]),(s.xmm\[[0-7]\])\)',lambda m:name+'('+m[1]+','+m[2]+')',text)
        if n!=count:raise ValueError('Actual hull-line SIMD seam changed '+opcode)
    sites=[]
    for line in original.splitlines():
        pc=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if pc and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line):sites.append(int(pc[1],16))
    prefix='''static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,address,{},{}};if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}std::array<std::byte,4>data{};std::memcpy(data.data(),&value,4);s.memory.write(address,data);s.gp[4]=address;if(s.observe){event.written.assign(data.begin(),data.end());s.observe(event);}}
static __m128 sub(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
'''
    output=prefix+'static const std::set<std::uint32_t>io_sites{'+','.join(hex(pc)+'u'for pc in sites)+'};\n'+text
    (scratch/'solver_hull_line_reference.inc').write_text(output)
    return original,sites,dict(whole_body_sha256=BODY_PIN,phased_reference_sha256=hashlib.sha256(output.encode()).hexdigest(),atomic_push_transport=True,ordered_simd_operations=10,original_image_text_unmodified=True)
def inputs(image):
    rows=edge_inputs(image)
    for row in rows:
        data=bytearray.fromhex(row['banks'][0]['before']);base=0x61000000
        context=row['gp'][1];stack=row['gp'][4]
        def put(address,value):struct.pack_into('<I',data,address-base,value)
        put(context+0x48,base+0x7000);put(context+0x50,base+0x8000);put(base+0x7080,0)
        vertex=bytes(data[0x2100:0x2110]);data[0x8000:0x8010]=vertex
        row.update(entry=0x3136a0,name='line-'+str(row['id']))
        # The physical observer seeds three historical ABI words. The line
        # owner consumes only the return word; both banks still agree exactly.
        assert struct.unpack_from('<3I',data,stack-base)==(row['return_pc'],row['matrix'],row['source'])
        row['banks'][0]['before']=data.hex()
        row['banks'].append(dict(address=0x46a000,before=image.read(0x46a000,0x1000).hex()))
    # Exact zero shape children isolate the final rounded SSE addition.
    # Each square and the XY sum are finite; only Z+XY overflows.
    row=copy.deepcopy(rows[0]);row.update(id=len(rows),name='line-late-first-sum-overflow',before_fp=[0x37f,0x4100,0x1f80],live_x87=[])
    data=bytearray.fromhex(row['banks'][0]['before'])
    for i in range(3):
        struct.pack_into('<4f',data,0x1000+16*i,*(1.0 if j==i else 0.0 for j in range(4)))
    struct.pack_into('<4f',data,0x1030,-1.2e19,-1.2e19,-1.2e19,0)
    struct.pack_into('<4f',data,0x8000,1.0,0,0,0)
    for offset in(0x5000,0x6000):struct.pack_into('<I',data,offset,0x4613a0)
    row['banks'][0]['before']=data.hex();rows.append(row)
    return rows

def mutants(scratch,command,path):
    kernel='halo2/native_tu15_solver_hull_line_kernel.hpp'
    frame='physics/runtime/native_tu15_solver_hull_line_frame.hpp'
    cases=[
        ('early_first_sum','src',frame,'first_sum_before_sign=false','first_sum_before_sign=true'),
        ('omitted_negative_direction','include',kernel,'d.negative=view.xor_bits(d.direction,d.sign);','d.negative=d.direction;'),
        ('inverted_append_choice','src',frame,'return !s_.flags.zf;','return s_.flags.zf;'),
    ]
    proofs={}
    for name,folder,relative,old,new in cases:
        source=(ROOT/folder/relative).read_text()
        if source.count(old)!=1:raise ValueError('Actual hull-line mutation anchor drift: '+name)
        overlay=scratch/name;header=overlay/relative;header.parent.mkdir(parents=True);header.write_text(source.replace(old,new,1))
        trial=command.copy();trial.insert(trial.index('-I'+str(ROOT/'include')),'-I'+str(overlay));trial[-1]=str(overlay/'line')
        built=subprocess.run(trial,capture_output=True,text=True)
        if built.returncode:raise ValueError('Hull-line mutant did not compile '+name+': '+built.stderr[-6000:])
        with path.open('rb')as inp:run=subprocess.run([trial[-1]],stdin=inp,capture_output=True,text=True)
        if not run.returncode or 'Private hull line phased reference differs'not in run.stderr:raise ValueError('Hull-line mutant lacks expected differential failure '+name+': '+run.stderr[-6000:])
        proofs[name]=dict(rejected=True,diagnostic=run.stderr[-5000:])
    return proofs

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--hardware',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--mutations',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    from research_mp_sandbox_update_functions import Xbe
    from validate_native_tu15_prt_helpers import PIN,physical_original
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['src/physics/physics_live_support_tu15_generated.cpp','include/halo2/native_tu15_solver_hull_line_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_line_frame.hpp','tests/native_tu15_solver_hull_line_frame_tests.cpp','tools/validate_native_solver_hull_edge_frame_tu15.py','tools/native_solver_hull_line_replacements.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is not None:
                relative=str(target.relative_to(ROOT))
                if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU15 original image')
        image=Xbe(a.xbe);original,sites,report['reference']=reference(scratch);rows=inputs(image);path=scratch/'inputs.json';path.write_text(json.dumps(rows,separators=(',',':')))
        report['input_sha256']=sha(path)
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_line_frame_tests.cpp'),'-o',str(scratch/'line')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull-line compile failed: '+built.stderr[-10000:])
        with path.open('rb')as inp:run=subprocess.run([str(scratch/'line')],stdin=inp,capture_output=True,text=True)
        if run.returncode:raise ValueError('Private hull-line phased comparison failed: '+run.stderr[-10000:])
        report['phased']=json.loads(run.stdout.strip().splitlines()[-1])
        if a.prefixes:
            with path.open('rb')as inp:run=subprocess.run([str(scratch/'line'),'--prefixes'],stdin=inp,capture_output=True,text=True)
            if run.returncode:raise ValueError('Private hull-line prefix comparison failed: '+run.stderr[-10000:])
            report['prefixes']=json.loads(run.stdout.strip().splitlines()[-1])
        if a.hardware:
            traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=int(0x31371c<=pc<=0x313756))for pc in sites]
            traces +=[dict(entry=pc,arguments=1 if pc==0x313290 else 2,gp=True,xmm=True)for pc in(0x313390,0x313290,0x1f6610,0x2e8d20,0x306790)]
            observed,proof=physical_original(image,rows,scratch,entries=(0x3136a0,0x313390,0x313290,0x1f6610,0x2e8d20,0x306790),regions=((0x61000000,0x10000),(0x44a000,0x1000),(0x461000,0x2000),(0x46a000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            expected=[];executed=set();parents=children=0
            for case in observed['cases']:
                if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Genuine complete hull-line did not return')
                packets=[];entries=[]
                for event in case['calls']:
                    saved={key:value for key,value in event.items()if key!='arguments'}
                    if event['entry']in sites:packets.append(saved)
                    else:
                        append=0x313768 if 0x313768 in case['executed_sites'] else 0x31377a
                        pc=(0x3136b6,0x3133ab,0x313425,0x313713,0x3133ab,0x313425,append)[len(entries)];width=3 if pc in(0x3133ab,0x313425)else 5
                        entries.append(dict(target=event['entry'],site=pc,return_pc=pc+width,arguments=event['arguments'],abi=saved))
                stores=[]
                for event in case['writes']:
                    data=bytes.fromhex(event['written']);before=bytes.fromhex(event['before'])
                    split_sites={0x31370e,0x31341d,0x313461,0x3132b8,0x3132ca,0x3132e1,0x2e8d2c,0x30679d,0x3067bf}
                    if event['instruction']in split_sites:
                        if len(data)!=16:raise ValueError('Authenticated split vector store width changed')
                        stores.extend(dict(instruction=event['instruction'],address=event['address']+n,before=before[n:n+8].hex(),written=data[n:n+8].hex())for n in(0,8))
                    else:
                        if len(data)==16 and event['instruction']!=0x1f6681:raise ValueError('Unknown genuine vector store protocol')
                        stores.append(event)
                stop=0x313772 if 0x313772 in case['executed_sites']else 0x313784;row=rows[case['id']]
                expected.append(dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,children=entries,stopped=False,banks=sorted(case['banks'],key=lambda b:b['address']),writes=stores));executed.update(case['executed_sites']);parents+=len(packets);children+=len(entries)
            output=scratch/'outputs.json'
            with path.open('rb')as inp,output.open('wb')as out:run=subprocess.run([str(scratch/'line'),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
            if run.returncode:raise ValueError('Original-input hull-line replay failed: '+run.stderr.decode()[-8000:])
            actual=json.loads(output.read_text())
            for n,(want,got)in enumerate(zip(expected,actual,strict=True)):
                if want!=got:
                    field=next(key for key in want if want[key]!=got.get(key));report['first_difference']=dict(case=rows[n]['name'],field=field,expected=want[field],actual=got.get(field));raise ValueError('Whole unchanged original hull-line differs '+rows[n]['name']+' '+field)
            parent_sites={int(m[1],16)for m in re.finditer(r'services.instruction\(0x([0-9a-f]+)u\)',original)}
            if len(parent_sites)!=77 or not parent_sites<=executed:raise ValueError('Missing actual hull-line sites: '+str([hex(pc)for pc in sorted(parent_sites-executed)]))
            report['hardware']=dict(cases=len(rows),parent_sites=len(parent_sites),executed_parent_sites=len(parent_sites&executed),complete_parent_pre_io_packets=parents,genuine_child_entry_packets=children,original_text_unmodified=True,supplied_child_outcomes=False,comparison_sha256=hashlib.sha256(json.dumps(actual,sort_keys=True).encode()).hexdigest(),original=proof)
        if a.mutations:report['compiled_defects']=mutants(scratch,command,path)
        report.update(passed=True,scope='Private dense masked/aligned backed physical-publication view with genuine Pair/Append/shape children. No source/public/default/child-policy or physical partial-vector-fault closure.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())and hashlib.sha256(body().encode()).hexdigest()==BODY_PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull-line source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
