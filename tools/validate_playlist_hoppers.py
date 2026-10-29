#!/usr/bin/env python3
"""Frozen Rust/service qualification; invoke through tools/run_test.py."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT=Path(__file__).resolve().parents[1]


def native_qualification(scratch,output,prepare_client=False,jobs=2,eos_client_config=None):
    from validate_online_protocol_security import freeze,identity
    client_config=None
    if prepare_client:
        selection=json.loads((ROOT/'gameplay-runtime-selection.json').read_bytes())
        client_config=eos_client_config or (ROOT/selection['runtime']).parent/'eos-client.env'
        if not client_config.is_file():
            raise ValueError('Preparing an ONLINE client requires --eos-client-config when the selected runtime is offline')
    source=scratch/'source';pins=freeze(source,True)
    # Rust tests and catalog qualification must consume the same immutable
    # metadata as the service, not the freezer's borrowed multiplayer tree.
    (source/'multiplayer').unlink()
    for base in (ROOT/'multiplayer/master_server',):
        for path in base.rglob('*'):
            if not path.is_file() or any(p in ('__pycache__','target') for p in path.relative_to(base).parts):continue
            if path.suffix in ('.so','.dll','.dylib','.pyc'):continue
            relative=path.relative_to(ROOT);target=source/relative;target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(path.read_bytes());pins[str(relative)]=identity(target)
    for name in ('hopper_selection_tu15_original.json','matchmaking_assignment_original.json'):
        target=source/'tests/fixtures'/name
        if target.is_symlink():target.unlink()
        target.write_bytes((ROOT/'tests/fixtures'/name).read_bytes())
        pins['tests/fixtures/'+name]=identity(target)
    (output/'source-manifest.json').write_text(json.dumps(pins,indent=2)+'\n')
    tests=['original_game_variant_buffers','online_matchmaking_client','native_session_configuration',
           'native_session_configuration_loading','native_request_ingress','native_lan_admission_security',
           'native_lan_startup','native_lan_snapshot_stream','native_mcc_multiplayer_accuracy_tu15']
    command=['python3',str(source/'tools/run_tests.py'),'--build-worker','--source',str(source),'--jobs',str(jobs),'--test-jobs','1',
             '--target','source_game_variant_tests','--target','online_matchmaking_tests','--target','multiplayer_tests',
             '--target','multiplayer_engine_combat_tu15_tests',
             '--filter','^('+'|'.join(tests)+')$','--output',str(output/'cmake'),
             '--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF','--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF',
             '--cmake-arg=-DHALO2_ONLINE_RUST_OFFLINE=ON','--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE=',
             '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT='+str(ROOT),
             '--cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP='+str(ROOT/'assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map'),
             '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL='+str(ROOT/'build-multiplayer/_deps/halo2_datachannel-src'),
             '--cmake-arg=-DHALO2_DISCORD_SOCIAL_SDK_ROOT='+str(ROOT/'.local/discord-sdk/discord_social_sdk'),
             '--retain-report','Testing/Temporary/LastTest.log','--retain-report','multiplayer-accuracy.json']
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1');env.pop('HALO2_EOS_SDK_ROOT',None)
    result=subprocess.run(command,env=env)
    client_command=None
    if result.returncode==0 and prepare_client:
        client_command=['python3','-B',str(ROOT/'tools/build_launcher_engine.py'),'--worker',
            '--reuse-owned-build','--prepare-only','--root',str(ROOT),'--output',str(output/'client'),
            '--jobs',str(jobs),'--eos-client-config',str(client_config)]
        result=subprocess.run(client_command,env=env)
    report={'passed':result.returncode==0,'command':command,'client_command':client_command,'scratch':str(scratch),
            'live_source_changes':[name for name,expected in pins.items() if not (ROOT/name).is_file() or identity(ROOT/name)!=expected],
            'scope':'Frozen native option/client/configuration/ingress tests; no installed runtime, live EOS or played-match acceptance'}
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return result.returncode


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--native',action='store_true')
    parser.add_argument('--prepare-client',action='store_true',help='Also qualify an uninstalled client from the same owned native build')
    parser.add_argument('--jobs',type=int,default=2)
    parser.add_argument('--eos-client-config',type=Path)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    if args.prepare_client and not args.native:parser.error('--prepare-client requires --native')
    if not 1<=args.jobs<=16:parser.error('--jobs must be 1..16')
    if args.native:return native_qualification(scratch,args.output,args.prepare_client,args.jobs,args.eos_client_config)
    source=scratch/'source';source.mkdir()
    manifest={}
    for relative in ('rust/online_protocol','rust/online_security_ffi','multiplayer/master_server','tests/fixtures'):
        for path in (ROOT/relative).rglob('*'):
            if not path.is_file() or any(n in ('target','__pycache__','.git') for n in path.relative_to(ROOT/relative).parts):continue
            if path.suffix in ('.so','.dll','.dylib','.pyc'):continue
            data=path.read_bytes();name=str(path.relative_to(ROOT));destination=source/name
            destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data)
            manifest[name]=hashlib.sha256(data).hexdigest()
    # Optional canonical catalog sources are covered by the service copy.
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',CARGO_TARGET_DIR=str(scratch/'cargo-target'),
             CARGO_INCREMENTAL='0',RUSTUP_TOOLCHAIN='1.96.0')
    report={'source_sha256':manifest,'scratch':str(scratch),'steps':[],'passed':False}
    args.output.mkdir(parents=True,exist_ok=True)
    steps=[('rust-tests',['cargo','test','--locked','--offline','--manifest-path',str(source/'rust/online_protocol/Cargo.toml')]),
           ('rust-clippy',['cargo','clippy','--locked','--offline','--manifest-path',str(source/'rust/online_protocol/Cargo.toml'),'--all-targets','--','-D','warnings']),
           ('service-library',['cargo','build','--locked','--offline','--release','--manifest-path',str(source/'rust/online_security_ffi/Cargo.toml')]),
           ('backend',['python3','-B','-m','unittest','discover','-s','multiplayer/master_server/tests','-t','.'])]
    env['H2_SECURITY_LIBRARY']=str(scratch/'cargo-target/release/libhalo2_online_security_ffi.so')
    try:
        for label,command in steps:
            began=time.monotonic()
            with (args.output/(label+'.log')).open('w') as log:
                result=subprocess.run(command,cwd=source,env=env,stdout=log,stderr=subprocess.STDOUT)
            report['steps'].append({'name':label,'command':command,'exit_code':result.returncode,'seconds':round(time.monotonic()-began,3)})
            print(label,result.returncode,flush=True)
            if result.returncode: return result.returncode
        report['passed']=True
        return 0
    finally:
        report['live_source_changes']=[name for name,digest in manifest.items()
             if not (ROOT/name).is_file() or hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=digest]
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':
    raise SystemExit(main())
