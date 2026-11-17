#!/usr/bin/env python3
"""Whole unchanged313390 plus both genuine1F6610 support children, no outcomes."""
import argparse,hashlib,json,os,platform,re,struct,subprocess
from pathlib import Path
import validate_native_solver_hull_support_pair_frame_tu15 as base
from validate_native_tu15_prt_helpers import physical_original,PIN
from native_solver_hull_support_pair_mutants import run_mutants
DRIVER='''#define main support_pair_component_main
#include "native_tu15_solver_hull_support_pair_tests.cpp"
#undef main
int main(int argc,char**){try{if(argc==1)return support_pair_component_main();J rows;std::cin>>rows;J outputs=J::array();for(const auto&row:rows){Memory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();const auto got=run(memory,abi,true);J banks=J::array();for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});outputs.push_back(J{{"abi",got.abi},{"packets",got.packets},{"children",got.children},{"stopped",got.stopped},{"banks",banks},{"writes",memory.writes}});}std::cout<<outputs.dump()<<"\\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\\n";return 1;}}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mutations',action='store_true');p.add_argument('--include-zero-support',action='store_true');p.add_argument('--include-box-support',action='store_true');args=p.parse_args()
    if args.include_box_support and not args.include_zero_support:p.error('--include-box-support requires --include-zero-support')
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    root=base.ROOT;scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['tools/native_solver_hull_support_pair_shape_inputs.py','tools/validate_native_box_shape_support_tu15.py','tools/native_zero_shape_support_replacements.py','tools/native_solver_hull_support_pair_zero_inputs.py','tools/validate_native_zero_shape_support_tu15.py','tools/native_solver_hull_support_pair_mutants.py','include/halo2/native_tu15_solver_hull_support_pair_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp','tests/native_tu15_solver_hull_support_pair_tests.cpp','tools/validate_native_solver_hull_support_pair_frame_tu15.py',str(Path(__file__).relative_to(root))];pending=list(names);seen=set(names)
    while pending:
        current=root/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            path=next((v for v in(current.parent/header,root/'include'/header,root/'src'/header,root/'tests'/header)if v.is_file()),None)
            if path is None:continue
            relative=str(path.relative_to(root))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:base.sha(root/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if base.sha(args.xbe)!=PIN:raise ValueError('Wrong unchanged original image')
        from research_mp_sandbox_update_functions import Xbe
        from oracle_native_campaign_screen_lifecycle_tu15 import _Code
        image=Xbe(args.xbe);code=_Code(image,'.text');report['reference'],body=base.reference(scratch);rows=base.inputs(image)
        if args.include_zero_support:
            from native_solver_hull_support_pair_zero_inputs import mixed_inputs
            from validate_native_zero_shape_support_tu15 import body as zero_body
            zero_body();rows=mixed_inputs(image,rows)
        if args.include_box_support:
            from native_solver_hull_support_pair_shape_inputs import box_inputs
            from validate_native_box_shape_support_tu15 import body as box_body
            box_body();rows=box_inputs(image,rows)
        source=scratch/'driver.cpp';source.write_text(DRIVER);binary=scratch/'probe';command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(root/'tests'),'-I'+str(scratch),str(source),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if args.include_zero_support:command.insert(1,'-DHALO2_SUPPORT_PAIR_ZERO_CHILD=1')
        if args.include_box_support:command.insert(1,'-DHALO2_SUPPORT_PAIR_BOX_CHILD=1')
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Whole support-pair compile failed: '+built.stderr[-9000:])
        inputs=scratch/'inputs.json';inputs.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=base.sha(inputs)
        with inputs.open('rb')as inp:phased=subprocess.run([str(binary)],stdin=inp,capture_output=True,text=True)
        if phased.returncode:raise ValueError('Whole support-pair phased comparison failed: '+phased.stderr[-10000:])
        report['source']=json.loads(phased.stdout.strip().splitlines()[-1]);all_sites=set();parent_io=set();traces=[];ordered_math=[]
        for line in body.splitlines():
            match=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not match:continue
            pc=int(match[1],16);all_sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:
                parent_io.add(pc);traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True))
            math=re.search(r'_mm_(mul|add)_ps\(s.xmm\[([0-7])\],s.xmm\[([0-7])\]\)',line)
            if math:
                instruction=code.at(pc);expected=f'xmm{math[2]},xmm{math[3]}'
                if instruction.mnemonic!=math[1]+'ps'or instruction.op_str.replace(' ','')!=expected:raise ValueError('Original SIMD operand ownership drift')
                ordered_math.append(dict(pc=f'{pc:x}',bytes=instruction.bytes.hex(),operands=instruction.op_str))
        traces.append(dict(entry=0x1f6610,arguments=2,gp=True,xmm=True))
        if args.include_zero_support:traces.append(dict(entry=0x2e8d20,arguments=2,gp=True,xmm=True))
        if args.include_box_support:traces.append(dict(entry=0x306790,arguments=2,gp=True,xmm=True))
        regions=((0x61000000,0x10000),(0x44a000,0x1000),(0x461000,0x2000))
        observed,proof=physical_original(image,rows,scratch,entries=((0x313390,0x1f6610,0x2e8d20,0x306790)if args.include_box_support else(0x313390,0x1f6610,0x2e8d20)if args.include_zero_support else(0x313390,0x1f6610)),regions=regions,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        expected=[];executed=set();packets=children=return_words=0
        for case in observed['cases']:
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Whole original support-pair did not return')
            row=rows[case['id']];parent_packets=[];child_packets=[]
            for trace in case['calls']:
                if trace['entry']in parent_io:parent_packets.append({key:value for key,value in trace.items()if key!='arguments'})
                elif trace['entry']==0x1f6610 or(args.include_zero_support and trace['entry']==0x2e8d20)or(args.include_box_support and trace['entry']==0x306790):
                    site=(0x3133ab,0x313425)[len(child_packets)];return_pc=site+3;address=trace['gp'][4]
                    matches=[w for w in case['writes']if w['instruction']==site and w['address']==address and w['written']==struct.pack('<I',return_pc).hex()]
                    if len(matches)!=1:raise ValueError('Original actual CALL return-word/SP witness differs')
                    child_packets.append(dict(site=site,return_pc=return_pc,arguments=trace['arguments'],abi={key:value for key,value in trace.items()if key!='arguments'}));return_words+=1
            if len(child_packets)!=2:raise ValueError('Whole original did not execute both genuine declared support children')
            stores=[]
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before'])
                if event['instruction']in(0x31341d,0x313461)or(args.include_zero_support and event['instruction']==0x2e8d2c)or(args.include_box_support and event['instruction']in(0x30679d,0x3067bf)):
                    if len(raw)!=16 or code.at(event['instruction']).bytes[:2]!=bytes.fromhex('0f29'):raise ValueError('Parent MOVAPS projection drift')
                    stores.extend(dict(instruction=event['instruction'],address=event['address']+n,before=before[n:n+8].hex(),written=raw[n:n+8].hex())for n in(0,8))
                else:stores.append(event)
            abi=dict(entry=0x31346b,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],fp=case['after_fp'],live_x87=row['live_x87'])
            expected.append(dict(abi=abi,packets=parent_packets,children=child_packets,stopped=False,banks=sorted(case['banks'],key=lambda b:b['address']),writes=stores));executed.update(case['executed_sites']);packets+=len(parent_packets);children+=len(child_packets)
        outputs=scratch/'outputs.json'
        with inputs.open('rb')as inp,outputs.open('wb')as out:run=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
        if run.returncode:raise ValueError('Whole original-input native replay failed: '+run.stderr.decode()[-9000:])
        actual=json.loads(outputs.read_text())
        for n,(want,got)in enumerate(zip(expected,actual,strict=True)):
            if want!=got:
                field=next(key for key in want if want[key]!=got.get(key));report['first_difference']=dict(case=rows[n]['name'],field=field,expected=want[field],actual=got.get(field));raise ValueError('Whole support-pair unchanged-i386 differs '+rows[n]['name']+' '+field)
        if args.mutations:report['semantic_mutants']=run_mutants(root,scratch,command,inputs)
        report.update(passed=True,hardware=dict(cases=len(rows),parent_sites=len(all_sites),executed_parent_sites=len(all_sites&executed),full_parent_pre_io_packets=packets,complete_actual_child_entry_packets=children,actual_return_word_witnesses=return_words,genuine_child_stack_alias_cases=32,original_text_unmodified=True,supplied_child_outcomes=False,zero_support_included=args.include_zero_support,box_support_included=args.include_box_support,ordered_original_simd=ordered_math,comparison_sha256=hashlib.sha256(json.dumps(actual,sort_keys=True).encode()).hexdigest(),physical_vector_rmw_page_fault_atomicity=False,original=proof))
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(base.sha(root/name)==value for name,value in pins.items())and hashlib.sha256(base.body().encode()).hexdigest()==base.PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Whole support-pair source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in('passed','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
