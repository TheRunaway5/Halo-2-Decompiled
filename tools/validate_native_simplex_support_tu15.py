#!/usr/bin/env python3
"""Private actual30E930 support-query/point-transform stage with real1F6610."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from validate_native_simplex_direction_tu15 import ROOT,BODIES,original_bodies,sha

RANGES=((0x30ee84,0x30eeae),(0x30f274,0x30f312),(0x30f458,0x30f48e),(0x30f40b,0x30f453))

def reference(scratch):
    body=next(iter(original_bodies().values()));blocks=[]
    for begin,end in RANGES:
        lines=[]
        for line in body.splitlines():
            found=re.search(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+)u,s\)',line)
            if found and begin<=int(found[1],16)<=end:lines.append(line)
        blocks.append('\n'.join(lines))
    sites=set()
    for block in blocks:
        sites.update(int(x,16)for x in re.findall(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+)u,s\)',block))
    if len(sites)!=103:raise ValueError('Support stage source coverage drift: '+str(len(sites)))
    fragments=[];unmodified=[]
    endings=('first_candidate','second_candidate','first_candidate','iteration')
    for n,(block,end)in enumerate(zip(blocks,endings,strict=True)):
        text=re.sub(r'^L[0-9a-f]+: ','',block,flags=re.M)
        text=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',text)
        text=text.replace('goto L30b784;','return N::second_support;').replace('goto L30b968;','return N::last_first;').replace('goto L30bae7;','return N::finalize;').replace('goto L30af10;','return N::iteration;')
        unmodified.append(f'static N stage_unmodified_{n}(WorldState&s){{\n{text}\nreturn N::{end};\n}}')
        text=text.replace('s.push(','atomic_push(s,')
        # Exact hardware SHL primitive only: original modeled undefined AF is
        # not authority for a following pre-I/O packet. Normal policy unchanged.
        text=text.replace('s.flags.shift(s.reg(0,4,0),0x4u,4)','native_simplex_candidate_detail::shift4(s,s.reg(0,4,0))').replace('s.flags.shift(s.reg(2,4,0),0x4u,4)','native_simplex_candidate_detail::shift4(s,s.reg(2,4,0))').replace('s.flags.shift(s.reg(1,4,0),0x4u,4)','native_simplex_candidate_detail::shift4(s,s.reg(1,4,0))')
        fragments.append(f'static N stage_{n}(WorldState&s){{\n{text}\nreturn N::{end};\n}}')
    source='''static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,address,{},{}};if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);s.memory.write(address,bytes);s.gp[4]=address;if(s.observe){event.written.assign(bytes.begin(),bytes.end());s.observe(event);}}
static std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){switch(p){case 0x418ae0u:return 0x461960u;case 0x30b3c1u:return 0x30eeb1u;case 0x30b825u:return 0x30f315u;case 0x30b9a1u:return 0x30f491u;default:throw std::runtime_error("Unknown support mapping");}}
static std::uint32_t sign_extend(std::uint32_t x,unsigned){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));}
'''+ '\n'.join(fragments+unmodified)+'''
static N run_stage_unmodified(WorldState&s,E entry){switch(entry){case E::first:return stage_unmodified_0(s);case E::second:return stage_unmodified_1(s);case E::last_first:return stage_unmodified_2(s);case E::transform_second:return stage_unmodified_3(s);}throw std::runtime_error("Invalid support entry");}
static N run_stage(WorldState&s,E entry){switch(entry){case E::first:return stage_0(s);case E::second:return stage_1(s);case E::last_first:return stage_2(s);case E::transform_second:return stage_3(s);}throw std::runtime_error("Invalid support entry");}
'''
    (scratch/'simplex_support_reference.inc').write_text(source)
    return dict(complete_parent_body_sha256=BODIES,parent_sites=len(sites),stage_sha256=hashlib.sha256(source.encode()).hexdigest(),child_owner='1f6610',child='Previously qualified complete native support owner, genuine original hardware continuation separately compared',prefix_reference='Unmodified whole-body-pinned normal oracle plus stopped-prefix oracle with three exact immediate SHL flag primitives and CPU-atomic PUSH write/commit/observer transport'),blocks,sites

def hardware(args,scratch,binary,blocks,all_sites):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    from research_mp_sandbox_update_functions import Xbe
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30f274);rows=[];requests=[];traces=[]
    for line in '\n'.join(blocks).splitlines():
        match=re.search(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+)u,s\)',line)
        if match and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line):traces.append(dict(entry=int(match[1],16),arguments=0,gp=True,xmm=True))
    traces.append(dict(entry=0x1f6610,arguments=0,gp=True,xmm=True))
    base,size,work,stack=0x61000000,0x8000,0x61000100,0x61007000
    ro=[]
    for address in (0x44a000,0x461000,0x462000):
        section=next(s for s in image.sections if s.va<=address and address+0x1000<=s.va+s.rsize)
        ro.append((address,image.data[section.raw+address-section.va:section.raw+address-section.va+0x1000]))
    if struct.unpack_from('<I',ro[2][1],0x100)[0]!=0x1f6610:raise ValueError('Genuine polygon support vtable+30 changed')
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    finite=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    entries=(0x30ee84,0x30f274,0x30f458,0x30f40b)
    for k in range(192):
        entry=k%4;profile=k//4
        work=stack-0x20-((k%3)+2)*16 if k%32 in (8,14) else stack+0x70-(((k//3)%3)+14)*16 if k%32==9 else base+0x100
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=(k//4)%4;gp[2]=work+0x100;gp[3]=work+0x120;gp[4]=stack;gp[5]=base+0x6000;gp[6]=work;gp[7]=work+4
        first,second,cache,matrix=base+0x1000,base+0x2000,base+0x4000,base+0x5000
        for object in (first,second,matrix,work):
            for row in range(20):
                for lane in range(4):put(object+row*16+lane*4,finite((1+rng.randrange(31))/8)if k<96 else edges[(k+row*3+lane*5)%len(edges)])
        put(first,0x4620d0);put(second,0x4620d0);put(first+0x14,4);put(second+0x14,4)
        put(gp[5]+8,first);put(gp[5]+12,second);put(gp[5]+16,matrix);put(gp[5]+20,cache);put(cache+8,0x04040000)
        put(work,k%3);put(work+4,(k//3)%3);put(work+0x10,0);put(stack+0x4c,0)
        if k%20==0:put(work,4)
        if k%20==1:put(work+4,4)
        if k%20==2:put(work+0x10,1)
        if k%20==5:put(work+0x10,0xffffffff)
        if k%20==4:put(work+0x10,1)
        if k%20==10:put(work,4)
        if k%20==6:put(stack+0x4c,1)
        put(stack,0x63000000);put(stack+4,0x12345678);put(stack+8,0x87654321)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((profile%4)<<10);status=0x4100|((profile%8)<<11)
        mxcsr=0x1f80|((profile%4)<<13)|(0x8040 if profile&4 else 0)|(profile&0x3f);depth=(0,1,4,8)[profile%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'support-{k}',entry=entries[entry],gp=gp,eflags=flags,matrix=0x12345678,source=0x87654321,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=1000,stop_before_entries=[0x30eeb1,0x30f315,0x30f491,0x30f5d7,0x30ea00]+([0x30f274]if entry==0 else[])+([0x30f458]if entry==1 else[]),banks=[dict(address=base,before=bank.hex())]+[dict(address=a,before=b.hex())for a,b in ro]))
        requests.append((entry,gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30e930,0x1f6610),regions=((base,size),)+tuple((a,0x1000)for a,b in ro),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('support.in','support.expected','support.actual'));segments=[];packets=0;executed=set();vector_ops={};child_entries=0
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            entry,gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original support did not reach its continuation: '+name+' '+str(case.get('fault')))
            inp.write(struct.pack('<14I',entry,*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank)
            for a,b in ro:inp.write(b)
            segments.append((out.tell(),name,'normal_abi'));emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for region in case['banks']:out.write(bytes.fromhex(region['after']))
            events=[]
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before']);pc=event['instruction'];address=event['address']
                if len(raw)==16 and pc in all_sites:
                    instruction=code.at(pc)
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=b'\x0f\x29':raise ValueError('Support vector-store opcode drift')
                    vector_ops[f'{pc:x}']=instruction.bytes.hex();events.extend((pc,address+at,before[at:at+8],raw[at:at+8])for at in (0,8))
                else:events.append((pc,address,before,raw))
            out.write(struct.pack('<I',len(events)))
            for pc,address,before,raw in events:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            calls=[p for p in case['calls']if p['entry']==0x1f6610];child_entries+=len(calls)
            if calls:last=0x1f6688
            elif entry==3:last=0x30f453
            elif entry==0:last=0x30ee8f if struct.unpack_from('<I',bank,gp[6]-base)[0]==4 else 0x30ee99
            elif entry==1:last=0x30f27f if struct.unpack_from('<I',bank,gp[7]-base)[0]==4 else 0x30f289
            else:last=0x30f45e if struct.unpack_from('<I',bank,stack+0x4c-base)[0] else 0x30f46f if struct.unpack_from('<I',bank,gp[6]-base)[0]==4 else 0x30f479
            out.write(struct.pack('<3I',last,len(calls),len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
            executed.update(set(case['executed_sites'])&all_sites)
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native physical support failed: '+r.stderr.decode()[-5000:])
    a=expected.read_bytes();b=actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 support differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=all_sites:raise ValueError('Physical support coverage incomplete: '+','.join(f'{p:x}'for p in sorted(all_sites-executed)))
    return dict(cases=len(rows),parent_source_sites=len(executed),full_parent_pre_io_and_child_entry_packets=packets,actual_support_entries=child_entries,complete_normal_abi_banks_ordered_stores=True,native_parent_child_literal_callbacks=0,physical_vector_opcodes=vector_ops,runtime_vector_protocol=[8,8],physical_fault_atomicity=False,comparison_sha256=hashlib.sha256(b).hexdigest(),original=proof,whole_parent_closed=False)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_support_kernel.hpp','src/physics/runtime/native_tu15_simplex_support_abi.hpp','tests/native_tu15_simplex_support_tests.cpp',str(Path(__file__).relative_to(ROOT)),'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_polygon_support_frame.hpp','include/halo2/native_tu15_polygon_support_kernel.hpp','src/physics/runtime/physics_step_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/native_tu15_simplex_candidate_abi.hpp']
    pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in (current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    paths=sorted(seen);pins={name:sha(ROOT/name)for name in paths}
    try:
        report['reference'],blocks,sites=reference(scratch)
        binary=scratch/'support';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_support_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise RuntimeError('Support fixture compile failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise RuntimeError('Support fixture failed:\n'+report['diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        if args.xbe:report['hardware']=hardware(args,scratch,binary,blocks,sites)
        text=(ROOT/'include/halo2/native_tu15_simplex_support_kernel.hpp').read_text();report['semantic_mutants']={}
        changes={
            'reuse_stale_inverse_y':('d.y_reload=memory.vector_read(P::inverse_y_reload,d);','d.y_reload=d.y;'),
            'omit_transform_translation':('d.zt=add(d.z_term,d.translation);','d.zt=d.z_term;'),
            'ignore_progress_guard':('if(memory.progress_blocks(memory.word_read(P::progress,d),d))','if(memory.progress_blocks(memory.word_read(P::progress,d),d)&&false)'),
        }
        for name,(needle,replacement)in changes.items():
            if text.count(needle)!=1:raise ValueError('Support mutant seam drift: '+name)
            directory=scratch/name;(directory/'halo2').mkdir(parents=True);variant=text.replace(needle,replacement);(directory/'halo2/native_tu15_simplex_support_kernel.hpp').write_text(variant)
            changed=command.copy();at=changed.index('-I'+str(ROOT/'include'));changed[at:at]=['-I'+str(directory)];changed[-1]=str(directory/'mutant')
            built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise ValueError('Support mutant compile failed: '+built.stderr[-5000:])
            rejected=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
            if rejected.returncode==0 or'Native simplex support mismatch'not in rejected.stderr:raise ValueError('Support mutant not rejected: '+name+'\n'+rejected.stderr[-5000:])
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=rejected.stderr[-1000:])
        original_bodies()
        report['sources_unchanged']=pins=={name:sha(ROOT/name)for name in paths}
        if not report['sources_unchanged']:raise ValueError('Support proof inputs changed')
        report.update(passed=True,whole_parent_closed=False,physical_fault_atomicity=False)
    except Exception as error:report['error']=str(error);raise
    finally:report['source_sha256']=pins;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['diagnostic'].strip());return 0
if __name__=='__main__':raise SystemExit(main())
