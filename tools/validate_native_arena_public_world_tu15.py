#!/usr/bin/env python3
"""Build one frozen private candidate and exercise the actual public World.

The same owned runtime contains native and retained Arena paths. Only frozen
copies get proof instrumentation; launcher selection and original refs stay
untouched. Both production scheduling hooks remain empty.
"""
import argparse,hashlib,importlib.util,json,os,re,shutil,subprocess,sys,time
from pathlib import Path
from build_launcher_engine import freeze_source,source_hashes
from collect_native_production_closure import prepare_peer_source
from native_metadata import retain_bytes,FILE_LIMIT
import native_arena_initialize_continuation_replacements as binding
import native_arena_world_producer_inputs as world_inputs
import native_arena_world_policy_prototype as publication
ROOT=Path(__file__).resolve().parents[1]
HEADER='tests/native_tu15_arena_public_world_proof.hpp'
INCLUDE='#include "../../'+HEADER+'"\n'
WORLD=world_inputs.WORLD
ARENA='src/physics/physics_game_arena_native.cpp'
UNIT='src/physics/physics_game_arena_runtime_generated.cpp'
def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def reference_inventory(directory):
    files={};resolved_files=[]
    def visit(current,relative,ancestors):
        resolved=current.resolve(strict=True)
        if resolved in ancestors:raise ValueError('Borrowed reference directory cycle '+str(current))
        resolved_files.append(resolved)
        for child in sorted(current.iterdir()):
            if child.name in('__pycache__','.pytest_cache'):continue
            name=relative/child.name
            if child.is_dir():
                files[name.as_posix()]=dict(kind='directory',bytes=0,sha256=None)
                visit(child,name,ancestors|{resolved})
            elif child.is_file():
                files[name.as_posix()]=dict(kind='file',bytes=child.stat().st_size,sha256=sha(child))
                resolved_files.append(child.resolve(strict=True))
            else:raise ValueError('Borrowed reference is neither a file nor directory '+str(child))
    visit(Path(directory),Path(),set())
    return files,resolved_files
def other_scratch_owner(path,owner):
    path=Path(path).resolve(strict=True)
    if path.is_relative_to(owner):return False
    parts=path.parts
    return any(parts[n:n+2]==('.local','test-scratch')for n in range(len(parts)-1))or any(part.startswith('halo2-test-')for part in parts)
def canonicalize_borrowed_links(source):
    """Borrow original references directly, never through a disposable donor.

    freeze_source deliberately borrows these directories. A second freeze
    otherwise points through the first owner's tree, whose removal can break
    a later legitimate incremental CMake configure after a successful build.
    """
    source=Path(source).resolve(strict=True)
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if not source.is_relative_to(owner):raise ValueError('Reference repair requires the current owned source copy')
    paths=[source/name for name in('assets','resources','analysis','.tools','Halo 2 Vista installer')]
    for name in('tests','multiplayer'):
        parent=source/name
        if parent.is_dir():paths.extend(parent.iterdir())
    rows=[];copied_bytes=0
    for path in paths:
        if not path.is_symlink():continue
        previous=str(path.readlink());resolved=path.resolve(strict=True)
        if not resolved.is_dir():raise ValueError('Borrowed reference directory changed type '+str(path))
        relative=path.relative_to(source);is_fixture=relative.parts[0]in('tests','multiplayer')
        row=dict(path=str(relative),previous_target=previous,borrowed_source=str(resolved))
        files,dependencies=reference_inventory(resolved)if is_fixture else({},[])
        temporary=other_scratch_owner(resolved,owner)or any(other_scratch_owner(p,owner)for p in dependencies)
        if temporary:
            if not is_fixture:raise ValueError('Nonfixture reference still depends on another disposable owner '+str(resolved))
            candidate_path=ROOT/relative
            candidate=candidate_path.resolve(strict=True)if candidate_path.is_dir()else None
            candidate_files,candidate_dependencies=reference_inventory(candidate)if candidate else(None,[])
            if candidate and files==candidate_files and not other_scratch_owner(candidate,owner)and not any(other_scratch_owner(p,owner)for p in candidate_dependencies):
                if reference_inventory(resolved)[0]!=files or reference_inventory(candidate)[0]!=files:
                    raise ValueError('Borrowed reference changed while authenticating stable Root '+str(relative))
                resolved=candidate;row['matched_stable_root_exactly']=True
            else:
                copied_bytes+=sum(item['bytes']for item in files.values())
                if copied_bytes>2*1024**3:raise ValueError('Required owned reference copies exceed the declared2GiB input bound')
                staging=source.parent/('arena-owned-reference-'+str(len(rows)))
                staging.mkdir()
                for name,item in files.items():
                    target=staging/name
                    if item['kind']=='directory':target.mkdir(parents=True,exist_ok=True)
                    else:target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(resolved/name,target)
                if reference_inventory(staging)[0]!=files or reference_inventory(resolved)[0]!=files:
                    raise ValueError('Exact owned reference materialization differs '+str(relative))
                path.unlink();staging.replace(path);resolved=path.resolve(strict=True)
                row['materialized_owned_copy']=True
        if other_scratch_owner(resolved,owner):raise ValueError('Reference still resolves through another disposable owner '+str(resolved))
        if not row.get('materialized_owned_copy'):
            path.unlink();path.symlink_to(resolved,target_is_directory=True)
        if path.resolve(strict=True)!=resolved:raise ValueError('Borrowed reference canonicalization differs '+str(path))
        if is_fixture:
            row['files']=files;row['reference_bytes']=sum(item['bytes']for item in files.values())
            row['tree_sha256']=hashlib.sha256(json.dumps(files,sort_keys=True,separators=(',',':')).encode()).hexdigest()
            if any(other_scratch_owner(p,owner)for p in reference_inventory(path)[1]):
                raise ValueError('Nested reference still resolves through another disposable owner '+str(path))
        row['original_owner']=str(resolved);rows.append(row)
    return rows
def audit_generated_inputs(build,owner):
    """Reject foreign owners in actual CMake/compiler input path spellings.

    An owned source tree alone does not authenticate a cached include or a
    dependency path which resolves through a different disposable owner.
    Missing foreign paths also refuse: their disappearance cannot erase the
    recorded dependency. This reads generated metadata, never build products.
    """
    owner=Path(owner).resolve(strict=True);build=Path(build).resolve(strict=True)
    if not build.is_relative_to(owner):raise ValueError('Generated-input audit requires the current owned build')
    raw=[]
    for item in sorted(build.rglob('*')):
        if not item.is_file()or not(item.name in('CMakeCache.txt','Makefile','build.ninja','rules.ninja','link.txt','compile_commands.json')or item.suffix in('.cmake','.d','.make','.rsp')):continue
        raw.append((str(item.relative_to(build)),item.read_bytes()))
    return audit_generated_records(raw,owner)

def audit_generated_records(raw_records,owner):
    # The same parser can authenticate a read-only, in-memory metadata capture
    # taken before the outer disposable owner is removed. It confers no source
    # or build ownership; callers retain the capture's exact stage/provenance.
    owner=Path(owner).resolve(strict=False)
    records={};external={};paths=set();spellings=set()
    for item,raw in raw_records:
        if item in records:raise ValueError('Duplicate captured build metadata path '+item)
        records[item]=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())
        # The scratch-owner portion has no whitespace. Checking every absolute
        # spelling also catches quoted CMake lists and escaped Make depfiles.
        for spelling in re.findall(rb'/[^\s\x00\x22\x27;<>|()]+',raw):
            name=spelling.decode(errors='surrogateescape').replace('\\ ', ' ').rstrip(',:')
            if name in spellings:continue
            spellings.add(name)
            path=Path(name)
            if not path.is_absolute():continue
            resolved=path.resolve(strict=False)
            for candidate in(path,resolved):
                parts=candidate.parts
                temporary=any(parts[n:n+2]==('.local','test-scratch')for n in range(len(parts)-1))or any(part.startswith('halo2-test-')for part in parts)
                if temporary and not candidate.is_relative_to(owner):
                    raise ValueError('Generated build input references a foreign disposable owner: '+str(item)+' -> '+str(candidate))
            paths.add(str(resolved))
            if not resolved.is_relative_to(owner)and resolved.is_file()and resolved.is_relative_to(ROOT):
                external[str(resolved)]=dict(bytes=resolved.stat().st_size,sha256=sha(resolved))
    if not records:raise ValueError('No generated CMake/compiler inputs to authenticate')
    return dict(files=len(records),absolute_paths=len(paths),metadata_sha256=hashlib.sha256(json.dumps(records,sort_keys=True,separators=(',',':')).encode()).hexdigest(),external_workspace_files=external,foreign_scratch_inputs=[])

def verify_reference_pins(rows):
    checked=[]
    for row in rows:
        if 'files'not in row:continue
        current,dependencies=reference_inventory(Path(row['original_owner']))
        if current!=row['files']:raise ValueError('Borrowed reference bytes changed after the owned freeze: '+row['path'])
        checked.append(dict(path=row['path'],tree_sha256=row['tree_sha256'],files=len(current),bytes=row['reference_bytes']))
    return checked
def prepare(source,scratch,qualified=False,source_owner=False):
    authority=world_inputs.prepare(source,scratch)
    world=(source/WORLD).read_text();start=world.index('        arena=std::make_unique<SourcePhysicsGameArenaNative>');end=world.index('        // Actual32CBF8',start);span=world[start:end]
    if binding.digest(span)!=world_inputs.PINS['construction']:raise ValueError('Actual World Arena construction scope drift')
    producer_world=publication.world_source(world)if qualified else world
    producer_start=producer_world.index('        arena=std::make_unique<SourcePhysicsGameArenaNative>');producer_end=producer_world.index('        // Actual32CBF8',producer_start)
    producer_span=producer_world[producer_start:producer_end]
    replacement='''        {
        arena_public_world_proof::Construction arena_world_proof{bank,last_instruction,first_person_phase_active,weapon_effect_phase_active};
'''+producer_span+'''        arena_world_proof.finish(a,*arena,first_person_phase_trace);
        }
'''
    private_world=INCLUDE+producer_world[:producer_start]+replacement+producer_world[producer_end:]
    if private_world[len(INCLUDE):].replace(replacement,producer_span,1)!=producer_world:raise ValueError('World instrumentation changed unrelated source')
    owner=(source/ARENA).read_text();joined='  const auto joined=impl_->services();\n'
    if owner.count(joined)!=1:raise ValueError('Actual Arena const-table call boundary differs')
    if qualified:
        private_owner=INCLUDE+publication.arena_source(owner)
    else:
        marked=joined+'  arena_public_world_proof::CallBorrow public_world_call{impl_->shared,joined};\n'
        private_owner=INCLUDE+owner.replace(joined,marked,1)
        if private_owner[len(INCLUDE):].replace(marked,joined,1)!=owner:raise ValueError('Arena publication instrumentation changed unrelated source')
    generated=(source/UNIT).read_text()
    if source_owner:
        import native_arena_world_initialize_replacements as world_binding
        canonical=world_binding.replace_arena_world_initialize(generated)
        authenticated_body=world_binding.BODY.search(canonical)[0]
        raw=authenticated_body
        bound=authenticated_body.replace('halo2::native_arena_world_initialize_owner_detail::bridge<false>',
            'halo2::arena_public_world_proof::owner_entry',1)
        if bound==authenticated_body:raise ValueError('Exact World Arena source-owner bridge counter seam drift')
    else:
        raw,bound=binding.body(generated);canonical=binding.original.remove_arena_initialize(generated)
        prefix=binding.PREFIX.replace('native_arena_continuation_entry(s)','halo2::arena_public_world_proof::entry(s,[](auto&current,auto target,auto site,bool tail){dispatch(current,target,site,tail);})',1).replace('case 0u:return;','case 0u:return;case 1u:break;',1)
        bound=bound.replace(binding.PREFIX,prefix,1)
    for old,new in(
        ('dispatch(s,target,0x1252a4u,false);','halo2::arena_public_world_proof::child(s,target,0x1252a4u,[&]{dispatch(s,target,0x1252a4u,false);});'),
        ('run_217090(s);','halo2::arena_public_world_proof::child(s,selected(0x217090u,s.image),0x1252bau,[&]{run_217090(s);});'),
        ('s.push(0x1252edu,selected(0x1252f2u,s.image));run_1656a0(s);','s.push(0x1252edu,selected(0x1252f2u,s.image));halo2::arena_public_world_proof::child(s,selected(0x1656a0u,s.image),0x1252edu,[&]{run_1656a0(s);});'),
        ('s.push(0x125321u,selected(0x125326u,s.image));run_1656a0(s);','s.push(0x125321u,selected(0x125326u,s.image));halo2::arena_public_world_proof::child(s,selected(0x1656a0u,s.image),0x125321u,[&]{run_1656a0(s);});')):
        if bound.count(old)!=1:raise ValueError('Actual public Arena child measurement seam differs')
        bound=bound.replace(old,new,1)
    bound,count=re.subn(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',r'halo2::arena_public_world_proof::statement(\1);if(s.services.instruction){s.services.instruction(\1);}',bound)
    if count!=45 or bound.count(binding.REPEAT)!=1:raise ValueError('Exact public Arena literal counter seam drift')
    bound=bound.replace(binding.REPEAT,'halo2::arena_public_world_proof::statement(0x1252b8u);'+binding.REPEAT,1)
    private_generated=INCLUDE+canonical.replace(raw,bound,1)
    changes=[]
    for name,before,after in((WORLD,world,private_world),(ARENA,owner,private_owner),(UNIT,generated,private_generated)):
        changes.append(dict(path=name,before_sha256=binding.digest(before),after_sha256=binding.digest(after)))
        (source/name).write_text(after)
    authority['auxiliary_receiver_extraction']=dict(normalization_arm_omitted=authority['normalization_arm_omitted'],scope=authority['scope'])
    authority.update(private_overlay=changes,scheduling_hooks_changed=False,actual_world_constructor=True,wider_world_required_dispatch_retained=True,normalization_arm_omitted=False,qualified_publication=qualified,actual_source_owner_prepared_bridge=source_owner,retained_parent_pin=binding.original.PIN,scope='Actual complete World constructor/caller/Services/provider source with only the three authenticated private Arena proof overlays; the entire normalization and wider required dispatcher remain present. The auxiliary extracted receiver fixture is not the execution authority for this public gate. Source-owner mode wraps the actual prepared bridge for counters; raw-format admission and strict retagging have separate source-interface/authentication proofs.')
    return authority
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--jobs',type=int,default=4);p.add_argument('--smoke',action='store_true');p.add_argument('--source-root',type=Path,default=ROOT);p.add_argument('--qualified-publication',action='store_true');p.add_argument('--publication-controls',action='store_true');p.add_argument('--source-owner',action='store_true');a=p.parse_args()
    if a.publication_controls and not a.qualified_publication:p.error('Publication controls require the qualified private producer')
    if a.source_owner and not a.qualified_publication:p.error('Source-owner execution requires the actual private producer')
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=scratch/'source';build=scratch/'build';out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,status='freezing',private_unbound=True,owned_scratch=str(scratch),source=str(source),build=str(build),steps=[],retained_large_artifacts=[],selected_runtime_modified=False)
    def save():
        data=(json.dumps(report,indent=2)+'\n').encode()
        logical=out/'report.json'
        encoded=Path(str(logical)+'.gz').exists()or Path(str(logical)+'.gz.index.json').exists()
        if len(data)<=FILE_LIMIT and not encoded:
            logical.write_bytes(data)
        else:
            storage=retain_bytes(data,logical,out)
            (out/'report-storage.json').write_text(json.dumps(dict(status=report['status'],passed=report['passed'],storage=storage),indent=2)+'\n')
    def run(label,command,env=None):
        step=dict(name=label,command=command,started=time.time());report['steps'].append(step);report['status']=label;save();print(json.dumps(dict(step=label,status='running')),flush=True)
        log=out/(label+'.log')
        with log.open('wb')as stream:done=subprocess.run(command,cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT)
        if log.stat().st_size>8*1024*1024:
            with log.open('rb')as stream:stream.seek(-8*1024*1024,os.SEEK_END);tail=stream.read()
            log.write_bytes(tail)
        step.update(exit_code=done.returncode,seconds=time.time()-step['started'],log=str(log))
        step['generated_input_audit']=audit_generated_inputs(build,scratch)
        step['reference_pins_after']=verify_reference_pins(report['borrowed_reference_owners'])
        save();print(json.dumps(dict(step=label,exit_code=done.returncode)),flush=True)
        if done.returncode:raise ValueError(label+' failed: '+str(log))
    save()
    try:
        origin=a.source_root.resolve()
        for attempt in range(3):
            try:pins=freeze_source(origin,source);break
            except RuntimeError:
                if attempt==2:raise
                shutil.rmtree(source)
        raw=(json.dumps(pins,sort_keys=True,indent=2)+'\n').encode();report['frozen_source_manifest']=retain_bytes(raw,out/'source-hashes.json',out)
        report['borrowed_reference_owners']=canonicalize_borrowed_links(source)
        report['frozen_source_origin']=str(origin)
        overlays=(HEADER,'src/physics/runtime/native_tu15_arena_initialize_source_frame.hpp','src/physics/runtime/native_tu15_arena_world_publication.hpp','src/physics/runtime/native_tu15_arena_world_initialize_owner.hpp','tools/native_arena_world_initialize_replacements.py','tools/native_arena_world_policy_prototype.py','tools/native_arena_world_producer_inputs.py','tools/native_arena_initialize_continuation_replacements.py','tools/validate_native_arena_public_world_tu15.py')
        report['owned_private_inputs']={}
        for name in overlays:
            target=source/name;target.parent.mkdir(parents=True,exist_ok=True);data=(ROOT/name).read_bytes();target.write_bytes(data);report['owned_private_inputs'][name]=dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
        report['actual_source_authority']=prepare(source,scratch,a.qualified_publication,a.source_owner);report['private_overlay_manifest_sha256']=binding.digest(json.dumps(source_hashes(source),sort_keys=True))
        selection_path=ROOT/'gameplay-runtime-selection.json';report['selection_before_sha256']=sha(selection_path);selection=json.loads(selection_path.read_text());maps=ROOT/selection['cache']/'halo2/h2_maps_win64_dx11';report['coagulation_sha256']=sha(maps/'coagulation.map')
        dependency,inputs=prepare_peer_source(ROOT/'build-perf-4k60/_seed/halo2_datachannel-src',scratch);report['dependency_inputs']=inputs;save();print(json.dumps(dict(source_frozen=True,manifest_sha256=report['frozen_source_manifest']['raw_sha256'],owned_scratch=str(scratch))),flush=True)
        environment=dict(os.environ,CARGO_NET_OFFLINE='true')
        command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--output',str(out/'engine-build'),'--jobs',str(a.jobs),'--target','halo2_native','--build-only','--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL='+str(dependency),'--cmake-arg=-DFETCHCONTENT_FULLY_DISCONNECTED=ON','--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT='+str(ROOT),'--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=OFF','--cmake-arg=-DHALO2_SPECIALIZE_NATIVE_OPERANDS=ON']
        run('engine-build',command,environment)
        runtime=build/'halo2_native';report['private_runtime_sha256']=sha(runtime)
        prepared=build/'native-operands/physics/physics_game_arena_runtime_generated.cpp'
        if not prepared.is_file():raise ValueError('Actual compiled prepared Arena source unavailable')
        prepared_body=binding.original.BODY.findall(prepared.read_text())
        if len(prepared_body)!=1 or len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',prepared_body[0]))!=45 or prepared_body[0].count(binding.REPEAT)!=1:raise ValueError('Actual prepared Arena callback format drift')
        report['actual_prepared_parent']=dict(path=str(prepared),sha256=binding.digest(prepared_body[0]),literal_callbacks=45,authored_direct_repeat_callbacks=1)
        spec=importlib.util.spec_from_file_location('arena_public_capture',source/'tools/perf_4k60/parity.py');capture=importlib.util.module_from_spec(spec);spec.loader.exec_module(capture)
        rows=[]
        for policy in((0,1)if a.smoke else range(12)):
            pair=[]
            for reference in(False,True):
                label='world-arena-'+str(policy)+'-'+('reference'if reference else'native');receipt=scratch/(label+'.json');env=dict(HALO2_ARENA_PUBLIC_WORLD_REPORT=str(receipt),HALO2_ARENA_PUBLIC_WORLD_POLICY=str(policy),XDG_CACHE_HOME=str(scratch/'cache'))
                if a.qualified_publication:env['HALO2_ARENA_PUBLIC_WORLD_QUALIFIED_PRODUCER']='1'
                if reference:env['HALO2_ARENA_PUBLIC_WORLD_REFERENCE']='1'
                result=capture.capture(runtime,label,1,2,0,0,env,scratch=scratch,maps=maps,size=(640,360),timeout_seconds=600)
                result.pop('_payload',None)
                if not receipt.is_file():raise ValueError('Actual public World emitted no Arena receipt: '+str(result))
                data=json.loads(receipt.read_text());pair.append(dict(capture=result,arena=data));report['runs']=rows+[dict(policy=policy,routes=pair)];save()
                if not result['valid']or not data['passed']:raise ValueError('Actual public World startup/capture failed '+label+': '+str(result))
                if not reference and(data['native_calls']!=1 or data['handoffs']!=int(policy!=0)or(not policy and data['retained_parent_statements'])):raise ValueError('Actual public World did not take expected native/default suffix')
                expected=(0,0x125281,0x1252b8,0x1252b8,0x1252d7,0x12532e,0x1252b8,0x1252b3,0x1252a9,0x1252bf,0x1252f2,0x125326)[policy]
                if not reference and data['handoff_pc']!=expected:raise ValueError('Actual public World returned the wrong retired-I/O/child frontier')
                if reference and(data['native_calls']or data['retained_parent_statements']<=0):raise ValueError('Actual public World reference did not execute retained body')
            left,right=(x['arena']for x in pair);diff=[key for key in left if left[key]!=right[key]]
            invariants=('escaped_abi','real_files','real_worker_retired','position','guard_changes')
            for key in invariants:
                if left[key]!=right[key]:raise ValueError('Actual empty-schedule public World invariant mismatch '+key)
            rows.append(dict(policy=policy,routes=pair,all_differing_fields=diff,empty_schedule_variation_preserved=True));report['runs']=rows;save()
        if a.publication_controls:
            report['publication_controls']=[]
            definitions=(
                ('missing-world-construction',WORLD,'                native_arena_world_publication_detail::Construction published_world_arena{*arena,bank,services(),this};','                /* Deliberate owned negative: no World construction publication. */'),
                ('missing-arena-borrow',ARENA,'  native_arena_world_publication_detail::Borrow published_world{*this,impl_->shared,joined,impl_.get(),&Impl::RequiredOwner::matches_world_publication};','  /* Deliberate owned negative: no Arena const-table publication. */'))
            baseline=rows[0]['routes'][1]['arena']
            def control_capture(label,expected_calls):
                receipt=scratch/(label+'.json')
                env=dict(HALO2_ARENA_PUBLIC_WORLD_REPORT=str(receipt),HALO2_ARENA_PUBLIC_WORLD_POLICY='0',HALO2_ARENA_PUBLIC_WORLD_QUALIFIED_PRODUCER='1',XDG_CACHE_HOME=str(scratch/'cache'))
                result=capture.capture(runtime,label,1,2,0,0,env,scratch=scratch,maps=maps,size=(640,360),timeout_seconds=600);result.pop('_payload',None)
                if not receipt.is_file():raise ValueError('Producer control emitted no actual World receipt '+label)
                data=json.loads(receipt.read_text())
                if not result['valid']or not data['passed']or data['native_calls']!=expected_calls or data['handoffs']:
                    raise ValueError('Actual producer control route failed '+label)
                if bool(data['retained_parent_statements'])==bool(expected_calls):raise ValueError('Actual producer control retained/default execution differs '+label)
                for key in('escaped_abi','real_files','real_worker_retired','position','guard_changes'):
                    if data[key]!=baseline[key]:raise ValueError('Actual producer control invariant differs '+label+' '+key)
                return dict(capture=result,arena=data)
            for label,name,old,new in definitions:
                target=source/name;original=target.read_text()
                if original.count(old)!=1:raise ValueError('Exact producer control source seam drift '+label)
                record=dict(name=label,source=name,original_sha256=binding.digest(original));report['publication_controls'].append(record)
                try:
                    modified=original.replace(old,new,1);target.write_text(modified);record['mutated_sha256']=binding.digest(modified)
                    changed=list(command);changed[changed.index('--output')+1]=str(out/('build-'+label))
                    run('build-'+label,changed,environment);record['mutant_runtime_sha256']=sha(runtime)
                    record['fallback']=control_capture(label,0);save()
                finally:
                    # Fresh write time is required so the same owned worker
                    # recompiles the restored source rather than reusing a
                    # previously produced negative object.
                    target.write_text(original)
                    if sha(target)!=record['original_sha256']:raise ValueError('Exact private producer source restoration failed '+label)
                    restored=list(command);restored[restored.index('--output')+1]=str(out/('restore-'+label))
                    run('restore-'+label,restored,environment);record['restored_runtime_sha256']=sha(runtime)
                record['restored_default']=control_capture('restored-'+label,1);record['source_restored_exactly']=True;save()
        if sha(selection_path)!=report['selection_before_sha256']:raise ValueError('Launcher selection drifted during public proof; no acceptance')
        report.update(passed=True,status='passed',scope='Actual public World constructor/caller/required provider, unchanged empty schedules and genuine Arena descendants in one frozen private engine. Exact source-retirement/REP handoffs and positive reference; no production installation or all-native child/runtime promotion.')
    except BaseException as error:report.update(status='failed',error=repr(error));raise
    finally:save()
    return 0
if __name__=='__main__':raise SystemExit(main())
