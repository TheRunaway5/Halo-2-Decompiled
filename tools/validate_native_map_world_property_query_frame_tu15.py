#!/usr/bin/env python3
"""Compare private TU1.5 MapWorld 1DB800 with unchanged original i386."""
import argparse,copy,hashlib,json,os,platform,random,shlex,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
FRAME='src/gameplay/objects/native_tu15_map_world_property_query_frame.hpp'
TEST='tests/native_tu15_map_world_property_query_frame_tests.cpp'
ENTRY=0x1db800
REGIONS=((0x46a000,4096),(0x61000000,16384),(0x62000000,4096))

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def corpus(image):
    rng=random.Random(ENTRY);rows=[]
    keys=(0xa000693,0xd000691,0xd000692,0,0xffffffff,0xd000693)
    values=(0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f800000,
            0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7fa05678)
    for n in range(144):
        banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
        def put(address,value,width=4):
            for (base,size),data in zip(REGIONS,banks,strict=True):
                if base<=address<=base+size-width:
                    data[address-base:address-base+width]=value.to_bytes(width,'little');return
            raise ValueError('MapWorld property input escaped bank')
        gp=[rng.getrandbits(32) for _ in range(8)]
        sp=0x62000800+n%16;record=0x61001000+n%16;environment=n//6
        scalar_out=0x61003000+n%8;byte_out=scalar_out+1 if n%3==0 else 0x61003110
        flag=bool(environment%2);key=keys[n%len(keys)]
        one=int.from_bytes(image.read(0x46a0d0,4),'little')
        scale=int.from_bytes(image.read(0x46a0c4,4),'little')
        if n>=48:
            one=values[environment%len(values)];scale=values[(environment+n%6)%len(values)]
        if key==keys[0] and flag and environment%3==0:
            # This scalar write changes the still-unread byte-output pointer.
            scalar_out=sp+4;one=0x61003100;byte_out=0x61003110
        gp[1]=key;gp[2]=scalar_out;gp[4]=sp;gp[6]=record
        put(record+4,0x2000 if flag else 0)
        put(record+0x1a,(0,1,127,128,255)[environment%5],1)
        put(record+0x1b,(255,128,127,1,0)[environment%5],1)
        put(0x46a0d0,one);put(0x46a0c4,scale)
        put(sp,0x63000000);put(sp+4,byte_out);put(sp+8,0)
        mx=0x1f80|((environment%4)<<13)|(0x40 if environment&4 else 0)|(0x8000 if environment&8 else 0)|(n&0x3f)
        rows.append(dict(id=n,name=f'map-world-query-{n}',entry=ENTRY,gp=gp,matrix=byte_out,source=0,
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],eflags=0x202|(rng.getrandbits(12)&0xcd5),
            before_fp=[(0x7f,0x27f,0x37f)[n%3],0x4100|((n%8)<<11),mx],
            live_x87=['0000000000000080ff3f']*(0,1,4,8)[n%4],max_steps=70,
            banks=[dict(address=base,before=data.hex()) for (base,_),data in zip(REGIONS,banks,strict=True)]))
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--prefixes',action='store_true')
    p.add_argument('--semantic-mutants',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(FRAME,TEST,'include/halo2/native_tu15_map_world_property_query_kernel.hpp',
        'tools/validate_native_map_world_property_query_frame_tu15.py','tools/validate_native_projectile_function_query_frame_tu15.py',
        'tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp')
    pins={name:sha(ROOT/name) for name in names}
    headers={path.resolve():sha(path) for tree in ('include','src','tests') for path in (ROOT/tree).rglob('*') if path.suffix in ('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        image=Xbe(a.xbe);code=common._Code(image,'.text');initial=common.metadata
        def metadata(image):
            result=initial(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;common.ENTRIES=(ENTRY,)
        sites=metadata(image)['sites']
        if len(sites)!=34 or any(code.at(site['instruction']).mnemonic=='call' for site in sites):
            raise ValueError('Original 1DB800 is not the authenticated 34-site leaf')
        traces=[dict(entry=site['instruction'],arguments=0,gp=True,xmm=True) for site in sites if site['operands']]
        rows=corpus(image);normal=len(rows)
        def observe(input_rows,folder):
            return physical_original(image,input_rows,folder,entries=(ENTRY,),regions=REGIONS,
                trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        observed,authority=observe(rows,scratch);report['authority']=authority
        if a.prefixes:
            stopped=[]
            for row,actual in zip(rows[:30],observed['cases'][:30],strict=True):
                for phase,key in (('before_read','reads'),('before_write','writes')):
                    for ordinal in range(len(actual[key])):
                        candidate=copy.deepcopy(row);candidate.update(id=len(stopped),name=row['name']+f'-{phase}-{ordinal}',
                            fault=dict(phase=phase,ordinal=ordinal));stopped.append(candidate)
            folder=scratch/'provider-prefixes';folder.mkdir()
            prefix,receipt=observe(stopped,folder);rows+=stopped;observed['cases']+=prefix['cases'];report['prefix_authority']=receipt
        coverage=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual['completed']!=('fault' not in row):raise ValueError('Original completion changed '+row['name'])
            coverage.update(actual['executed_sites'])
            for packet in actual['calls']:
                if packet.pop('arguments')!=[]:raise ValueError('Unexpected original call arguments')
            pc=actual['boundary']['instruction'] if 'fault'in row else actual['executed_sites'][-1]
            row.update(expected=boundary_packet(actual,row,pc),reads=actual['reads'],writes=actual['writes'],calls=actual['calls'])
            for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
        if coverage!={site['instruction'] for site in sites}:raise ValueError('Original 1DB800 site coverage incomplete')
        report.update(original_sites=len(sites),covered_sites=len(coverage),original_returns=normal,original_provider_prefixes=len(rows)-normal)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(rows,separators=(',',':')));report['compilers']=[]
        for compiler in a.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                *('-I'+str(ROOT/tree) for tree in ('include','src','tests')),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True,timeout=600)
            if built.returncode:raise ValueError('MapWorld query compile failed: '+built.stderr[-7000:])
            for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                path=Path(item).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200)
            if result.returncode:raise ValueError('MapWorld query comparison failed: '+(result.stdout+result.stderr)[-9000:])
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT/FRAME).read_text()
                controls=(
                    ('increment-instead-of-dec','s_.flags.increment(ecx,4,true)','s_.flags.increment(ecx,4,false)'),
                    ('wrong-first-byte','(second?0x1bu:0x1au)','(second?0x1bu:0x1bu)'),
                    ('wrong-ret-cleanup','s_.gp[4]+=4u;return','s_.gp[4]+=8u;return'),
                    ('drop-negative-check','if(s_.flags.cf||s_.flags.zf)','if(s_.flags.zf)'),
                    ('skip-clear-flag-constant-read','s_.xmm[0]=scalar(0x1db847u,0x46a0d0u);',
                     'if(s_.flags.zf){s_.xmm[0]=s_.xmm[1];return;}s_.xmm[0]=scalar(0x1db847u,0x46a0d0u);'),
                )
                for label,old,new in controls:
                    if original.count(old)!=1:raise ValueError('Missing exact MapWorld defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/FRAME;target.parent.mkdir(parents=True)
                    target.write_text(original.replace(old,new,1));changed=list(command);mutant=overlay/'bad'
                    changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(mutant)
                    built=subprocess.run(changed,capture_output=True,text=True,timeout=600)
                    if built.returncode:raise ValueError('MapWorld defect compile failed '+label+': '+built.stderr[-6000:])
                    tested=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=1200)
                    diagnostic=(tested.stdout+tested.stderr)[-6000:]
                    if not tested.returncode or 'differ' not in diagnostic:
                        raise ValueError('MapWorld defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,diagnostic=diagnostic))
            report['compilers'].append(dict(compiler=compiler,command=command,result=json.loads(result.stdout),compiled_mutations=mutations))
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name) for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
