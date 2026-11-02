"""Capture exact MCC/original Xbox authored material inputs.

The default template is environment/three-channel; another exact template may
be selected explicitly. This is a bounded, read-only authority census, not a shader implementation or
mapping guess. Native asset records retain their actual reference-image hash;
they are not relabeled as a behavior or framebuffer reference. Original/MCC
root, cooked banks, typed image roles, clock curves and compiled remap graphs
are retained as compressed required metadata evidence, with immutable pins.
Run through run_test.py. No raw map, executable, shader payload copy or frame
is retained. Only the explicitly requested bounded reports survive.
"""
import argparse
from collections import Counter
import hashlib
import json
import lzma
import os
from pathlib import Path
import struct

from mcc_content import MccCache
from export_mcc_native_world_cooked_fixture import FIELDS
import steam_workshop as workshop

TEMPLATE='shaders\\shader_templates\\opaque\\tex_bump_env_illum_3_channel'


def sha(data):return hashlib.sha256(data).hexdigest()
def digest(path):
    with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def canonical(value):return json.dumps(value,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()
def word(data,offset):return struct.unpack_from('<I',data,offset)[0]


class Census:
    def __init__(self,base,template=TEMPLATE):
        self.base=base;self.shared={};self.stems={};self.rows=[];self.errors=[];self.maps={};self.libraries={}
        self.template=template
        root=Path(__file__).resolve().parents[1]
        closure=json.loads((root/'tools/data/mcc-native-library-closure.json').read_text())
        self.authorities={row['map']:row['canonical_sha256']for row in closure['libraries']}

    def close(self):
        for value in self.shared.values():value.close()

    def resolve(self,cache,shared,handle):
        tag=cache.tags.get(handle)
        if tag is None:raise ValueError('Full salted shader reference is absent: '+hex(handle))
        if tag['offset']is not None:return cache,tag
        other=shared.tags.get(handle)
        if other and other['offset']is not None and (other['group'],other['name'])==(tag['group'],tag['name']):return shared,other
        return cache,tag

    def graph(self,cache,shared,tag):
        owner,tag=self.resolve(cache,shared,tag['datum']);at=tag['offset']
        if at is None:raise ValueError('Compiled template has no resident source owner')
        root=bytearray(owner.data[at:at+96]);assert len(root)==96
        definitions=owner.block(at+88,40);assert len(definitions)==3
        counts={str(field):owner.u32(at+field)for field in (0,16,24,40,48,56)}
        struct.pack_into('<I',root,92,0);graphs=[]
        for definition in definitions:
            raw=bytearray(owner.data[definition:definition+40]);arrays=[]
            for field,stride in ((0,10),(8,2),(16,10),(24,6),(32,4)):
                count,pointer=owner.u32(definition+field),owner.u32(definition+field+4)
                assert count<=65536
                struct.pack_into('<I',raw,field+4,0)
                if count:
                    offset=owner.pointer(pointer,owner.magic,count*stride)
                    data=bytearray(owner.data[offset:offset+count*stride])
                else:assert pointer==0;data=bytearray()
                references=[]
                if field==16:
                    for index in range(count):
                        assert word(data,index*stride)==int.from_bytes(b'spas','big')
                        _,reference=self.resolve(cache,shared,word(data,index*stride+4))
                        assert reference['group']=='spas'
                        references.append([reference['group'],reference['name']]);struct.pack_into('<I',data,index*stride+4,0)
                arrays.append(dict(field=field,count=count,raw_hex=data.hex(),typed_references=references))
            graphs.append(dict(definition_hex=raw.hex(),arrays=arrays))
        result=dict(root_hex=root.hex(),source_root_counts=counts,definitions=graphs)
        key=sha(canonical(result));self.stems.setdefault(key,dict(graph=result,owners=[]))['owners'].append(
            dict(map=cache.path.name,datum=tag['datum'],owner=str(owner.path),template=tag['name']))
        return key

    def source(self,cache,shared,tag):
        at=tag['offset'];definitions=cache.block(at+32,124);assert definitions
        pp=definitions[0];banks={}
        for field,stride in FIELDS:
            count,pointer=cache.u32(pp+field),cache.u32(pp+field+4);assert count<=65536
            if count:
                offset=cache.pointer(pointer,cache.magic,count*stride);raw=bytes(cache.data[offset:offset+count*stride])
            else:raw=b''
            banks[str(field)]=dict(count=count,hex=raw.hex())
        images=[]
        for datum,frame,bits in struct.iter_unpack('<III',bytes.fromhex(banks['4']['hex'])):
            if datum==0xffffffff:identity=['NONE']
            else:
                _,bitmap=self.resolve(cache,shared,datum);identity=[bitmap['group'],bitmap['name']]
                assert identity[0]=='bitm'
            images.append([identity,frame,bits])
        curves=[]
        for overlay in cache.block(pp+60,20):
            count,pointer=cache.u32(overlay+12),cache.u32(overlay+16);assert count<=65536
            offset=cache.pointer(pointer,cache.magic,count)if count else 0
            curves.append(dict(input=cache.u32(overlay),range=cache.u32(overlay+4),period_word=cache.u32(overlay+8),
                curve_hex=bytes(cache.data[offset:offset+count]).hex()))
        return dict(source_root=dict(root_hex=bytes(cache.data[at:at+84]).hex(),definition_count=len(definitions),
            trailing_definitions_zero=not any(cache.data[pp+124:definitions[-1]+124]),named_parameter_count=cache.u32(at+24)),
            source_banks=banks,source_images=images,source_overlays=curves)

    def native(self,directory,record,template):
        data=(directory/'payloads'/record['payload']).read_bytes()
        assert sha(data)+'.bin'==record['payload']and len(data)==record['bytes']
        references={field:identity for field,identity in record['references']}
        assert references.get(4)==['stem',template]
        pp=word(data,36);banks={}
        for field,stride in FIELDS:
            count,pointer=word(data,pp+field),word(data,pp+field+4)
            assert count<=65536 and pointer+count*stride<=len(data)
            banks[str(field)]=dict(count=count,hex=data[pointer:pointer+count*stride].hex()if count else '')
        image_base=word(data,pp+8);images=[]
        for index in range(banks['4']['count']):
            at=image_base+index*12;handle=word(data,at)
            identity=references.get(at,['NONE']if handle==0xffffffff else ['UNRESOLVED'])
            images.append([identity,word(data,at+4),word(data,at+8)])
        curves=[];at=word(data,pp+64)
        for index in range(banks['60']['count']):
            row=at+index*20;count,pointer=word(data,row+12),word(data,row+16)
            assert pointer+count<=len(data)
            curves.append(dict(input=word(data,row),range=word(data,row+4),period_word=word(data,row+8),curve_hex=data[pointer:pointer+count].hex()))
        return dict(native_shader_payload=record['payload'],native_root_hex=data[:84].hex(),native_banks=banks,
            native_images=images,native_overlays=curves,native_named_parameter_count=word(data,24))

    def inspect(self,path,shared_path,name,native_name=None):
        before=digest(path);shared_key=str(shared_path.resolve(strict=True))
        if shared_key not in self.shared:
            self.maps['shared:'+shared_key]=digest(shared_path);self.shared[shared_key]=MccCache(shared_path)
        shared=self.shared[shared_key];cache=MccCache(path)
        directory=self.base/'native-shaders'/(native_name or name);records={}
        try:
            if native_name:
                library_path=directory/'library.json';library=json.loads(library_path.read_text())
                assert sha(canonical(library))==self.authorities[native_name]
                self.libraries[native_name]=dict(path=str(library_path),sha256=digest(library_path),source_xbe_sha256=library['source_xbe_sha256'])
                records={(row['group'],row['name']):row for row in library['shader_tags']}
            for handle,tag in cache.tags.items():
                if tag['group']!='shad'or tag['offset']is None:continue
                source_stem=cache.tags[cache.u32(tag['offset']+4)]
                if source_stem['name']!=self.template:continue
                try:
                    graph=self.graph(cache,shared,source_stem)
                    row=dict(map=name,shader=tag['name'],datum=handle,template=self.template,source_stem_graph_sha256=graph,
                        native_pair=False,**self.source(cache,shared,tag))
                    record=records.get(('shad',tag['name']))
                    if record and any(field==4 and identity==['stem',self.template]for field,identity in record['references']):
                        row.update(native_pair=True,**self.native(directory,record,self.template))
                        row['normalized_images_exact']=row['source_images']==row['native_images']
                        row['bank_exact']={str(field):row['source_banks'][str(field)]==row['native_banks'][str(field)]for field,_ in FIELDS}
                    elif record:
                        identities=[identity for field,identity in record['references']if field==4]
                        assert len(identities)==1 and identities[0][0]=='stem'
                        row['other_original_material']=dict(template=identities[0][1],
                            scope='Same material path with a different original template; candidate evidence only, not an admitted pair',
                            **self.native(directory,record,identities[0][1]))
                    self.rows.append(row)
                except (ValueError,KeyError,IndexError,struct.error)as error:
                    self.errors.append(dict(map=name,shader=tag['name'],error=str(error)))
        finally:cache.close()
        assert digest(path)==before,'Source map changed while researching material inputs'
        self.maps[name]=dict(path=str(path),sha256=before)
        print(json.dumps(dict(map=name,materials=len(self.rows),paired=sum(row['native_pair']for row in self.rows),errors=len(self.errors))),flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-cache',type=Path,required=True)
    parser.add_argument('--template',default=TEMPLATE,help='Exact compiled template identity; defaults to the original environment/three-channel census')
    parser.add_argument('--candidates',type=Path,required=True)
    parser.add_argument('--maps',nargs='*')
    parser.add_argument('--all-stock-maps',action='store_true',help='Inspect all available authenticated stock map/library owners, including maps without the selected native template')
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--summary',type=Path,required=True);args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(owner)and not args.summary.resolve().is_relative_to(owner)
    base=args.base_cache.resolve(strict=True);candidates=json.loads(args.candidates.read_text());assert candidates['template']==args.template
    census=Census(base,args.template);maps=base/'halo2/h2_maps_win64_dx11'
    assert not(args.all_stock_maps and args.maps)
    selected=(sorted(name for name in census.authorities if(maps/(name+'.map')).is_file()and
        (base/'native-shaders'/name/'library.json').is_file())if args.all_stock_maps else
        [row['map']for row in candidates['native_libraries']if not args.maps or row['map']in args.maps])
    assert selected and (not args.maps or set(selected)==set(args.maps))
    try:
        for name in selected:census.inspect(maps/(name+'.map'),maps/'shared.map',name,name)
        mod=next(row for row in workshop.scan()['mods']if row['id']=='3360515088');item=Path(mod['directory'])
        resources=workshop.resource_inputs(item,base,workshop.read_mod(item));shared=Path(resources['maps']['shared.map'])
        for name in ('earthcity_e3','earthcity_e3_potato'):
            census.inspect(workshop.source_owned(item,'maps/'+name+'.map'),shared,'workshop:'+name)
        for key,cache in census.shared.items():assert digest(Path(key))==census.maps['shared:'+key]
    finally:census.close()
    report=dict(schema='H2_MCC_ENV_ILLUM_THREE_CHANNEL_AUTHORITY_V1'if args.template==TEMPLATE else'H2_MCC_MATERIAL_PAIR_AUTHORITY_V1',scope=__doc__,template=args.template,
        source_sha256=digest(Path(__file__)),candidate_report_sha256=digest(args.candidates),maps=census.maps,
        native_libraries=census.libraries,stems=census.stems,materials=census.rows,errors=census.errors,
        native_projection_admitted=False,owned_scratch=str(owner))
    raw=json.dumps(report,indent=2).encode()+b'\n';assert len(raw)<=16*1024*1024
    packed=lzma.compress(raw,preset=6);assert len(packed)<=1024*1024 and lzma.decompress(packed)==raw
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_bytes(packed)
    groups=Counter((row['source_stem_graph_sha256'],row['native_pair'])for row in census.rows)
    summary=dict(scope=__doc__,passed=not census.errors,materials=len(census.rows),paired=sum(row['native_pair']for row in census.rows),
        maps=len(selected)+2,graphs=len(census.stems),groups=[dict(graph=key[0],native_pair=key[1],count=count)for key,count in groups.items()],
        errors=census.errors,required_metadata_reference=dict(path=str(args.report),bytes=len(packed),sha256=sha(packed),decoded_bytes=len(raw),decoded_sha256=sha(raw)),
        source_maps_unchanged=True,native_projection_admitted=False,owned_scratch=str(owner),retained_large_artifacts=[])
    args.summary.write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps({k:summary[k]for k in ('materials','paired','maps','graphs','errors')}))
    return 0 if summary['passed']else 1


if __name__=='__main__':raise SystemExit(main())
