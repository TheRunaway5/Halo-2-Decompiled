#!/usr/bin/env python3
"""Qualify native material routing in a disposable, preserved source cohort."""
import argparse
import hashlib
import json
import os
from pathlib import Path,PurePosixPath
import shutil
import subprocess
import sys
import tarfile
from native_damage_material_kernel_tu15 import install_native_material_lookup,strip_native_material_lookup

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/destruction-goal-20261022'


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def diagnostics(text):
    additions=[
        ('#include "halo2/source_damage_model_native_tu15.hpp"\n',
         '#include "halo2/source_damage_material_native_tu15.hpp"\n'),
        ('  SourceDamageModelNativeActivity native_model_before=source_damage_model_native_activity();\n',
         '  SourceDamageMaterialNativeActivity native_material_before=source_damage_material_native_activity();\n'),
        ('        {"resolved",model.resolved-native_model_before.resolved}};\n',
         '''    const auto material=source_damage_material_native_activity();
    recorded["native_damage_material_activity"]={{"calls",material.calls-native_material_before.calls},
        {"groups_found",material.groups_found-native_material_before.groups_found},
        {"modifiers_found",material.modifiers_found-native_material_before.modifiers_found}};
'''),
        ('        "Actual source composition executed no native model-damage lookup");\n',
         '''  if(std::getenv("HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL") && name!="campaign_guntower_no_damage")
    check(source_damage_material_native_activity().calls>report.native_material_before.calls,
        "Actual source composition executed no native material-damage lookup");
''')]
    for anchor,addition in additions:
        if text.count(anchor)!=1:raise ValueError('Material diagnostics seam changed')
        text=text.replace(anchor,anchor+addition)
    return text


def validate(args,owner,source,report):
    def save():(args.output/'admission.json').write_text(json.dumps(report,indent=2)+'\n')
    def unchanged():
        return all(sha(source/name)==row['after']for name,row in report['changes'].items())and all(
            sha(source/name)==row['sha256']for name,row in report['sdk_references'].items())
    if not unchanged():raise ValueError('Prepared native source or SDK reference drifted')
    registration=json.loads(args.registration.read_text())if args.registration else None
    if registration and (Path(registration['owner']).resolve()!=owner or sha(source/'CMakeLists.txt')!=registration['cmake_after']):
        raise ValueError('Prepared full-cohort registration drifted')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(ghost|campaign_guntower_no_damage|wraith_minigun|campaign_flood_clump)$',
        '--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    for case in report['cases']:command+=['--retain-report',f'destruction-{case}.json']
    report.update(command=command,status='running',validation_not_run=False)
    if args.registration:report['full_cohort_registration']=str(args.registration.resolve())
    save();print(json.dumps(dict(status='building_native_world',owner=str(owner))),flush=True)
    result=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1',
        HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL='1',HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL='1'),
        stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report['validation_returncode']=result
    original=json.loads(args.original_qualification.read_text())
    report['original_qualification']=dict(path=str(args.original_qualification),sha256=sha(args.original_qualification),
        passed=original['passed']and original.get('whole_owner_component_ABI_and_banks_qualified',False))
    report['source_unchanged']=unchanged()
    if registration:report['source_unchanged'] &= sha(source/'CMakeLists.txt')==registration['cmake_after']
    report['passed']=result==0 and report['original_qualification']['passed']and report['source_unchanged']
    report['status']='passed'if report['passed']else'failed'
    save();print(json.dumps(dict(passed=report['passed'],report=str(args.output/'admission.json'))),flush=True)
    return 0 if report['passed']else result or 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--original-qualification',required=True,type=Path)
    parser.add_argument('--prepare-only',action='store_true')
    parser.add_argument('--resume',action='store_true',help='Validate this same still-owned prepared source/build')
    parser.add_argument('--registration',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    if args.prepare_only and args.resume:parser.error('Choose prepare or resume')
    args.output=args.output.resolve()
    if args.resume:
        report=json.loads((args.output/'admission.json').read_text())
        if Path(report['owner']).resolve()!=owner:raise ValueError('Prepared build belongs to another owner')
        return validate(args,owner,source,report)
    args.output.mkdir(parents=True,exist_ok=False)
    archive=BASE/'source-reference-v1/qualified-native-source.tar.gz'
    retention=json.loads((archive.parent/'retention.json').read_text())
    if sha(archive)!=retention['sha256']:raise ValueError('Required qualified source archive changed')
    report=dict(passed=False,owner=str(owner),source_reference=dict(path=str(archive),sha256=sha(archive)),
        changes={},source_unchanged=False,complete_goal_acceptance=False,retained_large_artifacts=[])
    def save():(args.output/'admission.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    # Materialize regular source before reference symlinks, never follow a
    # symlink during extraction and never extract outside the owned root.
    links=[]
    with tarfile.open(archive,'r:gz')as bundle:
        for member in bundle:
            name=PurePosixPath(member.name)
            if name.is_absolute()or '..'in name.parts or not name.parts or name.parts[0]not in ('source','qualification'):
                raise ValueError('Unexpected archived source path')
            target=owner.joinpath(*name.parts)
            if member.issym():links.append((target,member.linkname));continue
            if member.isdir():target.mkdir(parents=True,exist_ok=True);continue
            if not member.isfile():raise ValueError('Unexpected archived source member')
            target.parent.mkdir(parents=True,exist_ok=True)
            with bundle.extractfile(member)as incoming,target.open('xb')as output:shutil.copyfileobj(incoming,output)
            target.chmod(member.mode&0o777)
    for target,reference in links:
        if '.local/test-scratch/'in reference:raise ValueError('Archived reference points at a retired private owner')
        target.parent.mkdir(parents=True,exist_ok=True);target.symlink_to(reference)
    # The source-only reference intentionally excludes four SDK binaries.
    # Restore missing required inputs as read-only references to port-owned
    # files, authenticated against the archived SDK manifest before building.
    sdk=source/'external/upscaling';manifest=sdk/'manifest.json'
    report['sdk_manifest_sha256']=sha(manifest);report['sdk_references']={}
    for row in json.loads(manifest.read_text())['files']:
        relative=PurePosixPath(row['path'])
        if relative.is_absolute()or '..'in relative.parts:raise ValueError('SDK manifest path escaped its owner')
        target=sdk.joinpath(*relative.parts);restored=not target.exists()
        if restored:
            original=ROOT/'external/upscaling'/Path(*relative.parts)
            if original.stat().st_size!=row['bytes']or sha(original)!=row['sha256']:raise ValueError('Required SDK input differs from frozen manifest')
            target.parent.mkdir(parents=True,exist_ok=True);target.symlink_to(original)
        if target.stat().st_size!=row['bytes']or sha(target)!=row['sha256']:raise ValueError('Frozen SDK input changed')
        report['sdk_references'][str(target.relative_to(source))]=dict(sha256=row['sha256'],bytes=row['bytes'],
            restored_read_only_reference=restored,path=str(target.resolve()))
    save()
    def replace(name,text):
        path=source/name;before=sha(path)if path.exists()else None
        if path.is_symlink():raise ValueError('Native overlay would modify an external reference')
        path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text)
        report['changes'][name]=dict(before=before,after=sha(path))
    # Restore the sealed qualified overlay, independently of concurrent
    # workspace changes to neighboring native owners.
    delta_path=BASE/'material-native-source-delta-v1.json';delta=json.loads(delta_path.read_text())
    if delta['base_archive_sha256']!=sha(archive):raise ValueError('Material overlay belongs to another source archive')
    pins=json.loads((BASE/'material-native-frozen-owner-v1/source-hashes.json').read_text())
    for name,row in delta['changes'].items():
        relative=PurePosixPath(name)
        if relative.is_absolute()or '..'in relative.parts:raise ValueError('Material reference path escaped its owner')
        target=source/name;before=sha(target)if target.exists()else None
        if before!=row['before']:raise ValueError('Material reference preimage changed')
        if row['before']is None:
            target.parent.mkdir(parents=True,exist_ok=True);target.write_text(row['new_source'])
        else:
            result=subprocess.run(['patch','--batch','--forward','--silent','-p0','-d',str(source)],
                input=row['unified_diff'],text=True,capture_output=True)
            if result.returncode:raise ValueError('Sealed material source patch failed: '+result.stderr[-2000:])
        if sha(target)!=row['after']or (name in pins and sha(target)!=pins[name]):raise ValueError('Material overlay differs from qualified native source')
        report['changes'][name]=dict(before=before,after=sha(target))
    module='src/gameplay/units/source_damage_delivery_runtime_generated.cpp'
    text=(source/module).read_text();begin='// BEGIN native TU1.5 material damage\n';end='// END native TU1.5 material damage\n'
    if text.count(begin)!=1 or text.count(end)!=1:raise ValueError('Material platform guard seam changed')
    guarded=text.replace(begin,begin+'#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)\n').replace(end,'#endif\n'+end)
    expected=json.loads((BASE/'material-native-route-generation-v2.json').read_text())['generated_cpp_sha256']
    if hashlib.sha256(guarded.encode()).hexdigest()!=expected:raise ValueError('Qualified material platform guard changed')
    (source/module).write_text(guarded);report['changes'][module]['after']=sha(source/module)
    report['sealed_overlay']=dict(path=str(delta_path),sha256=sha(delta_path))
    report['workspace_module_sha256']=sha(ROOT/module)
    report['unrelated_workspace_module_drift_excluded']=sha(ROOT/module)!=expected
    report['cases']=['ghost','campaign_guntower_no_damage','wraith_minigun','campaign_flood_clump']
    if args.prepare_only:
        report.update(status='prepared',validation_not_run=True)
        save();print(json.dumps(dict(status='prepared',passed=False,owner=str(owner))),flush=True);return 0
    return validate(args,owner,source,report)


if __name__=='__main__':raise SystemExit(main())
