#!/usr/bin/env python3
"""Private real-GPU renewal regression linked to a live owned coherent client build.

Only a test translation unit is compiled. The frozen source, engine archives,
client executable and CMake tree remain unchanged; all new outputs are owned.
"""
import argparse,hashlib,json,os,shlex,shutil,subprocess,sys,time
from pathlib import Path
sys.dont_write_bytecode=True
from run_tests import validation_environment
ROOT=Path(__file__).resolve().parents[1]
def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cohort',type=Path,required=True);p.add_argument('--mcc-root',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--skip-renew-control',action='store_true');a=p.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:p.error('Run through tools/run_test.py')
    cohort=json.loads(a.cohort.read_text());owner=Path(cohort['scratch']).resolve(strict=True);build=owner/'build';scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert build.is_dir()and(owner/'current-source').is_dir()and owner!=scratch
    target=build/'CMakeFiles/native_texture_camera_active_gpu_tests.dir';source=ROOT/'tests/texture_camera/native_texture_camera_active_gpu.cpp';private=scratch/'test';private.mkdir();pins={}
    for name in('native_texture_camera_active_gpu.cpp','native_texture_camera_authored_setup.hpp','native_texture_camera_gpu_observation.hpp'):
        path=source.parent/name;shutil.copyfile(path,private/name);pins[str(path.relative_to(ROOT))]=sha(path)
    flags={}
    for line in(target/'flags.make').read_text().splitlines():
        if ' = 'in line:key,value=line.split(' = ',1);flags[key]=shlex.split(value)
    link=shlex.split((target/'link.txt').read_text());binary=scratch/'renewal-probe';obj=scratch/'renewal.o'
    for n,value in enumerate(link):
        if value.endswith('native_texture_camera_active_gpu.cpp.o'):link[n]=str(obj)
        elif value.startswith('--dependency-file='):link[n]='--dependency-file='+str(scratch/'link.d')
        if n and link[n-1]=='-o':link[n]=str(binary)
    borrowed={}
    for value in link:
        if value.endswith('.a'):
            path=Path(value);path=path if path.is_absolute()else build/path;borrowed[str(path)]=sha(path)
    compile=[link[0],*flags['CXX_DEFINES'],*flags['CXX_INCLUDES'],*flags['CXX_FLAGS'],'-c',str(private/source.name),'-o',str(obj)]
    report=dict(passed=False,scope=__doc__,engine_manifest_sha256=cohort['source_inputs_sha256'],cohort_report_sha256=sha(a.cohort),test_source_hashes=pins,borrowed_archive_hashes=borrowed,compile_command=compile,link_command=link,scratch=str(scratch),tool_sha256=sha(__file__),retained_large_artifacts=[])
    try:
        for label,command in(('compile',compile),('link',link)):
            run=subprocess.run(command,cwd=build,text=True,capture_output=True,timeout=240);report[label+'_returncode']=run.returncode;report[label+'_stderr']=run.stderr[-6000:]
            if run.returncode:raise RuntimeError(label+' failed')
        maps=a.mcc_root.resolve(strict=True)/'halo2/h2_maps_win64_dx11';assert(maps/'00a_introduction.map').is_file();native=scratch/'native';native.mkdir()
        command=[str(binary),'--inactive-renew-only','--producer',str(maps/'00a_introduction.map'),'--shared-dir',str(maps),'--prepared-root',str(a.mcc_root.resolve()),'--shaders',str(build/'shaders'),'--output',str(native)]
        if a.skip_renew_control:command.append('--inactive-skip-renew-control')
        env,report['validation_environment']=validation_environment(ROOT);env['HALO2_CAPTURE_BSP_DRAW_BINDINGS']='1'
        for key in('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):env[key]=str(scratch/key.lower())
        if not env.get('DISPLAY'):command=['xvfb-run','-a',*command]
        report['command']=command;started=time.monotonic();run=subprocess.run(command,cwd=ROOT,env=env,text=True,capture_output=True,timeout=120)
        report.update(returncode=run.returncode,elapsed_seconds=round(time.monotonic()-started,3),stdout_tail=run.stdout[-6000:],stderr_tail=run.stderr[-6000:],binary_sha256=sha(binary))
        if(native/'report.json').is_file():report['native_report']=json.loads((native/'report.json').read_text())
        report['passed']=run.returncode==0;report['exact_renewal_guard_red']=report.get('native_report',{}).get('error')=='Texture camera renewal requires its same unsubmitted renderer/frame'
        report['raw_ppm_count']=len(list(native.glob('*.ppm')));report['raw_ppm_bytes']=sum(path.stat().st_size for path in native.glob('*.ppm'))
    except Exception as error:report['harness_error']=str(error)
    report['borrowed_archives_unchanged']=all(sha(path)==pin for path,pin in borrowed.items());report['checkout_test_sources_unchanged']=all(sha(ROOT/path)==pin for path,pin in pins.items())
    report['passed']=report['passed']and report['borrowed_archives_unchanged']and report['checkout_test_sources_unchanged'];a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report.get(key)for key in('passed','compile_returncode','link_returncode','returncode','exact_renewal_guard_red','harness_error','native_report','scratch')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
