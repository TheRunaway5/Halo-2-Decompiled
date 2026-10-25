#!/usr/bin/env python3
"""Prepare exact original vehicle audio data for a future native resource owner.

No executable is included. Native tag binding, metadata relocation, voice
admission and live playback remain required. Existing assets/cache selection
are never replaced. Temporary staging belongs to the enclosing run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile

from mcc_prepared_cache_delta import _publish


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def require(value,message):
    if not value:raise ValueError(message)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('contract','destination','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    contract=json.loads(args.contract.read_text())
    require(contract['passed'] and contract['schema']=='H2_ORIGINAL_VEHICLE_FLYBY_IMPORT_CONTRACT_V1' and
            contract['distinct_original_contracts']==1 and len(contract['contexts'])==31,
            'Require the complete authenticated original AP import contract')
    destination=args.destination.absolute()
    require(destination.parent.is_dir() and not destination.exists() and not destination.is_symlink(),
            'Use a new destination under an existing owned asset directory')
    require(owner.stat().st_dev==destination.parent.stat().st_dev,'Atomic resource publication needs the same filesystem')
    normalized=next(iter(contract['contracts'].values()));context=contract['contexts'][0]
    require(sha(context['path'])==context['sha256'],'Original metadata map changed')
    literal=context['literal']
    require(len(literal['pitch_ranges'])==len(normalized['pitch_ranges'])==1,'Unexpected AP pitch shape')
    require(len(literal['pitch_ranges'][0]['permutations'])==6,'Unexpected AP permutation shape')
    require(normalized['pitch_ranges'][0].get('used_flags_hex')=='00',
            'Require the authenticated original initial permutation flags')
    pins={};payload=bytearray();permutations=[]
    for metadata,permutation in zip(normalized['pitch_ranges'][0]['permutations'],literal['pitch_ranges'][0]['permutations']):
        chunks=[];encoded=hashlib.sha256();encoded_bytes=0
        for part in permutation['parts']:
            path=Path(part['path'])
            if str(path) not in pins:
                pins[str(path)]=sha(path)
                require(pins[str(path)]==contract['resource_files'][str(path)],'Original resource changed')
            offset,size=part['offset'],part['bytes'];require(offset%512==0,'Original AP chunk is not sector aligned')
            sector_size=(size+511)&~511
            with path.open('rb') as stream:stream.seek(offset);sector=stream.read(sector_size)
            require(len(sector)==sector_size and hashlib.sha256(sector[:size]).hexdigest()==part['sha256'],
                    'Original payload or sector extent changed')
            bank_offset=len(payload);require(bank_offset%512==0,'Prepared chunk lost its sector alignment')
            payload.extend(sector);encoded.update(sector[:size]);encoded_bytes+=size
            chunks.append(dict(offset=bank_offset,bytes=size,sector_bytes=sector_size,source_flags=part['flags'],
                               encoded_sha256=part['sha256'],sector_sha256=hashlib.sha256(sector).hexdigest()))
        require(encoded_bytes==metadata['encoded_bytes'] and encoded.hexdigest()==metadata['encoded_sha256'],
                'Original complete permutation changed')
        permutations.append(dict(**metadata,literal_hex=permutation['raw_hex'],chunks=chunks))
    require(sum(p['encoded_bytes'] for p in permutations)==76860,'Original AP resource total changed')
    manifest=dict(schema='H2_PREPARED_ORIGINAL_SOUND_BANK_V2',name=contract['name'],
        authority='Original Xbox authored data; no source executable or emulator dependency',
        original_contract=normalized,literal_sound_hex=literal['sound_hex'],
        literal_pitch_range_hex=literal['pitch_ranges'][0]['raw_hex'],
        encoded_file=dict(path='resources.bin',bytes=len(payload),sha256=hashlib.sha256(payload).hexdigest()),
        permutations=permutations,original_contract_sha256=context['normalized_contract_sha256'],
        provenance=dict(contract_sha256=sha(args.contract),tool_sha256=sha(Path(__file__)),
                        contexts=[dict(map=c['map'],sha256=c['sha256']) for c in contract['contexts']]),
        native_metadata_binding_complete=False,live_playback_verified=False)
    with tempfile.TemporaryDirectory(prefix='sound-bank-',dir=owner) as temporary:
        stage=Path(temporary)/'bank';stage.mkdir()
        (stage/'resources.bin').write_bytes(payload)
        (stage/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        require(sha(stage/'resources.bin')==manifest['encoded_file']['sha256'],'Prepared resource verification failed')
        require(all(sha(path)==pin for path,pin in pins.items()),'Original resource changed during preparation')
        _publish(stage,destination)
    report=dict(passed=True,owner=str(owner),schema=manifest['schema'],destination=str(destination),
                manifest_sha256=sha(destination/'manifest.json'),resource_sha256=sha(destination/'resources.bin'),
                resource_bytes=len(payload),encoded_bytes=76860,
                retained_asset_bytes=sum(p.stat().st_size for p in destination.iterdir()),
                retention_reason='Exact authored audio and original sector tails required by the pending native resource owner',
                original_inputs_preserved=True,cache_or_runtime_selection_changed=False,
                native_metadata_binding_complete=False,live_playback_verified=False,full_vehicle_parity=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,prepared_bank=str(destination),resource_bytes=len(payload))),flush=True)


if __name__=='__main__':main()
