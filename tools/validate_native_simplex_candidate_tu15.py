#!/usr/bin/env python3
"""Private candidate-improvement continuation; no whole30E930/default claim."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_simplex_direction_tu15 import BODIES,ROOT,original_bodies,sha

def hardware(args,scratch,binary,blocks):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    image=Xbe(args.xbe);rng=random.Random(0x30eeb1);rows=[];requests=[];traces=[];all_sites=set();owner_sites=[]
    for block in blocks:
        sites=set()
        for line in block.splitlines():
            match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
            if not match:continue
            pc=int(match[1],16);sites.add(pc);all_sites.add(pc)
            if re.search(r'\b[rvw]\(s.memory',line):
                delta=1 if('s.fp.st(0)=mul('in line or's.fp.comparison('in line)else 0
                traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta))
        owner_sites.append(sites)
    base,size=0x61000000,0x4000;entries=(0x30eeb1,0x30f315,0x30f491)
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    finite=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
    counts=[0,1,2,3,4,0xffffffff,0x80000000,0x10000000]
    for k in range(96):
        for side in range(3):
            bank=bytearray(rng.randbytes(size));work=base+0x100;sp=base+0x3000
            def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
            for offset in range(0x10,0x400,4):put(work+offset,finite((rng.randrange(33)-16)/4)if k<48 else edges[(k+offset//4)%len(edges)])
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[6]=work;gp[7]=work+4;gp[3]=work+0x300
            if side==1 and k>=64:gp[7]=sp+(0x34,0x38,0x3c)[k%3]
            for n in range(4):put(work+0x300+n*4,finite(1.0 if n==0 else 0.0));put(sp+0x70+n*4,finite(1.0 if n==0 else 0.0))
            put(gp[7]if side==1 else work,counts[k%len(counts)])
            put(sp+0x10,finite((0.,.125,1.,8.,-1.)[k%5])if k<48 else edges[(k*3)%len(edges)])
            if k<32:
                # Declared independent geometry exercises each real score
                # predicate. Every result remains computed by original/native
                # arithmetic; no branch outcome or child return is supplied.
                point=0xe0 if side==1 else 0x20
                for index in range(3):
                    for lane in range(4):put(work+point+index*16+lane*4,finite(float(index)if lane==0 else 0.))
                candidate=work+(((counts[k%len(counts)]+(14 if side==1 else 2))&0xffffffff)<<4&0xffffffff)
                for lane in range(4):put(candidate+lane*4,finite(4. if lane==0 else 0.))
                group=k//8;put(sp+0x10,finite(32. if group==1 else .125))
                if group==2:put(work+point+(0x20 if counts[k%len(counts)]==3 else 0x10),finite(8.))
                if group==3:put(work+point+0x10,finite(8.))
            put(sp,0x63000000);put(sp+4,0x12345678);put(sp+8,0x23456789)
            flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11)
            mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,7)[k%4]
            values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            rows.append(dict(id=len(rows),name=f'candidate-{side}-{k}',entry=entries[side],gp=gp,eflags=flags,matrix=0x12345678,source=0x23456789,
                before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=200,
                stop_before_entries=[0x30ea00,0x30f274,0x30f40b,0x30f458,0x30f5d7],banks=[dict(address=base,before=bank.hex())]))
            requests.append((side,gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30e930,),regions=((base,size),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('candidate.in','candidate.expected','candidate.actual'));segments=[];packets=0;executed=set();shift_counterexamples={}
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            side,gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original candidate failed its declared normal FP boundary: '+name)
            stopped=case['stopped_at'];allowed=({0x30ea00,0x30f274},{0x30f40b,0x30f458},{0x30ea00,0x30f5d7})[side]
            if stopped not in allowed:raise ValueError('Original candidate escaped its stage continuation: '+name)
            inp.write(struct.pack('<14I',side,*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);segments.append((out.tell(),name,'normal_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            out.write(bytes.fromhex(case['banks'][0]['after']));out.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);out.write(struct.pack('<III',event['instruction'],event['address'],len(raw)));out.write(bytes.fromhex(event['before']));out.write(raw)
            visited=set(case['executed_sites']);executed.update(visited);last=max(visited&owner_sites[side]);accepted=stopped in{0x30ea00,0x30f40b}
            out.write(struct.pack('<3I',last,accepted,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));
                emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
                if packet['entry']in{0x30eec0,0x30f329,0x30f4a0}and packet['eflags']&0x10 and side not in shift_counterexamples:
                    shift_counterexamples[side]=dict(name=name,declared_entry=f'{entries[side]:x}',initial_gp=gp,initial_full_flags=flags,
                        original_candidate_load_packet=packet,modeled_shift_AF=0,physical_shift_AF=1,
                        no_flag_masking=True,scope='Actual immediateSHL before this original operand; original Flags.shift prefix differs')
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native physical candidate failed: '+r.stderr.decode()[-5000:])
    a=expected.read_bytes();b=actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 candidate differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=all_sites:raise ValueError(f'Physical candidate source coverage incomplete: {len(executed)}/{len(all_sites)},missing='+','.join(f'{pc:x}'for pc in sorted(all_sites-executed)))
    return dict(cases=len(rows),source_sites=len(executed),complete_pre_io_gp_flags_xmm_fp_live80_packets=packets,complete_normal_abi_banks_ordered_stores=True,
        original=proof,comparison_sha256=hashlib.sha256(b).hexdigest(),declared_midowner_continuations=True,physical_memory_fault_atomicity=False,
        original_immediate_shift_AF_counterexamples=list(shift_counterexamples.values()))

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():print('Authenticated TU1.5 XBE is not configured');return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_candidate_kernel.hpp','src/physics/runtime/native_tu15_simplex_candidate_abi.hpp','tests/native_tu15_simplex_candidate_tests.cpp',str(Path(__file__).relative_to(ROOT)),'src/physics/runtime/native_tu15_raw_x87_scope.hpp','include/halo2/native_tu15_simplex_direction_kernel.hpp','src/physics/runtime/native_tu15_halfword_bit_query.hpp','src/physics/runtime/native_tu15_simplex_direction_abi.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp']
    pins={name:sha(ROOT/name)for name in paths}
    try:
        bodies=original_bodies();body=next(iter(bodies.values()));contact=bodies['src/physics/runtime/physics_contact_runtime_generated.cpp']
        ranges=[('0x30b3c1u','L30b4b5:'),('0x30b825u','L30b91b:'),('0x30b9a1u','L30ba91:')];blocks=[]
        for marker,end in ranges:
            start=body.rfind('\n',0,body.index('world_observation_pc('+marker))+1;block=body[start:body.index(end)]
            contact_start=contact.rfind('\n',0,contact.index('s.services.instruction('+marker))+1
            contact_block=contact[contact_start:contact.index(end)]
            canonical=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)',r'\1',block)
            # World adds its explicit active-MMX guard before each x87
            # operation. Contact has no MMX alias API. Native Scope admits
            # neither active-MMX nor sparse topology; keep the real World
            # guards in the compiled reference and compare numerical spelling
            # only after authenticating their exact count and placement.
            if canonical.count('s.require_x87();')!=12 or's.require_x87();'in contact_block:
                raise ValueError('World/contact candidate x87 ownership guard drift')
            if canonical.replace('s.require_x87();','')!=contact_block:raise ValueError('World/contact candidate numerical body differs')
            block=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',block);blocks.append(block)
        blocks[0]=blocks[0].replace('goto L30af10;','return true;')+'\nL30b784:return false;\n'
        blocks[1]+='\nL30b91b:return true;\nL30b968:return false;\n'
        blocks[2]=blocks[2].replace('goto L30af10;','return true;')+'\nL30bae7:return false;\n'
        source='\n'.join('static bool run_'+str(n)+'(WorldState&s){\n'+block+'}\n'for n,block in enumerate(blocks))
        source+='static bool run_stage(WorldState&s,unsigned side){return side==0?run_0(s):side==1?run_1(s):run_2(s);}\n'
        (scratch/'simplex_candidate_reference.inc').write_text(source)
        needle='s.flags.shift(s.reg(0,4,0),0x4u,4)'
        if source.count(needle)!=3:raise ValueError('Candidate original immediate-SHL seam drift')
        ordered=source.replace(needle,'halo2::native_simplex_candidate_detail::shift4(s,s.reg(0,4,0))')
        (scratch/'simplex_candidate_ordered_reference.inc').write_text(ordered)
        report['reference']=dict(whole_parent_body_sha256=BODIES,stage_sha256=hashlib.sha256(source.encode()).hexdigest(),sites=len(re.findall(r'services.instruction\(',source)),declared_midowner_continuations=True)
        report['reference'].update(world_contact_numerical_equality_under_no_mmx_admission=True,world_x87_guard_sites=36,world_guards_preserved_in_reference=True)
        report['reference'].update(ordered_shift_prefix_sha256=hashlib.sha256(ordered.encode()).hexdigest(),
            normal_authority='Unmodified pinned translated source',
            fault_prefix_authority='Same pinned source with only immediateSHL flags computed by native hardware; unchangedi386packets independently required',
            unmodified_modeled_full_fault_flags_verified=False,modeled_reference_gap='AF cleared by translated Flags.shift; native immediateSHL preserves actual hardware output; no bits masked')
        binary=scratch/'candidate';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_candidate_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise RuntimeError('Candidate fixture compile failed:\n'+result.stderr[-12000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise RuntimeError('Candidate fixture failed:\n'+report['diagnostic'])
        if args.xbe:report['original_hardware']=hardware(args,scratch,binary,blocks)
        kernel=ROOT/'include/halo2/native_tu15_simplex_candidate_kernel.hpp';text=kernel.read_text();report['semantic_mutants']={}
        changes={
            'wrong_initial_relation':('project(0u,s.first)&0x4100u','project(0u,s.first)&0x100u'),
            'skip_third_vertex':('if(s.count==3u){','if(false){'),
            'reversed_projection':('N::subtract(s.candidate,point)','N::subtract(point,s.candidate)'),
        }
        for name,(needle,replacement)in changes.items():
            if text.count(needle)!=1:raise ValueError('Candidate mutant seam drift: '+name)
            directory=scratch/name;(directory/'halo2').mkdir(parents=True);variant=text.replace(needle,replacement);(directory/'halo2/native_tu15_simplex_candidate_kernel.hpp').write_text(variant)
            changed=command.copy();at=changed.index('-I'+str(ROOT/'include'));changed[at:at]=['-I'+str(directory)];changed[-1]=str(directory/'mutant')
            built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise ValueError('Candidate mutant compile failed: '+built.stderr[-5000:])
            rejected=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
            if rejected.returncode==0 or'Native simplex candidate mismatch'not in rejected.stderr:raise ValueError('Candidate semantic mutant not rejected: '+name+'\n'+rejected.stderr[-5000:])
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=rejected.stderr[-3000:])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1]);report['sources_unchanged']=pins=={name:sha(ROOT/name)for name in paths};original_bodies()
        if not report['sources_unchanged']:raise ValueError('Candidate proof inputs changed')
        report.update(passed=True,original_hardware_verified=bool(args.xbe),full_parent_closed=False,physical_fault_atomicity_verified=False)
    except Exception as error:report['error']=str(error);raise
    finally:report['source_sha256']=pins;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['diagnostic'].strip());return 0
if __name__=='__main__':raise SystemExit(main())
