#!/usr/bin/env python3
"""Read-only original-byte/cache/hash audit of the carried weapon attachment suite."""
import hashlib,json,re,struct
from pathlib import Path
from oracle_mp_inventory_1_1 import ROOT,PIN,CACHE_PIN,Cache
from xbe import Xbe

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/weapon-attachment'
DOC=ROOT/'docs/MP_WEAPON_ATTACHMENT_RESEARCH.md'

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    report=json.loads((OUT/'report.json').read_text())
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==report['xbe_sha256']==PIN
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==report['cache_sha256']==CACHE_PIN
    assert sha(ROOT/'assets/disc/maps/shared.map')==report['shared_cache_sha256']
    for d in report['dependencies']:assert sha(ROOT/d['path'])==d['sha256'],d['path']
    instructions=executed=0
    for listing in report['instructions']:
        p=ROOT/listing['path'];assert sha(p)==listing['sha256']
        rows=p.read_text().splitlines();assert len(rows)==listing['instructions']
        for row in rows:
            address,data,_=row.split(' ',2);raw=bytes.fromhex(data)
            assert image.read(int(address,16),len(raw))==raw,(p,address)
        instructions+=len(rows);executed+=listing['executed_instruction_rows']
    records=0
    def verify_records(value):
        nonlocal records
        if isinstance(value,dict):
            if 'raw_hex'in value and 'file_offset'in value:
                at=value['file_offset'];raw=bytes.fromhex(value['raw_hex'])
                assert bytes(cache.data[at:at+len(raw)])==raw,at
                if 'address'in value:assert int(value['address'],16)==at+cache.magic
                records+=1
            for child in value.values():verify_records(child)
        elif isinstance(value,list):
            for child in value:verify_records(child)
    verify_records(report['cases'])
    resource_count=marker_outputs=root_outputs=attach_ok=attach_failed=0
    with (ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
        for c in report['cases']:
            used={e['service']for e in c['events']if 'service'in e}
            assert not used&{'000fb560','001dc8f0','001dc940','000cec00','00111490',
                '00111500','001cfde0','001d0a60','00115650','000cb060','000c9a40',
                '000bbf40','000c57a0','0016b240','001e20c0'}
            assert c['initial']['equipped']['clip']['index']==185
            assert len(bytes.fromhex(c['pose_fixture']['current_pose_hex']))==33*0x34
            assert len(c['authored_fields'][0]['nodes'])==33
            for f in c['authored_fields']:
                assert struct.unpack('<I',bytes.fromhex(f['model_link']['raw_hex']))[0]==int(f['model']['datum'],16)
                assert struct.unpack('<I',bytes.fromhex(f['render_model_link']['raw_hex']))[0]==int(f['render_model']['datum'],16)
                assert struct.unpack('<I',bytes.fromhex(f['node_block']['raw_hex'])[:4])[0]==len(f['nodes'])
                assert struct.unpack('<I',bytes.fromhex(f['marker_group_block']['raw_hex'])[:4])[0]==len(f['marker_groups'])
            unit,smg,br=c['authored_fields']
            assert unit['flags_bc']['bits']=='00448008'
            assert [f['name']for f in unit['fields']]==['right_hand','left_hand','right_hand_mc']
            assert unit['nodes'][20]['name']=='r_hand' and unit['nodes'][19]['name']=='l_hand'
            for weapon in (smg,br):
                assert weapon['fields'][0]['bits']=='00000000'
                assert not any(g['name']=='right_hand_mc'for g in weapon['marker_groups'])
            for resource in c['initial_weapon_resources']+[e for e in c['live_events']if e.get('service')=='00124f30']:
                descriptor=bytes.fromhex(resource['descriptor_hex'])
                at=int(resource['descriptor_pointer'],16)-cache.magic
                assert bytes(cache.data[at:at+20])==descriptor
                size,pointer=struct.unpack_from('<II',descriptor,4);location=pointer>>30
                assert size==resource['size'] and location==resource.get('location',0)
                assert pointer&0x3fffffff==resource['file_offset']
                if location==2:shared.seek(resource['file_offset']);raw=shared.read(size)
                else:raw=bytes(cache.data[resource['file_offset']:resource['file_offset']+size])
                assert hashlib.sha256(raw).hexdigest()==resource['raw_sha256'];resource_count+=1
            for call in c['calls']:
                entry=call['entry']
                if entry=='001dc8f0':
                    datum=int(call['render_model'],16);sid=int(call['marker_string_id'],16)
                    tag=cache.tags[datum]
                    groups=cache.block(tag['offset']+0x58,12)
                    found=next((i for i,p in enumerate(groups)if cache.u(p)==sid),0xffffffff)if sid not in (0,0xffffffff)else 0xffffffff
                    assert int(call['return_eax'],16)==found
                elif entry=='001dc940':
                    assert call['node_mapping_pointer']=='00000000' and call['mirror']==0 and call['maximum']==1
                    if call['group_index']=='ffffffff':
                        assert call['return_eax']=='00000000';continue
                    assert call['return_eax']=='00000001'
                    output=bytes.fromhex(call['output_hex']);marker=bytes.fromhex(call['marker_records'][0]['raw_hex'])
                    assert struct.unpack_from('<h',output)[0]==marker[2]
                    assert output[4:8]==struct.pack('<f',1)
                    assert output[0x2c:0x38]==marker[4:0x10] and output[0x6c:0x70]==marker[0x20:0x24]
                    assert len(bytes.fromhex(call['source_node_matrices'][0]['matrix_hex']))==0x34
                    marker_outputs+=1
                elif entry=='000b9c20'and call['marker']in ('00000000','0d001457'):
                    output=bytes.fromhex(call['output_hex'])
                    assert output[:2]==bytes(2)
                    identity=struct.pack('<13f',1,1,0,0,0,1,0,0,0,1,0,0,0)
                    assert output[4:0x38]==identity and output[0x6c:0x70]==bytes(4)
                    assert int(call['return_eax'],16)&0xffff==(1 if call['marker']=='00000000'else 0)
                    root_outputs+=1
                elif entry=='000ba2a0':
                    child=call['stack'][1];parent=call['stack'][0];node=int(call['stack'][2],16)&255
                    before=next(i for i in call['before']['items']if i['datum']==child)
                    after=next(i for i in call['after']['items']if i['datum']==child)
                    if before['parent']=='ffffffff':
                        assert after['parent']==parent and after['parent_node']==node
                        assert after['owner']==parent and after['scale_bits']=='3f800000'
                        attach_ok+=1
                    else:
                        assert before['parent']==after['parent'] and before['parent_node']==after['parent_node']
                        attach_failed+=1
            for observation in c['observations']:
                state=observation['state']
                assert len(bytes.fromhex(state['unit']['current_node_matrices_hex']))==33*0x34
                for item in state['items']:
                    raw=bytes.fromhex(item['raw_hex']);header=bytes.fromhex(item['header_hex'])
                    u=lambda p:struct.unpack_from('<I',raw,p)[0]
                    assert struct.unpack_from('<H',header)[0]==int(item['datum'],16)>>16
                    assert header[3]==2 and struct.unpack_from('<I',header,8)[0]==int(item['address'],16)
                    assert u(0xc)==int(item['sibling'],16) and u(0x10)==int(item['first_child'],16)
                    assert u(0x14)==int(item['parent'],16) and raw[0x18]==item['parent_node']
                    assert u(0x154)==int(item['owner'],16)
                    length,offset=struct.unpack_from('<HH',raw,0x114)
                    assert length==4*0x34 and raw[offset:offset+length].hex()==item['node_matrix_hex']
                    assert item['loaded']==(60 if item['definition']=='e57b0406'else 36)
                    assert item['reserve']==(120 if item['definition']=='e57b0406'else 72)
                    assert item['heat_bits']=='00000000'
                    assert item['row_index']==(255 if item['datum']=='00010002'else c['authored_row'])
            for site in c['reciprocal_sites']:
                raw=bytes.fromhex(site['bytes']);assert image.read(int(site['address'],16),len(raw))==raw
    cases={c['name']:c for c in report['cases']}
    assert len(cases)==5 and all(c['shared_rng']=='12345678'for name,c in cases.items()if name!='drop_br')
    assert cases['drop_br']['shared_rng']=='4b5c952c'
    dual=cases['secondary_smg']['final'];assert dual['unit']['first_child']=='00020003'
    dual_items={i['datum']:i for i in dual['items']}
    assert dual_items['00020003']['parent_node']==19 and dual_items['00020003']['sibling']=='00010002'
    assert dual_items['00010002']['parent_node']==20
    switch=cases['switch_back_to_smg'];assert [s['after']['current'][0]for s in switch['continuation']]==[1,1,1,1,0,0,0,0,0,0]
    drop_items={i['datum']:i for i in cases['drop_br']['final']['items']}
    assert drop_items['00020003']['parent']==drop_items['00020003']['owner']=='ffffffff'
    assert drop_items['00010002']['parent']=='ffffffff' and drop_items['00010002']['owner']=='00010001'
    assert attach_ok==10 and attach_failed==1
    assert sha(OUT/'report.json')=='657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f'
    reproduction=json.loads((OUT/'reproducibility.json').read_text())
    assert reproduction['byte_identical_report'] and reproduction['instruction_listing_hashes_match']
    assert reproduction['report_sha256']==reproduction['reexecution_sha256']==sha(OUT/'report.json')
    assert (OUT/'report.json').read_bytes()==(ROOT/reproduction['reexecution_report']).read_bytes()
    assert reproduction['source_tool_sha256']==report['dependencies'][0]['sha256']
    frozen={
        'grenades.json':'a1b3d10d9fe3b446007beae2ed97f324ce84622fac5a318fd934e1a2740b0122',
        'netgame/report.json':'3183939bad6d9abe3f13a5c64ececde234c5c73db12cdeb7199ebbde2318356f',
        'world/report.json':'f7b7d075ce0c2988283b013bc1e148c102afd2b67d50de9996c5dc016e644437',
        'target/report.json':'7bd7eacc38f6e64295540c5616e4bd8638426c0a05483db3837fd9934cab13b6',
        'controls/report.json':'a6d9af4e99823eb9d014e083e35bbc340a575bea91d0f3411665e59c6b2cf0e9',
        'spatial/report.json':'6b03f62ec351afeb0760a9052305a08a1cbc3d15a3175f80edfd672c76320b59',
        'equipped-pickup/report.json':'d6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b'}
    for path,pin in frozen.items():assert sha(OUT.parent/path)==pin,path
    links=[]
    for target in re.findall(r'\]\(([^)]+)\)',DOC.read_text()):
        if '://'in target:continue
        p=(DOC.parent/target.partition('#')[0]).resolve()
        assert p.exists()or p in (OUT/'audit.json',OUT/'reproducibility.json'),target
        links.append(target)
    audit=dict(report_sha256=sha(OUT/'report.json'),audit_tool_sha256=sha(Path(__file__)),
        document_sha256=sha(DOC),dependencies=report['dependencies'],
        reproducibility_sha256=sha(OUT/'reproducibility.json'),byte_identical_reexecution=True,
        cases=len(cases),calls=sum(len(c['calls'])for c in report['cases']),
        object_link_stores=sum(len(c['stores'])for c in report['cases']),
        complete_routine_or_fragment_rows=sum(len(c['rows'])for c in report['cases']),
        hardware_input_samples=sum(len(c['samples'])for c in report['cases']),
        instruction_listings=len(report['instructions']),byte_verified_instruction_rows=instructions,
        executed_instruction_rows_in_listings=executed,byte_verified_cache_records=records,
        byte_verified_resident_resource_records=resource_count,
        actual_marker_outputs=marker_outputs,defined_root_fallback_outputs=root_outputs,
        successful_parent_links=attach_ok,rejected_parent_links=attach_failed,
        resolved_doc_links=len(links),preserved_frozen_reports=frozen,
        scope='Read-only original-byte/cache/hash and defined-field consistency audit. Emulator matrix outputs retain Xbox reciprocal boundary; full scheduler, first-person channels and Havok remain unresolved.')
    (OUT/'audit.json').write_text(json.dumps(audit,indent=2)+'\n');print(json.dumps(audit,indent=2))

if __name__=='__main__':main()
