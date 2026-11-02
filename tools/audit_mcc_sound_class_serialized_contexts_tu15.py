#!/usr/bin/env python3
"""Authenticate raw MCC class namespaces against existing complete bank proofs.

The raw globals routes are distinct from Cache's relocated/cloned routes.
This exports exact additional namespace contexts without changing class words
or bypassing the complete typed admission used by the production projection.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from validate_mcc_weapon_vehicle_sound_resources import Bank,merge
from generate_mcc_sound_class_reference import canonical,generate,ROOT


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('inventory','preparation','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    profile_path=ROOT/'tools/data/mcc-sound-class-reference.json';raw=profile_path.read_bytes()
    assert generate(raw)==(ROOT/'src/audio/mcc_sound_class_reference.inc').read_text()
    profile=json.loads(raw);known={c['source_bank_sha256']for c in profile['contexts']+profile['published_contexts']}
    prep=json.loads(args.preparation.read_text());assert prep['passed']and prep['inputs_unchanged']
    catalog=Path(prep['catalog']);assert catalog.is_relative_to(owner)and sha(catalog)==prep['catalog_sha256']
    by_map={}
    for case in json.loads(catalog.read_text())['cases']:by_map.setdefault(case['map'],[]).append(case)
    inputs={str(p.resolve()):sha(p)for p in (args.inventory,args.preparation,catalog,profile_path,Path(__file__),
        ROOT/'tools/mcc_content.py',ROOT/'tools/validate_mcc_weapon_vehicle_sound_resources.py')}
    contexts={};records=[];shared=None;shared_path=None
    try:
        inventory=json.loads(args.inventory.read_text());assert inventory['passed']and inventory['vehicle_paths']==37
        for record in inventory['maps']:
            name=record['name']
            if name not in by_map:continue
            path=Path(record['mcc_path']);assert sha(path)==record['mcc_sha256'];inputs[str(path.resolve())]=record['mcc_sha256']
            local=Bank(path)
            try:
                needs=local.u32(local.index+12)!=0xffffffff and any(t['offset']is None for t in local.tags.values())
                if needs:
                    wanted=path.parent/'shared.map'
                    if shared_path!=wanted:
                        if shared:shared.close()
                        shared=Bank(wanted);shared_path=wanted;inputs[str(wanted.resolve())]=sha(wanted)
                    tags=merge(local,shared)
                else:tags={datum:(local,t)for datum,t in local.tags.items()}
                globals_=[(b,t)for b,t in tags.values()if t['group']=='matg'and t['offset']is not None];assert len(globals_)==1
                bank,g=globals_[0];assert g['name']=='globals\\globals';block=g['offset']+0xc0;assert bank.u32(block)==1
                sg=bank.block(block,44)[0];assert bank.u32(sg+8)==int.from_bytes(b'sncl','big')
                cb,ct=tags[bank.u32(sg+12)];assert ct['group']=='sncl'and ct['name']=='sound\\sound_classes'
                rows=cb.block(ct['offset'],120);assert len(rows)==54
                assert ct['address']+8<=cb.u32(ct['offset']+4)and cb.u32(ct['offset']+4)+54*120<=ct['address']+ct['size']
                source=b''.join(bytes(cb.data[a:a+120])for a in rows);digest=hashlib.sha256(source).hexdigest();assert digest in known
                native=bytearray(b''.join(source[n*120:n*120+92]for n in range(54)))
                for word in profile['deltas']:
                    at=word['class_index']*92+word['offset'];assert int.from_bytes(native[at:at+4],'little')==word['before_bits']
                    native[at:at+4]=word['after_bits'].to_bytes(4,'little')
                selected=set()
                for case in by_map[name]:
                    manifest=Path(case['directory'])/'manifest.json';assert sha(manifest)==case['sha256'];inputs[str(manifest)]=case['sha256']
                    m=json.loads(manifest.read_text());contract=m['original_contract'];cls=contract['sound_class'];selected.add(cls)
                    assert bytes(native[cls*92:(cls+1)*92])==bytes.fromhex(contract['sound_class_hex'])
                fields=lambda t:{k:t[k]for k in ('group','name','datum','address','size')}
                context=dict(globals=fields(g),legacy=fields(ct),sound_block=bytes(bank.data[block:block+8]).hex(),
                    sound_globals=bytes(bank.data[sg:sg+44]).hex(),class_root=bytes(cb.data[ct['offset']:ct['offset']+8]).hex(),source_bank_sha256=digest)
                key=canonical(context);contexts.setdefault(key,context|dict(source_maps=[]))['source_maps'].append(name)
                records.append(dict(map=name,source_map_sha256=record['mcc_sha256'],source_class_bank_sha256=digest,
                    already_qualified_complete_class_bank=True,destruction_bindings=len(by_map[name]),selected_classes=sorted(selected),
                    selected_original_class_rows_equal=True))
            finally:local.close()
    finally:
        if shared:shared.close()
    rows=sorted(contexts.values(),key=lambda c:canonical(c));assert len(records)==38
    assert all(sha(p)==digest for p,digest in inputs.items())
    report=dict(passed=True,goal_complete=False,scope=__doc__,serialized_contexts=rows,contexts_sha256=hashlib.sha256(canonical(rows)).hexdigest(),
        maps=records,inputs=inputs,inputs_unchanged=True,new_class_word_corrections=0,live_publication_verified=False,retained_large_artifacts=[])
    text=json.dumps(report,indent=2)+'\n';assert len(text.encode())<=1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(text)
    print(json.dumps(dict(passed=True,maps=len(records),serialized_contexts=len(rows),contexts_sha256=report['contexts_sha256'])))


if __name__=='__main__':main()
