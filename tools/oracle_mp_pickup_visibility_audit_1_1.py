#!/usr/bin/env python3
"""Read-only source/cache/result audit for independent outer pickup visibility."""
import hashlib,json,re,struct
from pathlib import Path
from oracle_mp_inventory_1_1 import ROOT,PIN,CACHE_PIN,Cache
from xbe import Xbe

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-visibility'
DOC=ROOT/'docs/MP_PICKUP_VISIBILITY_RESEARCH.md'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]

def main():
    r=json.loads((OUT/'report.json').read_text());cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==r['xbe_sha256']==PIN
    assert hashlib.sha256(cache.data).hexdigest()==r['cache_sha256']==CACHE_PIN
    assert sha(ROOT/'assets/disc/maps/shared.map')==r['shared_cache_sha256']
    for d in r['dependencies']:assert sha(ROOT/d['path'])==d['sha256'],d['path']
    instructions=0
    for listing in r['instructions']:
        p=ROOT/listing['path'];assert sha(p)==listing['sha256']
        lines=p.read_text().splitlines();assert len(lines)==listing['instructions']
        for line in lines:
            at,data,_=line.split(' ',2);raw=bytes.fromhex(data)
            assert image.read(int(at,16),len(raw))==raw,(p,at)
        instructions+=len(lines)
    cache_records=0
    def verify_records(value):
        nonlocal cache_records
        if isinstance(value,dict):
            if 'raw_hex'in value and 'file_offset'in value:
                at=value['file_offset'];at=int(at,16)if isinstance(at,str)else at
                raw=bytes.fromhex(value['raw_hex']);assert bytes(cache.data[at:at+len(raw)])==raw,at
                cache_records+=1
            for child in value.values():verify_records(child)
        elif isinstance(value,list):
            for child in value:verify_records(child)
    verify_records(r)
    bsp_ref=cache.block(cache.scenario+0x210,0x44)[0]
    chunk,base=cache.u(bsp_ref),cache.u(bsp_ref+8)
    bsp_magic=base-chunk;root=cache.u(chunk+4)-bsp_magic
    leaf_count=cache.u(root+0x2c);leaf_records=cache.u(root+0x30)-bsp_magic
    resources=0
    with (ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
        for e in r['live_events']:
            if e.get('service')!='00124f30':continue
            descriptor=bytes.fromhex(e['descriptor_hex'])
            at=int(e['descriptor_pointer'],16)-cache.magic
            assert bytes(cache.data[at:at+20])==descriptor
            size,pointer=struct.unpack_from('<II',descriptor,4);location=pointer>>30
            assert size==e['size']and location==e.get('location',0)and pointer&0x3fffffff==e['file_offset']
            if location==2:shared.seek(e['file_offset']);raw=shared.read(size)
            else:raw=bytes(cache.data[e['file_offset']:e['file_offset']+size])
            assert hashlib.sha256(raw).hexdigest()==e['raw_sha256'];resources+=1
    queries=r['outer_queries'];normal_offsets=retreats=terrain_hits=0
    for q in queries:
        raw=bytes.fromhex(q['result_hex']);lower=q['lower'];descriptor=bytes.fromhex(lower['descriptor_before_hex'])
        assert len(raw)==0x5c and len(descriptor)==0x28
        for location in (raw[0x14:0x1c],raw[0x1c:0x24]):
            leaf,cluster,bsp=struct.unpack('<IHH',location);assert bsp==0
            if leaf==0xffffffff:assert cluster==0xffff
            else:
                assert leaf<leaf_count
                assert cluster==struct.unpack_from('<H',cache.data,leaf_records+leaf*8)[0]
        assert lower['root']=='00000000' and lower['range_bits']==['00000000','3f800000']
        assert descriptor[0x1c:0x20]==b'\xff'*4 and descriptor[0x20]==0 and descriptor[0x24:0x28]==b'\xff'*4
        assert q['lower_wrapper']['initial_best_fraction']=='7f7fffff'
        assert q['lower_wrapper']['breakable_count']==256 and q['lower_wrapper']['breakable_pointer']=='04030001'
        if q['mask']in ('14800001','24800001'):
            assert int(q['mask'],16)&14==0
            assert q['lower_wrapper']['flags']==('00000029'if q['mask']=='14800001'else'00000009')
            assert q['ignored']==['00010001',q['ignored'][1]]
        stages={s['instruction']:s for s in q['correction_steps']}
        formed=stages['0016b8eb'];fraction=struct.unpack_from('<f',raw,4)[0]
        formed_fraction=struct.unpack('<f',bytes.fromhex(formed['fraction_bits'])[::-1])[0]
        expected=[f32(f32(o)+f32(f32(d)*formed_fraction))for o,d in zip(q['start']['value'],q['delta']['value'])]
        assert formed['point']['bits']==[f'{struct.unpack("<I",struct.pack("<f",v))[0]:08x}'for v in expected]
        lo=bytes.fromhex(lower['result_hex'])
        visited=struct.unpack_from('<I',lo,0x18)[0]
        assert len(lower['visited_leaves'])==min(visited,256)
        if visited:
            assert struct.unpack_from('<I',raw,0x14)[0]==lower['visited_leaves'][0]
            if '0016b96b'not in stages:
                assert struct.unpack_from('<I',raw,0x1c)[0]==lower['visited_leaves'][-1]
        if q['collision_type']==1:
            terrain_hits+=1
            assert raw[0x38:0x44]==b'\xff'*12
            assert raw[0x4c:0x58]==lo[8:0x14] and raw[0x58:0x5c]==lo[0x14:0x18]
            plane=bytes.fromhex(q['contact_cache']['plane_record']['raw_hex'])
            signed_plane=struct.unpack_from('<I',raw,0x54)[0]
            if signed_plane&0x8000:
                plane=b''.join(struct.pack('<f',0.0-v)for v in struct.unpack('<4f',plane))
            assert raw[0x28:0x38]==plane
            material=bytes.fromhex(q['contact_cache']['material_record']['raw_hex'])
            assert raw[0x24:0x26]==material[8:10]
            assert fraction<=struct.unpack_from('<f',lo)[0]
        else:assert q['collision_type']==0 and raw[4:8]==struct.pack('<f',1)
        if '0016b96b'in stages:
            normal_offsets+=1
            before,after=stages['0016b96b'],stages['0016b9b1']
            normal=struct.unpack_from('<3f',raw,0x28)
            values=[f32(f32(p)+f32(n*0.000244140625))for p,n in zip(before['point']['value'],normal)]
            assert after['point']['bits']==[f'{struct.unpack("<I",struct.pack("<f",v))[0]:08x}'for v in values]
            assert after['final_location_hex'][:8]!='ffffffff'
        retreats+=sum(s['instruction']=='0016ba30'for s in q['correction_steps'])
    assert len(r['authored'])==66 and all(a['candidate']['selected']=='ffffffff'for a in r['authored'])
    assert [sum(a['visibility'][str(row)]['visible']for a in r['authored'])for row in (0,1,12)]==[4,1,8]
    assert r['broad_query']['returned_datums']==['00030004','00020003','00040005']
    broad=next(c for c in r['calls']if c['entry']=='0014bd00'and c.get('radius_bits')=='42c80000')
    assert broad['returned_clusters']==[1,9,15,13,11,12,4,3,5,6,8,10,7,14,0,2]
    assert [n['candidate']['selected']for n in r['near']]==['00020003','00030004','00040005']
    for p in r['clipping_mask_replays']:
        original,replay=(queries[p[k]]for k in ('original_query_index','replay_query_index'))
        assert original['mask']=='14800001'and replay['mask']=='24800001'
        assert original['start']==replay['start']and original['delta']==replay['delta']and original['ignored']==replay['ignored']
    assert len(r['clipping_mask_replays'])==256 and normal_offsets==83 and retreats==0
    assert len(queries)==1763 and terrain_hits==1369
    assert sha(OUT/'report.json')=='bcdf9055fcaafaf68f71cc5732c47da9364a8526793b6238e0c3d4593d37397b'
    reproduction=json.loads((OUT/'reproducibility.json').read_text())
    assert reproduction['byte_identical_report']and reproduction['instruction_listing_hashes_match']
    assert reproduction['report_sha256']==reproduction['reexecution_sha256']==sha(OUT/'report.json')
    assert (OUT/'report.json').read_bytes()==(ROOT/reproduction['reexecution_report']).read_bytes()
    assert reproduction['source_tool_sha256']==r['dependencies'][0]['sha256']
    contact=r['conditional_source_contact']
    assert contact['before']['item_flags']=='0000'and contact['after']['item_flags']=='0020'
    assert contact['after']['contact_hex']=='0000ffff9f0b00ff00000000'
    assert contact['point']['bits']==['bfc0b87e','412a5c91','c1426667']
    assert contact['candidate']['selected']=='00040005'and contact['visibility']['visible']
    assert any(s['instruction']=='000fa410'and s['value']=='20'for s in r['contact_writes'])
    assert r['rng']=='12345678'
    used={e.get('service')for e in r['events']}
    assert not used&{'000c57a0','0016bac0','0016b240','0014b9d0','0011d4a0','001e20c0',
        '000fbfa0','000fb560','001dc8f0','001dc940','000bbf40','000cec00','00111490'}
    assert sum(e.get('service')=='00246a50'for e in r['events'])==2
    frozen={
        'grenades.json':'a1b3d10d9fe3b446007beae2ed97f324ce84622fac5a318fd934e1a2740b0122',
        'netgame/report.json':'3183939bad6d9abe3f13a5c64ececde234c5c73db12cdeb7199ebbde2318356f',
        'world/report.json':'f7b7d075ce0c2988283b013bc1e148c102afd2b67d50de9996c5dc016e644437',
        'target/report.json':'7bd7eacc38f6e64295540c5616e4bd8638426c0a05483db3837fd9934cab13b6',
        'controls/report.json':'a6d9af4e99823eb9d014e083e35bbc340a575bea91d0f3411665e59c6b2cf0e9',
        'spatial/report.json':'6b03f62ec351afeb0760a9052305a08a1cbc3d15a3175f80edfd672c76320b59',
        'equipped-pickup/report.json':'d6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b',
        'weapon-attachment/report.json':'657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f'}
    for path,pin in frozen.items():assert sha(OUT.parent/path)==pin,path
    links=[]
    for target in re.findall(r'\]\(([^)]+)\)',DOC.read_text()):
        if '://'in target:continue
        p=(DOC.parent/target.partition('#')[0]).resolve()
        assert p.exists()or p in (OUT/'audit.json',OUT/'reproducibility.json'),target
        links.append(target)
    audit=dict(report_sha256=sha(OUT/'report.json'),source_tool_sha256=r['dependencies'][0]['sha256'],
        audit_tool_sha256=sha(Path(__file__)),document_sha256=sha(DOC),dependencies=r['dependencies'],
        reproducibility_sha256=sha(OUT/'reproducibility.json'),byte_identical_reexecution=True,
        authored_starts=66,outer_segments=len(queries),ordinary_terrain_hits=terrain_hits,
        original_byte_instruction_rows=instructions,instruction_listings=len(r['instructions']),
        verified_cache_records=cache_records,normal_offset_corrections=normal_offsets,
        verified_resident_resource_records=resources,
        executed_retreat_iterations=retreats,clipping_mask_replays=256,
        outer_calls=len(r['calls']),routine_roots=len(r['rows']),resolved_doc_links=len(links),
        conditional_havok_boundary_calls=2,preserved_frozen_reports=frozen,
        scope='Byte/cache and defined outer-result audit, including independent float32 point/plane/normal-offset checks. Source-emitted clips are clear; shifted-mask replay proves normal correction. Retreat, complete Havok/world producers and Xbox reciprocal parity remain unproven.')
    (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n');print(json.dumps(audit,indent=2))

if __name__=='__main__':main()
