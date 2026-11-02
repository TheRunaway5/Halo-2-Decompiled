#!/usr/bin/env python3
"""Inspect authored region-state effect inputs in a required native BEFORE.

This is input inventory, not gameplay/audio acceptance. Layouts follow pinned
1704C0/170930 operands and the source cache tag/variant readers. No state,
candidate, effect, random value or callee outcome is changed or supplied.
"""
import argparse
import bisect
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.exists()
    archive=args.archive.resolve(strict=True);receipt=json.loads((archive.parent/'retention.json').read_text())
    assert sha(archive)==receipt['archive_sha256'] and receipt['member_roundtrip_verified']
    banks={};manifest=None;total=0
    process=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout,mode='r|') as stream:
            for member in stream:
                assert member.isfile();data=stream.extractfile(member).read();total+=len(data);assert total<256*1024*1024
                if member.name=='capture-entry.json':
                    assert manifest is None and hashlib.sha256(data).hexdigest()==receipt['capture_manifest_sha256'];manifest=json.loads(data)
                else:
                    assert member.name.startswith('before-') and member.name.endswith('.bin')
                    address=int(member.name[7:-4]);assert address not in banks;banks[address]=data
        assert process.wait()==0
    finally:
        process.stdout.close()
        if process.poll() is None:process.kill();process.wait()
    assert manifest and len(banks)==len(manifest['regions'])
    for row in manifest['regions']:assert len(banks[row['address']])==row['size']
    addresses=sorted(banks)
    def read(address,width=4):
        data=bytearray()
        while len(data)<width:
            index=bisect.bisect_right(addresses,address)-1;assert index>=0
            first=addresses[index];bank=banks[first];offset=address-first;assert 0<=offset<len(bank),hex(address)
            n=min(width-len(data),len(bank)-offset);data+=bank[offset:offset+n];address+=n
        return int.from_bytes(data,'little')
    def block(address,stride):
        count=read(address);assert count<65536
        data=read(address+4)
        for n in range(count):yield data+n*stride
    tags=read(0x4f0414);index=read(0x556d90);count=read(index+24);assert 0<count<=65536
    models=states=effects=0;records=[]
    for rank in range(count):
        row=tags+rank*16
        if read(row)!=0x686c6d74:continue
        datum=read(row+4);model=read(row+8)
        if not model:continue
        models+=1
        for variant_index,variant in enumerate(block(model+0x50,0x38)):
            mappings=[read(variant+4+n,1)for n in range(16)]
            for region_index,region in enumerate(block(variant+0x14,0x14)):
                for permutation_index,permutation in enumerate(block(region+8,0x20)):
                    for state_index,state in enumerate(block(permutation+0xc,0x18)):
                        states+=1;effect=read(state+0xc)
                        if effect==0xffffffff:continue
                        effects+=1;slot=effect&65535;binding=None
                        if slot<count:
                            effect_row=tags+slot*16
                            binding=dict(group=read(effect_row),datum=read(effect_row+4),address=read(effect_row+8),bytes=read(effect_row+12))
                        if len(records)<1024:
                            records.append(dict(model=datum,model_address=model,variant_index=variant_index,
                                variant_address=variant,region_definition_index=region_index,
                                model_regions=[n for n,value in enumerate(mappings)if value==region_index],
                                permutation_index=permutation_index,state_index=state_index,state_address=state,
                                permutation=read(state+4,1),properties=read(state+5,1),state=read(state+6,2),
                                effect=effect,parameter=read(state+0x10),probability_bits=read(state+0x14),
                                effect_binding=binding,effect_binding_matches=bool(binding and binding['group']==0x65666665 and binding['datum']==effect and binding['address'] and binding['bytes'])))
    result=dict(passed=True,scope=__doc__,archive=str(archive),archive_sha256=sha(archive),
        model_tags=models,authored_states=states,authored_effect_references=effects,records=records,
        records_truncated=effects>len(records),supplied_results=False,modified_inputs=False,
        full_gameplay_or_audio_verified=False,retained_large_artifacts=[],tool_sha256=sha(__file__))
    text=json.dumps(result,indent=2)+'\n';assert len(text.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(text)
    print(json.dumps({key:result[key]for key in ('passed','model_tags','authored_states','authored_effect_references','records_truncated')}))


if __name__=='__main__':main()
