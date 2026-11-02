"""Admit an exact produced original-sound candidate inside an active build owner."""
import hashlib
import json
import os
from pathlib import Path
from multiplayer_effect_history_http_test import input_pin, unchanged_inputs


def owned_original_inputs(args):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    binary=args.binary.resolve(strict=True);owner=binary.parent.parent
    if binary.parent.name!='build' or not scratch.is_relative_to(owner):
        raise ValueError('Original sound candidate requires its active owned build/scratch')
    report=args.owned_original_sounds_bundle.resolve(strict=True)
    pins=[input_pin(report,manifest=True)];produced=json.loads(report.read_text())
    if not(produced.get('passed')and produced.get('inputs_unchanged')and produced.get('private_candidate_inodes')and
           produced.get('contexts')==38 and produced.get('original_bindings')==908):
        raise ValueError('Incomplete production original sound bundle')
    candidate=Path(produced['candidate'])
    if candidate!=candidate.resolve(strict=True)or not candidate.is_relative_to(owner)or candidate==owner:
        raise ValueError('Original sound candidate escaped its disposable owner')
    provenance=candidate/'provenance.json';pins.append(input_pin(provenance,manifest=True))
    pointer=json.loads(provenance.read_text())['original_sound_catalog']
    if pointer!=produced['catalog']:raise ValueError('Candidate catalog differs from its producer')
    catalog=candidate/pointer['path'];pins.append(input_pin(catalog,manifest=True))
    if not catalog.resolve(strict=True).is_relative_to(candidate):raise ValueError('Catalog escaped its owned candidate')
    with catalog.open('rb')as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
    if digest!=pointer['sha256']or catalog.stat().st_size!=pointer['bytes']:raise ValueError('Original catalog pin differs')
    rows=[(p,r)for p,r in pointer['maps'].items()if r['original_map']=='lockout.map']
    if len(rows)!=1:raise ValueError('Original candidate lacks exact Lockout admission')
    relative,row=rows[0];map_path=candidate/relative;pins.append(input_pin(map_path,mcc_map=True))
    if not map_path.resolve(strict=True).is_relative_to(candidate):raise ValueError('Map escaped its owned candidate')
    with map_path.open('rb')as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
    if digest!=row['input_sha256']or map_path.stat().st_size!=row['input_bytes']:raise ValueError('Admitted original map pin differs')
    unchanged_inputs(pins);return map_path,pins

