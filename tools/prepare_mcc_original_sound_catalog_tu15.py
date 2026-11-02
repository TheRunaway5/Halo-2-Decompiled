#!/usr/bin/env python3
"""Produce an original-sound bundle and exact map admission in owned MCC scratch.

The native context producer converts each actual Cache and binds authenticated
original banks. Original maps are inputs to the separately pinned exporter;
this stage consumes its prepared banks. Existing prepared assets stay read-only.
Linux FICLONE creates distinct private files; no runtime is selected or promoted.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess

from mcc_native_map_owner_publication import verify_owner_index
from mcc_prepared_cache_delta import _clone_tree, _identity, _snapshot, _snapshot_sha

ROOT=Path(__file__).resolve().parents[1]
SCHEMA='H2_PREPARED_ORIGINAL_SOUND_CATALOG_TU15_V1'


def need(value,why):
    if not value:raise ValueError(why)


def sha(path):
    with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def source_snapshot(root):
    # Existing assets may have shared read-only inode owners. Clones always get
    # distinct private inodes; this stage never writes through those owners.
    entries={'.':_identity(root.lstat())}
    for path in sorted(root.rglob('*')):
        info=path.lstat();need(stat.S_ISDIR(info.st_mode)or stat.S_ISREG(info.st_mode),'Nonregular source cache owner')
        entries[path.relative_to(root).as_posix()]=_identity(info)
    return entries


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('source','destination','preparation','inventory','context-producer','output'):
        parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=args.source.resolve(strict=True);destination=args.destination.absolute();binary=args.context_producer.resolve(strict=True)
    need(source==args.source.absolute()and all(not p.is_symlink()for p in [source,*source.parents]),'Aliased source cache root')
    need(binary.is_relative_to(owner)and destination.is_relative_to(owner)and destination!=owner and
        not destination.exists()and not destination.is_symlink()and not destination.is_relative_to(source),'Require new owned candidate and native producer')
    args.output=args.output.absolute();args.output.mkdir(parents=True,exist_ok=False)
    pins={p.resolve():sha(p.resolve())for p in [args.preparation,args.inventory,binary,Path(__file__),
        ROOT/'tools/mcc_native_map_owner_publication.py',ROOT/'tools/mcc_prepared_cache_delta.py']}
    report=dict(passed=False,goal_complete=False,normal_gameplay_verified=False,world_or_device_verified=False,
        source=str(source),candidate=str(destination),retained_large_artifacts=[])
    candidate_identity=None
    try:
        prepared=json.loads(args.preparation.read_text());inventory=json.loads(args.inventory.read_text())
        need(prepared['passed']and prepared['sound_banks']==31 and prepared['contexts']==908,'Incomplete original bank preparation')
        need(prepared['inputs'][str(args.inventory.resolve())]==pins[args.inventory.resolve()],'Inventory differs from exporter admission')
        prototype=Path(prepared['catalog']).resolve(strict=True)
        need(prototype.is_relative_to(owner)and sha(prototype)==prepared['catalog_sha256'],'Exporter catalog differs from external pin')
        pins[prototype]=prepared['catalog_sha256'];cases=json.loads(prototype.read_text())['cases']
        need(len(cases)==908,'Incomplete original context set')
        names=sorted({row['map']for row in cases});need(len(names)==38,'Incomplete original map set')
        maps={row['name']:row for row in inventory['maps']}
        provenance=json.loads((source/'provenance.json').read_text())
        need('original_sound_catalog'not in provenance,'Source already declares an original catalog')
        need(verify_owner_index(source,provenance)is not None,'Original sounds require indexed prepared map owners')
        for name in names:
            row=maps[name];path=Path(row['mcc_path']).resolve(strict=True)
            need(path.is_relative_to(source)and sha(path)==row['mcc_sha256'],'Prepared source map differs from exact inventory owner')
            pins[path]=row['mcc_sha256']
        before=source_snapshot(source);destination.mkdir();candidate_identity=(destination.stat().st_dev,destination.stat().st_ino)
        report['clone']=_clone_tree(source,destination,before)
        bundle=destination/'original-sounds';bundle.mkdir()
        for case in cases:
            bank=Path(case['directory']);need(bank.parent==prototype.parent and bank.name.startswith('bank-'),'Exporter bank escaped its owner')
            target=bundle/bank.name;target.mkdir()
            need(sha(bank/'manifest.json')==case['sha256'],'Prepared manifest differs from exporter pin')
            # Reflink each bank's files into private inodes. Runtime loading
            # authenticates both metadata and every resource sector again.
            bank_snapshot=source_snapshot(bank)
            _clone_tree(bank,target,bank_snapshot)
        contexts=[];admission={};receipts=[]
        for n,name in enumerate(names):
            encoded=Path(maps[name]['mcc_path']).resolve(strict=True).relative_to(source)
            path=destination/encoded;receipt=args.output/(Path(name).stem+'.json')
            log=owner/('original-catalog-context-'+str(n)+'.log')
            command=[str(binary),str(path),name,str(prototype),prepared['catalog_sha256'],str(bundle),str(receipt)]
            with log.open('wb')as stream:
                code=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,timeout=600,
                    env=dict(os.environ,XDG_CACHE_HOME=str(owner/'producer-cache'))).returncode
            data=json.loads(receipt.read_text())if receipt.is_file()else{}
            if code or not data.get('passed'):
                (args.output/(Path(name).stem+'-failure.log')).write_text(log.read_text(errors='replace')[-16384:])
            need(code==0 and data.get('passed')and data.get('actual_cache_conversion_and_catalog_binding'),'Native context producer failed: '+name)
            contexts.append(data['context']);receipts.append(dict(map=name,path=str(receipt),sha256=sha(receipt)))
            admission[encoded.as_posix()]=dict(input_sha256=maps[name]['mcc_sha256'],input_bytes=path.stat().st_size,original_map=name)
            print(json.dumps(dict(case=n+1,total=38,map=name,passed=True)),flush=True)
        catalog=bundle/'catalog.json';catalog.write_text(json.dumps(dict(schema=SCHEMA,contexts=contexts),indent=2)+'\n')
        pointer=dict(schema=SCHEMA,path='original-sounds/catalog.json',sha256=sha(catalog),bytes=catalog.stat().st_size,maps=admission)
        provenance['original_sound_catalog']=pointer
        (destination/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
        need(verify_owner_index(destination,provenance)is not None,'Catalog changed prepared map admission')
        need(source_snapshot(source)==before,'Original prepared cache changed during catalog production')
        final=_snapshot(destination)
        report.update(passed=True,contexts=len(contexts),original_bindings=len(cases),catalog=pointer,
            context_receipts=receipts,source_tree_snapshot_sha256=_snapshot_sha(before),
            original_cache_unchanged=True,private_candidate_inodes=True,
            output_files=sum(stat.S_ISREG(row[2])for row in final.values()),
            output_logical_bytes=sum(row[4]for row in final.values()if stat.S_ISREG(row[2])),
            native_context_producer_sha256=pins[binary])
    except BaseException as error:
        report['error']=str(error)
        # Only this newly created candidate is a deletion candidate.
        if candidate_identity is not None and destination.exists():
            need(not destination.is_symlink()and(destination.stat().st_dev,destination.stat().st_ino)==candidate_identity,'Candidate owner changed before cleanup')
            shutil.rmtree(destination)
        raise
    finally:
        changed=[str(p)for p,h in pins.items()if not p.is_file()or sha(p)!=h]
        report.update(input_hashes={str(p):h for p,h in pins.items()},changed_inputs=changed,inputs_unchanged=not changed)
        if changed:report['passed']=False
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
