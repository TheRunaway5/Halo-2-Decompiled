#!/usr/bin/env python3
"""Export the finite NoAlpha authored-state variant, without changing assets.

The source passes' complete programs are opaque.  The separately retained
complete-payload comparison proves the sole NoAlpha difference is impl0's
RGB write mask.  Native shader programs come from the original TU library.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from mcc_content import MccCache

ROOT=Path(__file__).resolve().parents[1]
FIELDS=((4,12),(12,4),(20,16),(28,6),(36,2),(44,2),(52,10),(60,20),(68,4),(76,2),(84,4),(92,4),(100,12),(108,4))
TEMPLATE=((0,10),(8,2),(16,10),(24,6),(32,4))
SOURCE_PASS_SHA='58f9cc3554afa98ca465293ff448ba6a23c71bb4528518d300eba665dc44c57b'
SOURCE_STEM_SHA='b11171fc55c9a4d62a86bcdd98ca9ec2c995c06ed67905dd1f67a409dc260b55'

def sha(raw):return hashlib.sha256(raw).hexdigest()
def canonical(value):return json.dumps(value,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()
def u32(raw,at):return struct.unpack_from('<I',raw,at)[0]
def put(raw,at,value):struct.pack_into('<I',raw,at,value)
def tag_json(tag):return {key:tag[key] for key in ('group','name','datum','address','size')}

def generate(cache_root):
    maps=cache_root/'halo2/h2_maps_win64_dx11'
    local=MccCache(maps/'needle.map');shared=MccCache(maps/'shared.map')
    library_dir=cache_root/'native-shaders/01b_spacestation'
    library=json.loads((library_dir/'library.json').read_bytes())
    policy=json.loads((ROOT/'tools/data/mcc-native-library-closure.json').read_bytes())
    authority=next(row for row in policy['libraries'] if row['map']=='01b_spacestation')
    assert sha(canonical(library))==authority['canonical_sha256']
    records={(row['group'],row['name']):row for row in library['shader_tags']}
    source_stem=next(t for t in local.tags.values() if t['group']=='stem' and t['name'].endswith('\\tex_bump_no_alpha'))
    source_pass=next(t for t in local.tags.values() if t['group']=='spas' and t['name'].endswith('\\lightmap_tex_detail_no_alpha'))
    stem_record=records['stem','shaders\\shader_templates\\opaque\\tex_bump']
    pass_record=records['spas','shaders\\shader_passes\\lightmap\\lightmap_tex_detail']
    native_payloads={key:(library_dir/'payloads'/record['payload']).read_bytes() for key,record in [('stem',stem_record),('pass',pass_record)]}
    for key,record in [('stem',stem_record),('pass',pass_record)]:assert sha(native_payloads[key])+'.bin'==record['payload']
    namespace={};regions={};frames={}
    def resolve(handle):
        tag=local.tags.get(handle)
        assert tag is not None
        if tag['offset'] is None:
            actual=shared.tags[handle]
            assert all(actual[k]==tag[k] for k in ('group','name','datum')) and actual['offset'] is not None
            return shared,actual
        return local,tag
    def read(owner,address,count):
        if not count:return b''
        at=owner.pointer(address,owner.magic,count);raw=bytes(owner.data[at:at+count])
        for first,previous in regions.items():
            low,high=max(first,address),min(first+len(previous),address+len(raw))
            assert low>=high or previous[low-first:high-first]==raw[low-address:high-address]
        old=regions.get(address,b'');regions[address]=raw if len(raw)>len(old) else old
        return raw
    def name(handle):
        owner,tag=resolve(handle);namespace[handle]=tag_json(tag)
        if tag['group']=='bitm':frames[str(handle)]=owner.u32(tag['offset']+68)
        return owner,tag
    stem_raw=read(local,source_stem['address'],source_stem['size']);assert sha(stem_raw)==SOURCE_STEM_SHA
    pass_raw=read(local,source_pass['address'],source_pass['size']);assert sha(pass_raw)==SOURCE_PASS_SHA
    name(source_stem['datum']);name(source_pass['datum'])
    template=bytearray(stem_raw[:96]);definitions=read(local,u32(template,92),120);put(template,92,0)
    graph={'root_hex':template.hex(),'definitions':[]}
    for platform in range(3):
        definition=bytearray(definitions[platform*40:(platform+1)*40]);arrays=[]
        for field,stride in TEMPLATE:
            count,pointer=u32(definition,field),u32(definition,field+4)
            raw=bytearray(read(local,pointer,count*stride));refs=[]
            if field==16:
                for i in range(count):
                    _,tag=name(u32(raw,i*10+4));refs.append([tag['group'],tag['name']]);put(raw,i*10+4,0)
            arrays.append({'field':field,'count':count,'raw_hex':raw.hex(),'typed_references':refs});put(definition,field+4,0)
        graph['definitions'].append({'definition_hex':definition.hex(),'arrays':arrays})
    vertices=[]
    first_pp=local.block(source_pass['offset']+28,88)[0]
    for entry in local.block(first_pp,330):
        handle=local.u32(entry+256)
        if any(row['datum']==handle for row in vertices):continue
        owner,tag=name(handle);raw=read(owner,tag['address'],tag['size']);assert owner.u32(tag['offset'])==7
        vertices.append({'datum':handle,'name':tag['name'],'bytes':len(raw),'sha256':sha(raw),'vertex_classes':7})
    external=[]
    for entry in local.block(first_pp,330):
        for pixel in local.block(entry+306,56):
            for field,stride in [(24,4),(32,1),(40,1),(48,1)]:
                count=local.u32(pixel+field);pointer=local.u32(pixel+field+4)
                if count and not source_pass['address']<=pointer<=pointer+count*stride<=source_pass['address']+source_pass['size']:
                    raw=read(local,pointer,count*stride)
                    item={'address':pointer,'bytes':count*stride,'hex':raw.hex()}
                    if item not in external:external.append(item)
    native_stem={'group':'stem','name':stem_record['name'],'datum':0xd000f000,'address':0xf0000000,'size':len(native_payloads['stem'])}
    native_pass={'group':'spas','name':pass_record['name'],'datum':0xd000f001,'address':0xf0100000,'size':len(native_payloads['pass'])}
    namespace[native_stem['datum']]=native_stem;namespace[native_pass['datum']]=native_pass
    for record in (stem_record,pass_record):
        for _,target in record['references']:
            if any((row['group'],row['name'])==tuple(target) for row in namespace.values()):continue
            existing=next((t for t in local.tags.values() if (t['group'],t['name'])==tuple(target)),None)
            if existing is not None:name(existing['datum'])
            else:
                datum=0xd000f100+len(namespace)
                namespace[datum]={'group':target[0],'name':target[1],'datum':datum,'address':0xf1000000+len(namespace)*16,'size':16}
    profiles=[];cases=[]
    for shader in local.tags.values():
        if shader['group']!='shad' or shader['offset'] is None or local.u32(shader['offset']+4)!=source_stem['datum']:continue
        root=read(local,shader['address'],84);runtime=read(local,u32(root,16),80)
        definitions=read(local,u32(root,36),372);definition=definitions[:124]
        banks={str(field):bytearray(read(local,u32(definition,field+4),u32(definition,field)*stride)) for field,stride in FIELDS}
        assert all(not banks[str(field)] for field in (60,68,76,84))
        predictions=read(local,u32(root,48),u32(root,44)*8)
        image_roles=[]
        for i in range(len(banks['4'])//12):
            handle=u32(banks['4'],i*12)
            if handle==0xffffffff:image_roles.append([['bitm',''],u32(banks['4'],i*12+4),u32(banks['4'],i*12+8)]);continue
            _,image=name(handle);image_roles.append([[image['group'],image['name']],u32(banks['4'],i*12+4),u32(banks['4'],i*12+8)])
        for i in range(len(predictions)//8):name(u32(predictions,i*8+4))
        namespace[shader['datum']]=tag_json(shader)
        normalized_root=bytearray(root)
        for at in (4,16,36,48):put(normalized_root,at,0)
        normalized_definition=bytearray(definition);put(normalized_definition,0,0)
        for field,_ in FIELDS:put(normalized_definition,field+4,0)
        normalized_images=bytearray(banks['4'])
        for i in range(len(normalized_images)//12):
            if u32(normalized_images,i*12)!=0xffffffff:put(normalized_images,i*12,0)
        normalized_runtime=bytearray(runtime);runtime_slots={}
        for at in (4,12,48,56):
            value=u32(runtime,at);slot=next((i for i in range(len(banks['4'])//12) if u32(banks['4'],i*12)==value),-1)
            runtime_slots[str(at)]=slot
            if value!=0xffffffff:assert slot>=0;put(normalized_runtime,at,0)
        normalized_predictions=bytearray(predictions);prediction_roles=[]
        for i in range(len(predictions)//8):
            _,image=name(u32(predictions,i*8+4));prediction_roles.append([[image['group'],image['name']],u32(predictions,i*8)]);put(normalized_predictions,i*8+4,0)
        sid=u32(root,8);index=sid&65535;offset=local.u32(local.u32(60)+index*4);text=local.string(local.u32(52)+offset,local.u32(56)-offset);assert text=='liquid_thin_water'
        profile={'root_hex':normalized_root.hex(),'definition_hex':normalized_definition.hex(),
            'source_banks':{field:bytes(raw).hex() for field,raw in banks.items() if field!='4'},
            'images':image_roles,'images_hex':normalized_images.hex(),'runtime_hex':normalized_runtime.hex(),
            'runtime_slots':runtime_slots,'predictions_hex':normalized_predictions.hex(),'prediction_roles':prediction_roles,
            'source_string_id':sid,'source_string':text,'curves':[]}
        expected={field:bytes(raw).hex() for field,raw in banks.items()}
        native=native_payloads['stem'];pp=u32(native,92)
        original_lods=native[u32(native,pp+4):u32(native,pp+4)+u32(native,pp)*10]
        expected['28']=b''.join(bytes(4)+original_lods[i*10:i*10+2] for i in range(u32(native,pp))).hex()
        native_passes=native[u32(native,pp+20):u32(native,pp+20)+u32(native,pp+16)*10]
        expected['44']=b''.join(native_passes[i*10+8:i*10+10] for i in range(u32(native,pp+16))).hex()
        expected['52']=bytes(u32(native,pp+24)*10).hex()
        profiles.append(profile);cases.append({'shader':tag_json(shader),'profile':len(profiles)-1,'runtime_hex':runtime.hex(),
            'predictions_hex':predictions.hex(),'expected_banks':expected,'source_string_ids':{str(sid):text}})
    assert len(profiles)==2
    setup={'source_stem':tag_json(source_stem),'source_pass':tag_json(source_pass),'source_stem_sha256':SOURCE_STEM_SHA,
        'source_pass_sha256':SOURCE_PASS_SHA,'source_graph':graph,'external_pass_allocations':external,'vertex_owners':vertices,
        'native_stem_record':stem_record,'native_pass_record':pass_record,'native_source_map':'01b_spacestation',
        'native_library_canonical_sha256':authority['canonical_sha256'],'no_alpha_source_mask':7,'tu15_state_index':67,'tu15_state_value':0x00010101,
        'tu15_source_xbe_sha256':'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
        'tu15_literal_source':'2B035..2B096: RGB00010101 -> NV097_SET_COLOR_MASK0358, source cache4C4B28'}
    data={'schema':'H2_MCC_NATIVE_NO_ALPHA_V1','setup':setup,'profiles':profiles}
    fixture={'schema':'H2_MCC_NATIVE_NO_ALPHA_FIXTURE_V1','setup':setup,'native_stem':native_stem,'native_pass':native_pass,
        'native_stem_payload_hex':native_payloads['stem'].hex(),'native_pass_payload_hex':native_payloads['pass'].hex(),
        'namespace':list(namespace.values()),'bitmap_frame_counts':frames,
        'regions':[[address,raw.hex()] for address,raw in sorted(regions.items())],'cases':cases}
    local.close();shared.close();return data,fixture

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cache',type=Path,required=True);p.add_argument('--policy',type=Path,required=True);p.add_argument('--fixture',type=Path);p.add_argument('--check',action='store_true');a=p.parse_args()
    data,fixture=generate(a.cache.resolve())
    if a.check:
        if json.loads(a.policy.read_bytes())!=data:raise ValueError('Complete actual NoAlpha source policy changed')
    else:
        if not a.fixture:p.error('--fixture is required when exporting')
        a.policy.write_text(json.dumps(data,indent=2)+'\n');a.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    print(json.dumps({'profiles':len(data['profiles']),'policy_bytes':a.policy.stat().st_size,'actual_source_checked':a.check}))
if __name__=='__main__':main()
