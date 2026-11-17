#!/usr/bin/env python3
"""Retain the established narrow314710 value-domain regressions after core reuse."""
import argparse,hashlib,json,os,platform,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
IMAGE_PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
ARCHIVE_PIN='cc2e90e06e3df1b013361f7fa13e14a9507aa059ececd3d9cf53a2a402e161b5'
MANIFEST_PIN='aad35a246ce4481532ae49815cbf3920fa2aba494ff55a88ff155887d8612b6e'
def digest(path):
    with Path(path).open('rb')as source:return hashlib.file_digest(source,'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--mode',choices=('declared','genuine'),required=True);parser.add_argument('--entry',choices=('context','append'),default='context');parser.add_argument('--report',type=Path,required=True);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--archive',type=Path);args=parser.parse_args()
    if args.entry=='append'and args.mode!='declared':parser.error('Append retains its separate declared original suite')
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file()or not all(shutil.which(cc)for cc in('g++','clang++'))or(args.mode=='genuine'and(not args.archive or not args.archive.is_file())):
        print('SKIP hull-context value original requires its reference assets and Linux x86 compilers/ptrace');return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);output=scratch/'legacy-output';tool='validate_native_solver_hull_append_tu15.py'if args.entry=='append'else'validate_native_solver_hull_query_tu15.py'if args.mode=='declared'else'validate_native_solver_hull_query_defined_data_tu15.py'
    names=['src/physics/native_tu15_solver_hull_query.cpp','include/halo2/native_tu15_solver_hull_query.hpp','include/halo2/native_tu15_solver_hull_context_kernel.hpp','include/halo2/native_tu15_solver_hull_append_kernel.hpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp','src/physics/native_tu15_solver_hull_single_vertex.cpp','src/physics/native_tu15_solver_hull_support.cpp','src/physics/native_tu15_contact_shape.cpp','tools/validate_native_solver_hull_query_tu15.py','tools/validate_native_solver_hull_query_actual_tu15.py','tools/validate_native_solver_hull_query_defined_data_tu15.py','tools/validate_native_solver_hull_append_tu15.py',str(Path(__file__).relative_to(ROOT))]
    pins={name:digest(ROOT/name)for name in names};report=dict(passed=False,mode=args.mode,entry=args.entry,scratch_owner=str(scratch),retained_large_artifacts=[],source_sha256=pins,raw_owner_admission_or_child_closure_qualified=False,compiler_configuration='The existing legacy subprojects run ordinary Release GCC and Clang builds; an outer sanitizer setting does not enable sanitizers in those unchanged subprojects.')
    try:
        if digest(args.xbe)!=IMAGE_PIN:raise ValueError('Wrong actual original image')
        command=[sys.executable,'-B',str(ROOT/'tools'/tool),'--output',str(output)]
        if args.mode=='genuine':
            manifest=args.archive.parent/'capture-native-before.json'
            if digest(args.archive)!=ARCHIVE_PIN or digest(manifest)!=MANIFEST_PIN:raise ValueError('Genuine source BEFORE provenance drift')
            report['required_reference_input']=dict(archive=str(args.archive.resolve()),archive_sha256=ARCHIVE_PIN,archive_bytes=args.archive.stat().st_size,capture_manifest_sha256=MANIFEST_PIN,unchanged_existing_reference=True)
            command+=['--archive',str(args.archive),'--archive-sha256',ARCHIVE_PIN]
        report['command']=command;result=subprocess.run(command,capture_output=True,text=True)
        legacy=output/'report.json'
        if legacy.is_file():report['legacy_report']=json.loads(legacy.read_text())
        report['compiler_evidence']={str(path.relative_to(output)):json.loads(path.read_text())for path in output.glob('*.json')if path.name!='report.json'}
        report['nested_cmake_receipts']={str(path.relative_to(output)):json.loads(path.read_text())for path in output.glob('*/report.json')}
        if result.returncode or not report.get('legacy_report',{}).get('passed'):
            report['diagnostic']=(result.stdout+result.stderr)[-16000:];raise ValueError('Established hull-query '+args.mode+' value regression failed')
        report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(digest(ROOT/name)==pin for name,pin in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Hull-query value source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);text=json.dumps(report,indent=2)+'\n'
    if len(text.encode())>=1024*1024:raise ValueError('Bounded hull-query value report exceeded1MiB')
    args.report.write_text(text);print(json.dumps({key:report[key]for key in('passed','mode','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
