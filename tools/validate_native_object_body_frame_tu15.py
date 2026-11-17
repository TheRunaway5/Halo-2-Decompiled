#!/usr/bin/env python3
"""Private salted object-body lookup against unchanged physical TU1.5 i386."""
import argparse,copy,hashlib,json,os,platform,random,re,shlex,struct,subprocess
from pathlib import Path
from validate_native_tu15_prt_helpers import physical_original,PIN
ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x4ec000,4096),(0x61000000,8192),(0x62000000,4096))
INPUTS=('include/halo2/native_tu15_object_body_kernel.hpp','src/gameplay/objects/native_tu15_object_body_frame.hpp','tests/native_tu15_object_body_frame_tests.cpp','tools/validate_native_object_body_frame_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
INPUTS+=('tests/native_tu15_object_body_push_publication_tests.cpp',)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus():
    rng=random.Random(0xbcdf0);rows=[]
    for k in range(128):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(address,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Object-body input escaped bank')
        def word(address):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-4:return int.from_bytes(bank[address-base:address-base+4],'little')
            raise ValueError('Object-body stack escaped bank')
        gp=[rng.getrandbits(32)for _ in range(8)];sp=0x62000800+k%16;mode=k%8;alias=k//32
        index=(0,1,31,65535)[k//8%4];salt=0 if mode==3 else(1,0x7fff,0x8000,0xffff)[k//16%4];datum=(salt<<16)|index
        stride=(0,1,12,0xfffffff0)[k//8%4];record=sp-8 if alias==1 else sp-4 if alias==2 else 0x61000400+k%16
        objects=sp-0x4c if alias==3 else 0x61000100;rows_base=(record-index*stride)&0xffffffff
        kind=(0,1,7,31,32,63,127,255)[k//8%8];mask=(1<<(kind&31))if mode in(6,7)else 0
        gp[0]=0xffffffff if mode==0 else datum;gp[4]=sp;gp[7]=rows_base if alias==3 else(kind<<24)|salt
        put(0x4ecbd0,objects);put(objects+0x38,0xffffffff if mode==1 else index if mode==2 else index+1)
        put(objects+0x24,stride);put(objects+0x44,rows_base)
        put(record,0 if mode==3 else(salt^1)if mode==4 else salt,2);put(record+3,kind,1);put(record+8,rng.getrandbits(32))
        put(sp,0x63000000);put(sp+4,mask)
        depth=(0,1,4,8)[k%4];values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','0000000000000080ffbf')
        rows.append(dict(id=k,name=f'object-body-{k}-mode{mode}-alias{alias}',entry=0xbcdf0,gp=gp,
            eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10),0x4100|((k%8)<<11),0x1f80|((k%4)<<13)|(k&0x3f)],
            live_x87=[values[n%4]for n in range(depth)],matrix=word(sp+4),source=word(sp+8),max_steps=60,
            banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--source-publication',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    pins={name:sha(ROOT/name)for name in INPUTS};headers={path.resolve():sha(path)for tree in('src','include')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original image')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from oracle_smg_firing_1_1 import Xbe
        original_metadata=common.metadata
        def metadata(image):
            result=original_metadata(image);code=common._Code(image,'.text')
            for site in result['sites']:
                if site['instruction']==0xbce3d:
                    instruction=code.at(0xbce3d)
                    if bytes(instruction.bytes)!=bytes.fromhex('8554240c') or instruction.mnemonic!='test':raise ValueError('Original type-mask TEST opcode drift')
                    if site['operands']!=[dict(base=4,index=-1,scale=1,displacement=12,bytes=4,read=False,write=False)]:raise ValueError('Capstone type-mask TEST operand metadata changed')
                    # Capstone reports no access flags for this genuine TEST.
                    # Its exact unchanged opcode owns a read, never a store.
                    site['operands'][0]['read']=True
                if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;common.ENTRIES=(0xbcdf0,);image=Xbe(a.xbe);meta=metadata(image)
        sites={site['instruction']for site in meta['sites']};traces=[dict(entry=site['instruction'],arguments=0,gp=True,xmm=True)for site in meta['sites']if site['operands']or common._Code(image,'.text').at(site['instruction']).mnemonic=='push']
        if len(sites)!=35:raise ValueError('Original whole lookup extent changed')
        rows=corpus();observed,proof=physical_original(image,rows,scratch,entries=(0xbcdf0,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        report['authority']=proof;report['explicit_test_read_metadata']={'instruction':'bce3d','original_bytes':'8554240c','bytes':4,'read':True,'write':False}
        normal_count=len(rows)
        if a.prefixes:
            prefix=[]
            for k,row in enumerate(rows):
                if k%32>=16:continue
                for phase,key in (('before_read','reads'),('before_write','writes')):
                    for ordinal in range(len(observed['cases'][k][key])):
                        candidate=copy.deepcopy(row);candidate.update(id=len(prefix),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefix.append(candidate)
            folder=scratch/'prefixes';folder.mkdir()
            stopped,prefix_proof=physical_original(image,prefix,folder,entries=(0xbcdf0,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            rows+=prefix;observed['cases']+=stopped['cases'];report['prefix_authority']=prefix_proof
        covered=set();packet_count=0
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual['completed']!=('fault'not in row):raise ValueError('Original lookup completion differs '+row['name'])
            covered.update(actual['executed_sites']);calls=actual['calls']
            for packet in calls:
                if packet.pop('arguments')!=[]:raise ValueError('Unexpected child call')
            pc=actual['boundary']['instruction']if 'fault'in row else actual['executed_sites'][-1]
            row.update(expected=dict(entry=pc,gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],xmm=actual['boundary_xmm'],fp=actual['after_fp'],live_x87=actual.get('boundary_live_x87',row['live_x87'])),reads=actual['reads'],writes=actual['writes'],calls=calls)
            for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
            packet_count+=len(calls)
        if sites!=covered:raise ValueError('Original lookup coverage incomplete: '+str(sorted(sites-covered)))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_object_body_frame_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            compiled=subprocess.run(command,capture_output=True,text=True)
            if compiled.returncode:raise ValueError('Lookup compile failed: '+compiled.stderr[-7000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
            if done.returncode:raise ValueError('Lookup original comparison failed: '+(done.stdout+done.stderr)[-10000:])
            publication=None
            if a.source_publication:
                source_test=str(ROOT/'tests/native_tu15_object_body_push_publication_tests.cpp');publication_binary=scratch/(Path(compiler).name+'-publication')
                source_command=[source_test if item==str(ROOT/'tests/native_tu15_object_body_frame_tests.cpp')else item for item in command]
                source_command[source_command.index('-o')+1]=str(publication_binary)
                checked=subprocess.run(source_command,capture_output=True,text=True)
                if checked.returncode:raise ValueError('Source PUSH publication compile failed: '+checked.stderr[-6000:])
                checked=subprocess.run([str(publication_binary)],capture_output=True,text=True)
                if checked.returncode:raise ValueError('Source PUSH publication differs: '+(checked.stdout+checked.stderr)[-6000:])
                publication=dict(command=source_command,result=json.loads(checked.stdout),scope='Actual shared retained two-PUSH transport versus private source-publication view; not whole owner/source binding')
            mutations=[]
            if a.semantic_mutants:
                relative='src/gameplay/objects/native_tu15_object_body_frame.hpp';original=(ROOT/relative).read_text()
                definitions=(
                    ('unsigned-limit','return s_.flags.sf!=s_.flags.of;','return s_.gp[1]<limit;'),
                    ('unsigned-salt','std::int16_t(salt)','std::uint16_t(salt)'),
                    ('live-zero-salt','return salt!=0;','return true;'),
                    ('narrow-type-shift','s_.reg(1,1,0)','(s_.reg(1,1,0)&15u)'),
                    ('wrong-body-field','read(0xbce43u,g[7]+8u)','read(0xbce43u,g[7]+4u)'),
                    ('swapped-saved-registers','g[7]=pop(0xbce46u);g[6]=pop(0xbce47u);','g[6]=pop(0xbce46u);g[7]=pop(0xbce47u);'))
                for label,old,new in definitions:
                    if original.count(old)!=1:raise ValueError('Missing object-body defect witness '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(original.replace(old,new,1));mutant=overlay/'probe'
                    changed=list(command);changed[1:1]=['-I'+str(overlay/'src')];changed[changed.index('-o')+1]=str(mutant)
                    built=subprocess.run(changed,capture_output=True,text=True)
                    if built.returncode:raise ValueError('Object-body defect compile failed '+label+': '+built.stderr[-5000:])
                    tested=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=(tested.stdout+tested.stderr)[-6000:]
                    if tested.returncode==0 or not any(text in diagnostic for text in ('differs','Unbacked object-body','Object-body bank extent')):raise ValueError('Object-body defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,exit_code=tested.returncode,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),source_publication=publication,compiled_mutations=mutations))
        report.update(passed=True,physical_paths=len(rows),normal_returns=normal_count,stopped_prefixes=len(rows)-normal_count,source_sites=len(sites),full_original_pre_io_packets=packet_count,authority=proof,fixture_sha256=sha(fixture),compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
