#!/usr/bin/env python3
"""Qualify immutable original-bank client playback in an active owned build.

The bundle producer's exact catalog/map pins admit all 38 contexts. Each native
process owns disposable Cache/SDL state; only bounded reports remain. This
does not certify native World timing, network delivery or physical audio.
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
    with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def need(value,why):
    if not value:raise ValueError(why)
def save(path,data):
    raw=json.dumps(data,indent=2)+'\n';need(len(raw.encode())<=1024*1024,'Report exceeds its bounded budget');path.write_text(raw)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('binary','bundle','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    binary=args.binary.resolve(strict=True);need(binary.is_relative_to(owner),'Require an active owned binary')
    produced=json.loads(args.bundle.read_text())
    need(produced.get('passed')and produced.get('inputs_unchanged')and produced.get('private_candidate_inodes')and
         produced.get('contexts')==38 and produced.get('original_bindings')==908,'Require the complete produced bundle')
    candidate=Path(produced['candidate']);need(candidate==candidate.resolve(strict=True)and candidate.is_relative_to(owner),'Candidate escaped its owned build')
    provenance=candidate/'provenance.json';pointer=json.loads(provenance.read_text())['original_sound_catalog']
    need(pointer==produced['catalog'],'Bundle producer/catalog differs')
    catalog=candidate/pointer['path'];need(catalog.resolve(strict=True).is_relative_to(candidate),'Catalog escaped its candidate')
    need(sha(catalog)==pointer['sha256']and catalog.stat().st_size==pointer['bytes'],'Catalog pin differs')
    pins={p.resolve():sha(p)for p in [Path(__file__),binary,args.bundle,provenance,catalog]}
    args.output=args.output.absolute();args.output.mkdir(parents=True,exist_ok=False)
    work=owner/('immutable-client-'+args.output.name);work.mkdir(exist_ok=False)
    report={'passed':False,'goal_complete':False,'world_factory_or_network_or_device_verified':False,'cases':[],'input_hashes':{str(p):d for p,d in pins.items()}}
    for ordinal,(relative,row)in enumerate(sorted(pointer['maps'].items())):
        path=candidate/relative;need(path.resolve(strict=True).is_relative_to(candidate),'Map escaped its candidate')
        need(sha(path)==row['input_sha256']and path.stat().st_size==row['input_bytes'],'Admitted map differs')
        pins[path.resolve()]=row['input_sha256'];result=args.output/(Path(row['original_map']).stem+'.json')
        command=[sys.executable,'-B',str(ROOT/'tools/run_test.py'),'--scratch-root',str(work/'cases'),'--',
                 str(binary),str(path),str(result)]
        log=work/(str(ordinal)+'.log')
        try:
            with log.open('wb')as stream:
                run=subprocess.run(command,env=dict(os.environ,SDL_AUDIODRIVER='dummy'),stdout=stream,stderr=subprocess.STDOUT,timeout=180)
            entry={'context':row['original_map'],'exit_code':run.returncode,'passed':False,'command':command}
            if result.is_file():
                need(result.stat().st_size<=1024*1024,'Native report exceeds its bounded budget');native=json.loads(result.read_text())
                entry.update({k:native[k]for k in ('original_bindings','pitch_definitions','original_encoded_routes','original_pcm_voices','negative_controls')if k in native})
                entry.update(report=str(result),sha256=sha(result),passed=run.returncode==0 and native.get('passed')is True)
            if not entry['passed']:entry['diagnostic_tail']=log.read_text(errors='replace')[-4096:]
        except subprocess.TimeoutExpired:
            entry={'context':row['original_map'],'passed':False,'timed_out':True,'command':command}
        report['cases'].append(entry);save(args.output/'report.json',report)
        print(json.dumps({k:entry[k]for k in ('context','passed','exit_code')if k in entry}),flush=True)
    good=[r for r in report['cases']if r['passed']]
    report['totals']={k:sum(r.get(k,0)for r in good)for k in ('original_bindings','pitch_definitions','original_encoded_routes','original_pcm_voices','negative_controls')}
    report['input_hashes']={str(p):d for p,d in pins.items()};report['inputs_unchanged']=all(sha(p)==d for p,d in pins.items())
    report['passed']=len(good)==38 and report['totals']['original_bindings']==908 and report['inputs_unchanged']
    case_root=work/'cases';report['case_scratch_removed']=not case_root.exists()or not any(case_root.iterdir())
    need(report['case_scratch_removed'],'Case scratch survived native validation')
    save(args.output/'report.json',report);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

