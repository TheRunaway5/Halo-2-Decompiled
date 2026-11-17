#!/usr/bin/env python3
"""Qualify staged 293290 source binding through the genuine format-four caller."""
import argparse
import copy
import json
import os
import platform
import re
import shlex
import subprocess
import sys
from pathlib import Path

import audit_native_codec_format_four_owners_tu15 as audit
import native_codec_format_four_decode_replacements as binding
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT
from specialize_native_operands import specialize
from validate_native_codec_format_four_decode_tu15 import inputs

ROOT = audit.ROOT
TEST = ROOT/'tests/native_tu15_codec_format_four_source_tests.cpp'
MODEL = 'src/rendering/models/source_model_render_tu15_generated.cpp'
TARGETS = (0x2963d0,0x296410,0x2cec60)
SOURCE_FRAME = 'src/physics/runtime/native_tu15_codec_format_four_source_frame.hpp'
SOURCE_OWNER = 'src/physics/runtime/native_tu15_codec_format_four_source_owner.hpp'
DISPATCH_SOURCES = tuple(ROOT/'src/physics'/name for name in (
    'native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp',
    'native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp','native_tu15_physical_epoch.cpp',
))
DEFECTS = (
    (SOURCE_FRAME,'uncommitted-push','at(pc);s_.push(pc,value);retired(pc);',
     'at(pc);physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[4]-4u,4u,value);s_.gp[4]-=4u;retired(pc);'),
    (SOURCE_FRAME,'missing-returned-child-handoff','retired(p.call);','(void)p.call;'),
    (SOURCE_OWNER,'swallowed-foreign-continuation','if(resume.owner!=&token)throw;','(void)token;'),
    (SOURCE_FRAME,'wrong-repeated-translation-store','store_scalar(0x2934a3u,g[0]+16u,1u);',
     'store_scalar(0x2934a3u,g[0]+20u,1u);'),
)


def raw_body(source,pc):
    return binding.exact(re.compile(rf'^static void run_{pc:x}\(WorldState &s\) \{{.*?^\}}',re.S|re.M),source)[0]


def instrument(body):
    pattern=re.compile(r'(?=if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}|s\.instruction_literal\((0x[0-9a-f]+u)\);)')
    tagged,count=pattern.subn(lambda m:'format_four_retained('+(m[1]or m[2])+');',body)
    if count!=221:raise ValueError('Incomplete exact format-four retained instrumentation')
    return tagged


def route_projection(dispatcher,source,local):
    # The exact full dispatcher was already authenticated. Keep its authored
    # default and only the arms reachable in this row-eight fixture.
    arms=[audit.dispatch_arm(dispatcher,pc)for pc in local]
    if not all(arms):raise ValueError('Missing real row-eight child dispatch arm')
    signature=dispatcher[:dispatcher.index('{')+1]
    if 'switch(target){' in dispatcher:
        if dispatcher.count('default:')!=1 or not dispatcher.endswith('s.required(site,target,tail);return;}}'):
            raise ValueError('Exact real source dispatcher default changed')
        projected=signature+'switch(target){'+''.join(arms)+dispatcher[dispatcher.index('default:'):]
    else:
        if not dispatcher.endswith('s.required(site,target,tail);}'):
            raise ValueError('Exact real source dispatcher required-owner tail changed')
        projected=signature+''.join(arms)+'s.required(site,target,tail);}'
    return '\n'.join(raw_body(source,pc)for pc in local)+'\n'+projected


def namespace(name,source,body,local,candidate):
    _,resolver,dispatcher,state=audit.parts(source)
    caller=binding.exact(binding.CALLER,source)[0]
    uncalled={int(pc,16)for pc in re.findall(r'run_([0-9a-f]+)\(s\);return;',caller)}-{audit.ENTRY}
    stubs='\n'.join(f'static void run_{pc:x}(WorldState&){{throw std::logic_error("Fixture left genuine 293290 tail");}}'
                    for pc in sorted(uncalled))
    return '\n'.join((f'namespace {name} {{ using namespace halo2;using namespace halo2::physics_world_runtime_detail;',
        state,resolver,stubs,route_projection(dispatcher,source,local),instrument(body)if candidate else body,caller,
        'static void execute(WorldState& s){run_284290(s);}','}'))


def external_children(primary,model,literal):
    def external(name,source,pcs,export):
        resolver=binding.exact(binding.RESOLVER,source)[0];state=binding.exact(binding.STATE,source)[0]
        # Preserve the actual export's scoped FP/ABI construction, selected
        # case and exception publication. Authenticate the owning excerpt.
        start=source.index(export+'(M &m,std::uint32_t entry,')
        wrapper=source[start:source.index('\n}',start)+2]
        if not all(wrapper.count(f'case {pc:#x}u:run_{pc:x}(s);break;')==1 for pc in pcs):
            raise ValueError('Missing genuine external row-eight entry case')
        stack='FpScope scope(fp);LiveState s{m,observe,services,image};s.load(abi);'
        finish='} catch(...) {abi=s.save();throw;}\n    abi=s.save();'
        if stack not in wrapper or finish not in wrapper:
            raise ValueError('External row-eight ABI wrapper publication changed')
        body='\n'.join(raw_body(source,pc)for pc in pcs)
        arms=''.join(f'case {pc:#x}u:run_{pc:x}(s);break;'for pc in pcs)
        return '\n'.join((f'namespace {name} {{ using namespace halo2;using namespace halo2::physics_world_runtime_detail;',
            state,resolver,body,
            'static void execute(M&m,std::uint32_t entry,SourcePhysicsContactRuntimeAbi&abi,FP fp,',
            '    const SourcePhysicsContactRuntimeServices&services,const O&observe){',
            'constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;',stack,
            'try {switch(entry){'+arms+'default:throw std::logic_error("Unsupported real row-eight provider entry");}',
            finish,'}','}'))
    return external('format_primary_child_'+str(int(literal)),primary,TARGETS[:2],'source_run_physics_live_support_tu15')+'\n'+external(
        'format_model_child_'+str(int(literal)),model,TARGETS[2:],'source_run_model_render_tu15')


def reject(source,controls):
    for name,changed in controls.items():
        if changed==source:raise ValueError('Missing exact format-four mutation seam '+name)
        try:binding.installed_entries(changed)
        except ValueError:continue
        raise ValueError('Accepted strict format-four source mutation '+name)


def prepare(scratch,report,syntax,current_only):
    if not current_only:
        measured=dict(owners=[]);audit.measure(scratch,measured);report['generation']=measured
    else:
        for owner,path,*_ in audit.UNITS:
            text=(ROOT/path).read_text();(scratch/f'{owner}-current-raw.cpp').write_text(text)
            (scratch/f'{owner}-current-prepared.cpp').write_text(specialize(text)[0])
    model=(ROOT/MODEL).read_text()
    blocks=[];variants=[]
    for literal,form in enumerate(('raw','prepared')):
        primary=(scratch/f'primary-current-{form}.cpp').read_text()
        blocks.append(external_children(primary,specialize(model)[0]if literal else model,bool(literal)))
    for owner,*_ in audit.UNITS:
        for stage in ('current',)if current_only else ('current','fresh'):
            for literal,form in enumerate(('raw','prepared')):
                text=(scratch/f'{owner}-{stage}-{form}.cpp').read_text()
                if binding.authenticate(text)[0]!=(owner,stage,form):raise ValueError('Wrong exact 293290 proof context')
                bound=binding.replace_format_four_decode(text)
                if binding.remove_format_four_decode(bound)!=text or binding.replace_format_four_decode(bound)!=bound or binding.installed_entries(bound)!=['293290']:
                    raise ValueError('Format-four staged default failed byte-exact reversal or idempotence')
                if not literal:
                    prepared=specialize(bound)[0];independent=specialize(text)[0]
                    if binding.retag_specialized(bound,prepared)!=binding.replace_format_four_decode(independent):
                        raise ValueError('Authentic raw-to-prepared format-four overlay differs')
                body=binding.exact(binding.BODY,bound)[0];entry=binding.PREFIXES[literal];label=binding.name(binding.SITES[0])+': '
                caller=binding.exact(binding.CALLER,bound)[0]
                reject(bound,{'wrong-tag':bound.replace(entry,binding.PREFIXES[not literal],1),
                              'missing-label':bound.replace(label,'',1),'missing-include':bound.replace(binding.INCLUDE,'',1),
                              'foreign-call':bound+'\n// '+binding.OWNER,'foreign-label':bound+'\n// '+binding.LABEL,
                              'caller-drift':bound.replace(caller,caller.replace('run_293290(s);return;','run_293290(s); return;',1),1),
                              'resolver-drift':bound.replace('switch(p)','switch((p))',1),
                              'wrapper-drift':bound.replace('using physics_world_runtime_detail::WorldState::WorldState;','using physics_world_runtime_detail::WorldState::WorldState; ',1),
                              'owner-body-drift':bound.replace(body,body.replace('0x293290u','0x293291u',1),1),
                              'ordered-site':bound.replace(body,body.replace('halo2::'+binding.MATH+'::multiply_packed(','_mm_mul_ps(',1),1)})
                for pc in binding.SITES:
                    label=binding.name(pc)+': '
                    moved=body.replace(label,'',1).replace(entry,entry+'\n'+label,1)
                    reject(bound,{'relocated-successor':bound.replace(body,moved,1)})
                if syntax:
                    file=scratch/f'{owner}-{stage}-{form}-proposed.cpp';file.write_text(bound)
                    command=[syntax,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(file)]
                    done=subprocess.run(command,capture_output=True,text=True,timeout=450)
                    if done.returncode:raise ValueError('Complete proposed 293290 unit syntax failed: '+done.stderr[-10000:])
                index=len(variants)
                local=TARGETS[:2]if owner=='primary'else ()
                ordered_reference=binding.ordered_body(binding.exact(binding.BODY,text)[0],bool(literal))
                blocks.extend((namespace(f'format_bound_{index}',text,body,local,True),
                               namespace(f'format_reference_{index}',text,ordered_reference,local,False)))
                variants.append(dict(owner=owner,stage=stage,form=form,literal=bool(literal),
                                     strict_context_mutants=10+len(binding.SITES),whole_proposed_cpp_syntax=bool(syntax)))
    macros='\n#define FORMAT_FOUR_VARIANTS(X) '+' '.join(
        f'X({i},{int(v["literal"])},{int(v["owner"]=="primary")})'for i,v in enumerate(variants))+'\n'
    (scratch/'codec_format_four_sources.inc').write_text('\n'.join(blocks)+macros)
    return variants


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append')
    parser.add_argument('--whole-syntax')
    for option in ('smoke','sanitizers','current-only','semantic-mutants'):parser.add_argument('--'+option,action='store_true')
    parser.add_argument('--only-defect',action='append',choices=[name for _,name,_,_ in DEFECTS])
    args=parser.parse_args()
    if args.only_defect and args.semantic_mutants:parser.error('Use one semantic defect selection mode')
    if 'HALO2_TEST_SCRATCH' not in os.environ:parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    paths={path.resolve():audit.sha(path.read_bytes())for tree in ('tools','src','include','tests')for path in (ROOT/tree).rglob('*')
           if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/SOURCE_FRAME,ROOT/SOURCE_OWNER,ROOT/MODEL,TEST,*DISPATCH_SOURCES,*(ROOT/path for _,path,*_ in audit.UNITS)}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                ordered_sse_sites=[hex(pc)for pc in binding.ORDERED],
                source_projection='Complete retained body with only the eleven original destination-ordered SSE expressions')
    try:
        report['variants']=prepare(scratch,report,args.whole_syntax,args.current_only)
        rows=inputs(Xbe(NEW_DEFAULT),args.smoke)
        fixture=scratch/'format-four-source.json'
        fixture.write_text(json.dumps(rows,separators=(',',':')))
        negative=scratch/'format-four-source-mutants.json'
        defects=DEFECTS if args.semantic_mutants else tuple(defect for defect in DEFECTS if defect[1] in (args.only_defect or ()))
        if defects:negative.write_text(json.dumps(rows[:16],separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),
                     *map(str,DISPATCH_SOURCES),str(TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(command,capture_output=True,text=True,timeout=650)
            if done.returncode:raise ValueError('Actual format-four source compile failed: '+done.stderr[-14000:])
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path in paths:used.add(path)
            modes={}
            for mode in ('default',) if args.only_defect else ('default','forced-reference','unrelated-control'):
                env=os.environ.copy();env.pop('HALO2_REFERENCE_CODEC_FORMAT_FOUR_DECODE',None)
                if mode=='forced-reference':env['HALO2_REFERENCE_CODEC_FORMAT_FOUR_DECODE']='1'
                if mode=='unrelated-control':env['HALO2_REFERENCE_PACKED_ROTATION']='1'
                done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env,timeout=650)
                if done.returncode:raise ValueError('Actual format-four source differs in '+mode+': '+(done.stdout+done.stderr)[-12000:])
                modes[mode]=json.loads(done.stdout)
                print(f'{compiler} format-four {mode}: {modes[mode]["comparisons"]} comparisons passed',flush=True)
            result=dict(compiler=compiler,modes=modes,compiled_defects=[]);report['compilers'].append(result)
            print(compiler+' actual format-four source routes passed',flush=True)
            if defects:
                env=os.environ.copy();env.pop('HALO2_REFERENCE_CODEC_FORMAT_FOUR_DECODE',None)
                for subject,name,old,new in defects:
                    source=(ROOT/subject).read_text()
                    if source.count(old)!=1:raise ValueError('Missing exact format-four compiled mutation seam '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);header=overlay/subject
                    header.parent.mkdir(parents=True);header.write_text(source.replace(old,new,1));probe=overlay/'probe'
                    command_copy=[command[0],'-I'+str(overlay/'src'),*command[1:]]
                    command_copy[command_copy.index('-o')+1]=str(probe);command_copy[command_copy.index('-MF')+1]=str(overlay/'probe.d')
                    done=subprocess.run(command_copy,capture_output=True,text=True,timeout=650)
                    if done.returncode:raise ValueError('Format-four source mutant compile failed: '+done.stderr[-12000:])
                    done=subprocess.run([str(probe),str(negative)],capture_output=True,text=True,env=env,timeout=650)
                    diagnostic=done.stdout+done.stderr
                    markers=('Source decoder resumed after a foreign exception',) if name=='swallowed-foreign-continuation' else (
                        'Format-four source differs','Wrong source decoder postchild successor',
                        'CombatAction decoder missed returned-provider policy change')
                    if not done.returncode or not any(marker in diagnostic for marker in markers):
                        raise ValueError('Format-four source mutant survived '+name+': '+diagnostic[-9000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-9000:]))
                    print(compiler+' rejected format-four source defect '+name,flush=True)
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        used.update(Path(module.__file__).resolve() for module in tuple(sys.modules.values())
                    if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):paths[path]for path in sorted(used)}
        report['source_after']={name:audit.sha((ROOT/name).read_bytes())for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
