#!/usr/bin/env python3
"""Compile the actual frozen Host and World contexts for process-input capture.

Run through run_test.py. Only small syntax reports/source hashes survive; no
kernel, input, gate, admission or runtime state is modified.
"""
import argparse,hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path

def sha(path):
    with path.open('rb')as source:return hashlib.file_digest(source,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in('base','stage','host-flags','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);overlay=scratch/'overlay';overlay.mkdir()
    report={'passed':False,'owned_scratch':str(scratch),'retained_large_artifacts':[],'source_or_physics_modified':False}
    try:
        tests=overlay/'tests';tests.mkdir();shutil.copytree(a.base/'tests/helpers',tests/'helpers')
        for q in(a.base/'tests').iterdir():
            if q.is_file()and q.suffix in('.h','.hpp'):shutil.copyfile(q,tests/q.name)
        manifest=json.loads((a.stage/'source-pins.json').read_text())
        for name,digest in manifest['staged_hashes'].items():
            q=a.stage/'staged'/name;assert sha(q)==digest
            out=overlay/name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(q,out)
        flags=[]
        for line in a.host_flags.read_text().splitlines():
            if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS =')):flags+=shlex.split(line.split('=',1)[1])
        assert '-I'+str(a.base/'tests')not in flags
        common=['-I'+str(overlay/'include'),'-I'+str(a.base/'include'),'-I'+str(a.base/'src'),'-I'+str(a.base/'src/physics')]
        inputs=[('Host','tests/physics_multiplayer_acceptance.cpp',flags),('World','src/physics/native_tu15_world_integration_residuals.cpp',['-std=c++20','-O0','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math'])]
        results=[]
        for cc in('g++','clang++'):
            for kind,name,options in inputs:
                command=[cc,*common,*options,'-fsyntax-only',str(overlay/name)]
                run=subprocess.run(command,capture_output=True,text=True,timeout=180)
                (a.output/(cc+'-'+kind+'.log')).write_text(run.stdout+run.stderr)
                results.append({'compiler':cc,'translation_unit':kind,'exit_code':run.returncode,'command':command})
                assert run.returncode==0,run.stderr[-5000:]
        report.update(passed=True,compiler_results=results,staged_hashes=manifest['staged_hashes'],actual_host_flags_sha256=sha(a.host_flags),source_hashes={str(Path(__file__)):sha(Path(__file__))},no_test_include_search_path=True,scope='Both complete actual frozen Host/World translation units; syntax context only. Parent interface/lifetime/source proofs and actual live selection are separate.')
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(q.stat().st_blocks*512 for q in scratch.rglob('*')if q.is_file())
        (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
