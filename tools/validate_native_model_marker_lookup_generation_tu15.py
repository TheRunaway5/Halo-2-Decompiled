#!/usr/bin/env python3
"""Replay six real model-marker generators and compile complete owner units."""
import argparse,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
CHILD_WRAPPER="""import json,runpy,sys
from pathlib import Path
metadata=Path(sys.argv[1]);sys.argv=sys.argv[2:]
sys.path.insert(0,str(Path(sys.argv[0]).parent))
try:runpy.run_path(sys.argv[0],run_name='__main__')
finally:metadata.write_text(json.dumps(sorted({str(Path(m.__file__).resolve())for m in tuple(sys.modules.values())if getattr(m,'__file__',None)}|{str(Path(sys.argv[0]).resolve())})))
"""
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='clang++')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args();os.chdir(ROOT);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    import native_model_marker_lookup_replacements as binding
    from native_model_marker_lookup_inputs import UNITS
    from specialize_native_operands import specialize
    from validate_native_camera_instance_bitmap_generation_tu15 import generate_ai
    tools={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'tools').rglob('*.py')}
    headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    units={unit:sha(ROOT/unit)for unit in UNITS}
    used_tools={str(Path(__file__).relative_to(ROOT)),'tools/native_sandbox_dispatch_preservation.py','tools/native_halfword_bit_query_replacements.py'}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],units=[],canonical_sha256=units)
    try:
        for index,name in((0,'primary'),(2,'scene'),(3,'sandbox'),(4,'motion'),(5,'death'),(1,'ai')):
            unit=UNITS[index];row=dict(owner=name,canonical=unit);report['units'].append(row)
            generated=scratch/(name+'.cpp')
            if name=='ai':
                command,provenance=generate_ai(scratch);row['original_references']=provenance
            else:
                script={'primary':'generate_physics_live_support_tu15.py','scene':'generate_scene_device_constructor_tu15.py','sandbox':'generate_sandbox_special_tu15.py','motion':'generate_vehicle_motion_20261011.py','death':'generate_death_startup_runtime_1_1.py'}[name]
                command=[sys.executable,'-B',str(ROOT/'tools'/script),'--output',str(generated)]
                command+=['--directory',str(scratch/(name+'-generation'))]if name in('scene','sandbox')else['--manifest',str(scratch/(name+'.json'))]
                if name=='motion':command+=['--correspondence',str(ROOT/'analysis/multiplayer/sandbox-source/vehicle-motion-20261011/correspondence-verified/report.json')]
                metadata=scratch/(name+'-python-dependencies.json')
                done=subprocess.run([sys.executable,'-B','-c',CHILD_WRAPPER,str(metadata),*command[2:]],capture_output=True,text=True,timeout=300)
                if done.returncode:raise ValueError('Genuine '+name+' generator failed '+done.stderr[-7000:])
                used_tools.update(str(Path(value).relative_to(ROOT))for value in json.loads(metadata.read_text())if Path(value).is_relative_to(ROOT/'tools'))
            source=generated.read_text();match,body,mode,owner=binding.authenticate(source)
            canonical=(ROOT/unit).read_text();canonical_match,canonical_body,canonical_mode,_=binding.authenticate(canonical)
            if owner!=('death-startup'if name=='death'else name)or mode!=canonical_mode:
                raise ValueError('Fresh marker owner/observation mismatch '+name)
            old_projected=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',canonical_body)
            if body not in(canonical_body,old_projected):raise ValueError('Fresh marker body differs from authenticated canonical '+name)
            installed=binding.prefix(mode)in match[0]
            if args.require_binding and not installed:raise ValueError('Fresh default marker prefix missing '+name)
            bound=binding.replace_model_marker_lookup(source)
            if binding.replace_model_marker_lookup(bound)!=bound:raise ValueError('Fresh marker binding not idempotent '+name)
            count=0
            for changed in(
                source.replace(match[0],match[0].replace('0x58u','0x54u',1),1),
                bound.replace(binding.prefix(mode),binding.prefix('canonical'if mode=='actual'else'actual'),1),
            ):
                if changed in(source,bound):raise ValueError('Fresh marker mutation witness missing '+name)
                try:binding.replace_model_marker_lookup(changed)
                except ValueError:count+=1
                else:raise ValueError('Fresh marker changed body/mode admitted '+name)
            state=match[2]
            dispatch=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            changed=source.replace(dispatch,dispatch.replace('run_'+match[1]+'(s);','run_30bd0(s);',1),1)
            if changed==source:raise ValueError('Fresh dispatch mutation witness missing '+name)
            try:binding.replace_model_marker_lookup(changed)
            except ValueError:count+=1
            else:raise ValueError('Fresh marker changed complete dispatcher accepted '+name)
            generated.write_text(bound)
            if state=='State':prepared=bound;receipt={}
            else:prepared,receipt=specialize(bound)
            if binding.prefix(mode)not in binding.BODY.search(prepared)[0]:raise ValueError('Prepared native marker prefix missing '+name)
            prepared_path=scratch/(name+'-prepared.cpp');prepared_path.write_text(prepared)
            syntax=[];dependencies=set()
            for artifact in(generated,prepared_path)if state!='State'else(generated,):
                dep=artifact.with_suffix('.d')
                cmd=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(artifact)]
                done=subprocess.run(cmd,capture_output=True,text=True,timeout=300)
                if done.returncode:raise ValueError('Complete '+name+' unit syntax failed '+done.stderr[-7000:])
                syntax.append(dict(command=cmd,sha256=sha(artifact),passed=True))
                dependencies.update(Path(value).resolve()for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]))
            row.update(passed=True,command=command,body=binding.digest(body),original_installed_default=installed,native_prefix_and_idempotence=True,complete_body_context_authenticated=True,full_unit_equal=bound==canonical,raw_prepared_syntax=syntax,strict_fresh_mutants_rejected=count,header_hashes={str(path.relative_to(ROOT)):headers[path]for path in dependencies if path in headers},actual_owner_groups={key:value for key,value in receipt.items()if key.endswith('actual_owner_entries')})
            if name=='primary':
                if receipt['native_bsp_surface_2d_actual_owner_entries']!=['2a0230']or receipt['native_bsp_surface_polygon_actual_owner_entries']!=['1e6e80']:
                    raise ValueError('Earlier actual BSP owner routing was lost')
            if name=='sandbox':
                import native_sandbox_dispatch_preservation as preserve
                if preserve.preserve(bound)!=bound or preserve.mappings(preserve.dispatcher(bound)[0])!=preserve.mappings(preserve.dispatcher(canonical)[0]):
                    raise ValueError('Canonical Sandbox required-owner branch lost')
        used_tools.update(str(Path(module.__file__).resolve().relative_to(ROOT))for module in tuple(sys.modules.values())if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        relevant_tools={name:tools[name]for name in sorted(used_tools)}
        for name,pin in relevant_tools.items():
            if sha(ROOT/name)!=pin:raise ValueError('Generation preparation tool changed '+name)
        for name,pin in units.items():
            if sha(ROOT/name)!=pin:raise ValueError('Canonical owner changed '+name)
        for row in report['units']:
            for name,pin in row['header_hashes'].items():
                if sha(ROOT/name)!=pin:raise ValueError('Generation header changed '+name)
        report.update(passed=True,preparation_tools_sha256=relevant_tools,generator_dependency_wrapper_sha256=hashlib.sha256(CHILD_WRAPPER.encode()).hexdigest(),scope='Exact regenerated native marker prefixes, complete contexts and whole-unit syntax; execution and callee semantics are qualified separately')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
