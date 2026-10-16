#!/usr/bin/env python3
"""Read-only tag directory/schema inventory; no full runtime support claim.
No arbitrary datum scanning, cache conversion, game-state or GPU execution.
"""
import argparse
from bisect import bisect_right
from collections import Counter, OrderedDict
import hashlib
import json
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zlib

ROOT=Path(__file__).resolve().parents[1]
PLUGINS=ROOT/'tools/reference/assembly/src/Assembly/Plugins'
MCC_SCHEMAS=ROOT/'analysis/multiplayer/pc-gameplay-tag-comparison-20261011/schemas/Halo2MCC'
NULL=0xffffffff

def sha(path):
 with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def group(v):return v.to_bytes(4,'big').decode('ascii',errors='backslashreplace')

class Image:
 def __init__(self,path):
  self.path=path;self.file=path.open('rb');self.physical=path.stat().st_size;self.header=self.raw(0,min(2048,self.physical));self.version=struct.unpack_from('<I',self.header,4)[0]
  assert self.header[:4]==b'daeh' and self.version in (8,13),'Unsupported cache format'
  self.size=struct.unpack_from('<I',self.header,8)[0];self.chunks=[];self.starts=[];self.cache=OrderedDict()
  if self.version==13:
   self.chunk_size,_,table,count=struct.unpack_from('<4I',self.header,776);assert 0<self.chunk_size<=4*1024*1024 and 0<count<65536
   self.starts=[896];logical=896
   for i in range(count):
    size,address=struct.unpack('<ii',self.raw(table+i*8,8));assert address>=0
    decoded=self.decode(size,address);self.chunks.append((size,address,len(decoded)));logical+=len(decoded);self.starts.append(logical)
   assert logical==self.size,'Decoded extent disagrees with header'
  else:assert self.size==self.physical,'Physical extent disagrees with header'
 def close(self):self.cache.clear();self.file.close()
 def raw(self,at,n):
  assert 0<=at<=at+n<=self.physical;self.file.seek(at);data=self.file.read(n);assert len(data)==n;return data
 def decode(self,size,address):
  if size<0:data=self.raw(address,-size)
  else:
   assert size<=8*1024*1024
   decoder=zlib.decompressobj();data=decoder.decompress(self.raw(address,size),self.chunk_size+1)
   assert decoder.eof and not decoder.unconsumed_tail,'Oversized/incomplete cache chunk'
  assert 0<len(data)<=self.chunk_size;return data
 def read(self,at,n):
  assert 0<=at<=at+n<=self.size
  if self.version!=13:return self.raw(at,n)
  result=bytearray()
  while n:
   if at<896:
    count=min(n,896-at);result.extend(self.header[at:at+count])
   else:
    i=bisect_right(self.starts,at)-1;assert 0<=i<len(self.chunks)
    if i not in self.cache:
     self.cache[i]=self.decode(*self.chunks[i][:2])
     if len(self.cache)>2:self.cache.popitem(last=False)
    self.cache.move_to_end(i);off=at-self.starts[i];count=min(n,len(self.cache[i])-off);assert count>0;result.extend(self.cache[i][off:off+count])
   at+=count;n-=count
  return bytes(result)
 def u(self,at):return struct.unpack('<I',self.read(at,4))[0]

class Directory:
 def __init__(self,path):
  self.image=Image(path);m=self.image;self.index=m.u(16);self.platform='mcc' if m.version==13 else 'vista' if m.read(300,32).split(b'\0')[0]==b'11081.07.04.30.0934.main' else 'xbox'
  assert m.u(self.index+28)==0x74616773
  self.group_count=m.u(self.index+4);self.count=m.u(self.index+24);assert 0<self.group_count<=32768 and 0<self.count<=65536
  header_magic=m.u(self.index)-(self.index+32);self.table=m.u(self.index+8)-header_magic;self.groups={}
  for i in range(self.group_count):
   row=struct.unpack('<3I',m.read(self.index+32+i*12,12));assert row[0] not in self.groups;self.groups[row[0]]=row
  if self.platform=='xbox':self.magic=m.u(self.table+8)-(self.index+m.u(20));name_base,name_size,name_table=m.u(708),m.u(712),m.u(716)
  elif self.platform=='vista':self.magic=m.u(32)-self.index;name_base,name_size,name_table=m.u(720),m.u(724),m.u(728)
  else:self.magic=m.u(720)-self.index;name_base,name_size,name_table=m.u(36),m.u(40),m.u(44)
  names=m.read(name_base,name_size);offsets=m.read(name_table,self.count*4);raw=m.read(self.table,self.count*16);self.tags={};self.invalid=[];self.unloaded=[];self.aliases=[]
  for i in range(self.count):
   code,datum,address,size=struct.unpack_from('<4I',raw,i*16)
   if datum==NULL:continue
   off=struct.unpack_from('<I',offsets,i*4)[0];assert off<len(names);end=names.find(b'\0',off);assert end>=off
   t=dict(slot=i,datum=datum,group=group(code),group_value=code,address=address,size=size,name=names[off:end].decode('utf-8',errors='replace'));self.tags[datum]=t
   if datum&0xffff!=i or i>=32768 or code not in self.groups:self.invalid.append(t)
   if not address:self.unloaded.append(t)
   elif not size:self.aliases.append(t)
  self.scenario=m.u(self.index+12);self.globals=m.u(self.index+16);self.scenario_type=None
  if self.scenario!=NULL:
   t=self.tags.get(self.scenario)
   if not t or t['group']!='scnr':self.invalid.append(dict(reason='invalid_scenario_header',datum=self.scenario))
   else:self.scenario_type=struct.unpack('<H',m.read(t['address']-self.magic+0x10,2))[0]
  if self.globals!=NULL and (self.globals not in self.tags or self.tags[self.globals]['group']!='matg'):self.invalid.append(dict(reason='invalid_globals_header',datum=self.globals))
 def schema(self,code):
  banks=[PLUGINS/'Halo2Xbox',PLUGINS/'Halo2'] if self.platform=='xbox' else [MCC_SCHEMAS,PLUGINS/'Halo2'] if self.platform=='mcc' else [PLUGINS/'Halo2']
  return next((p/(re.sub(r'[<>:"/\\|?*]', '_', code).strip()+'.xml') for p in banks if (p/(re.sub(r'[<>:"/\\|?*]', '_', code).strip()+'.xml')).is_file()),None)
 def report(self,catalog,output):
  counts=Counter(t['group'] for t in self.tags.values());missing=[code for code in counts if self.schema(code) is None]
  result=dict(path=str(self.image.path.resolve()),sha256=sha(self.image.path),platform=self.platform,scenario_type=self.scenario_type,declared_groups=self.group_count,tag_slots=self.count,valid_tag_rows=len(self.tags),tag_groups=dict(sorted(counts.items())),uncatalogued_groups=[group(g) for g in self.groups if group(g) not in catalog],catalog_groups_not_declared=[g for g in catalog if int.from_bytes(g.encode(),'big') not in self.groups],invalid_directory_rows=self.invalid,expected_unloaded_bsp_lightmaps=[t for t in self.unloaded if t['group'] in ('sbsp','ltmp')],other_unloaded_tags=[t for t in self.unloaded if t['group'] not in ('sbsp','ltmp')],zero_size_aliases=len(self.aliases),present_groups_without_schema={g:counts[g] for g in missing},schema_sha256={g:sha(self.schema(g)) for g in counts if self.schema(g)})
  with (output/(self.image.path.stem+'-'+self.platform+'-tags.jsonl')).open('w') as f:
   for t in self.tags.values():f.write(json.dumps(t,ensure_ascii=True)+'\n')
  return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--map',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 catalog=dict((code,name) for name,code in re.findall(r'^HALO2_TAG_GROUP\((\w+), "(.{4})"\)',(ROOT/'include/halo2/tags/groups.def').read_text(),re.M));assert len(catalog)==120
 d=Directory(args.map)
 try:result=d.report(catalog,args.output)
 finally:d.image.close()
 result.update(scope=__doc__,tool_sha256=sha(Path(__file__)),catalog_sha256=sha(ROOT/'include/halo2/tags/groups.def'),catalog_size=len(catalog),catalog_names=catalog)
 (args.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(args.map.name,result['platform'],'tags',result['valid_tag_rows'],'uncatalogued',len(result['uncatalogued_groups']),'other_unloaded',len(result['other_unloaded_tags']),'no_schema',result['present_groups_without_schema'],flush=True)
if __name__=='__main__':main()
