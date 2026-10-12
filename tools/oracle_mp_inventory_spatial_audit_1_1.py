#!/usr/bin/env python3
"""Read-only audit of the bounded pickup spatial evidence, then save its freeze."""
import hashlib
import json
import re
import struct
from pathlib import Path

from oracle_mp_inventory_1_1 import ROOT, PIN, CACHE_PIN
from xbe import Xbe

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/spatial'
DOC=ROOT/'docs/MP_PICKUP_SPATIAL_RESEARCH.md'
TOOL=ROOT/'tools/oracle_mp_inventory_spatial_1_1.py'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    report=OUT/'report.json'
    r=json.loads(report.read_text())
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN==r['xbe_sha256']
    cache=(ROOT/'assets/disc/maps/lockout.map').read_bytes()
    assert hashlib.sha256(cache).hexdigest()==CACHE_PIN==r['cache_sha256']
    assert sha(TOOL)==r['oracle_tool_sha256']
    for name,expected in r['source_tool_dependencies'].items():
        assert sha(ROOT/'tools'/name)==expected
    instruction_rows=0
    for listing in r['instructions']:
        p=ROOT/listing['listing']
        assert sha(p)==listing['sha256']
        original=[]
        for line in p.read_text().splitlines():
            address,data,_=line.split(' ',2)
            data=bytes.fromhex(data)
            assert image.read(int(address,16),len(data))==data,(p,address)
            original.append(data)
        assert len(original)==listing['instruction_count']
        assert hashlib.sha256(b''.join(original)).hexdigest()==listing['original_instruction_bytes_sha256']
        instruction_rows+=len(original)
    records=0
    def verify_record(record):
        nonlocal records
        data=bytes.fromhex(record['raw_hex'])
        at=int(record['file_offset'],16)
        assert cache[at:at+len(data)]==data
        records+=1
    for v in r['terrain_metadata']['blocks'].values():
        verify_record(v['data'])
    for key in ('root','collision_bsp','collision_materials'):
        verify_record(r['terrain_metadata'][key])
    verify_record(r['multi_cluster_case']['portal_record'])
    for row in r['authored_starts']:
        pose=row['pose']
        raw=bytes.fromhex(pose['authored_record_hex'])
        at=int(pose['file_offset'],16)
        assert cache[at:at+len(raw)]==raw
        assert pose['root']['bits']==[f'{x:08x}' for x in struct.unpack_from('<3I',raw)]
        assert pose['facing_bits']==f'{struct.unpack_from("<I",raw,12)[0]:08x}'
        assert row['candidate']['object']=='ffffffff'
        records+=1
    hits=0
    for query in r['collision_queries']:
        assert int(query['mask'],16)&0xe==0
        lower=query['lower']
        assert lower['flags']==('00000029' if query['mask']=='14800001' else '00000009')
        assert lower['range_bits']==['00000000','3f800000']
        assert lower['root_node']=='00000000' and lower['initial_best_fraction_bits']=='7f7fffff'
        assert (lower['prior_leaf'],lower['prior_class'],lower['prior_plane'])==('ffffffff',0,'ffffffff')
        assert lower['breakable_count']==256 and lower['breakable_pointer']=='04030001'
        assert lower['origin']==query['start'] and lower['delta']==query['delta']
        if query['collision_type']!=1:
            continue
        hits+=1
        contact=query['terrain_cache_contact']
        for key in ('surface_record','plane_record','collision_material'):
            verify_record(contact[key])
        material=bytes.fromhex(contact['collision_material']['raw_hex'])
        assert struct.unpack_from('<H',material,8)[0]==query['material']
        surface=bytes.fromhex(contact['surface_record']['raw_hex'])
        assert struct.unpack_from('<H',surface)[0]==int(contact['plane'],16)&0xffff
    for address,v in r['original_constants'].items():
        assert image.read(int(address,16),4)==struct.pack('<I',int(v['bits'],16))
    assert r['multi_cluster_case']['memberships']==[9,15]
    assert len(set(r['multi_cluster_case']['query']['returned']))==3
    assert r['bounded_registry_queries'][0]['returned']==['00030004','00020003','00040005']
    assert not set(r['used_services'])&{'000bbf40','000c57a0','0016b240','001e20c0','00246a50'}
    links=[]
    for target in re.findall(r'\]\(([^)]+)\)',DOC.read_text()):
        if '://' in target:
            continue
        path,_,anchor=target.partition('#')
        p=(DOC.parent/path).resolve()
        assert p.exists() or p==OUT/'audit.json',target
        if anchor:
            headings=re.findall(r'^#+ (.+)$',p.read_text(),re.M)
            anchors={re.sub(r'[^a-z0-9 _-]','',x.lower()).replace(' ','-') for x in headings}
            assert anchor in anchors,target
        links.append(target)
    frozen={
        'analysis/multiplayer/integration/inventory-source/grenades.json':
            'a1b3d10d9fe3b446007beae2ed97f324ce84622fac5a318fd934e1a2740b0122',
        'analysis/multiplayer/integration/inventory-source/controls/report.json':
            'a6d9af4e99823eb9d014e083e35bbc340a575bea91d0f3411665e59c6b2cf0e9',
        'tools/oracle_mp_inventory_controls_1_1.py':
            'bf5eec2c5a1dbf7c2bfe8568bb18a04cc5cee1881e01f434723db43dfe26a33b',
        'docs/MP_PICKUP_CONTROLS_RESEARCH.md':
            '904ad67d9f9d8781529d40691470d34accb94da8cae5daea59092db77e6e7150',
    }
    for path,expected in frozen.items():
        assert sha(ROOT/path)==expected,path
    preserved={}
    for name in ('world','target','netgame'):
        p=ROOT/'analysis/multiplayer/integration/inventory-source'/name/'report.json'
        prior=json.loads(p.read_text())
        tool=ROOT/'tools'/f'oracle_mp_inventory_{name}_1_1.py'
        assert sha(tool)==prior['oracle_tool_sha256']
        preserved[name]=dict(report_sha256=sha(p),tool_sha256=sha(tool))
    audit=dict(report_sha256=sha(report),oracle_tool_sha256=sha(TOOL),
        audit_tool_sha256=sha(Path(__file__)),research_document_sha256=sha(DOC),
        xbe_sha256=PIN,cache_sha256=CACHE_PIN,
        complete_routine_or_fragment_rows=len(r['rows']),authored_start_records=len(r['authored_starts']),
        original_instruction_rows_byte_verified=instruction_rows,instruction_listings=len(r['instructions']),
        original_visibility_calls=sum(x['entry']=='000c57a0' for x in r['spatial_calls']),
        original_collision_segments=len(r['collision_queries']),terrain_hits=hits,
        authored_cache_records_byte_verified=records,resolved_doc_links=len(links),
        preserved_frozen_files=frozen,preserved_previous_reports=preserved,
        scope='Read-only byte/hash/layout audit of bounded original execution evidence; no live Xbox or complete settling proof.')
    (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n')
    print(json.dumps(audit,indent=2))


if __name__=='__main__':
    main()
