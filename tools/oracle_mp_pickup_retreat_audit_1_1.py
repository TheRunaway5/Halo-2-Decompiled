#!/usr/bin/env python3
"""Read-only original-byte/cache and independent endpoint retreat arithmetic audit."""
import hashlib,json,re,struct
from pathlib import Path
from oracle_mp_inventory_1_1 import ROOT,PIN,CACHE_PIN,Cache
from oracle_mp_pickup_retreat_1_1 import VERTICES,EDGES
from xbe import Xbe

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-retreat'
DOC=ROOT/'docs/MP_PICKUP_RETREAT_RESEARCH.md'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(x):return f'{struct.unpack("<I",struct.pack("<f",x))[0]:08x}'
def value(b):return struct.unpack('<f',struct.pack('<I',int(b,16)))[0]
def vecbits(v):return [bits(x)for x in v]
def point(origin,delta,fraction):return [f32(f32(o)+f32(f32(d)*fraction))for o,d in zip(origin,delta)]

def main():
    r=json.loads((OUT/'report.json').read_text());cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==r['xbe_sha256']==PIN
    assert hashlib.sha256(cache.data).hexdigest()==r['cache_sha256']==CACHE_PIN
    assert sha(ROOT/'assets/disc/maps/shared.map')==r['shared_cache_sha256']
    for d in r['dependencies']:assert sha(ROOT/d['path'])==d['sha256'],d['path']
    assert image.read(0x4652ec,4)==struct.pack('<I',0x39800000)
    assert image.read(0x4652e8,4)==struct.pack('<I',0x3d000000)
    instructions=0
    for listing in r['instructions']:
        p=ROOT/listing['path'];assert sha(p)==listing['sha256']
        lines=p.read_text().splitlines();assert len(lines)==listing['instructions']
        for line in lines:
            at,data,_=line.split(' ',2);raw=bytes.fromhex(data)
            assert image.read(int(at,16),len(raw))==raw,(p,at)
        instructions+=len(lines)
    cache_records=0
    def verify_records(v):
        nonlocal cache_records
        if isinstance(v,dict):
            if 'raw_hex'in v and 'file_offset'in v:
                at=v['file_offset'];at=int(at,16)if isinstance(at,str)else at
                raw=bytes.fromhex(v['raw_hex']);assert bytes(cache.data[at:at+len(raw)])==raw,at
                cache_records+=1
            for c in v.values():verify_records(c)
        elif isinstance(v,list):
            for c in v:verify_records(c)
    verify_records(r)
    bsp_ref=cache.block(cache.scenario+0x210,0x44)[0]
    chunk,base=cache.u(bsp_ref),cache.u(bsp_ref+8);magic=base-chunk
    root=cache.u(chunk+4)-magic;collision=cache.u(root+0x18)-magic
    nodes=cache.u(collision+4)-magic;planes=cache.u(collision+0xc)-magic
    leaf_records=cache.u(root+0x30)-magic;leaf_count=cache.u(root+0x2c)
    def classify(v):
        node=0;visits=0
        while True:
            visits+=1;assert visits<=cache.u(collision)
            at=nodes+node*8;plane=struct.unpack_from('<h',cache.data,at)[0]
            n=struct.unpack_from('<4f',cache.data,planes+plane*16)
            distance=f32(f32(f32(f32(v[2]*n[2])+f32(v[1]*n[1]))+f32(v[0]*n[0]))-n[3])
            side=int(distance>=0.0)
            child=int.from_bytes(cache.data[at+2+side*3:at+5+side*3],'little')
            if child&0x800000:return 0xffffffff if child==0xffffff else child&0x7fffff
            node=child
    def location(v):
        leaf=classify(v)
        cluster=0xffff if leaf==0xffffffff else struct.unpack_from('<H',cache.data,leaf_records+leaf*8)[0]
        assert leaf==0xffffffff or leaf<leaf_count
        return struct.pack('<IHH',leaf,cluster,0).hex()
    resources=0
    with (ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
        for e in r['live_events']:
            if e.get('service')!='00124f30':continue
            descriptor=bytes.fromhex(e['descriptor_hex']);at=int(e['descriptor_pointer'],16)-cache.magic
            assert bytes(cache.data[at:at+20])==descriptor
            size,pointer=struct.unpack_from('<II',descriptor,4);where=pointer>>30
            assert size==e['size']and where==e.get('location',0)and pointer&0x3fffffff==e['file_offset']
            if where==2:shared.seek(e['file_offset']);raw=shared.read(size)
            else:raw=bytes(cache.data[e['file_offset']:e['file_offset']+size])
            assert hashlib.sha256(raw).hexdigest()==e['raw_sha256'];resources+=1
    classifiers=0
    for call in r['calls']:
        if call['entry']=='0014b9d0':
            assert call['root']=='00000000'and call['bsp']=='800ab3c8'
            assert int(call['return_eax'],16)==classify(call['point']['value'])
            classifiers+=1
        elif call['entry']=='0011d4a0':assert call['output_hex']==location(call['point']['value'])
    queries=r['outer_queries'];terrain_hits=0
    for q in queries:
        assert q['mask']in('14800001','24800001')and int(q['mask'],16)&14==0
        lower=q['lower'];descriptor=bytes.fromhex(lower['descriptor_before_hex'])
        assert q['lower_wrapper']['flags']==('00000029'if q['mask']=='14800001'else'00000009')
        assert q['lower_wrapper']['initial_best_fraction']=='7f7fffff'
        assert q['lower_wrapper']['breakable_count']==256 and q['lower_wrapper']['breakable_pointer']=='04030001'
        assert lower['root']=='00000000'and lower['range_bits']==['00000000','3f800000']
        assert descriptor[0x1c:0x20]==b'\xff'*4 and descriptor[0x20]==0 and descriptor[0x24:0x28]==b'\xff'*4
        raw=bytes.fromhex(q['result_hex']);lo=bytes.fromhex(lower['result_hex'])
        formed=next(s for s in q['retreat_trace']if s['instruction']=='0016b8eb')
        assert formed['point']['bits']==vecbits(point(q['start']['value'],q['delta']['value'],value(formed['fraction_bits'])))
        if q['collision_type']==1:
            terrain_hits+=1;assert raw[0x38:0x44]==b'\xff'*12
            assert raw[0x4c:0x58]==lo[8:0x14]and raw[0x58:0x5c]==lo[0x14:0x18]
            p=bytes.fromhex(q['contact_cache']['plane_record']['raw_hex'])
            if struct.unpack_from('<I',raw,0x54)[0]&0x8000:
                p=b''.join(struct.pack('<f',0.0-v)for v in struct.unpack('<4f',p))
            assert raw[0x28:0x38]==p
            if q['contact_cache']['material']==0xffff:
                assert q['contact_cache']['material_absent']and raw[0x24:0x26]==b'\xff'*2
            else:assert raw[0x24:0x26]==bytes.fromhex(q['contact_cache']['material_record']['raw_hex'])[8:10]
        else:assert q['collision_type']==0 and raw[4:8]==struct.pack('<f',1)
    iterations=clamps=valid_exits=none_exits=positive_exits=fallbacks=0;counts=[]
    assert len(r['cases'])==len(VERTICES)+len(EDGES)
    for case in r['cases']:
        q=queries[case['query_index']];st=q['retreat_trace'];provenance=case['provenance']
        assert q['caller']=='0016bb0b'and q['mask']=='24800001'and q['ignored']==['ffffffff','00010001']
        assert q['collision_type']==1 and int(q['return_eax'],16)&255==1
        if 'vertex'in provenance:
            start=struct.unpack_from('<3f',bytes.fromhex(provenance['vertex_record']['raw_hex']))
            end=provenance['endpoint']['value']
            assert provenance['endpoint']==queries[provenance['endpoint_source_query']]['start']
        else:
            v=[struct.unpack_from('<3f',bytes.fromhex(p['raw_hex']))for p in provenance['vertex_records']]
            start,end=v[provenance['direction']],v[1-provenance['direction']]
            edge=bytes.fromhex(provenance['edge_record']['raw_hex'])
            assert list(struct.unpack_from('<HH',edge))==provenance['vertices']
        assert q['start']['bits']==vecbits(start)
        assert q['delta']['bits']==vecbits([f32(b-a)for a,b in zip(start,end)])
        assert case['origin_location_hex']==location(start)and case['endpoint_location_hex']==location(end)
        before=next(s for s in st if s['instruction']=='0016b96b')
        after=next(s for s in st if s['instruction']=='0016b9b1')
        normal=[value(b)for b in before['normal_bits'][:3]]
        shifted=[f32(p+f32(n*0.000244140625))for p,n in zip(before['point']['value'],normal)]
        assert after['point']['bits']==vecbits(shifted)and after['final_location_hex']==location(shifted)
        assert after['final_location_hex'][:8]=='ffffffff'
        dot=f32(f32(f32(normal[2]*q['delta']['value'][2])+f32(normal[1]*q['delta']['value'][1]))+f32(normal[0]*q['delta']['value'][0]))
        dot_stage=next(s for s in st if s['instruction']=='0016ba07');assert dot_stage['xmm0_bits']==bits(dot)
        fallback=any(s['instruction']=='0016ba27'for s in st);fallbacks+=fallback
        step=f32(0.000244140625/abs(dot))if dot!=0.0 else 0.03125
        assert fallback==(dot==0.0)
        loops=[]
        for s in st:
            if s['instruction']=='0016ba30':loops.append([s])
            elif loops:loops[-1].append(s)
        assert len(loops)==case['retreats']and loops
        previous=after
        for loop in loops:
            pre=loop[0];written=next(s for s in loop if s['instruction']=='0016ba4d')
            formed=next(s for s in loop if s['instruction']=='0016ba80')
            located=next(s for s in loop if s['instruction']=='0016ba85')
            assert all(s['caller']==q['caller']and s['mask']==q['mask']and s['initial_location_hex']==after['initial_location_hex']for s in loop)
            assert pre['xmm4_bits']==bits(step)
            assert pre['fraction_bits']==previous['fraction_bits']and pre['final_location_hex']==previous['final_location_hex']
            fraction=max(f32(value(pre['fraction_bits'])-step),0.0)
            assert written['fraction_bits']==bits(fraction)
            assert formed['point']['bits']==vecbits(point(start,q['delta']['value'],fraction))
            assert located['point']==formed['point']and located['final_location_hex']==location(formed['point']['value'])
            assert located['fraction_bits']==written['fraction_bits']
            assert pre['xmm3_bits']=='00000000'
            if loop is not loops[-1]:assert fraction>0 and located['final_location_hex'][:8]=='ffffffff'
            iterations+=1;clamps+=fraction==0;previous=located
        final=st[-1];assert final['instruction']=='0016baa6'
        assert final['fraction_bits']==previous['fraction_bits']==q['fraction_bits']
        assert final['point']==previous['point']==q['point']and final['final_location_hex']==previous['final_location_hex']==q['final_location_hex']
        valid=final['final_location_hex'][:8]!='ffffffff';valid_exits+=valid;none_exits+=not valid
        positive_exits+=value(final['fraction_bits'])>0
        if value(final['fraction_bits'])>0:assert valid
        counts.append(dict(vertex=provenance.get('vertex'),edge=provenance.get('edge'),direction=provenance.get('direction'),
            iterations=len(loops),final_fraction_bits=q['fraction_bits'],final_location_hex=q['final_location_hex']))
    assert r['rng']=='12345678'
    used={e.get('service')for e in r['events']}
    assert not used&{'000c57a0','0016bac0','0016b240','0014b9d0','0011d4a0','001e20c0','00246a50','000fbfa0','000fb560','001dc8f0','001dc940'}
    frozen={
        'grenades.json':'a1b3d10d9fe3b446007beae2ed97f324ce84622fac5a318fd934e1a2740b0122',
        'netgame/report.json':'3183939bad6d9abe3f13a5c64ececde234c5c73db12cdeb7199ebbde2318356f',
        'world/report.json':'f7b7d075ce0c2988283b013bc1e148c102afd2b67d50de9996c5dc016e644437',
        'target/report.json':'7bd7eacc38f6e64295540c5616e4bd8638426c0a05483db3837fd9934cab13b6',
        'controls/report.json':'a6d9af4e99823eb9d014e083e35bbc340a575bea91d0f3411665e59c6b2cf0e9',
        'spatial/report.json':'6b03f62ec351afeb0760a9052305a08a1cbc3d15a3175f80edfd672c76320b59',
        'equipped-pickup/report.json':'d6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b',
        'weapon-attachment/report.json':'657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f',
        'pickup-visibility/report.json':'bcdf9055fcaafaf68f71cc5732c47da9364a8526793b6238e0c3d4593d37397b'}
    for p,pin in frozen.items():assert sha(OUT.parent/p)==pin,p
    assert len(queries)==32 and terrain_hits==30
    assert (iterations,clamps,valid_exits,none_exits,positive_exits,fallbacks)==(2122,12,23,1,12,0)
    assert sha(OUT/'report.json')=='3b19c3140ea0681a9f1be85580c41d6dd0e28c49583977eb2f58aa6001300713'
    reproduction=json.loads((OUT/'reproducibility.json').read_text())
    assert reproduction['byte_identical_report']and reproduction['instruction_listing_hashes_match']
    assert reproduction['report_sha256']==reproduction['reexecution_sha256']==sha(OUT/'report.json')
    assert (OUT/'report.json').read_bytes()==(ROOT/reproduction['reexecution_report']).read_bytes()
    assert reproduction['source_tool_sha256']==r['dependencies'][0]['sha256']
    links=[]
    for target in re.findall(r'\]\(([^)]+)\)',DOC.read_text()):
        if '://'in target:continue
        p=(DOC.parent/target.partition('#')[0]).resolve()
        assert p.exists()or p==OUT/'audit.json',target
        links.append(target)
    audit=dict(report_sha256=sha(OUT/'report.json'),source_tool_sha256=r['dependencies'][0]['sha256'],
        audit_tool_sha256=sha(Path(__file__)),document_sha256=sha(DOC),dependencies=r['dependencies'],
        reproducibility_sha256=sha(OUT/'reproducibility.json'),byte_identical_reexecution=True,
        original_byte_instruction_rows=instructions,instruction_listings=len(r['instructions']),
        verified_cache_records=cache_records,verified_resident_resource_records=resources,
        independently_verified_classifications=classifiers,outer_segments=len(queries),ordinary_terrain_hits=terrain_hits,
        cases=len(r['cases']),retreat_iterations=iterations,zero_clamps=clamps,valid_location_exits=valid_exits,
        none_location_exits=none_exits,positive_fraction_exits=positive_exits,zero_dot_fallback_cases=fallbacks,
        iteration_counts=counts,preserved_frozen_reports=frozen,resolved_doc_links=len(links),
        scope='Unchanged original-byte/cache and independent float32 delta/dot/step/fraction/point, packed-BSP classifier and per-iteration location audit. Cache-derived terrain probes, not asserted live C57A0 emissions; active world/Havok and Xbox reciprocal parity remain unresolved.')
    (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n');print(json.dumps(audit,indent=2))

if __name__=='__main__':main()
