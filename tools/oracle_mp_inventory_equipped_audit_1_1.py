#!/usr/bin/env python3
"""Read-only byte/hash audit of separate allocated equipped pickup evidence."""
import hashlib,json,re,struct
from pathlib import Path
from oracle_mp_inventory_1_1 import ROOT,PIN,CACHE_PIN
from xbe import Xbe

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/equipped-pickup'
DOC=ROOT/'docs/MP_PICKUP_EQUIPPED_RESEARCH.md'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    r=json.loads((OUT/'report.json').read_text())
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==r['xbe_sha256']==PIN
    cache=(ROOT/'assets/disc/maps/lockout.map').read_bytes()
    assert hashlib.sha256(cache).hexdigest()==r['cache_sha256']==CACHE_PIN
    for d in r['tool_dependencies']:assert sha(ROOT/d['path'])==d['sha256'],d['path']
    instruction_count=0
    for listing in r['instructions']:
        p=ROOT/listing['listing'];assert sha(p)==listing['sha256']
        rows=p.read_text().splitlines();assert len(rows)==listing['instruction_count']
        for line in rows:
            address,data,_=line.split(' ',2);raw=bytes.fromhex(data)
            assert image.read(int(address,16),len(raw))==raw,(p,address)
        instruction_count+=len(rows)
    resources=animations=0
    assert sha(ROOT/'assets/disc/maps/shared.map')==r['shared_cache_sha256']
    with (ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
        for c in r['cases']:
            assert c['initial']['equipped']['clip']['index']==185
            assert c['controlled_unit_gate']==['00000003','00000003']
            assert c['initial_inventory']['slots'][0]=='00010002'
            assert c['target']['datum']=='00020003'
            assert c['final']['seed']=='12345678'
            for resource in c['initial_weapon_resources']+[e for e in c['live_events']if e.get('service')=='00124f30']:
                descriptor=bytes.fromhex(resource['descriptor_hex'])
                at=int(resource['descriptor_pointer'],16)-0x7e42c800
                assert cache[at:at+20]==descriptor
                size,pointer=struct.unpack_from('<II',descriptor,4)
                location=pointer>>30
                assert size==resource['size'] and location==resource.get('location',0)
                assert pointer&0x3fffffff==resource['file_offset']
                if location==2:
                    shared.seek(resource['file_offset']);raw=shared.read(size)
                else:raw=cache[resource['file_offset']:resource['file_offset']+size]
                assert hashlib.sha256(raw).hexdigest()==resource['raw_sha256']
                resources+=1
            for a in c['authored_animation_records']:
                at=a['file_offset'];raw=bytes.fromhex(a['raw_hex'])
                assert cache[at:at+len(raw)]==raw;animations+=1
            used={e['service']for e in c['events']if 'service'in e}
            assert [(x['instruction'],x['before'],x['value'])for x in c['control_gate_writes'][:2]]==[
                ('000cd415','00000000','00000002'),('000cd427','00000002','00000003')]
            assert all((x['instruction'],x['before'],x['value'])==('00151e0f','00000003','00000003')
                       for x in c['control_gate_writes'][2:])
            assert not used&{'000cec00','00111490','00111500','001cfde0','001d0a60',
                '00115650','000cb060','000c9a40','000bbf40','000c57a0','0016b240','001e20c0','00246a50'}
            for q in c['collision_queries']:assert q['mask']=='14800001' and 'result_hex'in q
            if c['accepted']:
                target=next(w for w in c['final']['weapons']if w['datum']=='00020003')
                assert target['owner']=='00010001' and target['heat_bits']=='00000000'
                assert target['row_index']==c['authored_row']
                assert bytes.fromhex(c['map_row_entry_hex'])[:4]==struct.pack('<I',0x20003)
                assert not any(x['collidable']or x['noncollidable']for x in c['final_registry'])
    assert len(r['cases'])==9 and sum(c['accepted']for c in r['cases'])==5
    switch=r['cases'][-1]
    assert [x['after']['current'][0]for x in switch['continuation']]==[1,1,1,1,0,0,0,0,0,0]
    assert switch['continuation'][0]['after']['manager']['secondary_clip']['index']==206
    assert switch['continuation'][4]['after']['manager']['secondary_clip']['index']==207
    frozen={
        'grenades.json':'a1b3d10d9fe3b446007beae2ed97f324ce84622fac5a318fd934e1a2740b0122',
        'controls/report.json':'a6d9af4e99823eb9d014e083e35bbc340a575bea91d0f3411665e59c6b2cf0e9',
        'spatial/report.json':'6b03f62ec351afeb0760a9052305a08a1cbc3d15a3175f80edfd672c76320b59',
        'world/report.json':'f7b7d075ce0c2988283b013bc1e148c102afd2b67d50de9996c5dc016e644437',
        'target/report.json':'7bd7eacc38f6e64295540c5616e4bd8638426c0a05483db3837fd9934cab13b6',
        'netgame/report.json':'3183939bad6d9abe3f13a5c64ececde234c5c73db12cdeb7199ebbde2318356f'}
    for path,expected in frozen.items():assert sha(OUT.parent/path)==expected,path
    links=[]
    for target in re.findall(r'\]\(([^)]+)\)',DOC.read_text()):
        if '://'in target:continue
        p=(DOC.parent/target.partition('#')[0]).resolve()
        assert p.exists()or p==OUT/'audit.json',target
        links.append(target)
    audit=dict(report_sha256=sha(OUT/'report.json'),audit_tool_sha256=sha(Path(__file__)),
        research_document_sha256=sha(DOC),source_tool_dependencies=r['tool_dependencies'],
        cases=len(r['cases']),accepted_cases=sum(c['accepted']for c in r['cases']),
        complete_routine_or_fragment_rows=sum(len(c['rows'])for c in r['cases']),
        input_samples=sum(len(c['samples'])for c in r['cases']),source_calls=sum(len(c['calls'])for c in r['cases']),
        terrain_segments=sum(len(c['collision_queries'])for c in r['cases']),
        instruction_listings=len(r['instructions']),byte_verified_instruction_rows=instruction_count,
        byte_verified_resource_records=resources,byte_verified_animation_records=animations,
        resolved_doc_links=len(links),preserved_frozen_reports=frozen,
        scope='Read-only original-byte/resource/hash audit; no live Xbox, full engine loop, marker attachment or Havok proof.')
    (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n');print(json.dumps(audit,indent=2))

if __name__=='__main__':main()
