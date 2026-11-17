#!/usr/bin/env python3
"""Unchanged314710 segments with input-only replay of genuine RDTSC values."""
import argparse,hashlib,json,os,platform,random,re,struct,subprocess
from pathlib import Path
import validate_native_solver_hull_context_tu15 as base
from generated_routine_ownership import function_bodies
from validate_native_tu15_prt_helpers import physical_original,PIN

DRIVER='''#define main hull_context_component_main
#include "native_tu15_solver_hull_context_tests.cpp"
#undef main
int main(int argc,char**){try{if(argc==1)return hull_context_component_main();J inputs;std::cin>>inputs;J outputs=J::array();for(const auto&row:inputs){Memory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();const auto got=run(memory,abi,true,row.at("continuation"),0,false,row.at("clock_inputs").get<std::vector<std::uint64_t>>());J banks=J::array();for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});outputs.push_back(J{{"abi",got.abi},{"packets",got.packets},{"clock_entries",got.clocks},{"child",got.child},{"stopped",got.stopped},{"banks",banks},{"writes",memory.writes},{"architectural_reads",memory.architectural_reads()}});}std::cout<<outputs.dump()<<"\\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\\n";return 1;}}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file()or platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686'):
        print('SKIP genuine hull-context original requires its XBE and Linux x86 ptrace');return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);root=base.ROOT
    names=['include/halo2/native_tu15_solver_hull_context_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_context_frame.hpp','tests/native_tu15_solver_hull_context_tests.cpp','tests/native_tu15_mopp_ray_frame_test_memory.hpp','tools/validate_native_solver_hull_context_tu15.py',str(Path(__file__).relative_to(root))]
    pending=list(names);seen=set(names)
    while pending:
        current=root/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            path=next((v for v in(current.parent/header,root/'include'/header,root/'src'/header)if v.is_file()),None)
            if path is None:continue
            relative=str(path.relative_to(root))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:base.digest(root/name)for name in sorted(seen)};report=dict(passed=False,scratch_owner=str(scratch),production_bound=False,retained_large_artifacts=[],source_sha256=pins)
    try:
        if base.digest(args.xbe)!=PIN:raise ValueError('Wrong unchanged original XBE')
        from research_mp_sandbox_update_functions import Xbe
        from oracle_native_campaign_screen_lifecycle_tu15 import _Code
        report['reference']=base.reference(scratch);source=scratch/'driver.cpp';source.write_text(DRIVER);binary=scratch/'probe'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(root/'tests'),'-I'+str(scratch),str(source),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Hull-context original fixture compile failed: '+built.stderr[-8000:])
        canonical=subprocess.run([str(binary)],capture_output=True,text=True)
        if canonical.returncode:raise ValueError('Current hull-context canonical segments failed: '+canonical.stderr[-8000:])
        report['canonical_segments']=json.loads(canonical.stdout.strip().splitlines()[-1]);report['semantic_mutants']=base.semantic_mutants(scratch,command)
        image=Xbe(args.xbe);code=_Code(image,'.text')
        for pc in(0x31472e,0x3147b1):
            if code.at(pc).bytes.hex()!='0f31':raise ValueError('Genuine hull RDTSC opcode drift')
        body=next(b for sig,b in function_bodies((root/'src/physics/physics_live_support_tu15_generated.cpp').read_text())if sig.symbol=='314710');all_sites=set();traces=[]
        for line in body.splitlines():
            found=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not found:continue
            pc=int(found[1],16);all_sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line or'timestamp_counter'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True))
        rng=random.Random(0x314710);rows=[];bankbase=0x61000000
        for k in range(128):
            continuation=k>=64;bank=bytearray(rng.randbytes(0x8000));profile=bytearray(rng.randbytes(0x1000));sp=bankbase+0x7000+k%16;bp=sp-4;frame=(bp&~15)-0x70;gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=frame if continuation else sp
            def put(at,value):
                target,baseaddr=(profile,0x48c000)if 0x48c000<=at<0x48d000 else(bank,bankbase)
                struct.pack_into('<I',target,at-baseaddr,value&0xffffffff)
            put(sp,0x63000000)
            for n,value in enumerate((bankbase+0x1000,bankbase+0x1100,bankbase+0x2000,bankbase+0x3000,bankbase+0x4000,bankbase+0x5000)):put(sp+4+n*4,value)
            if continuation:gp[5]=bp;put(bp,rng.getrandbits(32));put(frame,rng.getrandbits(32))
            cursor=frame+0xc if k%4==3 else bankbase+0x6000;put(0x48c974,cursor);put(0x48c978,cursor+(12 if k%3 else 0));cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,8)[k%4];values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
            current_sp=gp[4];top_words=struct.unpack_from('<3I',bank,current_sp-bankbase)
            row=dict(id=k,name=f'hull-context-{"continuation"if continuation else"entry"}-{k}',entry=0x31479b if continuation else 0x314710,continuation=continuation,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=top_words[0],matrix=top_words[1],source=top_words[2],before_fp=[cw,0x4100|((k%8)<<11),mxcsr],live_x87=live,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=100,banks=[dict(address=bankbase,before=bank.hex()),dict(address=0x48c000,before=profile.hex())])
            if not continuation:row['stop_before_entries']=[0x313bb0]
            rows.append(row)
        original,proof=physical_original(image,rows,scratch,entries=(0x314710,),regions=((bankbase,0x8000),(0x48c000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        expected=[];clock_events=[];executed=set();packet_count=0
        def packet(pc,case):return dict(entry=pc,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],fp=case['after_fp'],live_x87=rows[case['id']]['live_x87'])
        for case in original['cases']:
            row=rows[case['id']]
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Genuine hull context did not reach declared boundary')
            if not row['continuation']and case.get('stopped_at')!=0x313bb0:raise ValueError('Genuine hull child boundary differs')
            clocks=[];entry=[]
            for trace in case['calls']:
                if trace['entry']in(0x31472e,0x3147b1):entry.append(trace)
                if trace['entry']in(0x314730,0x3147b3):
                    value=(trace['gp'][2]<<32)|trace['gp'][0];clocks.append(value);clock_events.append(dict(case=row['name'],instruction=f'{trace["entry"]-2:x}',value=f'{value:016x}'))
            if len(clocks)!=len(entry):raise ValueError('Original clock event/order/count authentication failed')
            row['clock_inputs']=clocks;stores=[]
            for event in case['writes']:
                before,raw=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                if len(raw)==16:
                    if code.at(event['instruction']).bytes[:2]!=bytes.fromhex('0f29'):raise ValueError('Original hull-context vector opcode drift')
                    stores.extend(dict(instruction=event['instruction'],address=event['address']+n,before=before[n:n+8].hex(),written=raw[n:n+8].hex())for n in(0,8))
                else:stores.append(event)
            packets=[{key:value for key,value in trace.items()if key!='arguments'}for trace in case['calls']]
            entry=[{key:value for key,value in trace.items()if key!='arguments'}for trace in entry]
            expected.append(dict(abi=packet(0x3147cd if row['continuation']else 0x314796,case),packets=packets,clock_entries=entry,child=None if row['continuation']else packet(0x313bb0,case),stopped=not row['continuation'],banks=sorted(case['banks'],key=lambda b:b['address']),writes=stores))
            packet_count+=len(packets);executed.update(case['executed_sites'])
        inputs=scratch/'native-inputs.json';outputs=scratch/'native-output.json';inputs.write_text(json.dumps(rows,separators=(',',':')))
        with inputs.open('rb')as inp,outputs.open('wb')as out:run=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
        if run.returncode:raise ValueError('Hull-context original-input fixture failed: '+run.stderr.decode()[-8000:])
        actual=json.loads(outputs.read_text())
        for n,(want,got)in enumerate(zip(expected,actual,strict=True)):
            got.pop('architectural_reads')
            if want!=got:
                field=next(key for key in want if want[key]!=got.get(key));detail=dict(case=rows[n]['name'],field=field,expected=want[field],actual=got.get(field));report['first_difference']=detail;raise ValueError('Unchanged hull-context segment differs '+rows[n]['name']+' '+field)
        report.update(passed=True,hardware=dict(segments=len(rows),genuine_313bb0_entry_stops=64,independently_declared_postchild_continuations=64,parent_sites=len(all_sites),executed_parent_sites=len(all_sites&executed),full_pre_io_gp_flags_xmm_fp_live80_packets=packet_count,clock_input_replay_events=len(clock_events),clock_input_replay_sha256=hashlib.sha256(json.dumps(clock_events,sort_keys=True).encode()).hexdigest(),comparison_sha256=hashlib.sha256(json.dumps(actual,sort_keys=True).encode()).hexdigest(),original_text_unmodified=True,supplied_child_outcomes=False,whole_owner_completion_qualified=False,physical_vector_page_fault_atomicity=False,original=proof),clock_scope='Genuine unchanged-original RDTSC external-input replay into native only; fullPC/order/count packets authenticated. No original patch, child outcome, clock-platform owner closure or joined whole-parent execution is claimed.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(base.digest(root/name)==pin for name,pin in pins.items())and base.owner_digest()==base.PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Hull-context original probe source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in('passed','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
