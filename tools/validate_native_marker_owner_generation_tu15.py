#!/usr/bin/env python3
"""Replay real inherited-marker generators and compile complete owner units."""
import argparse,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='clang++');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    os.chdir(ROOT);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    import native_marker_owner_replacements as binding
    from native_marker_owner_inputs import UNITS,EFFECT,HELPER,PUBLIC
    from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER
    from validate_native_camera_instance_bitmap_generation_tu15 import generate_ai
    from specialize_native_operands import specialize
    tools={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'tools').rglob('*.py')}
    headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    original={name:(ROOT/name).read_text()for name in(*UNITS,EFFECT,HELPER,PUBLIC)}
    source_pins={name:sha(ROOT/name)for name in original}
    used={'tools/native_model_marker_lookup_replacements.py','tools/native_object_parent_mode_replacements.py','tools/native_halfword_bit_query_replacements.py','tools/native_selected_operand_binding.py'}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],units=[],source_before=source_pins)
    syntax=[];dependencies=set()
    def compile_unit(path,extra=()):
        dep=path.with_suffix('.d')
        command=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),*extra,'-MD','-MF',str(dep),str(path)]
        done=subprocess.run(command,capture_output=True,text=True,timeout=300)
        if done.returncode:raise ValueError('Whole inherited-marker CPP syntax failed '+done.stderr[-7000:])
        syntax.append(dict(command=command,sha256=sha(path),passed=True))
        dependencies.update(Path(value).resolve()for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]))
    try:
        for index,name in((0,'primary'),(2,'scene'),(3,'motion'),(1,'ai')):
            path=scratch/(name+'.cpp');row=dict(owner=name,canonical=UNITS[index]);report['units'].append(row)
            if name=='ai':command,provenance=generate_ai(scratch);row['original_references']=provenance
            else:
                script={'primary':'generate_physics_live_support_tu15.py','scene':'generate_scene_device_constructor_tu15.py','motion':'generate_vehicle_motion_20261011.py'}[name]
                command=[sys.executable,'-B',str(ROOT/'tools'/script),'--output',str(path)]
                command+=['--directory',str(scratch/'scene-generation')]if name=='scene'else['--manifest',str(scratch/(name+'.json'))]
                if name=='motion':command+=['--correspondence',str(ROOT/'analysis/multiplayer/sandbox-source/vehicle-motion-20261011/correspondence-verified/report.json')]
                metadata=scratch/(name+'-python.json')
                done=subprocess.run([sys.executable,'-B','-c',CHILD_WRAPPER,str(metadata),*command[2:]],capture_output=True,text=True,timeout=300)
                if done.returncode:raise ValueError('Genuine inherited-marker generator failed '+done.stderr[-7000:])
                used.update(str(Path(value).relative_to(ROOT))for value in json.loads(metadata.read_text())if Path(value).is_relative_to(ROOT/'tools'))
            source=path.read_text();match,body,owner=binding.authenticate(source);_,baseline,old_owner=binding.authenticate(original[UNITS[index]])
            projected=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',baseline)
            if owner!=name or old_owner!=name or body not in(baseline,projected):raise ValueError('Fresh inherited-marker identity/body mismatch '+name)
            prefix=binding.prefix(owner);installed=prefix in match[0]
            if args.require_binding and(not installed or binding.replace_marker_owner(original[UNITS[index]])!=original[UNITS[index]]):raise ValueError('Installed inherited-marker prefix missing '+name)
            bound=binding.replace_marker_owner(source)
            if binding.replace_marker_owner(bound)!=bound:raise ValueError('Fresh inherited-marker installer not idempotent')
            rejected=0
            for changed in(
                source.replace(match[0],match[0].replace('0x14u','0x18u',1),1),
                bound.replace(prefix,binding.EFFECT_PREFIX,1)):
                if changed in(source,bound):raise ValueError('Fresh inherited-marker mutation witness missing')
                try:binding.replace_marker_owner(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Changed fresh inherited-marker source admitted')
            path.write_text(bound);prepared,receipt=specialize(bound)
            if prefix not in binding.BODY.search(prepared)[0]:raise ValueError('Prepared inherited-marker prefix missing')
            prepared_path=scratch/(name+'-prepared.cpp');prepared_path.write_text(prepared)
            compile_unit(path);compile_unit(prepared_path)
            row.update(passed=True,command=command,body_sha256=binding.digest(body),installed_before=installed,idempotent=True,strict_mutations_rejected=rejected,whole_unit_equal=bound==original[UNITS[index]],actual_owner_groups={key:value for key,value in receipt.items()if key.endswith('actual_owner_entries')})
        helper=binding.replace_effect(original[HELPER],original[EFFECT])
        if args.require_binding and(helper!=original[HELPER]or original[EFFECT].count(binding.INCLUDE)!=1):raise ValueError('Installed actual Effect inherited-marker prefix/include missing')
        if binding.replace_effect(helper,original[EFFECT])!=helper:raise ValueError('Effect inherited-marker installer not idempotent')
        (scratch/Path(HELPER).name).write_text(helper)
        effect=scratch/'effect.cpp';effect.write_text(original[EFFECT]if binding.INCLUDE in original[EFFECT]else binding.INCLUDE+original[EFFECT])
        compile_unit(effect,('-I'+str(scratch),'-I'+str(ROOT/'src/gameplay/weapons')))
        public=binding.replace_value(original[PUBLIC])
        if args.require_binding and public!=original[PUBLIC]:raise ValueError('Installed public inherited-marker DATA missing')
        public_path=scratch/'public.cpp';public_path.write_text(public);compile_unit(public_path)
        used.update(str(Path(module.__file__).resolve().relative_to(ROOT))for module in tuple(sys.modules.values())if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        tool_pins={name:tools[name]for name in sorted(used)}
        header_pins={str(path.relative_to(ROOT)):headers[path]for path in dependencies if path in headers}
        for name,pin in(*source_pins.items(),*tool_pins.items(),*header_pins.items()):
            if sha(ROOT/name)!=pin:raise ValueError('Inherited-marker generation input changed '+name)
        report.update(passed=True,whole_cpp_syntax=syntax,preparation_tools=tool_pins,header_hashes=header_pins,scope='Exact full source ownership, fresh raw/prepared routing and complete CPP syntax; numerical/service/public-policy execution qualified separately')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
