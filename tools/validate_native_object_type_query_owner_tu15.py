#!/usr/bin/env python3
"""Private TypeQuery entry admission and raw/prepared retired-provider policy."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess
from pathlib import Path
import validate_native_object_type_query_source_frame_tu15 as source
import validate_native_object_body_routes_tu15 as lookup
from oracle_smg_firing_1_1 import Xbe
ROOT=Path(__file__).resolve().parents[1]
OWNER='src/gameplay/objects/native_tu15_object_type_query_source_owner.hpp'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
MAIN=r'''
int main(int argc,char**argv)try{
    check(argc==2,"Supply private TypeQuery owner inputs");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    const bool reference=native_object_type_query_owner_detail::reference();
    const auto refusals=refusal_probe();
    unsigned comparisons{},calls{},handoffs{},normal_native{},raw_fallbacks{};
    std::set<std::uint32_t>resume_sites;
    for(const auto&row:rows){
        const auto initial=seed(row);
        for(unsigned format=0;format<3;++format)for(bool observed:{false,true}){
            if(format==2u&&observed)continue;
            auto a=initial,b=initial;
            const auto expected=run(a,row,false,observed,0,0,format);
            check(!expected.stopped&&expected.error.empty(),"TypeQuery original normal return missing");
            const auto entered=native_object_type_query_owner_detail::activity();
            const auto statements=format?source_reference_raw::parent_statements:source_reference::parent_statements;
            const auto actual=run(b,row,true,observed,0,0,format);
            equal(a,b,expected,actual,"normal-owner-"+std::to_string(format));
            const auto delta=native_object_type_query_owner_detail::activity()-entered;
            check(delta==(!reference&&format!=1u?1u:0u),"TypeQuery owner entry admission differs");
            if(delta){
                check((format?source_reference_raw::parent_statements:source_reference::parent_statements)==statements,
                    "Native TypeQuery entered retained parent");
                ++normal_native;
            }
            raw_fallbacks+=format==1u&&!delta;calls+=delta;++comparisons;
            if(row.at("id").get<unsigned>()%64u>=2u)continue;
            for(unsigned ordinal=1;ordinal<=a.io_count;++ordinal)for(unsigned action:{6u,7u,8u}){
                auto old=initial,current=initial;
                const auto before=run(old,row,false,observed,ordinal,action,format);
                const auto count=native_object_type_query_owner_detail::activity();
                const auto got=run(current,row,true,observed,ordinal,action,format);
                equal(old,current,before,got,"owner-policy-"+std::to_string(format)+"-"+std::to_string(ordinal));
                const auto entered_now=native_object_type_query_owner_detail::activity()-count;
                check(entered_now==(!reference&&format!=1u?1u:0u),"TypeQuery owner policy admission differs");
                if(got.resume){resume_sites.insert(got.resume);++handoffs;}
                calls+=entered_now;++comparisons;
            }
        }
    }
    check(reference?calls==0u:normal_native==rows.size()*3u,"TypeQuery native normal coverage absent");
    check(reference?resume_sites.empty():resume_sites.size()==24u,"TypeQuery exact continuation coverage differs");
    check(raw_fallbacks==rows.size()*2u,"Raw callback-bearing entry was not wholly retained");
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"native_calls",calls},
        {"normal_zero_retained_calls",normal_native},{"raw_callback_whole_entry_fallbacks",raw_fallbacks},
        {"handoffs",handoffs},{"positive_continuation_sites",resume_sites},{"untouched_refusals",refusals},
        {"own_reference",reference},{"production_bound",false}}.dump()<<'\n';
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
'''
def prepare(scratch):
    proof=source.prepare(scratch)
    include=scratch/'object_type_query_source_reference.inc'
    prepared=include.read_text().replace('static std::set<std::uint32_t>coverage;','static std::uint64_t parent_statements{};static std::set<std::uint32_t>coverage;',1)
    prepared=prepared.replace('coverage.insert(','++parent_statements;coverage.insert(')
    include.write_text(prepared)
    current=(ROOT/source.UNIT).read_text();raw=source.bodies(current)
    helper=source.one(current,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}','sign extension') if hasattr(source,'one') else re.findall(r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',current,re.M|re.S)[0]
    child=raw[0xe0900]
    parent=raw[0x10de40]
    start=parent.index('{')+1
    jump='\nswitch(resume){case 0u:break;'+''.join(f'case 0x{pc:x}u:goto N{pc:x};'for pc in source.FRONTIERS)+'default:throw std::runtime_error("Invalid raw TypeQuery continuation");}\n'
    for pc in source.FRONTIERS:
        needle=f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}'
        if parent.count(needle)!=1:raise ValueError('Raw TypeQuery continuation changed')
        parent=parent.replace(needle,f'N{pc:x}: '+needle,1)
    parent=parent[:start]+jump+parent[start:]
    parent=parent.replace('static void run_10de40(WorldState &s)','static void retained(WorldState &s,std::uint32_t resume)',1)
    parent=parent.replace('if(s.services.instruction){','++parent_statements;if(s.services.instruction){')
    dispatcher='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){if(target!=0xe0900u||site!=0x10de95u||tail)throw std::runtime_error("Undeclared type-query callback");run_e0900(s);}\n'
    (scratch/'object_type_query_raw_reference.inc').write_text('static std::uint64_t parent_statements{};\n'+helper+'\n'+child+'\n'+dispatcher+parent+'\n')
    text=(ROOT/source.TEST).read_text()
    text=text.replace('#include "gameplay/objects/native_tu15_object_type_query_source_frame.hpp"','#include "gameplay/objects/native_tu15_object_type_query_source_owner.hpp"',1)
    text=text.replace('namespace {\nstruct Memory','namespace source_reference_raw {\n#include "object_type_query_raw_reference.inc"\n}\nnamespace {\nstruct Memory',1)
    old='unsigned selected=0,unsigned action=0){'
    if text.count(old)!=1:raise ValueError('TypeQuery run signature changed')
    text=text.replace(old,'unsigned selected=0,unsigned action=0,unsigned format=0){',1)
    text=text.replace('WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};',
        'if(format==2u){services.instruction={};cursor={};}\n    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};',1)
    begin=text.index('        if(native){');end=text.index('    }catch(const Boundary&)',begin)
    text=text[:begin]+'''        const auto retained=[&](std::uint32_t next){if(format)source_reference_raw::retained(state,next);else source_reference::retained(state,next);};
        if(native){
            const auto child=[&](auto&v,auto target,auto site,bool tail){if(format)source_reference_raw::dispatch(v,target,site,tail);else source_reference::dispatch(v,target,site,tail);};
            const auto next=format?native_object_type_query_owner_detail::bridge<false>(state,child):native_object_type_query_owner_detail::bridge<true>(state,child);
            if(next!=0xffffffffu){out.resume=next;retained(next);}
        }else retained(0u);
'''+text[end:]
    text=text.replace('int main(int argc,char**argv)','int source_frame_unused_main(int argc,char**argv)',1)
    refusals=lookup.MAIN[:lookup.MAIN.index('int main(')]
    refusals=refusals.replace('native_object_body_owner_detail','native_object_type_query_owner_detail')
    refusals=refusals.replace('::bridge(state);','::bridge(state,[](auto&,auto,auto,bool){throw std::runtime_error("Refused TypeQuery called child");});')
    harness=scratch/'type-query-owner.cpp';harness.write_text(text+'\n'+refusals+'\n'+MAIN)
    return harness,proof
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(OWNER,source.FRAME,source.TEST,'tools/validate_native_object_type_query_owner_tu15.py','tools/validate_native_object_type_query_source_frame_tu15.py','tools/validate_native_object_type_query_frame_tu15.py')
    pins={name:sha(ROOT/name)for name in names};report=dict(passed=False,production_bound=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    headers={path.resolve():sha(path)for tree in('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc','.cpp')}
    try:
        if sha(a.xbe)!=source.physical.PIN:raise ValueError('Wrong original input')
        harness,proof=prepare(scratch);report['qualification']=proof
        source.physical.REGIONS=tuple(sorted((*source.physical.REGIONS,(0x46a000,4096),(0x4f0000,4096))))
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(source.physical.corpus(Xbe(a.xbe),True),separators=(',',':')))
        results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(harness),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('TypeQuery owner compile failed '+built.stderr[:3500]+built.stderr[-3500:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            modes={}
            for mode in('default','own-reference','unrelated-control'):
                env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_REFERENCE_')}
                if mode=='own-reference':env['HALO2_REFERENCE_OBJECT_TYPE_QUERY']='1'
                if mode=='unrelated-control':env['HALO2_REFERENCE_OBJECT_BODY']='1'
                done=subprocess.run([str(binary),str(fixture)],env=env,capture_output=True,text=True)
                if done.returncode:raise ValueError('TypeQuery owner mode failed '+mode+': '+(done.stdout+done.stderr)[-7000:])
                modes[mode]=json.loads(done.stdout)
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT/OWNER).read_text()
                definitions=(
                    ('ignore-own-reference','if(reference()||','if(false||','own-reference'),
                    ('ignore-returned-policy','return admitted<Literal>(current);','return true;','default'),
                    ('replay-completed-entry','return next?next:0xffffffffu;','return next?next:0u;','default'),
                    ('ignore-cached-literal-certificate','s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor','s.instruction_cursor==&s.services.instruction_cursor','default'),
                    ('admit-raw-callback','if constexpr(!Literal)if(s.services.instruction)return false;','if constexpr(!Literal)if(false)return false;','default'))
                for label,old,new,mode in definitions:
                    if original.count(old)!=1:raise ValueError('TypeQuery owner defect seam changed '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/OWNER
                    target.parent.mkdir(parents=True);target.write_text(original.replace(old,new,1))
                    changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(overlay/'bad')
                    built=subprocess.run(changed,capture_output=True,text=True,timeout=600)
                    if built.returncode:raise ValueError('TypeQuery owner defect compile failed '+label+': '+built.stderr[-5000:])
                    env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_REFERENCE_')}
                    if mode=='own-reference':env['HALO2_REFERENCE_OBJECT_TYPE_QUERY']='1'
                    bad=subprocess.run([str(overlay/'bad'),str(fixture)],env=env,capture_output=True,text=True,timeout=1200)
                    diagnostic=(bad.stdout+bad.stderr)[-5000:]
                    expected=('Type-query source continuation differs','TypeQuery owner entry admission differs',
                        'TypeQuery owner policy admission differs','Private object lookup untouched refusal failed',
                        'Native TypeQuery entered retained parent')
                    if not bad.returncode or not any(value in diagnostic for value in expected):
                        raise ValueError('TypeQuery owner defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,modes=modes,compiled_mutations=mutations))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
