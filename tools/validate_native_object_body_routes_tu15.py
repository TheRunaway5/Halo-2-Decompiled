#!/usr/bin/env python3
"""Execute private current/fresh prepared UnitView candidates, with actual suffixes."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import native_object_body_replacements as binding
from specialize_native_operands import specialize
from validate_native_object_body_source_frame_tu15 import prepare as prepare_reference,UNIT
from validate_native_object_body_source_generation_tu15 import GENERATOR,parts
from validate_native_object_body_frame_tu15 import corpus
ROOT=Path(__file__).resolve().parents[1]
TEST='tests/native_tu15_object_body_source_frame_tests.cpp'
TEST_PIN='eb85f7413eed1810fe78ee25da39447d29c98ba93054dca6e32193eb5ad78687'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
MAIN=r'''
unsigned refusal_probe(){
    if(halo2::native_object_body_owner_detail::reference())return 0u;
    for(unsigned kind=0;kind<24u;++kind){
        auto memory=seed(J{{"banks",J::array()}});SourcePhysicsWorldRuntimeServices services;
        SourcePhysicsContactRecordObserver observer=[](const auto&){};
        services.instruction=[](std::uint32_t){};
        std::array sites{0x38846eu,0x21f590u,0x33b595u};
        auto&cursor=services.instruction_cursor;bool trace=true;
        cursor.position=&memory.pc;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=kind!=23u;cursor.observation_only_stores=true;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        J callback_log=J::array();memory.state=&state;memory.services=&services;memory.trace=&trace;memory.callback_log=&callback_log;
        SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};state.load(abi);
        if(kind!=23u)check(halo2::native_object_body_owner_detail::admitted(state),"Private object lookup baseline guard refused");
        auto external=cursor;std::array extra{sites[0],sites[1],sites[2],sites[2]};
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{};asm volatile("fnstcw %0":"=m"(cw));
        switch(kind){
            case 0:state.image=SourcePhysicsContactImage::MapPack11;break;
            case 1:state.image=SourcePhysicsContactImage(99);break;
            case 2:state.active_mmx.emplace();break;
            case 3:state.fp.count=9u;break;case 4:state.fp.top=8u;break;
            case 5:state.fp.status|=0x8080u;break;
            case 6:services.instruction={};break;
            case 7:services.instruction=[different=1](std::uint32_t){(void)different;};break;
            case 8:cursor.position=nullptr;break;case 9:cursor.executor=std::thread::id{};break;
            case 10:cursor.literal_service_sites=false;break;case 11:state.instruction_cursor=&external;break;
            case 12:cursor.callback_type=nullptr;break;case 13:sites[2]^=1u;break;
            case 14:cursor.service_sites=std::span(sites).first(2);break;
            case 15:cursor.first_trace=&trace;break;case 16:cursor.second_trace=&trace;break;
            case 17:cursor.observation_only_stores=false;break;case 18:cursor.callback_type=&typeid(int);break;
            case 19:_mm_setcsr(mxcsr&~0x80u);break;
            case 20:services.instruction={};state.instruction_cursor=nullptr;break;
            case 21:cursor.service_sites=extra;break;
            case 22:{const auto changed=std::uint16_t(cw&~1u);asm volatile("fldcw %0"::"m"(changed));break;}
            case 23:cursor.literal_service_sites=true;break;
        }
        const auto gp=state.gp;const auto flags=state.flags.bits(),other=state.other_flags,depth=state.fp.count,top=state.fp.top;
        const auto status=state.fp.status;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);
        const auto entered=halo2::native_object_body_owner_detail::activity();
        std::uint32_t result=0xffffffffu;bool threw{};
        try{result=halo2::native_object_body_owner_detail::bridge(state);}catch(...){threw=true;}
        const bool unchanged=state.gp==gp&&state.flags.bits()==flags&&state.other_flags==other&&state.fp.count==depth&&
            state.fp.top==top&&state.fp.status==status&&std::memcmp(xmm.data(),state.xmm,128)==0&&
            !memory.io_count&&!memory.pc&&halo2::native_object_body_owner_detail::activity()==entered;
        _mm_setcsr(mxcsr);asm volatile("fldcw %0"::"m"(cw));
        check(!threw&&!result&&unchanged,"Private object lookup untouched refusal failed "+std::to_string(kind));
    }
    return 24u;
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply actual object lookup routes corpus");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    unsigned comparisons{},native_calls{},literal_statements{},resumed_cases{},normal_calls{};
    const auto refusals=refusal_probe();
    const bool reference=halo2::native_object_body_owner_detail::reference();
    for(const auto&row:rows)for(bool observed:{false,true})for(unsigned variant=0;variant<2u;++variant){
        route_selector=-1;auto baseline=seed(row);
        const auto normal=run(baseline,row,!reference,observed);
        for(unsigned ordinal=0;ordinal<=baseline.io_count;++ordinal){
            if(ordinal&&row.at("id").get<unsigned>()%32u>=8u)continue;
            const unsigned last=ordinal?9u:0u;
            for(unsigned action=ordinal?1u:0u;action<=last;++action){
                auto old=seed(row),actual=seed(row);route_selector=-1;
                const auto expected=run(old,row,!reference,observed,ordinal,action);
                const auto before=halo2::native_object_body_owner_detail::activity();
                const auto literals=candidate_source::literals;
                route_selector=int(variant);
                const auto got=run(actual,row,true,observed,ordinal,action);
                equal(old,actual,expected,got,row.at("name").get<std::string>()+"-candidate-"+std::to_string(variant)+"-"+std::to_string(ordinal)+"-"+std::to_string(action));
                const auto entries=halo2::native_object_body_owner_detail::activity()-before;
                check(entries==(reference?0u:1u),"Actual object lookup candidate admission differs");
                native_calls+=entries;literal_statements+=candidate_source::literals-literals;
                if(!ordinal&&!reference){check(candidate_source::literals==literals,"Normal native lookup entered retained statements");++normal_calls;}
                resumed_cases+=!reference&&candidate_source::literals!=literals;
                ++comparisons;
            }
        }
    }
    check(reference?literal_statements>0u:(native_calls==comparisons&&resumed_cases>0u),"Actual object lookup route controls absent");
    std::cout<<J{{"passed",true},{"source_variants",2},{"comparisons",comparisons},
        {"native_calls",native_calls},{"retained_statements",literal_statements},
        {"native_calls_with_retained_suffix",resumed_cases},{"normal_zero_retained_calls",normal_calls},
        {"untouched_live_refusals",refusals},{"own_reference",reference},
        {"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
'''
def prepare(scratch,require_binding=False):
    prepare_reference(scratch)
    fresh=scratch/'fresh.cpp'
    command=[os.sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--manifest',str(scratch/'fresh.json')]
    done=subprocess.run(command,capture_output=True,text=True)
    if done.returncode:raise ValueError('Actual routes genuine regeneration failed: '+done.stderr[-7000:])
    actual_sources=[(ROOT/UNIT).read_text(),fresh.read_text()]
    if require_binding:
        for value in actual_sources:
            if binding.replace_object_body(value)!=value:raise ValueError('Actual current/fresh object lookup default is not installed')
            binding.installed_entries(value)
    values=[specialize(value)[0]for value in actual_sources]
    blocks=['static std::uint64_t literals{};',parts(values[0])['sign_extend']]
    for n,value in enumerate(values):
        candidate=binding.replace_object_body(value)
        if binding.remove_object_body(candidate)!=binding.remove_object_body(value):raise ValueError('Candidate exact route reconstruction failed')
        body=parts(candidate)['body'].replace('run_bcdf0(',f'candidate_{n}(',1)
        body=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'++literals;s.instruction_literal(\1);',body)
        blocks.append(body)
    blocks.append('static void route(WorldState&s,unsigned n){switch(n){case 0u:candidate_0(s);return;case 1u:candidate_1(s);return;default:throw std::runtime_error("Invalid private route");}}')
    (scratch/'object_body_candidates.inc').write_text('\n'.join(blocks)+'\n')
    original=(ROOT/TEST).read_text()
    if sha(ROOT/TEST)!=TEST_PIN:raise ValueError('Complete source transport harness drift')
    seam='''            out.resume=native_object_body_detail::run_source_frame(state,continuation);
            if(out.resume)source_reference::retained(state,out.resume);'''
    if original.count(seam)!=1 or original.count('namespace {')!=1:raise ValueError('Actual source route harness seam drift')
    original=original.replace(seam,'''            if(route_selector>=0)candidate_source::route(state,unsigned(route_selector));
            else {'''+seam+'''
            }''',1)
    anchor='namespace candidate_source {\n#include "object_body_candidates.inc"\n}\nstatic int route_selector=-1;\nnamespace {'
    original=original.replace('namespace {',anchor,1).replace('int main(int argc,char**argv)','int baseline_probe_main(int argc,char**argv)',1)
    harness=scratch/'routes.cpp';harness.write_text('#include "gameplay/objects/native_tu15_object_body_source_owner.hpp"\n'+original+'\n'+MAIN)
    return harness,dict(genuine_generation_command=command,source_contexts=2,prepared_only_runtime_scope=True,
        exact_baseline_harness_sha256=TEST_PIN,retained_source_operations_changed=False)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    names=[TEST,UNIT,GENERATOR,'tools/native_object_body_replacements.py',
        'src/gameplay/objects/native_tu15_object_body_source_owner.hpp',
        'src/gameplay/objects/native_tu15_object_body_source_frame.hpp','tools/validate_native_object_body_routes_tu15.py']
    pins={name:sha(ROOT/name)for name in names};report=dict(passed=False,production_bound=False,
        owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        harness,proof=prepare(scratch,args.require_binding);fixture=scratch/'inputs.json';fixture.write_text(json.dumps(corpus(),separators=(',',':')))
        results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(harness),'-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Actual candidate route compile failed: '+done.stderr[-8000:])
            modes={}
            for mode in('default','own-reference'):
                environment=os.environ.copy();environment.pop('HALO2_REFERENCE_OBJECT_BODY',None)
                if mode=='own-reference':environment['HALO2_REFERENCE_OBJECT_BODY']='1'
                done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=environment)
                if done.returncode:raise ValueError('Actual candidate route failed '+mode+': '+(done.stdout+done.stderr)[-10000:])
                modes[mode]=json.loads(done.stdout)
            mutations=[]
            if args.semantic_mutants:
                relative='src/gameplay/objects/native_tu15_object_body_source_owner.hpp';source=(ROOT/relative).read_text()
                definitions=(
                    ('ignore-own-reference','if(reference()||','if(false||','own-reference'),
                    ('ignore-returned-policy','return admitted<Literal>(current);','return true;','default'),
                    ('replay-completed-entry','return next?next:0xffffffffu;','return next?next:0u;','default'),
                    ('ignore-cached-literal-certificate','s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor','s.instruction_cursor==&s.services.instruction_cursor','default'))
                for name,old,new,mode in definitions:
                    if source.count(old)!=1:raise ValueError('Actual owner defect seam drift '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative
                    target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
                    changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(overlay/'bad')
                    built=subprocess.run(changed,capture_output=True,text=True)
                    if built.returncode:raise ValueError('Actual owner defect compile failed '+name+': '+built.stderr[-5000:])
                    environment=os.environ.copy();environment.pop('HALO2_REFERENCE_OBJECT_BODY',None)
                    if mode=='own-reference':environment['HALO2_REFERENCE_OBJECT_BODY']='1'
                    bad=subprocess.run([str(overlay/'bad'),str(fixture)],capture_output=True,text=True,env=environment)
                    diagnostic=(bad.stdout+bad.stderr)[-5000:]
                    if not bad.returncode or not any(text in diagnostic for text in('Object-body source continuation differs','Actual object lookup candidate admission differs','Private object lookup untouched refusal failed','Normal native lookup entered retained statements')):
                        raise ValueError('Actual owner defect survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,modes=modes,compiled_mutations=mutations))
        report.update(passed=True,source_qualification=proof,compilers=results,production_bound=args.require_binding)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
