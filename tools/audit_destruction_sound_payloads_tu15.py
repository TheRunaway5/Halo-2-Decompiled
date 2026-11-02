#!/usr/bin/env python3
"""Pair named Xbox/MCC destruction resources and compare same-PC decoding.

This does not qualify the PC decoder against Xbox hardware or qualify audible
delivery. Assets remain read-only; manifests and decoded PCM are disposable.
Run through tools/run_test.py with the actual production decoder probe.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from audit_destruction_contracts_tu15 import Reader
from inspect_multiplayer import Cache, ROOT
from original_xbox_pose_case import BASE, sha
from validate_mcc_weapon_vehicle_sound_resources import Bank, merge, PIN


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--inventory',type=Path,required=True)
    p.add_argument('--binary',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if a.output.exists():p.error('Preserve prior audit evidence')
    a.output.mkdir(parents=True)
    inventory=json.loads(a.inventory.read_text())
    assert inventory['passed'] and inventory['vehicle_paths']==37
    sounds={s for owner in inventory['owners'] for s in owner['sounds']}
    scope=BASE/'authored-scope.json';scope_data=json.loads(scope.read_text())
    inputs={str(a.inventory.resolve()):sha(a.inventory),str(scope):sha(scope)}
    resources={}
    catalogs={'original':{},'mcc':{}}
    def pin(path,expected=None):
        path=Path(path).resolve()
        if str(path) not in inputs:inputs[str(path)]=sha(path)
        if expected is not None and inputs[str(path)]!=expected:raise ValueError('Changed authenticated input: '+str(path))
    def retain(platform,key,row,metadata):
        payload=hashlib.sha256()
        for chunk in row['chunks']:
            path=Path(chunk['path']);pin(path)
            assert chunk['bytes']>0 and chunk['offset']+chunk['bytes']<=path.stat().st_size
            if str(path) not in resources:resources[str(path)]=path.open('rb')
            file=resources[str(path)];file.seek(chunk['offset']);data=file.read(chunk['bytes'])
            assert len(data)==chunk['bytes'];payload.update(data)
        row['encoded_sha256']=payload.hexdigest()
        signature=(row['codec'],row['channels'],row['rate'],row['samples'],row['sound_class'],row['encoded_sha256'],
            tuple((c.get('pcm_bytes'),c.get('reset_after')) for c in row['chunks']))
        variants=catalogs[platform].setdefault(key,{})
        if signature not in variants:variants[signature]=row|{'metadata_maps':[]}
        if str(metadata) not in variants[signature]['metadata_maps']:
            variants[signature]['metadata_maps'].append(str(metadata))
    def named(cache,mcc,tag,g,resource_for):
        read=Reader(cache,mcc);u=cache.u32 if mcc else cache.u
        h=lambda at:struct.unpack_from('<H',cache.data,at)[0]
        tables=getattr(cache,'destruction_sound_tables',{})
        if g not in tables:
            tables[g]=(cache.block(g+(0x18 if mcc else 0x10),4),
                cache.block(g+(0x28 if mcc else 0x20),12),
                cache.block(g+(0x30 if mcc else 0x28),16),
                cache.block(cache.block(g+0x60,8)[0],16) if mcc else cache.block(g+0x40,12))
            cache.destruction_sound_tables=tables
        imports,ranges,perms,chunks=tables[g]
        def name(at):return read.symbol(u(imports[h(at)]))
        s=tag['offset'];encoding=cache.data[s+4]
        if mcc:
            codec_row=cache.block(g,3)[cache.data[s+3]]
            rate,encoding,codec=cache.data[codec_row:codec_row+3]
            rate=(22050,44100,32000,48000)[rate];channels=(1,2,2,4)[encoding]
            assert not h(s)&0x800,'Remastered-only sound entered Classic inventory'
        else:
            rate=(22050,44100,32000)[cache.data[s+3]];codec=cache.data[s+5];channels=encoding+1
        if encoding==2:codec=4;channels=2
        assert codec in (1,5) and channels in (1,2),'Unreviewed destruction codec'
        for pitch in range(cache.data[s+10]):
            r=ranges[h(s+8)+pitch]
            for permutation in range(h(r+10)):
                at=perms[h(r+8)+permutation]
                if mcc:
                    info=cache.block(at+8,8)[0];samples=u(info)//(channels*2);first,count=h(info+4),h(info+6)
                    assert u(info)%(channels*2)==0
                else:samples=u(at+8);first,count=h(at+12),h(at+14)
                assert count and first+count<=len(chunks)
                parts=[]
                for c in chunks[first:first+count]:
                    pointer,flags=u(c),u(c+4)
                    path,offset=resource_for(pointer)
                    part={'path':str(path.resolve()),'offset':offset,'bytes':flags&(0x1fffffff if mcc else 0x3fffffff)}
                    if mcc:part.update(pcm_bytes=u(c+8),reset_after=bool(flags&0x20000000))
                    parts.append(part)
                key=(tag['name'],name(r),name(at))
                retain('mcc' if mcc else 'original',key,dict(name=tag['name'],pitch=pitch,permutation=permutation,
                    codec=codec,channels=channels,rate=rate,samples=samples,sound_class=cache.data[s+2],chunks=parts),cache.path)
    result={'passed':False,'scope':__doc__,'schema_commit':PIN,'inventory_sha256':sha(a.inventory),
        'tool_sha256':sha(Path(__file__)),'binary_sha256':sha(a.binary),'inputs':inputs,
        'xbox_hardware_pcm_verified':False,'audible_delivery_verified':False,'retained_large_artifacts':[]}
    try:
        for item in scope_data['maps']:
            path=Path(item['path']);pin(path,item['sha256']);c=Cache(path)
            try:
                gestalts=[t for t in c.tags.values() if t['group']=='ugh!' and t['offset'] is not None]
                assert len(gestalts)==1
                def original_resource(pointer):
                    location=pointer>>30
                    return (path if location==0 else ROOT/'assets/disc/maps'/('', 'mainmenu.map','shared.map','single_player_shared.map')[location],pointer&0x3fffffff)
                for tag in c.tags.values():
                    if tag['group']=='snd!' and tag['name'] in sounds and tag['offset'] is not None:
                        named(c,False,tag,gestalts[0]['offset'],original_resource)
            finally:c.data.close();c.stream.close()
        maps=sorted({Path(row['mcc_path']).resolve() for row in inventory['maps']})
        shared=Bank(maps[0].parent/'shared.map');pin(shared.path)
        try:
            for path in maps:
                pin(path);local=Bank(path)
                try:
                    needs_shared=local.u32(local.index+12)!=0xffffffff and any(t['offset'] is None for t in local.tags.values())
                    tags=merge(local,shared) if needs_shared else {datum:(local,t) for datum,t in local.tags.items()}
                    gb,gt=tags[local.u32(local.index+16)];assert gt['group']=='matg' and gt['offset'] is not None
                    sg=gb.block(gt['offset']+0xc0,0x2c);assert len(sg)==1
                    cb,ct=tags[gb.u32(sg[0]+12)];assert ct['group']=='sncl' and ct['offset'] is not None
                    classes=cb.block(ct['offset'],0x78)
                    for bank,tag in tags.values():
                        if tag['group']!='snd!' or tag['name'] not in sounds or tag['offset'] is None:continue
                        gestalts=[t for t in bank.tags.values() if t['group']=='ugh!' and t['offset'] is not None];assert len(gestalts)==1
                        cls=classes[bank.data[tag['offset']+2]]
                        resource=path.parent/('sounds_en.dat' if cb.u16(cls+8)&0x80 else 'sounds_neutral.dat')
                        named(bank,True,tag,gestalts[0]['offset'],lambda pointer:(resource,pointer))
                finally:local.close()
        finally:shared.close()
        original,mcc=catalogs['original'],catalogs['mcc']
        result.update(sound_count=len(sounds),named_original_keys=len(original),named_mcc_keys=len(mcc),
            original_sample_size_word_equals_encoded_bytes=all(row['samples']==sum(c['bytes'] for c in row['chunks']) for variants in original.values() for row in variants.values()),
            unmatched_original=[list(k) for k in sorted(original.keys()-mcc.keys())],
            unmatched_mcc=[list(k) for k in sorted(mcc.keys()-original.keys())],
            original_variant_counts={json.dumps(k):len(v) for k,v in original.items() if len(v)>1},
            mcc_variant_counts={json.dumps(k):len(v) for k,v in mcc.items() if len(v)>1})
        cases=[]
        for key in sorted(original.keys()&mcc.keys()):
            for x in original[key].values():
                for y in mcc[key].values():cases.append(dict(key=key,original=x,mcc=y))
        assert cases and {k[0] for k in mcc}==sounds
        with tempfile.TemporaryDirectory(prefix='destruction-payload-',dir=scratch) as owned:
            work=Path(owned);manifest=work/'manifest.json';manifest.write_text(json.dumps({'cases':cases}))
            result.update(manifest_sha256=sha(manifest),manifest_bytes=manifest.stat().st_size)
            # Retain only named metadata and offsets, never encoded or PCM bytes.
            metadata=json.dumps({'cases':cases},indent=2)+'\n';assert len(metadata.encode())<2*1024*1024
            (a.output/'paired-metadata.json').write_text(metadata)
            command=[str(a.binary.resolve()),str(manifest),str((a.output/'pc-waveforms.json').resolve())]
            process=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=900)
            result.update(command=command,exit_code=process.returncode,native_output=process.stdout[-8192:])
        result['fixture_scratch_removed']=not work.exists()
        result['inputs_unchanged']=all(sha(Path(path))==pin for path,pin in inputs.items()) and sha(a.binary)==result['binary_sha256']
        result['passed']=(process.returncode==0 and result['inputs_unchanged'] and not result['unmatched_original'] and not result['unmatched_mcc'])
    except Exception as error:result['error']=str(error)
    finally:
        for file in resources.values():file.close()
    (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result.get(k) for k in ('passed','sound_count','named_original_keys','named_mcc_keys','error','native_output')}),flush=True)
    return 0 if result['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
