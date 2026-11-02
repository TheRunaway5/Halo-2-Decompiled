#!/usr/bin/env python3
"""Resolve actual captured phmo StringIDs through the same imported MCC cache table."""
import argparse,json,os,hashlib,struct,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cache',required=True,type=Path);p.add_argument('--census',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert os.environ.get('HALO2_TEST_SCRATCH');data=a.cache.read_bytes();encoded_sha=hashlib.sha256(data).hexdigest();word=lambda p:struct.unpack_from('<I',data,p)[0];assert word(0)==0x68656164 and word(4)==13
 if struct.unpack_from('<H',data,28)[0]&1:
  chunk_size,table,count_chunks=word(776),word(784),word(788);extent=word(8);assert 896<extent<=1024**3 and 0<count_chunks<=65536;decoded=bytearray(data[:896])
  for i in range(count_chunks):
   stored,at=struct.unpack_from('<iI',data,table+i*8);assert stored and at>=896 and at+abs(stored)<=len(data);raw=data[at:at+abs(stored)];part=zlib.decompress(raw)if stored>0 else raw;assert 0<len(part)<=min(chunk_size,extent-len(decoded));decoded+=part
  assert len(decoded)==extent;data=bytes(decoded)
 count,strings,size,indices=struct.unpack_from('<4I',data,48);assert count<=1000000 and strings+size<=len(data) and indices+4*count<=len(data);j=json.loads(a.census.read_text());physics=next(v for v in j['snapshots'][0]['vehicle_components']if v['object']==3802202162)['authored_physics'];names=[]
 for kind,rows in [('node',physics['nodes']),('limited_hinge',physics['limited_hinges'])]:
  for row in rows:
   sid=row['name_string_id'];index=sid&0xffffff;assert index<count;offset=word(indices+index*4);assert offset<size;end=data.index(b'\0',strings+offset,strings+size);text=data[strings+offset:end].decode();assert len(text)==sid>>24;names.append({'kind':kind,'index':row['index'],'string_id':sid,'name':text,'index_offset':indices+index*4,'string_offset':strings+offset})
 out={'passed':True,'cache':str(a.cache.resolve()),'cache_sha256':encoded_sha,'decoded_sha256':hashlib.sha256(data).hexdigest(),'cache_header_version':word(4),'census_sha256':hashlib.sha256(a.census.read_bytes()).hexdigest(),'source_accessors':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest()for p in ['src/rendering/models/model.cpp','include/halo2/tags/types.hpp','src/assets/mcc_cache_decode.cpp']},'names':names,'scope':'Literal authored metadata only; no gameplay/physics trigger inferred or state modified','retained_large_artifacts':[]};(a.output/'report.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(names,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
