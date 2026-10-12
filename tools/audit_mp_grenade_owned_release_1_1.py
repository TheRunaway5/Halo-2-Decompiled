#!/usr/bin/env python3
"""Independently audit the source-admitted grenade preparation/real stop."""
import argparse
import json
from pathlib import Path
import oracle_mp_grenade_owned_release_1_1 as oracle
from oracle_smg_firing_1_1 import Xbe,PIN,CACHE_PIN
from inspect_multiplayer import Cache
import capstone

ROOT=oracle.ROOT
OUT=oracle.OUT
motion=oracle.motion


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve();assert not args.output.exists()
    first=OUT/'measured-owned';second=OUT/'repeat-owned'
    assert (first/'report.json').read_bytes()==(second/'report.json').read_bytes()
    report=json.loads((first/'report.json').read_text())
    assert report['oracle_sha256']==motion.digest(ROOT/'tools/oracle_mp_grenade_owned_release_1_1.py')
    assert not report['new_supplied_gameplay_fields'] and not report['new_supplied_source_returns'] and not report['instruction_patches']
    previous=ROOT/'analysis/multiplayer/integration/grenade-effect-pool/checkpoint-manifest.json'
    freeze=json.loads(previous.read_text())
    prior_pins={**freeze['source_files'],**freeze['verified_prior_pins']}
    for name,digest in prior_pins.items():assert motion.digest(ROOT/name)==digest,name
    prefix=report['prior_admitted_begin_correlation']
    assert prefix['manifest_sha256']==oracle.PIN
    old=json.loads((ROOT/prefix['report']).read_text())
    assert motion.digest(ROOT/prefix['report'])==prefix['report_sha256']
    for key in ('calls','states','reads','stores','points'):
        lo,hi=prefix['ranges'][key];assert lo==0 and report[key][lo:hi]==old[key],key
    continuation=report['continuation'];stages=continuation['stages']
    assert len(stages)==7 and not continuation['rows'] and len(continuation['inputs'])==1
    assert continuation['inputs'][0]['flags']=='00000000' and continuation['inputs'][0]['selector']==0
    assert all(s['completion']=='complete_original_return' and s['invalid'] is None for s in stages[:-1])
    stop=stages[-1]
    assert stop['entry']=='000de960' and stop['completion']=='blocked_original_instruction'
    assert stop['invalid']['instruction']=='0016d2c3' and stop['invalid']['address']=='00000034'
    assert stop['invalid']['registers']['edx']=='00000000'
    assert stop['invalid']['registers']['esi']=='e7ac0627' and stop['invalid']['registers']['edi']=='e2710002'
    assert '0018a9c8' in stop['invalid']['recent'] and '0018a9ce' in stop['invalid']['recent']
    start=continuation['ranges']['calls'][0]
    calls=report['calls'][start:]
    def one(entry):
        values=[c for c in calls if c['entry']==entry];assert len(values)==1,(entry,len(values));return values[0]
    update=one('000e99b0');prepare=one('000e8d60');allocate=one('000b8a30');descriptor=one('000b8820')
    assert update['caller']=='000e7ec0' and prepare['caller']=='000e9a3f' and allocate['caller']=='000e8ed8'
    assert all('return_cpu' not in c and c['id'] in continuation['unreturned_original_calls'] for c in (update,prepare,allocate))
    assert 'return_cpu' in descriptor and descriptor['return_esp_delta']==16
    request=next(p for p in allocate['packets'] if p['field']=='stack0_original_creation_descriptor_c4')
    assert len(bytes.fromhex(request['raw_hex']))==0xc4
    assert not any(c['entry'] in {'000ba2a0','000e8f30','0018aa90','0018ad70'} for c in calls)
    final=continuation['final_state'];grenade=final['grenade']
    assert grenade['phase']==1 and grenade['timer']==2 and grenade['release_timer']==8
    assert grenade['active_words']==['00400000','00000000'] and grenade['counts']==[1,0]
    assert grenade['held_object']=='ffffffff' and grenade['held_type']==-1
    assert [o['datum'] for o in final['objects']]==['e26f0000','e2700001','e2710002']
    projectile=final['objects'][-1]
    assert projectile['definition']=='e7a80623' and projectile['body_size']==522
    start=continuation['ranges']['stores'][0];stores=report['stores'][start:]
    debit=[s for s in stores if s['instruction']=='000e8dfd']
    assert len(debit)==1 and debit[0]['before_hex']=='02' and debit[0]['written_hex']=='01'
    increment=[s for s in stores if s['instruction']=='000e99dc']
    assert len(increment)==1 and increment[0]['before_hex']=='01' and increment[0]['written_hex']=='02'
    assert increment[0]['order']<debit[0]['order']<allocate['order']
    assert not any(s['instruction'] in {'000e8ef2','000e8f00','000e8f04'} for s in stores)
    assert all(s['clock']==report['states'][0]['clock'] for s in report['states'])
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert motion.digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
    assert motion.digest(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
    files={};checked=0
    for directory in (first,second):
        assert (directory/'stderr.log').read_bytes()==b''
        for listing in report['instructions']:
            path=directory/listing['file'];assert motion.digest(path)==listing['sha256']
            assert path.read_bytes()==(second/listing['file']).read_bytes()
            for line in path.read_text().splitlines():
                address,raw,_=line.split(' ',2);data=bytes.fromhex(raw)
                assert image.read(int(address,16),len(data))==data;checked+=1
        for path in directory.rglob('*'):
            if path.is_file():files[str(path.relative_to(ROOT))]=motion.digest(path)
    cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    source=[]
    for name,address,size in [('looping-sound-request',0x18a9c0,0x72),('datum-pool-stop',0x16d2c0,0x15),('retained-action-owner-call',0xdedd0,9),('object-looping-sound-call',0xbfcc6,5)]:
        ins=list(cs.disasm(image.read(address,size),address))
        raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins).encode()
        path=args.output.parent/'source-bytes'/(name+'.asm');motion.frozen_write(path,raw)
        files[str(path.relative_to(ROOT))]=motion.digest(path);source.append(dict(name=name,address=f'{address:08x}',size=size,file=str(path.relative_to(ROOT))))
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    metadata=[]
    for datum in (0xe7a80623,0xe7ab0626,0xe7ac0627):
        tag=cache.tags[datum];metadata.append(dict(tag=tag,raw_hex=bytes(cache.data[tag['offset']:tag['offset']+tag['size']]).hex()))
    meta_path=args.output.parent/'original-tags.json';motion.frozen_write(meta_path,motion.encoded(metadata));files[str(meta_path.relative_to(ROOT))]=motion.digest(meta_path)
    for name in ('tools/oracle_mp_grenade_owned_release_1_1.py','tools/audit_mp_grenade_owned_release_1_1.py','docs/MP_GRENADE_OWNED_PREPARATION_RESEARCH.md'):
        files[name]=motion.digest(ROOT/name)
    files[str(previous.relative_to(ROOT))]=motion.digest(previous)
    value=dict(schema=1,source_files=files,verified_prior_pins=prior_pins,
        independently_identical_pair=['measured-owned','repeat-owned'],original_xbe_sha256=PIN,original_cache_sha256=CACHE_PIN,
        prior_admitted_begin_correlation=prefix,source_instruction_rows=checked,
        complete_observed_calls=len(report['calls']),complete_observed_states=len(report['states']),ordered_original_stores=len(report['stores']),
        increment=increment[0],debit=debit[0],creation_request=request,partial_projectile=projectile,
        final_action=grenade,genuine_stop=stop,source_stop_bytes=source,
        correction='RetainedE7E60/E99B0 executes withinDE960 viaDEDD4, afterC7E90, not directly withinC7E90. Earlier immutable report bytes already record the true root.',
        scope='Actual admitted grenade retained update increments1→2, debits2→1 and invokes originalB8A30. Real header/bodyE2710002 is partially constructed; absent4F485C looping-sound pool stops16D2C3 before constructor return, attachment or held/release/fuse/explosion. No new game field, return, clock, frame, PVS or success is supplied.')
    motion.frozen_write(args.output,motion.encoded(value))
    manifest=dict(value);manifest['source_files']={**files,str(args.output.relative_to(ROOT)):motion.digest(args.output)}
    manifest['native_gameplay_update_applied']=False
    path=OUT/'manifest.json';assert not path.exists();motion.frozen_write(path,motion.encoded(manifest))
    print(json.dumps(dict(audit_sha256=motion.digest(args.output),manifest_sha256=motion.digest(path),
        source_instruction_rows=checked,source_files=len(manifest['source_files']),prior_pins=len(prior_pins))),flush=True)


if __name__=='__main__':main()
