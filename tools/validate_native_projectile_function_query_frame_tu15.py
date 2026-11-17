#!/usr/bin/env python3
"""Private childless100E70 native frame against unchanged original TU1.5 i386."""
import argparse,copy,hashlib,json,os,platform,random,re,shlex,subprocess
from pathlib import Path
from validate_native_tu15_prt_helpers import physical_original,PIN
ROOT=Path(__file__).resolve().parents[1]
FRAME='src/gameplay/objects/native_tu15_projectile_function_query_frame.hpp'
TEST='tests/native_tu15_projectile_function_query_frame_tests.cpp'
REGIONS=((0x46a000,4096),(0x4ec000,4096),(0x4f0000,4096),(0x61000000,16384),(0x62000000,4096))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def boundary_packet(actual,row,pc):
    return dict(entry=pc,gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],
        xmm=actual['boundary_xmm'],fp=actual['after_fp'],
        live_x87=actual.get('boundary_live_x87',row['live_x87']))
def corpus(image,extended_fp=False):
    rng=random.Random(0x100e70);rows=[]
    keys=(0xe000562,0x6000564,0xf000563,0x11000566,0x12000565,0,0x7fffffff,0xffffffff)
    values=(0x00000000,0x80000000,0x3f000000,0x3f800000,0x40000000,0xbf800000,0x7fc01234,0x7fa05678)
    special=(0,0x80000000,1,0x80000001,0x007fffff,0x807fffff,0x00800000,0x80800000,
        0x3f800000,0xbf800000,0x40000000,0x7f800000,0xff800000,0x7fc01234,0xffc0a55a,0x7fa05678)
    for k in range(608 if extended_fp else 96):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(address,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
            raise ValueError('Projectile input escaped declared banks')
        gp=[rng.getrandbits(32)for _ in range(8)];sp=0x62000800+k%16;index=(0,1,31,65535)[k%4]
        body=0x61001000+k%16;definition=0x61002000;slot=0x61000500
        key=keys[k%8]if k<64 else 0x12000565;variant=k//8 if k<64 else k-64
        out=(0x61003000,sp-4,body+0x12c,definition+0xe0)[k//24%4]
        byteout=(0x61003100,sp-3,out+1,definition+0x194)[k//24%4]
        gp[4]=sp
        put(0x4ecbd0,0x61000100);put(0x61000144,(slot-index*12)&0xffffffff);put(slot+8,body)
        put(body,(rng.getrandbits(16)<<16)|1);put(0x4f0414,0x61000800);put(0x61000818,definition)
        put(body+0x12c,(0,2,8,0x100,0x10a,0xffffffff,0,0)[variant%8])
        put(body+0x158,values[variant%8]);put(body+0x168,values[(variant+2)%8])
        put(definition+0xe0,values[(variant+3)%8]);put(definition+0x17c,0x3f800000 if variant%4 else 0)
        put(definition+0x180,0x3f800000 if variant%5 else 0)
        put(definition+0x18c,0x7fc0a55a if variant%7==6 else 0)
        put(definition+0x194,0x7fc0b66b if variant%7==6 else 0x3f800000)
        if k>=96:
            n=(k-96)%32;v=n//2;key=0xf000563 if not n%2 else 0x12000565
            put(body+0x12c,0);put(body+0x168,special[v]);put(definition+0xe0,special[(5*v+7)%16])
            put(definition+0x17c,0x3f800000);put(definition+0x180,0x3f800000)
            put(definition+0x18c,special[(v+4)%16]);put(definition+0x194,special[(7*v+3)%16])
        put(0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'))
        datum=(rng.getrandbits(16)<<16)|index
        put(sp,0x63000000);put(sp+4,datum);put(sp+8,key);put(sp+12,out);put(sp+16,byteout)
        depth=(0,1,4,8)[k%4];extended=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','0000000000000080ffbf')
        # The shared physical runner republishes these two legacy-named
        # fields as the actual first/second stack arguments.
        rows.append(dict(id=k,name=f'projectile-query-{k}-key{key:x}-variant{variant}',entry=0x100e70,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10),0x4100|((k%8)<<11),0x1f80|((k%4)<<13)|(k&0x3f)],live_x87=[extended[n%4]for n in range(depth)],matrix=datum,source=key,max_steps=150,banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)]))
        if k>=96:
            environment=(k-96)//32;mode=environment//4
            rows[-1]['before_fp'][2]=0x1f80|((environment%4)<<13)|(0x40 if mode&1 else 0)|(0x8000 if mode&2 else 0)|(k&0x3f)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--extended-fp',action='store_true');p.add_argument('--observers',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(FRAME,TEST,'include/halo2/native_tu15_projectile_function_query_kernel.hpp','tools/validate_native_projectile_function_query_frame_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
    pins={name:sha(ROOT/name)for name in names};headers={p.resolve():sha(p)for name in('include','src','tests')for p in(ROOT/name).rglob('*')if p.suffix in('.hpp','.h','.inc','.cpp')}
    report=dict(passed=False,production_bound=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original reference image')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from oracle_smg_firing_1_1 import Xbe
        initial=common.metadata
        def metadata(image):
            result=initial(image);code=common._Code(image,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;common.ENTRIES=(0x100e70,);image=Xbe(a.xbe);meta=metadata(image);code=common._Code(image,'.text');sites={v['instruction']for v in meta['sites']}
        if len(sites)!=91:raise ValueError('Original whole projectile CFG changed')
        traces=[dict(entry=v['instruction'],arguments=0,gp=True,xmm=True)for v in meta['sites']if v['operands']or code.at(v['instruction']).mnemonic=='push']
        rows=corpus(image,a.extended_fp);observed,proof=physical_original(image,rows,scratch,entries=(0x100e70,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);report['authority']=proof;normal=len(rows)
        report['extended_fp_domain']=dict(enabled=a.extended_fp,additional_inputs=512 if a.extended_fp else 0,rounding_modes=4,ftz_daz_combinations=4,values='signedzero/subnormal/normal/Inf/qNaN/sNaN')
        if a.prefixes:
            stopped=[]
            for row,actual in zip(rows[:16],observed['cases'][:16],strict=True):
                for phase,key in(('before_read','reads'),('before_write','writes')):
                    for ordinal in range(len(actual[key])):
                        candidate=copy.deepcopy(row);candidate.update(id=len(stopped),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));stopped.append(candidate)
            folder=scratch/'prefixes';folder.mkdir();prefix,receipt=physical_original(image,stopped,folder,entries=(0x100e70,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=stopped;observed['cases']+=prefix['cases'];report['prefix_authority']=receipt
        if a.observers:
            # Obtain real retired instruction states, including PUSH's committed
            # ESP, by stopping unchanged i386 immediately after every store.
            # No state is inferred from a later read or supplied to the original.
            stopped=[];origins=[]
            for index,(row,actual) in enumerate(zip(rows[:normal],observed['cases'][:normal],strict=True)):
                for ordinal in range(len(actual['writes'])):
                    candidate=copy.deepcopy(row)
                    candidate.update(id=len(stopped),name=row['name']+f'-observer-throw-{ordinal}',
                        fault=dict(phase='after_write',ordinal=ordinal),observer=True)
                    stopped.append(candidate);origins.append((index,ordinal))
            folder=scratch/'retired-stores';folder.mkdir()
            retired,receipt=physical_original(image,stopped,folder,entries=(0x100e70,),regions=REGIONS,
                trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            states=[[] for _ in range(normal)]
            for row,actual,(index,ordinal) in zip(stopped,retired['cases'],origins,strict=True):
                if actual['completed'] or actual['boundary']['phase']!='after_write':raise ValueError('Original observer frontier did not stop after store')
                if len(actual['writes'])!=ordinal+1:raise ValueError('Original retired store count changed')
                states[index].append(boundary_packet(actual,row,actual['boundary']['instruction']))
                row['observer_states']=copy.deepcopy(states[index])
            for row,state in zip(rows[:normal],states,strict=True):row.update(observer=True,observer_states=state)
            rows+=stopped;observed['cases']+=retired['cases']
            report.update(observer_authority=receipt,observer_normal_calls=normal,
                original_retired_store_boundaries=len(stopped),
                observer_domain='readonly GP/XMM/flags; ordinary/hostile host FP; real after-store throw prefixes; preimage failures and live observer replacement remain open')
        covered=set();packets=0
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual['completed']!=('fault'not in row):raise ValueError('Original completion differs '+row['name'])
            covered.update(actual['executed_sites']);calls=actual['calls']
            for packet in calls:
                if packet.pop('arguments')!=[]:raise ValueError('Undeclared projectile arguments')
            pc=actual['boundary']['instruction']if'fault'in row else actual['executed_sites'][-1]
            row.update(expected=boundary_packet(actual,row,pc),reads=actual['reads'],writes=actual['writes'],calls=calls)
            for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
            packets+=len(calls)
        report.update(normal_original_returns=normal,original_stopped_prefixes=len(rows)-normal,original_sites=len(sites),covered_sites=len(covered),uncovered_sites=[hex(pc)for pc in sorted(sites-covered)],original_pre_io_packets=packets)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True,timeout=600)
            if built.returncode:raise ValueError('Projectile private compile failed '+built.stderr[-7000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200)
            if done.returncode:raise ValueError('Projectile native comparison failed '+(done.stdout+done.stderr)[-10000:])
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT/FRAME).read_text()
                definitions=(
                    ('software-shift-status','g[0]=shift4(s_,g[0]);','g[0]=s_.flags.shift(g[0],4u,4);'),
                    ('wrong-ret-cleanup','s_.gp[4]+=16u;return s_.gp[0];','s_.gp[4]+=12u;return s_.gp[0];'),
                    ('wrong-second-flag','s_.flags.right(g[1],3u,4,false)','s_.flags.right(g[1],2u,4,false)'),
                    ('skip-ratio-denominator-read','scalar(0x100ef7u,s_.gp[2]+0xe0u)','_mm_set_ss(1.0f)'),
                    ('commuted-nan-multiply','asm volatile("mulss %1,%0":"+x"(first):"x"(second));return first;','asm volatile("mulss %1,%0":"+x"(second):"x"(first));return second;'))
                if a.observers:
                    definitions+=(('skip-byte-observer','if(s_.observe){event.written.assign(data.begin(),data.end());','if(false){event.written.assign(data.begin(),data.end());'),)
                for label,old,new in definitions:
                    if original.count(old)!=1:raise ValueError('Missing projectile defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/FRAME;target.parent.mkdir(parents=True);target.write_text(original.replace(old,new,1))
                    changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(overlay/'bad')
                    compiled=subprocess.run(changed,capture_output=True,text=True,timeout=600)
                    if compiled.returncode:raise ValueError('Projectile defect compile failed '+label+': '+compiled.stderr[-5000:])
                    bad=subprocess.run([str(overlay/'bad'),str(fixture)],capture_output=True,text=True,timeout=1200)
                    diagnostic=(bad.stdout+bad.stderr)[-6000:]
                    if not bad.returncode or not any(value in diagnostic for value in('differ','Unbacked','bank extent')):
                        raise ValueError('Projectile defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
