#!/usr/bin/env python3
"""Genuine packed-rotation owner generation/context audit and whole-unit syntax."""
import argparse,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def parts(source):
    from native_packed_rotation_source_inputs import function,HELPERS
    body=function(source,'run_296180');resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)
    if not resolver or not dispatcher or not state:raise ValueError('Missing complete generated packed rotation context')
    for helper,pin in HELPERS.items():
        if digest(function(source,helper))!=pin:raise ValueError('Fresh packed rotation helper drift '+helper)
    return body,resolver[0],dispatcher[0],state[0]
def dispatcher_map(resolver,dispatcher):
    selected={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    case=r'case 0x([0-9a-f]+)u:run_([0-9a-f]+)\(s\);return;'
    branch=r'if\(target==selected\(0x([0-9a-f]+)u,s.image\)\)\{run_([0-9a-f]+)\(s\);return;\}'
    head='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail)'
    if not dispatcher.startswith(head):raise ValueError('Different full dispatcher call ABI')
    body=dispatcher[len(head):]
    if re.fullmatch(r'\{switch\(target\)\{(?:'+case+r')+default:s.required\(site,target,tail\);return;\}\}',body):rows=[(int(a,16),b)for a,b in re.findall(case,body)]
    elif re.fullmatch(r'\{(?:'+branch+r')+s.required\(site,target,tail\);\}',body):
        rows=[]
        for a,b in re.findall(branch,body):
            if int(a,16)not in selected:raise ValueError('Selected dispatch target missing')
            rows.append((selected[int(a,16)],b))
    else:raise ValueError('Unknown complete dispatcher/fallback grammar')
    result=dict(rows)
    if len(result)!=len(rows):raise ValueError('Duplicate generated dispatcher targets')
    return result
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='clang++');parser.add_argument('--candidate-preparation',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    os.chdir(ROOT);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    from native_packed_rotation_source_inputs import UNITS,CONTEXTS,canonical_body,authenticate
    from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER
    from validate_native_camera_instance_bitmap_generation_tu15 import generate_ai
    source_pins={name:sha(ROOT/name)for name in UNITS};tool_pins={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'tools').rglob('*.py')};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,audit_only=True,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],units=[],source_before=source_pins);used=set();dependencies=set()
    try:
        for index,name in enumerate(('primary','ai','model')):
            output=scratch/(name+'.cpp');row=dict(owner=name,canonical=UNITS[index]);report['units'].append(row)
            if name=='ai':command,provenance=generate_ai(scratch);row['genuine_references']=provenance
            else:
                script='generate_physics_live_support_tu15.py'if name=='primary'else'generate_model_render_tu15.py'
                command=[sys.executable,'-B',str(ROOT/'tools'/script),'--output',str(output),'--manifest'if name=='primary'else'--directory',str(scratch/(name+'-generation'))]
                imported=scratch/(name+'-imports.json');done=subprocess.run([sys.executable,'-B','-c',CHILD_WRAPPER,str(imported),*command[2:]],capture_output=True,text=True,timeout=300)
                if done.returncode:raise ValueError('Real packed rotation owner generation failed '+(done.stdout+done.stderr)[-7000:])
                used.update(str(Path(value).relative_to(ROOT))for value in json.loads(imported.read_text())if Path(value).is_relative_to(ROOT/'tools'))
            fresh=output.read_text();current=(ROOT/UNITS[index]).read_text();owner,_=authenticate(current)
            if owner!=index:raise ValueError('Canonical packed rotation owner mismatch')
            before=parts(current);after=parts(fresh)
            projected=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',canonical_body(before[0]))
            if canonical_body(after[0])not in(canonical_body(before[0]),projected):raise ValueError('Fresh complete packed rotation body has more than identity-selected projection')
            if digest(after[3])not in{context[3]for context in CONTEXTS}:raise ValueError('Fresh complete LiveState is not an already-pinned class')
            old_map=dispatcher_map(before[1],before[2]);new_map=dispatcher_map(after[1],after[2]);extra_fa=False
            if new_map!=old_map:
                from native_halfword_bit_query_replacements import canonical_body as fa_canonical,PIN as FA_PIN
                from native_packed_rotation_source_inputs import function
                if name!='primary'or set(new_map)-set(old_map)!={0xfa5d0}or new_map[0xfa5d0]!='fa5d0'or digest(fa_canonical(function(fresh,'run_fa5d0')))!=FA_PIN:raise ValueError('Fresh dispatcher target map changed')
                reduced=dict(new_map);del reduced[0xfa5d0]
                if reduced!=old_map:raise ValueError('Fresh dispatcher changes beyond exact FA owner')
                extra_fa=True
            dep=scratch/(name+'.d');syntax=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(output)]
            done=subprocess.run(syntax,capture_output=True,text=True,timeout=300)
            if done.returncode:raise ValueError('Whole fresh packed rotation CPP syntax failed '+done.stderr[-7000:])
            dependencies.update(Path(value).resolve()for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]))
            row.update(passed=True,command=command,context_sha256=list(map(digest,after)),current_context_sha256=list(map(digest,before)),all_dispatch_targets_preserved=True,exact_extra_fa5d0=extra_fa,whole_unit_equal=fresh==current,whole_unit_syntax=syntax)
            if args.candidate_preparation:
                from native_packed_rotation_replacements import replace_packed_rotation,remove_packed_rotation,PREFIX,INCLUDE
                from specialize_native_operands import specialize
                row['candidates']=[]
                for stage,text in(('current',current),('fresh',fresh)):
                    bound=replace_packed_rotation(text)
                    if args.require_binding and bound!=text:raise ValueError('Packed rotation current/fresh default is not installed')
                    rebound=replace_packed_rotation(remove_packed_rotation(bound))
                    # Later owning wrappers can prepend their existing includes
                    # after this common hook. Restore only our exact include to
                    # its authored location; every other byte must reconstruct.
                    if rebound.count(INCLUDE)!=1 or bound.count(INCLUDE)!=1:raise ValueError('Packed exact include reversal changed ownership')
                    include_at=bound.index(INCLUDE);without_include=rebound.replace(INCLUDE,'',1)
                    rebound=without_include[:include_at]+INCLUDE+without_include[include_at:]
                    if replace_packed_rotation(bound)!=bound or rebound!=bound:raise ValueError('Packed candidate exact reversal/idempotence failed')
                    baseline,old_receipt=specialize(text);prepared,receipt=specialize(bound)
                    previous={key:value for key,value in old_receipt.items()if'entries'in key and key!='native_packed_rotation_entries'}
                    if previous!={key:receipt[key]for key in previous}:raise ValueError('Packed candidate changed prior preparation owner groups')
                    if receipt.get('native_packed_rotation_entries')!=['296180']:raise ValueError('Prepared packed rotation own-entry receipt missing')
                    prepared_parts=parts(prepared)
                    if prepared_parts[0].count(PREFIX)!=1 or prepared.count(INCLUDE)!=1:raise ValueError('Prepared packed candidate prefix/include missing')
                    candidate_path=scratch/(name+'-'+stage+'-bound.cpp');candidate_path.write_text(bound)
                    prepared_path=scratch/(name+'-'+stage+'-prepared.cpp');prepared_path.write_text(prepared)
                    syntax_commands=[]
                    for candidate_path in(candidate_path,prepared_path):
                        dep=candidate_path.with_suffix('.d');compile_command=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(candidate_path)]
                        done=subprocess.run(compile_command,capture_output=True,text=True,timeout=300)
                        if done.returncode:raise ValueError('Whole raw/prepared packed candidate CPP failed '+done.stderr[-7000:])
                        dependencies.update(Path(value).resolve()for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]));syntax_commands.append(compile_command)
                    row['candidates'].append(dict(stage=stage,prepared_context_sha256=list(map(digest,prepared_parts)),prior_owner_groups=previous,whole_cpp_syntax=syntax_commands,exact_reversal=True,idempotent=True))
        used.update(str(Path(module.__file__).resolve().relative_to(ROOT))for module in tuple(sys.modules.values())if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        used.update(('tools/native_packed_rotation_source_inputs.py','tools/native_model_marker_lookup_replacements.py','tools/native_halfword_bit_query_replacements.py'))
        if args.candidate_preparation:used.update(('tools/native_packed_rotation_replacements.py','tools/validate_native_packed_rotation_value_tu15.py'))
        inputs={**source_pins,**{name:tool_pins[name]for name in used},**{str(path.relative_to(ROOT)):headers[path]for path in dependencies if path in headers}}
        changed=[name for name,pin in inputs.items()if sha(ROOT/name)!=pin]
        if changed:raise ValueError('Packed rotation generation inputs changed '+repr(changed))
        report.update(passed=True,source_after=inputs,prepared_or_default_execution_qualified=False)
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
