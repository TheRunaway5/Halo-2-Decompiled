#!/usr/bin/env python3
"""Run the real Cache/original preparation cohort in separate owned processes.

The supplied binary must be built in this active run_test.py owner. Catalog
admission comes from the independently pinned exporter report. Keep only small
per-map receipts; Cache snapshots and all temporary banks remain disposable.
--world selects the actual native World/restore integration executable instead
of the Cache preparation/Audio fixture. Neither mode certifies normal gameplay.
--catalog selects immutable portable catalog admission and preparation.
--map-admission selects the produced candidate's normal Cache/default preparation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('binary','preparation','inventory','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--timeout-seconds',type=int,default=600)
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--world',action='store_true',help='Run the native World initialization, sound and checkpoint integration test')
    mode.add_argument('--catalog',action='store_true',help='Run portable original catalog admission, binding and preparation')
    mode.add_argument('--map-admission',action='store_true',help='Run normal Cache and automatic preparation in a produced private candidate')
    parser.add_argument('--candidate',type=Path,help='Bundle producer report, required for --map-admission')
    parser.add_argument('--world-admission',choices=('countdown','direct'),default='direct',help='Default loading path when --world uses a produced candidate')
    parser.add_argument('--require-prefix-dat',action='store_true',help='Require complete typed MCC DAT byte/sector reads in --map-admission mode')
    parser.add_argument('--require-prefix-pcm',action='store_true',help='Require every retained PCM route to use the same immutable DAT snapshots')
    parser.add_argument('--require-prefix-world',action='store_true',help='Require retained-prefix native duration and restore replay in --world mode')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.require_prefix_dat or args.map_admission
    assert not args.require_prefix_pcm or(args.map_admission and args.require_prefix_dat)
    assert not args.require_prefix_world or args.world
    assert 30<=args.timeout_seconds<=900
    args.binary=args.binary.resolve(strict=True);assert args.binary.is_relative_to(owner)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    work=owner/('cache-matrix-'+args.output.name);work.mkdir(exist_ok=False)
    pins={p.resolve():sha(p.resolve())for p in (Path(__file__),ROOT/'tools/run_test.py',args.binary,args.preparation,args.inventory)}
    report=dict(passed=False,goal_complete=False,live_world_or_device_verified=False,native_mcc_prefix_dat_backing_verified=False,
        original_waveform_or_wet_feedback_verified=False,retained_large_artifacts=[],cases=[],
        mode='native-world-component'if args.world else'normal-map-admission-component'if args.map_admission else'catalog-component'if args.catalog else'cache-preparation-component')
    try:
        prepared=json.loads(args.preparation.read_text());assert prepared['passed']and prepared['sound_banks']==31
        assert prepared['inputs'][str(args.inventory.resolve())]==pins[args.inventory.resolve()]
        catalog=Path(prepared['catalog']).resolve(strict=True);assert catalog.is_relative_to(owner)and sha(catalog)==prepared['catalog_sha256']
        pins[catalog]=prepared['catalog_sha256'];cases=json.loads(catalog.read_text())['cases']
        assert len(cases)==prepared['contexts']==908
        by_name={r['name']:r for r in json.loads(args.inventory.read_text())['maps']}
        maps=sorted({c['map']for c in cases});assert len(maps)==38
        candidate=None
        if args.map_admission or(args.world and args.candidate):
            assert args.candidate
            pins[args.candidate.resolve()]=sha(args.candidate.resolve());produced=json.loads(args.candidate.read_text())
            assert produced['passed']and produced['inputs_unchanged']and produced['contexts']==38 and produced['original_bindings']==908
            candidate=Path(produced['candidate']).resolve(strict=True);assert candidate.is_relative_to(owner)
            pointer=produced['catalog'];assert json.loads((candidate/'provenance.json').read_text())['original_sound_catalog']==pointer
            pins[candidate/'provenance.json']=sha(candidate/'provenance.json')
            bundled=candidate/pointer['path'];assert sha(bundled)==pointer['sha256'];pins[bundled]=pointer['sha256']
            assert produced['input_hashes'][str(args.inventory.resolve())]==pins[args.inventory.resolve()]
        tested_paths={}
        for name in maps:
            assert Path(name).name==name
            m=by_name[name];path=Path(m['mcc_path']).resolve(strict=True);assert sha(path)==m['mcc_sha256'];pins[path]=m['mcc_sha256']
            if candidate:
                matching=[relative for relative,row in pointer['maps'].items()if row['original_map']==name]
                assert len(matching)==1
                path=(candidate/matching[0]).resolve(strict=True);assert path.is_relative_to(candidate)and sha(path)==m['mcc_sha256'];pins[path]=m['mcc_sha256']
            tested_paths[name]=path
        for ordinal,name in enumerate(maps):
            path=tested_paths[name];output=args.output/(Path(name).stem+'.json')
            command=[sys.executable,'-B',str(ROOT/'tools/run_test.py'),'--scratch-root',str(work/'cases'),'--',
                str(args.binary),str(path),name,str(catalog),prepared['catalog_sha256']]
            if args.world:command.append('@HALO2_TEST_SCRATCH@/world')
            command.append(str(output))
            if args.world and candidate:command.extend([args.world_admission,str(Path(by_name[name]['mcc_path']).resolve(strict=True))])
            log=work/(str(ordinal)+'.log');timed_out=False
            with log.open('wb')as stream:
                try:completed=subprocess.run(command,env=dict(os.environ,XDG_CACHE_HOME='@HALO2_TEST_SCRATCH@/cache',SDL_AUDIODRIVER='dummy'),
                    stdout=stream,stderr=subprocess.STDOUT,timeout=args.timeout_seconds);code=completed.returncode
                except subprocess.TimeoutExpired:code=124;timed_out=True
            data=json.loads(output.read_text())if output.is_file()else {}
            world_complete=not args.world or(data.get('original_world_checkpoint_restored')is True and
                data.get('resource_identity_retained_through_world_restore')is True and data.get('selection_duration_replay_equal')is True and
                data.get('old_epoch_consumers_refused')is True and data.get('fresh_binding_playback_after_restore')is True and
                data.get('actual_duration_caller_observations',0)>=data.get('original_bindings',1)>0 and
                data.get('audio_original_voices',0)==data.get('original_bindings',1))
            if args.world and candidate:
                world_complete=world_complete and data.get('normal_cache_catalog_admitted')is True and data.get('preparation_path')==args.world_admission
            if args.require_prefix_world:
                world_complete=world_complete and data.get('retained_prefix_native_duration_verified')is True and data.get('retained_prefix_duration_replay_equal')is True and data.get('retained_prefix_duration_caller_observations',0)>0
            catalog_complete=not args.catalog or(data.get('relocation_identity_equal')is True and data.get('input_storage_retained')is True and
                data.get('canonical_preparation_executed')is True and data.get('raw_cache_unchanged')is True and data.get('original_bindings',0)>0)
            map_complete=not args.map_admission or(data.get('normal_cache_catalog_admitted')is True and data.get('countdown_preparation_automatic')is True and
                data.get('direct_resources_metadata_paired')is True and data.get('equal_resource_identity_distinct_paired_owners')is True and
                data.get('raw_cache_unchanged')is True and data.get('original_bindings',0)>0)
            if args.require_prefix_dat:
                map_complete=map_complete and data.get('mcc_prefix_dat_routes_verified')is True and data.get('mcc_prefix_dat_routes',0)>0 and data.get('mcc_prefix_sector_bytes',0)>0
            if args.require_prefix_pcm:
                map_complete=map_complete and data.get('mcc_prefix_pcm_same_dat_owner_verified')is True and data.get('mcc_prefix_pcm_permutations',0)>0 and data.get('mcc_prefix_pcm_chunk_reads',0)>=data.get('mcc_prefix_dat_routes',1)
            record=dict(map=name,passed=code==0 and data.get('passed')is True and world_complete and catalog_complete and map_complete,exit_code=code,timed_out=timed_out,command=command,
                report=str(output)if output.is_file()else None,report_sha256=sha(output)if output.is_file()else None,coverage=data)
            if args.world:record['world_coverage_complete']=world_complete
            if args.catalog:record['catalog_coverage_complete']=catalog_complete
            if args.map_admission:record['map_admission_coverage_complete']=map_complete
            if not record['passed']:
                with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-16384));tail=stream.read().decode(errors='replace')
                (args.output/(Path(name).stem+'-failure.log')).write_text(tail)
            report['cases'].append(record)
            print(json.dumps(dict(case=ordinal+1,total=len(maps),map=name,passed=record['passed'],error=data.get('error'))),flush=True)
        report.update(passed=all(r['passed']for r in report['cases']),native_cache_contexts=len(maps),
            binary_sha256=pins[args.binary],preparation_sha256=pins[args.preparation.resolve()],catalog_sha256=prepared['catalog_sha256'])
        good=[c['coverage']for c in report['cases']if c['passed']]
        keys=(('original_bindings','encoded_routes','negative_controls')if args.catalog or args.map_admission else
              ('original_bindings','native_selections','stored_time_queries','native_duration_queries','actual_duration_caller_observations',
               'audio_original_voices','negative_controls')if args.world else
              ('baseline_bindings','baseline_stored_headers','baseline_permutation_times','original_bindings','original_routes','original_sector_reads',
               'preserved_mcc_timing_bindings','preserved_stored_headers','preserved_permutation_times','negative_controls'))
        report['passed_context_totals']={k:sum(row[k]for row in good)for k in keys}
        if args.require_prefix_dat:
            report['typed_MCC_DAT_bytes_qualified']=len(good)==38
            report['prefix_DAT_totals']={key:sum(row[key]for row in good)for key in
                ('mcc_prefix_dat_routes','mcc_prefix_sector_bytes','mcc_prefix_unaligned_offsets','mcc_prefix_high_offsets')}
        if args.require_prefix_pcm:
            report['prefix_PCM_totals']={key:sum(row[key]for row in good)for key in('mcc_prefix_pcm_permutations','mcc_prefix_pcm_chunk_reads')}
        if args.world:report['native_world_original_sound_qualified']=len(good)==38
        elif args.catalog:report['portable_original_sound_catalog_qualified']=len(good)==38
        elif args.map_admission:report['normal_map_original_sound_admission_qualified']=len(good)==38
        else:
            report['original_variation_audio_qualified']=len(good)==38 and all(row.get('variation_audio',{}).get('audio_original_voices',0)>0 for row in good)
            if report['original_variation_audio_qualified']:
                report['variation_audio_totals']={k:sum(row['variation_audio'][k]for row in good)for k in
                    ('provider_definitions','provider_encoded_routes','audio_original_voices','decoded_original_frames','audio_packets_with_different_mcc_frame_count')}
    except BaseException as error:report['error']=str(error);raise
    finally:
        changed=[str(p)for p,h in pins.items()if not p.is_file()or sha(p)!=h]
        report.update(input_hashes={str(p):h for p,h in pins.items()},changed_inputs=changed,inputs_unchanged=not changed)
        if changed:report['passed']=False
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
