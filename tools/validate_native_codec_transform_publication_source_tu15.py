#!/usr/bin/env python3
"""Compare the private 283E10 publication suffix to four exact raw/prepared copies."""
import argparse
import hashlib
import json
import os
import platform
import re
import shlex
import subprocess
import sys
from pathlib import Path

from native_codec_transform_packed_original import ENTRIES
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from specialize_native_operands import CALL, CASE, INSTRUCTION, MAPPING, specialize
from validate_native_codec_inverse_transform_source_tu15 import exact
from validate_native_codec_transform_publication_tu15 import (
    ALL_REGIONS, ENTRY, FRAME, KERNEL, RMW, ROOT, original_owners, physical_original_full, publication_rows, sha,
)
from validate_native_codec_transform_chain_tu15 import OWNERS

START, END, REPEAT = 0x283f7b,0x28404a,0x283fa7
TEST = 'tests/native_tu15_codec_transform_publication_source_tests.cpp'
SOURCE_OR = 'physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,s_.flags.logic(prior|mask,4u));retired(pc);'
SOURCE_REP = '''            physics_step_detail::w(s_.memory,s_.observe,pc,g[7],4u,value);
            const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
            // Operand preparation retags the outer statement only. The
            // authored trailing callback is direct, even after ECX reaches 0.
            g[6]+=delta;g[7]+=delta;--g[1];retired(pc);direct(pc);'''
PREPARED = ('c4a517a986cba653253c7ffb13034c781b58313e312160885ecb39d75176b303',
            '2dc77ca66f96c1c4816079ac326be10cfbcf0b3992dd68667c8436af9ce62531')
STATE = ('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
         '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575')
RESOLVER = ('1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
            '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657')
SIGN_EXTEND = 'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b'
TRAILING = 'retired(pc);direct(pc);'
DEFECTS = (
    ('late-source-rmw-flags',SOURCE_OR,
     'auto committed=s_.flags;const auto bits=committed.logic(prior|mask,4u);'
     'physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,bits);s_.flags=committed;retired(pc);'),
    ('early-source-repeat-cursors',SOURCE_REP,
     '''            const auto destination=g[7];
            const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
            g[6]+=delta;g[7]+=delta;--g[1];
            physics_step_detail::w(s_.memory,s_.observe,pc,destination,4u,value);retired(pc);direct(pc);'''),
    ('missing-final-source-repeat-callback',TRAILING,'retired(pc);if(g[1])direct(pc);'),
    ('incorrectly-retagged-trailing-repeat',TRAILING,'retired(pc);at(pc);'),
    ('missing-pure-source-callback','io_.note(0x283fb5u);g[2]=1u;','g[2]=1u;'),
    ('late-observer-from-empty-entry',SOURCE_OR,
     '''const bool initially_observed=bool(s_.observe);const auto bits=s_.flags.logic(prior|mask,4u);
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,bits);
        if(!initially_observed&&s_.observe){
            SourcePhysicsContactRecordStore event{pc,address,{},{}};
            for(unsigned i=0;i<4u;++i)event.written.push_back(std::byte(bits>>(i*8u)));
            s_.observe(event);
        }
        retired(pc);'''),
)


def retained_sources(code,scratch):
    blocks,contexts=[],{}
    scoped={pc for pc in _trace(code,ENTRY,set(ENTRIES)).code if START<=pc<END}
    for index,(name,pin) in enumerate(OWNERS.items()):
        text=(ROOT/name).read_text()
        prepared,_=specialize(text)
        raw_body=exact(text,r'static void run_283e10\(WorldState &s\) \{.*?\n\}')
        mapping=dict(CASE.findall(MAPPING.search(text)['cases']))
        def identity(match):
            operand=match[1]
            if mapping.get(operand)!=operand:
                raise ValueError('Nonidentity operand in scoped prepared publication owner: '+operand)
            return f'{operand}u'
        projection=INSTRUCTION.sub(lambda m:f's.instruction_literal({m[1]});',CALL.sub(identity,raw_body))
        for format_index,source in enumerate((text,prepared)):
            body=exact(source,r'static void run_283e10\(WorldState &s\) \{.*?\n\}')
            state=exact(source,r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;')
            resolver=exact(source,r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}')
            sign_extend=exact(source,r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}')
            digest=lambda value:hashlib.sha256(value.encode()).hexdigest()
            if (digest(body),digest(state),digest(resolver),digest(sign_extend))!=(
                    PREPARED[index] if format_index else pin,STATE[index],RESOLVER[index],SIGN_EXTEND):
                raise ValueError('Exact retained publication owner tuple changed: '+name)
            if format_index and projection!=body:raise ValueError('Prepared publication differs outside exact identity operands and callbacks')
            before=lambda pc:(f's.instruction_literal(0x{pc:x}u);' if format_index else
                              f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}')
            first=before(START);end='L28404a: '+before(END)
            if body.count(first)!=1 or body.count(end)!=1:raise ValueError('Exact retained publication cutoff drift')
            suffix=body[body.index(first):body.index(end)]
            pcs={int(pc,16)for pc in re.findall(r's\.(?:services\.instruction|instruction_literal)\(0x([0-9a-f]+)u\)',suffix)}
            trailing='if(s.services.instruction)s.services.instruction(0x283fa7u);'
            if pcs!=scoped or len(scoped)!=60 or suffix.count(trailing)!=1:
                raise ValueError('Exact retained publication callback set changed')
            blocks.append(f'namespace publication_source{2*index+format_index}{{using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
                +state+'\n'+resolver+'\n'+sign_extend+'\nstatic void legacy(WorldState &s) {\n'+suffix+'L28404a: return;\n}\n}\n')
            contexts[name+(':prepared' if format_index else ':raw')]=dict(body=digest(body),suffix=digest(suffix),
                state=digest(state),resolver=digest(resolver),sign_extend=digest(sign_extend))
    generated=''.join(blocks);(scratch/'codec_transform_publication_sources.inc').write_text(generated)
    return contexts,hashlib.sha256(generated.encode()).hexdigest()


def capture(image,code,scratch,smoke):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=ENTRIES
    # This execution supplies only actual live input packets and backing
    # bytes at the selected PC. Retained helper I/O is compared separately.
    rows=[row for row in publication_rows(image,smoke) if '-packed-chain-' in row['name']]
    for index,row in enumerate(rows):row.update(id=index,stop_before_entries=[START])
    folder=scratch/'physical-suffix-inputs';folder.mkdir()
    actual,proof=physical_original_full(image,rows,folder,entries=ENTRIES,regions=ALL_REGIONS,trace_extended=False)
    result=[]
    for row,observed in zip(rows,actual['cases'],strict=True):
        if not observed['completed'] or observed['stopped_at']!=START or observed['boundary_live_x87'] or 'active_mmx'in observed:
            raise ValueError('Authentic packed parent did not reach the source publication input')
        if not {0x283f1b,0x283f76,0x120570}<=set(observed['executed_sites']):
            raise ValueError('Authentic source publication input lacked EMMS or real returned child')
        result.append(dict(name=row['name'],gp=observed['boundary_gp'],xmm=observed['boundary_xmm'],
            eflags=observed['boundary_eflags'],fp=observed['after_fp'],
            banks=[dict(address=bank['address'],before=bank['after'])for bank in observed['banks']]))
    sites=[site for site in common.metadata(image)['sites']if START<=site['instruction']<END and site['operands']]
    successors={site['instruction']:site['instruction'] if site['instruction']==REPEAT else
                site['instruction']+code.at(site['instruction']).size for site in sites}
    stores=[site['instruction']for site in sites if site['instruction']not in RMW|{REPEAT} and any(op['write']for op in site['operands'])]
    if not RMW|{REPEAT}<=successors.keys():raise ValueError('Retained OR/REP publication metadata missing')
    return dict(cases=result,successors=successors,write_sites=stores),proof


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append')
    for option in ('smoke','sanitizers','semantic-mutants'):p.add_argument('--'+option,action='store_true')
    args=p.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','src','include','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/FRAME,ROOT/KERNEL,ROOT/TEST,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                exact_source_suffix='283F7B through publication; stop before 28404A',
                readonly_gp_fp_metadata_host_controls=True,
                source_observer_replacement_covered=True,source_rmw_flags_before_preimage=True,
                original_input_only=True,physical_post_read_write_provider_failure_qualified=False)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners=original_owners()
        contexts,source=retained_sources(code,scratch)
        inputs,original=capture(image,code,scratch,args.smoke)
        report.update(contexts=contexts,generated_source_sha256=source,original_capture=original,
                      authentic_suffix_inputs=len(inputs['cases']),successors=inputs['successors'])
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(inputs,separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/TEST),'-o',str(binary),
                 '-MD','-MF',str(dep)]
            if args.sanitizers:cmd.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(cmd,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Retained publication compile failed: '+done.stderr[-12000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Retained publication comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[])
            report['compilers'].append(result)
            print(f'{compiler} retained publication suffix passed',flush=True)
            if args.semantic_mutants:
                for name,old,new in DEFECTS:
                    text=(ROOT/FRAME).read_text()
                    if text.count(old)!=1:raise ValueError('Retained publication defect seam changed: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name)
                    header=overlay/FRAME
                    header.parent.mkdir(parents=True);header.write_text(text.replace(old,new,1))
                    mutant=overlay/'probe'
                    changed=[cmd[0],'-I'+str(overlay/'src'),*cmd[1:]]
                    changed[changed.index('-o')+1]=str(mutant);changed[changed.index('-MF')+1]=str(overlay/'probe.d')
                    done=subprocess.run(changed,capture_output=True,text=True,timeout=480)
                    if done.returncode:raise ValueError('Retained publication mutant compile failed: '+name+' '+done.stderr[-8000:])
                    done=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=480)
                    diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'Retained publication packets differ' not in diagnostic:
                        raise ValueError('Retained publication defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected retained publication defect {name}',flush=True)
        if original_owners()!=owners or retained_sources(code,scratch)!=(contexts,source):
            raise ValueError('Exact retained publication owner drift')
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        used.update(Path(module.__file__).resolve()for module in tuple(sys.modules.values())
            if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):before[path]for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name)for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
