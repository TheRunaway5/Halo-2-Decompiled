#!/usr/bin/env python3
"""Run existing metadata default/FP/live-stack tests on the typed sole owner."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-island-info-typed-data-stage-20261103/staged'
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    r=dict(passed=False,owned_scratch=str(owner),retained_large_artifacts=[],source_outcomes_supplied=False,live_integration_claimed=False)
    try:
        shadow=owner/'source';(shadow/'tools').mkdir(parents=True);(shadow/'src/physics').mkdir(parents=True);shutil.copytree(ROOT/'include',shadow/'include')
        # Preserve the established validator and both original trace bridges;
        # only its explicit source root is private. Original TU1.5 authority
        # paths remain the authenticated real inputs imported by its modules.
        scripts=('validate_native_island_partition_metadata_tu15.py','validate_native_island_partition_graph_tu15.py','run_test.py','run_tests.py')
        for n in scripts:shutil.copyfile(ROOT/'tools'/n,shadow/'tools'/n)
        for n in ('observe_native_island_partition_graph_tu15.cpp','observe_native_island_partition_metadata_tu15.cpp','observe_native_activation_agents_tu15.cpp'):shutil.copyfile(ROOT/'tools'/n,shadow/'tools'/n)
        for path in STAGE.rglob('*'):
            if path.is_file():
                dest=shadow/path.relative_to(STAGE);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,dest)
        pins={str(p.relative_to(shadow)):sha(p)for p in shadow.rglob('*')if p.is_file()}
        command=[sys.executable,str(shadow/'tools/validate_native_island_partition_metadata_tu15.py'),'--output',str(a.output/'defaults')]
        env=dict(os.environ);env['PYTHONPATH']=str(ROOT/'tools')+(os.pathsep+env['PYTHONPATH']if env.get('PYTHONPATH')else'')
        result=subprocess.run(command,capture_output=True,text=True,env=env);(a.output/'console.log').write_text((result.stdout+result.stderr)[-48000:]);r.update(command=command,exit_code=result.returncode);assert result.returncode==0,(result.stdout+result.stderr)[-5000:]
        proof=a.output/'defaults/report.json';d=json.loads(proof.read_text());assert d['passed']and all(v['exit_code']==0 for v in d['compiler_results'])
        assert all(sha(shadow/n)==h for n,h in pins.items())
        r.update(passed=True,source_inputs_unchanged=True,compiled_source_sha256=pins,existing_default_report_sha256=sha(proof),
          profiles_per_compiler=d['profiles_per_compiler'],declared_cases=d['declared_cases'],source_sites=d['source_sites'],
          scope='Existing3200B0/320100/320130/323420 default numeric interfaces through same typed-core refactor; both compilers/four rounding modes/four incoming nonempty x87 depths, fullbanks/FP/order/RET and original semantic mutants. No31F460 or live acceptance claim.')
    except BaseException as e:r['error']=str(e)or repr(e);raise
    finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file());save(a.output/'report.json',r)
if __name__=='__main__':main()
