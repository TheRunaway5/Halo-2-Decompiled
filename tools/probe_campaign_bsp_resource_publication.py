#!/usr/bin/env python3
"""Owned, same-state real BSP flags3 control; always retain the original failure.

Run through run_test.py. --syntax-only borrows no build. Full mode may run only
after the coordinator explicitly declares --owner built/ready. This temporary
diagnostic is to be retired after a permanent regression replaces it.
"""
import argparse,hashlib,json,os,re,shlex,shutil,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
MARKER='[DEBUG-campaign-bsp-publication-v2] '
OLD='''        auto completed=world.request_render_geometry(2u,row+0x28u);
        if(!completed)completed=world.request_render_geometry(0u,row+0x28u);
        if(!completed || !word(row+0x50u))
            throw FormatError("Campaign original BSP resource publication did not finish");'''
NEW=r'''        const auto initial=world.request_render_geometry(2u,row+0x28u);
        auto completed=initial;
        if(!completed)completed=world.request_render_geometry(0u,row+0x28u);
        if(!completed || !word(row+0x50u)) {
            using Diagnostic=nlohmann::json;
            Diagnostic diagnostic={{"bsp",bsp},{"row",row},{"initial_flags2",initial},
                {"completed_flags0",completed},{"phase","after_2_then_0_before_3"},
                {"flags3_attempted",false},{"flags3_result",nullptr},{"original_throw_preserved",true},
                {"control_scope","counterfactual_allocator_flags3_not_blanket_prefetch_policy"}};
            const auto small_error=[](const std::exception &e){return std::string(e.what()).substr(0,256);};
            const auto byte=[&](std::uint32_t at){return std::to_integer<unsigned>(memory.view(at,1)[0]);};
            const auto half=[&](std::uint32_t at){return Bytes(memory.view(at,2)).u16(0);};
            try {
                const auto clusters=word(bsp+0x9cu),cluster_base=word(bsp+0xa0u);
                const auto definitions=word(bsp+0x138u),definition_base=word(bsp+0x13cu);
                diagnostic["bsp_layout"]=Diagnostic::array({clusters,cluster_base,definitions,definition_base,
                    word(bsp+0x140u),word(bsp+0x144u)});
                diagnostic["row_domains"]=Diagnostic::array();
                for(const auto &[kind,base,count,stride]:std::array{
                    std::tuple{"cluster",cluster_base,clusters,176u},
                    std::tuple{"definition",definition_base,definitions,200u}}) {
                    if(row>=base && std::uint64_t(row-base)<std::uint64_t(count)*stride && (row-base)%stride==0)
                        diagnostic["row_domains"].push_back(Diagnostic{{"kind",kind},{"index",(row-base)/stride},{"stride",stride}});
                }
            }catch(const std::exception &e){diagnostic["classification_error"]=small_error(e);}
            const auto snapshot=[&]() {
                Diagnostic state=Diagnostic::object();const auto header=row+0x28u;
                try {
                    state["header_words"]=Diagnostic::array();
                    for(auto offset:{0u,4u,8u,12u,16u,20u,24u,28u,32u})state["header_words"].push_back(word(header+offset));
                    const auto delta=std::bit_cast<std::int16_t>(std::uint16_t(half(header+0x1cu)));
                    const auto publication=header+std::uint32_t(std::int32_t(delta));
                    state["row24"]=half(row+0x24u);state["linked"]=byte(header+0x1eu);
                    state["root_delta"]=delta;state["root"]=publication;
                    state["root_count"]=word(publication);state["root_pointer"]=word(publication+4u);
                    state["row50"]=word(row+0x50u);
                }catch(const std::exception &e){state["header_error"]=small_error(e);}
                try {
                    state["policy"]=Diagnostic::array({byte(0x4751acu),word(0x4f2d70u),word(0x4f2d74u),
                        word(0x4f2d78u),byte(0x4f2d80u)});
                    const auto physical=word(0x4f2d6cu),inflight=word(0x4f2d5cu);
                    state["globals"]=Diagnostic::array({physical,inflight,word(0x4f2d64u)});
                    Diagnostic cache={{"address",physical},{"fields",Diagnostic::array()},{"bands",Diagnostic::array()}};
                    for(auto off:{0x20u,0x24u,0x2cu,0x30u,0x34u,0x38u,0x3cu,0x40u,0x64u})cache["fields"].push_back(word(physical+off));
                    for(unsigned n=0;n<9;++n)cache["bands"].push_back(word(physical+0x44u+n*4u));
                    const auto pool=word(physical+0x64u),rows=word(pool+0x44u),bitmap=word(pool+0x48u);
                    const auto high=word(pool+0x38u),capacity=word(pool+0x20u);
                    cache["pool"]=Diagnostic::array({pool,capacity,word(pool+0x24u),high,word(pool+0x3cu),rows,bitmap});
                    cache["rows"]=Diagnostic::array();const auto count=std::min({high,capacity,512u});
                    for(unsigned n=0;n<count && cache["rows"].size()<6;++n)if(word(bitmap+(n>>5u)*4u)&(1u<<(n&31u))) {
                        Diagnostic entry=Diagnostic::array({n});for(unsigned k=0;k<6;++k)entry.push_back(word(rows+n*24u+k*4u));
                        cache["rows"].push_back(std::move(entry));
                    }
                    cache["sample_limit"]=6;cache["scan_bound"]=count;
                    state["physical"]=std::move(cache);
                    state["inflight_pool"]=Diagnostic::array({inflight,word(inflight+0x20u),word(inflight+0x24u),
                        word(inflight+0x38u),word(inflight+0x3cu),word(inflight+0x44u)});
                    const auto token=word(header+0x20u),index=token&65535u;
                    if(token!=0xffffffffu && index<capacity && index<512u) {
                        state["token_physical"]=Diagnostic::array();for(unsigned k=0;k<6;++k)state["token_physical"].push_back(word(rows+index*24u+k*4u));
                        if(index<word(inflight+0x20u)) {
                            const auto tracking=word(inflight+0x44u)+index*12u;
                            state["tracking"]=Diagnostic::array({tracking,half(tracking),byte(tracking+2u),byte(tracking+3u),word(tracking+4u),word(tracking+8u)});
                        }
                    }
                }catch(const std::exception &e){state["loader_error"]=small_error(e);}
                return state;
            };
            diagnostic["before"]=snapshot();
            diagnostic["flags3_attempted"]=true;
            try {diagnostic["flags3_result"]=world.request_render_geometry(3u,row+0x28u);}
            catch(const std::exception &e){diagnostic["flags3_error"]=small_error(e);}
            catch(...){diagnostic["flags3_error"]="non-standard exception";}
            diagnostic["after"]=snapshot();
            // Complete the JSON locally, then make one <=PIPE_BUF stderr write.
            // Never interleave individual field writes with engine stdout.
            try {
                const std::string marker="[DEBUG-campaign-bsp-publication-v2] ";
                auto line=marker+diagnostic.dump()+"\n";
                if(line.size()>4096u) {
                    diagnostic["pool_samples_omitted_for_atomic_log"]=true;
                    for(auto key:{"before","after"})if(diagnostic[key].contains("physical"))diagnostic[key]["physical"].erase("rows");
                    line=marker+diagnostic.dump()+"\n";
                }
                if(line.size()<=4096u)(void)::write(STDERR_FILENO,line.data(),line.size());
                else {
                    const std::string failed=marker+"{\"diagnostic_error\":\"bounded record exceeded PIPE_BUF\",\"original_throw_preserved\":true}\n";
                    (void)::write(STDERR_FILENO,failed.data(),failed.size());
                }
            }catch(...) {
                const char failed[]="[DEBUG-campaign-bsp-publication-v2] {\"diagnostic_error\":\"JSON assembly failed\",\"original_throw_preserved\":true}\n";
                (void)::write(STDERR_FILENO,failed,sizeof(failed)-1);
            }
            // This probe never lets successful control publication hide RED.
            throw FormatError("Campaign original BSP resource publication did not finish");
        }'''

def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def instrument(text):
    if text.count(OLD)!=1:raise ValueError('Actual SourceCampaignView request/guard pattern changed')
    if text.count('#include <bit>')!=1:raise ValueError('Source include seam changed')
    return text.replace('#include <bit>','#include <bit>\n#include <algorithm>\n#include <tuple>\n#include <nlohmann/json.hpp>\n#include <unistd.h>').replace(OLD,NEW)
def parse_records(text):
    decoder=json.JSONDecoder();out=[];at=0
    while True:
        start=text.find(MARKER,at)
        if start<0:break
        start+=len(MARKER);start+=len(text[start:])-len(text[start:].lstrip())
        try:record,end=decoder.raw_decode(text,start)
        except ValueError:at=start;continue
        out.append(record);at=end
    return out
def save(path,value):path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(value,indent=2)+'\n')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--owner',type=Path);p.add_argument('--syntax-only',action='store_true');p.add_argument('--report-output',type=Path,required=True);p.add_argument('--mcc-root',type=Path);p.add_argument('--frames',type=int,default=30000);p.add_argument('--timeout',type=int,default=210);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=a.report_output.resolve()
    if report.exists():raise ValueError('Preserve previous diagnostic receipts; choose a new report path')
    if not 1<=a.frames<=30000 or not 1<=a.timeout<=900:raise ValueError('Diagnostic frame/time budget exceeds finite scope')
    # A raw_decode control catches the earlier line-start extraction failure.
    assert parse_records('unflushedstdout'+MARKER+'{"x":1}\notheroutput')==[{'x':1}]
    receipt=dict(passed=False,scope='Isolated diagnostic capture only; original engine guard always throws after realflags3control',owned_scratch=str(scratch),retained_large_artifacts=[],engine_behavior_fixed=False,full_campaign_qualified=False,script_calls_supplied=False,helper_sha256=sha(__file__))
    try:
        if a.syntax_only:
            source=ROOT/'src/campaign/source_campaign_view_tu15.cpp';private=scratch/source.name;private.write_text(instrument(source.read_text()))
            cmd=['c++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I/usr/include/SDL2','-D_GNU_SOURCE=1','-D_REENTRANT',str(private)]
            run=subprocess.run(cmd,cwd=scratch,text=True,capture_output=True);receipt.update(syntax_only=True,command=cmd,source_sha256=sha(source),diagnostic_sha256=sha(private),exit_code=run.returncode,stderr_tail=run.stderr[-12000:],passed=run.returncode==0,build_borrowed=False,validation_not_run=True,parser_control_passed=True);save(report,receipt);return run.returncode
        if not a.owner or not a.mcc_root:raise ValueError('Full diagnostic requires coordinator-declared ready --owner and actual --mcc-root')
        owner=a.owner.resolve(strict=True);build=owner/'build';frozen=owner/'current-source'
        if owner==scratch or owner in scratch.parents:raise ValueError('Borrowed build and private diagnostic must have separate owners')
        source=frozen/'src/campaign/source_campaign_view_tu15.cpp';flags_path=build/'CMakeFiles/halo2_systems.dir/flags.make';link_path=build/'CMakeFiles/halo2_native.dir/link.txt';worker=build/'eos_sdk_worker'
        flags={}
        for line in flags_path.read_text().splitlines():
            if ' = 'in line:
                key,value=line.split(' = ',1);flags[key]=shlex.split(value)
        link=shlex.split(link_path.read_text());inputs={source,flags_path,link_path,worker}
        compiler=Path(shutil.which(link[0])or link[0]).resolve(strict=True);inputs.add(compiler)
        for token in link[1:]:
            candidate=Path(token);candidate=candidate if candidate.is_absolute()else build/candidate
            if candidate.is_file()and(token.endswith(('.a','.o','.so'))or'.so.'in token):inputs.add(candidate.resolve())
        pins={str(path):sha(path)for path in sorted(inputs)};source_text=source.read_text()
        if hashlib.sha256(source_text.encode()).hexdigest()!=pins[str(source)]:raise ValueError('Borrowed source changed before private instrumentation')
        receipt.update(owner=str(owner),borrowed_input_hashes=pins,source_sha256=sha(source))
        private=scratch/source.name;private.write_text(instrument(source_text));obj=scratch/'campaign.o';runtime=scratch/'halo2_native'
        compile_cmd=[link[0],*flags.get('CXX_DEFINES',[]),*flags['CXX_INCLUDES'],*flags['CXX_FLAGS'],'-MMD','-MF',str(scratch/'campaign.d'),'-c',str(private),'-o',str(obj)]
        run=subprocess.run(compile_cmd,cwd=build,text=True,capture_output=True);receipt.update(compile_command=compile_cmd,compile_exit_code=run.returncode,compile_stderr_tail=run.stderr[-12000:]);save(report,receipt)
        if run.returncode:return run.returncode
        # Source dependency inputs, including the frozen headers, are pinned.
        dependency=(scratch/'campaign.d').read_text().replace('\\\n',' ');dependencies=shlex.split(dependency.split(':',1)[1]);dependency_paths=[(Path(name)if Path(name).is_absolute()else build/name).resolve()for name in dependencies]
        receipt['compiler_dependency_hashes']={str(path):sha(path)for path in dependency_paths if path!=private.resolve()}
        command=link[:];command.insert(1,str(obj));command[command.index('-o')+1]=str(runtime)
        command=[('--dependency-file='+str(scratch/'link.d'))if token.startswith('--dependency-file=')else token for token in command]
        run=subprocess.run(command,cwd=build,text=True,capture_output=True);receipt.update(link_command=command,link_exit_code=run.returncode,link_stderr_tail=run.stderr[-12000:]);save(report,receipt)
        if run.returncode:return run.returncode
        shutil.copy2(worker,scratch/'eos_sdk_worker')
        for name in ['shaders','data','resources']:
            if(build/name).exists():os.symlink(build/name,scratch/name)
        receipt.update(diagnostic_sha256=sha(private),runtime_sha256=sha(runtime),eos_worker_sha256=sha(worker),runtime_helper_sha256=sha(ROOT/'tools/test_campaign_new_game_graphics.py'),original_requests_preserved=True,original_throw_preserved=True,control_flags=3,control_implementation='Counterfactual actual world.request_render_geometry; not blanket prefetch policy; actual appended-unit DLrequest is independent of domain selector; no readiness/allocation/eviction supplied')
        command=[sys.executable,str(ROOT/'tools/test_campaign_new_game_graphics.py'),'--runtime',str(runtime),'--mcc-root',str(a.mcc_root.resolve()),'--frames',str(a.frames),'--timeout',str(a.timeout),'--acceptance','opening_handoff','--report-output',str(report.with_name(report.stem+'-runtime.json'))]
        run=subprocess.run(command,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);receipt.update(runtime_command=command,runtime_returncode=run.returncode,stdout_tail=run.stdout[-6000:])
        log=scratch/'campaign-graphics.log';text=log.read_text(errors='replace')if log.exists()else run.stdout;records=parse_records(text);receipt['records']=records
        unchanged=all(Path(name).exists()and sha(name)==pin for name,pin in pins.items())and all(Path(name).exists()and sha(name)==pin for name,pin in receipt['compiler_dependency_hashes'].items())
        captured=len(records)==1 and records[0].get('flags3_attempted')is True and records[0].get('original_throw_preserved')is True and 'before'in records[0]and'after'in records[0]
        receipt.update(borrowed_inputs_unchanged=unchanged,diagnostic_capture_passed=captured and unchanged,temporary_runtime_retained=False,parser_control_passed=True,validation_not_run=False)
        save(report,receipt);print(json.dumps(dict(report=str(report),diagnostic_capture_passed=receipt['diagnostic_capture_passed'],runtime_returncode=run.returncode,flags3_result=records[0].get('flags3_result')if len(records)==1 else None,engine_behavior_fixed=False)))
        return 0 if captured and unchanged else 1
    except BaseException as error:
        receipt.update(error=repr(error));save(report,receipt);raise
if __name__=='__main__':raise SystemExit(main())
