#!/usr/bin/env python3
"""Fresh exact Arena generation, reversible owners and whole-unit syntax."""
import argparse,hashlib,json,os,platform,re,subprocess,sys
from pathlib import Path
import native_arena_initialize_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
CORRESPONDENCE='analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/correspondence-repeat/complete-report.json'
UNITS=('src/physics/physics_game_arena_runtime_generated.cpp','src/physics/physics_game_arena_native.cpp','src/physics/native_tu15_arena_bootstrap.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=(*UNITS,CORRESPONDENCE,'tools/generate_physics_game_arena_tu15_20261012.py','tools/native_arena_initialize_replacements.py','tools/native_arena_owner_policy_prototype.py','tools/validate_native_arena_initialize_generation_tu15.py','src/physics/runtime/native_tu15_arena_publication.hpp','src/physics/runtime/native_tu15_arena_schedule_policy.hpp')
    pins={n:sha(ROOT/n)for n in names};report=dict(passed=False,private_unbound=not a.require_binding,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        generated=scratch/'fresh-arena.cpp';manifest=scratch/'generation.json';generation_command=[sys.executable,'-B',str(ROOT/names[4]),'--correspondence',str(ROOT/CORRESPONDENCE),'--output',str(generated),'--manifest',str(manifest)]
        done=subprocess.run(generation_command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Authentic fresh Arena generation failed '+(done.stdout+done.stderr)[-6000:])
        fresh=binding.remove_arena_initialize(generated.read_text());current=binding.remove_arena_initialize((ROOT/UNITS[0]).read_text())
        if fresh!=current:raise ValueError('Complete fresh 17-owner Arena output differs from canonical source')
        bound=binding.replace_arena_initialize(fresh)
        if "native_arena_world_initialize_owner_detail" in (ROOT/UNITS[0]).read_text():
            from native_arena_world_initialize_replacements import replace_arena_world_initialize
            bound=replace_arena_world_initialize(bound)
        if binding.remove_arena_initialize(bound)!=fresh or binding.replace_arena_initialize(bound)!=bound:raise ValueError('Generated Arena overlay is not exact/idempotent')
        before=re.findall(r'static void run_[0-9a-f]+\(WorldState &s\) \{.*?\n\}',fresh,re.S)
        after=re.findall(r'static void run_[0-9a-f]+\(WorldState &s\) \{.*?\n\}',bound,re.S)
        if len(before)!=17 or len(after)!=17 or sum(x!=y for x,y in zip(before,after))!=1:raise ValueError('Arena overlay changed another complete owner')
        owner=(ROOT/UNITS[1]).read_text();bootstrap=(ROOT/UNITS[2]).read_text()
        prepared=(bound,binding.replace_arena_owner(owner),binding.replace_arena_bootstrap(bootstrap))
        if a.require_binding and tuple((ROOT/n).read_text()for n in UNITS)!=prepared:raise ValueError('Required installed Arena source/producer/bootstrap binding missing')
        shared=(ROOT/'src/physics/runtime/native_tu15_arena_schedule_policy.hpp').read_text()
        if shared.count(binding.BOOTSTRAP_BARRIER)!=1:raise ValueError('Shared idle barrier differs from exact original bootstrap')
        commands=[]
        for index,text in enumerate(prepared):
            path=scratch/('unit-'+str(index)+'.cpp');path.write_text(text)
            for compiler in a.compiler or('g++','clang++'):
                command=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)];done=subprocess.run(command,capture_output=True,text=True)
                if done.returncode:raise ValueError('Whole prepared Arena unit failed '+(done.stdout+done.stderr)[-8000:])
                commands.append(dict(command=command,passed=True))
        report.update(passed=True,generation_command=generation_command,generation_manifest=json.loads(manifest.read_text()),fresh_output_equals_canonical=True,unchanged_complete_other_owners=16,bound_body_sha256=binding.digest(binding.BODY.search(bound)[0]),prepared_source_sha256=[binding.digest(x)for x in prepared],whole_unit_syntax=commands,public_header_unchanged=True,original_body_pin=binding.PIN,original_resolver_pin=binding.RESOLVER_PIN,original_dispatcher_pin=binding.DISPATCH_PIN,original_export_pin=binding.EXPORT_PIN)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={n:sha(ROOT/n)for n in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
