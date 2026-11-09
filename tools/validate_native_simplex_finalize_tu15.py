#!/usr/bin/env python3
"""Private30E930 normalization/cache/epilogue; real estimate service retained."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from validate_native_simplex_direction_tu15 import BODIES,ROOT,original_bodies,sha

def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    from research_mp_sandbox_update_functions import Xbe
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30f5d7);rows=[];requests=[];traces=[];all_sites=set()
    for line in block.splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);all_sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.pop()'in line or pc==0x30f612:
            traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True))
    base,size=0x61000000,0x4000;work=base+0x100;stack=base+0x3000
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    finite=lambda x:struct.unpack('<I',struct.pack('<f',x))[0];counts=(0,1,2,3,4,0xffffffff,0x80000000)
    for k in range(128):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack;gp[5]=stack+0x80+k%16;gp[6]=work;gp[7]=work+4;gp[3]=work+0x300
        for offset in (0x20,0xa0,0x300):
            for lane in range(4):put(work+offset+lane*4,finite((1+rng.randrange(31))/8)if k<64 else edges[(k+offset//16+lane*3)%len(edges)])
        put(work,counts[k%7]);put(work+4,counts[(k//7)%7])
        plane,cache=base+0x1000,base+0x1200
        if k%8==1:plane=stack
        if k%8==2:cache=stack
        if k%8==3:cache=stack+0x20
        if k%8==4:cache=work+0x3c
        if k%8==5:plane=work+0x20
        put(gp[5]+0x18,plane);put(gp[5]+0x14,cache);put(gp[5],0x3456789a);put(gp[5]+4,0x63000000)
        put(stack,0x63000000);put(stack+4,0x12345678);put(stack+8,0x23456789);put(stack+0x20,0xaabbccdd^k)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11)
        mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,8)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'finalize-{k}',entry=0x30f5d7,gp=gp,eflags=flags,matrix=0x12345678,source=0x23456789,
            before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=300,banks=[dict(address=base,before=bank.hex())]))
        requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30e930,),regions=((base,size),),trace_calls=traces,trace_extended=True)
    inputs,expected,actual=(scratch/name for name in ('finalize.in','finalize.expected','finalize.actual'));segments=[];packets=0;executed=set();vector_ops={}
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original finalization did not return with its real FP bank: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);segments.append((out.tell(),name,'normal_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            out.write(bytes.fromhex(case['banks'][0]['after']));events=[]
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before']);pc=event['instruction'];address=event['address']
                if len(raw)==16:
                    instruction=code.at(pc)
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=b'\x0f\x29':raise ValueError('Finalization vector-store opcode drift')
                    vector_ops[f'{pc:x}']=instruction.bytes.hex();events.extend((pc,address+at,before[at:at+8],raw[at:at+8])for at in (0,8))
                else:events.append((pc,address,before,raw))
            out.write(struct.pack('<I',len(events)))
            for pc,address,before,raw in events:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            estimate=[p for p in case['calls']if p['entry']==0x30f612]
            if len(estimate)!=1:raise ValueError('Original estimate instruction count differs')
            out.write(struct.pack('<3I',0x30f6dd,1,estimate[0]['xmm'][1][0]));out.write(struct.pack('<I',len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
            executed.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native physical finalization failed: '+r.stderr.decode()[-5000:])
    a=expected.read_bytes();b=actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 finalization differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=all_sites:raise ValueError('Physical finalization coverage incomplete: '+','.join(f'{p:x}'for p in sorted(all_sites-executed)))
    return dict(cases=len(rows),source_sites=len(executed),full_gp_flags_xmm_fp_live80_pre_io_packets=packets,
        complete_normal_abi_banks_store_operands=True,real_estimate_inputs_verified=len(rows),fixture_estimate_owner='Independent host-CPU RSQRT policy; no supplied output',
        production_journal_function_preserved_but_not_exercised=True,physical_vector_opcodes=vector_ops,runtime_vector_protocol=[8,8],physical_fault_atomicity=False,
        comparison_sha256=hashlib.sha256(b).hexdigest(),original=proof,declared_midowner_continuations=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():print('Authenticated TU1.5 XBE is not configured');return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_finalize_kernel.hpp','src/physics/runtime/native_tu15_simplex_finalize_abi.hpp','tests/native_tu15_simplex_finalize_tests.cpp',str(Path(__file__).relative_to(ROOT)),'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_simplex_direction_abi.hpp','include/halo2/native_tu15_simplex_direction_kernel.hpp']
    pins={name:sha(ROOT/name)for name in paths}
    try:
        bodies=original_bodies();world=next(iter(bodies.values()));contact=bodies['src/physics/runtime/physics_contact_runtime_generated.cpp']
        block=world[world.index('L30bae7:'):world.index('throw std::runtime_error("Original solver fell')]
        original_contact=contact[contact.index('L30bae7:'):contact.index('throw std::runtime_error("Original solver fell')]
        canonical=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)',r'\1',block)
        if canonical!=original_contact:raise ValueError('World/contact finalization numerical body differs')
        actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',block)
        helper_pins={'physics_world_runtime_generated.cpp':('ObservedWorldState','7c93aac13aa7930e203cbe9cd111ec6a112037e49cf659f8fd6781ba20792ec2'),
            'physics_contact_runtime_generated.cpp':('State','33f4a0a09fe0f539aa501f17ce0ed65dccd1294a8315b9c4bed317da890ffa7b')}
        helpers=[]
        for name,(state_type,pin)in helper_pins.items():
            text=(ROOT/'src/physics/runtime'/name).read_text();match=re.search(r'std::uint32_t source_estimate\('+state_type+r' &s,std::uint32_t input\) \{.*?\n\}',text,re.S)
            if not match or hashlib.sha256(match[0].encode()).hexdigest()!=pin:raise ValueError('Complete source estimate owner wrapper drift: '+name)
            helpers.append(match[0].replace(state_type+' &s','WorldState &s',1))
        if helpers[0]!=helpers[1]:raise ValueError('World/contact estimate owner policy differs')
        helper=helpers[0]+'\n'
        source=helper+'static void run_stage(WorldState&s){\n'+actual+'}\n';(scratch/'simplex_finalize_reference.inc').write_text(source)
        report['reference']=dict(complete_parent_body_sha256=BODIES,stage_sha256=hashlib.sha256(source.encode()).hexdigest(),source_sites=78,real_estimate_callback_preserved=True,declared_midowner_continuation=True)
        report['reference']['estimate_owner_helper_sha256']={name:pin for name,(state,pin)in helper_pins.items()}
        binary=scratch/'finalize';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_finalize_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise RuntimeError('Finalization fixture compile failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise RuntimeError('Finalization fixture failed:\n'+report['diagnostic'])
        if args.xbe:report['original_hardware']=hardware(args,scratch,binary,actual)
        text=(ROOT/'include/halo2/native_tu15_simplex_finalize_kernel.hpp').read_text();report['semantic_mutants']={}
        changes={
            'wrong_newton_half':('0x3f000000u','0x3f800000u'),
            'truncate_live_bound':('s.bound=read(P::first_bound,s.work);','s.bound=(read(P::first_bound,s.work)&255u);'),
            'bypass_estimate_owner':('memory.estimate(s,L::low(s.length))','0x3f800000u'),
            'stale_distance_word':('read(P::distance_reload,s.constant)','L::low(s.distance)'),
        }
        for name,(needle,replacement)in changes.items():
            if text.count(needle)!=1:raise ValueError('Finalization mutant seam drift: '+name)
            directory=scratch/name;(directory/'halo2').mkdir(parents=True);variant=text.replace(needle,replacement);(directory/'halo2/native_tu15_simplex_finalize_kernel.hpp').write_text(variant)
            changed=command.copy();at=changed.index('-I'+str(ROOT/'include'));changed[at:at]=['-I'+str(directory)];changed[-1]=str(directory/'mutant')
            built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise ValueError('Finalization mutant compile failed: '+built.stderr[-5000:])
            rejected=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
            if rejected.returncode==0 or'Native simplex finalization mismatch'not in rejected.stderr:raise ValueError('Finalization mutant not rejected: '+name+'\n'+rejected.stderr[-5000:])
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=rejected.stderr[-3000:])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1]);original_bodies();report['sources_unchanged']=pins=={name:sha(ROOT/name)for name in paths}
        if not report['sources_unchanged']:raise ValueError('Finalization proof inputs changed')
        report.update(passed=True,original_hardware_verified=bool(args.xbe),physical_fault_atomicity_verified=False,whole_parent_closed=False)
    except Exception as error:report['error']=str(error);raise
    finally:report['source_sha256']=pins;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['diagnostic'].strip());return 0
if __name__=='__main__':raise SystemExit(main())
