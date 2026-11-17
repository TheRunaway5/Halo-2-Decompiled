#!/usr/bin/env python3
"""Private complete packed quaternion decoder against unchanged TU1.5 i386."""
import argparse,copy,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from native_packed_rotation_original import physical_original_rotation
REGIONS=((0x510000,8192),(0x61000000,8192),(0x62000000,8192))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(smoke):
    rng=random.Random(0x296180);rows=[]
    from validate_native_object_parent_mode_tu15 import corpus as parent
    live=parent('TitleUpdate15',True)[3]['live_x87']
    patterns=((0,0,0,0),(1,0,0,0),(-32768,32767,-1,1),(1,2,3,4),(32767,32767,32767,32767),(-1,-2,-3,-4))
    for case in range(16 if smoke else 192):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];sp=0x62000100+case%16;packed=0x61000100+case%16;output=0x61000300+(case%16)*16
        if case%16==0:output=sp-16
        index=(0,1,7,65535)[case%4];base=(packed-0x20-index*8)&0xffffffff
        def put(address,value,width=4):
            for(region,size),bank in zip(REGIONS,banks):
                if region<=address<=region+size-width:bank[address-region:address-region+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Packed rotation fixture escaped declared banks')
        put(0x510f94,index);put(0x510f60,base);put(0x510fa0,output);put(sp,0x63000000);put(sp+4,0);put(sp+8,0)
        quantized=patterns[case%len(patterns)]if case<len(patterns)*4 else tuple(rng.randint(-32768,32767)for _ in range(4))
        for n,value in enumerate(quantized):put(packed+n*2,value,2)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;profile=case//4;depth=(0,1,4,8)[profile%4]
        cw=(0x7f,0x27f,0x37f)[profile//16%3]|((profile//4%4)<<10)
        sw=((case%8)<<11)|(case%8<<8)|((case&1)<<14)|(case&0x3f)
        mxcsr=0x1f80|(((profile//4+case)%4)<<13)|((case*17)&0x3f)|(0,0x40,0x8000,0x8040)[case//4%4]
        rows.append(dict(id=case,name=f'packed-{case}',entry=0x296180,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,sw,mxcsr],live_x87=live[:depth],matrix=0,source=0,max_steps=200,banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--smoke',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--mmx',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT);args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=('include/halo2/native_tu15_packed_rotation_kernel.hpp','src/physics/runtime/native_tu15_packed_rotation_frame.hpp','tests/native_tu15_packed_rotation_tests.cpp','tools/native_packed_rotation_original.py','tools/validate_native_packed_rotation_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/native_mopp_ray_dense_original.py','tools/native_sphere_frustum_mmx_original.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/oracle_native_campaign_screen_lifecycle_tu15.py')
    pins={name:sha(ROOT/name)for name in files};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_unbound=True,retained_vector_half_atomicity_qualified=False,opaque_input_requested=args.mmx,intermediate_mmx_qualified=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        assert sha(args.xbe)==NEW_PIN;image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original=common.metadata;common.ENTRIES=(0x296180,)
        def metadata(source):
            result=original(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];io=[site['instruction']for site in sites if site['operands']]+[0x2961db];rows=corpus(args.smoke)
        normal_count=len(rows)
        if args.mmx:
            rng=random.Random(0x296180a11)
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal_count]);row.update(id=len(rows),name=f'packed-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
        opaque_count=len(rows)-normal_count
        for offset in range(1,16):
            row=copy.deepcopy(rows[offset%normal_count]);row.update(id=len(rows),name=f'packed-alignment-{offset}',phase='alignment_fault',alignment_fault=True)
            bank=next(bank for bank in row['banks']if bank['address']==0x510000);data=bytearray.fromhex(bank['before']);struct.pack_into('<I',data,0xfa0,0x61000300+offset);bank['before']=data.hex();rows.append(row)
        regions=REGIONS+(((0x64000000,4096),)if args.mmx else())
        actual,proof=physical_original_rotation(image,rows,scratch,entries=(0x296180,),regions=regions,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,opaque_input=args.mmx)
        if args.prefixes:
            prefixes=[]
            indices=list(range(min(8,normal_count)))+([normal_count+n for n in(0,1,7,15)if n<opaque_count]if args.mmx else[])
            for index in indices:
                observed=actual['cases'][index]
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(observed[key]):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}{ordinal}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        if phase=='after_write'and event['instruction']==0x2961e1:row['native_observer_ordinal']=ordinal+1
                        prefixes.append(row)
            target=scratch/'prefixes';target.mkdir();stopped,prefix_proof=physical_original_rotation(image,prefixes,target,entries=(0x296180,),regions=regions,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,opaque_input=args.mmx)
            rows+=prefixes;actual['cases']+=stopped['cases'];proof['physical_prefixes']=prefix_proof
        coverage=set()
        for row,observed in zip(rows,actual['cases'],strict=True):
            assert observed['completed']==('phase'not in row);coverage.update(observed['executed_sites'])
            pc=observed['boundary']['instruction']if'phase'in row else 0x2961e7
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed['boundary_live_x87'])
            if'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            if row.get('alignment_fault'):row['execution_flags_before_fault']=observed['calls'][-1]['eflags']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for packet in row['calls']:assert packet.pop('arguments')==[]
            for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_packed_rotation_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-7000:]
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert done.returncode==0,dict(exit_code=done.returncode,diagnostic=(done.stdout+done.stderr)[-10000:])
            mutations=[]
            if args.semantic_mutants:
                kernel='include/halo2/native_tu15_packed_rotation_kernel.hpp';frame='src/physics/runtime/native_tu15_packed_rotation_frame.hpp'
                definitions=(('wrong-norm-pair',kernel,'const auto opposite=view.opposite(squared);','const auto opposite=view.alternate(squared);'),('missing-source-emms',frame,'s_.emms();s_.fp.top=0;','(void)s_;'),('lost-converted-high-upper-pair',frame,'converted_high_=_mm_movelh_ps(_mm_movehl_ps(converted,converted),_mm_movehl_ps(s_.xmm[2],s_.xmm[2]));','converted_high_=_mm_setzero_ps();'),('missing-movaps-alignment',frame,'if(output_address&15u){','if(false){'))
                for name,relative,old,new in definitions:
                    source=(ROOT/relative).read_text()
                    if source.count(old)!=1:raise ValueError('Missing packed rotation defect witness '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));mutant=overlay/'test'
                    mutated=list(command);mutated[1:1]=['-I'+str(overlay/'include'),'-I'+str(overlay/'src')];mutated[mutated.index('-o')+1]=str(mutant)
                    checked=subprocess.run(mutated,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-5000:]
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                    if checked.returncode==0 or not any(message in diagnostic for message in('packed rotation full returned/stopped ABI differs','packed rotation completion differs','packed rotation original pre-I/O packets differ','Retained vector halves changed CPU ABI')):raise ValueError('Packed rotation defect survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,physical_paths=len(rows),original_normal_paths=normal_count,original_opaque_paths=opaque_count,actual_alignment_faults=15,source_sites=sorted(hex(pc)for pc in coverage),original=proof,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
