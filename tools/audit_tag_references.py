#!/usr/bin/env python3
"""Audit schema-declared references and exact Xbox BSP resource bindings.
Candidates from community schemas are not proof of missing assets; exclusions
and complete source locations are retained. No arbitrary datum-word scanning.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
from audit_tag_completeness import Directory,NULL,sha,group

class Audit:
 def __init__(self,directory,output):
  self.d=directory;self.m=directory.image;self.counts=Counter();self.issues=Counter();self.samples=[];self.compiled={};self.schemas={};self.output=output;self.events=(output/'issues.jsonl').open('w');self.bsp_errors=[];self.bindings=[];self.visits=0
  self.meta_low=self.d.index+self.m.u(20);self.meta_high=self.meta_low+self.m.u(24)
  self.owners={}
  for t in self.d.tags.values():
   if t['address'] and t['size']:self.owners[t['address'],t['group']]=max(t['size'],self.owners.get((t['address'],t['group']),0))
 def issue(self,kind,t,field,**extra):
  self.issues[kind]+=1;e=dict(reason=kind,owner_group=t['group'],owner=t['name'],owner_datum=f"{t['datum']:08x}",field=field,**extra);self.events.write(json.dumps(e)+'\n')
  if len(self.samples)<32:self.samples.append(e)
 def nodes(self,code):
  if code in self.compiled:return self.compiled[code]
  path=self.d.schema(code)
  if not path:self.compiled[code]=None;return None
  self.schemas[str(path)]=sha(path)
  def compile_nodes(node):
   result=[];names=Counter()
   for e in node:
    if 'offset' not in e.attrib:continue
    name=e.get('name','');names[name]+=1;name=name+('#'+str(names[name]) if names[name]>1 else '')
    kind=e.tag.lower()
    if 'unknown' in name.lower() or name.lower()=='do not use':continue
    if kind=='tagref':result.append((kind,name,int(e.get('offset'),0),e.get('withGroup','true').lower()!='false',None))
    elif kind=='tagblock':
     children=compile_nodes(e)
     if children:result.append((kind,name,int(e.get('offset'),0),int(e.get('elementSize'),0),children))
   return result
  self.compiled[code]=compile_nodes(ET.parse(path).getroot());return self.compiled[code]
 def location(self,address,n,bsp):
  if bsp and bsp['address']<=address and address+n<=bsp['address']+bsp['size']:return bsp['file']+address-bsp['address']
  off=address-self.d.magic
  if not self.meta_low<=off<=off+n<=self.meta_high:raise ValueError('pointer_outside_resident_allocation')
  return off
 def walk_tag(self,t,bsp=None):
  nodes=self.nodes(t['group'])
  if nodes is None:self.issue('missing_schema',t,'');return
  size=t['size'] or self.owners.get((t['address'],t['group']),0)
  if not size:self.issue('unbounded_alias',t,'');return
  self.counts['tags_walked']+=1;active=set()
  def walk(nodes,base,prefix='',depth=0,root=False):
   if depth>32:self.issue('schema_depth_limit',t,prefix);return
   key=(id(nodes),base)
   if key in active:self.issue('schema_pointer_cycle',t,prefix);return
   active.add(key)
   try:
    for kind,name,relative,mode,children in nodes:
     field=prefix+name;address=base+relative;extent=8 if kind=='tagblock' or mode else 4
     try:
      if root and relative+extent>size:raise ValueError('root_schema_field_outside_tag')
      at=self.location(address,extent,bsp);raw=self.m.read(at,extent)
      if kind=='tagblock':
       count,pointer=struct.unpack('<iI',raw)
       if count<0 or count>65536 or mode<=0:raise ValueError('invalid_schema_block_count')
       self.counts['reference_blocks_checked']+=1
       if not count:continue
       self.location(pointer,count*mode,bsp)
       for index in range(count):walk(children,pointer+index*mode,field+f'[{index}]/',depth+1)
      else:
       self.counts['references_checked']+=1;datum=struct.unpack_from('<I',raw,4 if mode else 0)[0]
       if datum==NULL:self.counts['null_references']+=1;continue
       target=self.d.tags.get(datum)
       if target is None:
        slot=datum&0xffff;other=next((v for v in self.d.tags.values() if v['slot']==slot),None)
        self.issue('unresolved_reference_candidate',t,field,source_address=f'{address:08x}',file_offset=at,raw=raw.hex(),target_datum=f'{datum:08x}',slot_occupant=other);continue
       self.counts['resolved_references']+=1
       if mode:
        declared=struct.unpack_from('<I',raw)[0]
        if declared not in self.d.groups:
         self.issue('unknown_declared_group_schema_candidate',t,field,file_offset=at,raw=raw.hex(),target=target['name']);continue
        if declared not in self.d.groups[target['group_value']]:self.issue('reference_group_mismatch_candidate',t,field,file_offset=at,raw=raw.hex(),target_group=target['group'],target=target['name'])
       if not target['address']:
        if target['group'] in ('sbsp','ltmp'):self.counts['references_to_bsp_owned_tags']+=1
        else:self.issue('referenced_unloaded_tag_candidate',t,field,target=target)
     except (ValueError,AssertionError) as error:self.issue(str(error) or 'invalid_source_range',t,field,source_address=f'{address:08x}')
   finally:active.remove(key)
  walk(nodes,t['address'],root=True)
 def bsps(self):
  if self.d.scenario==NULL:return []
  scenario=self.d.tags[self.d.scenario];at=self.location(scenario['address']+0x210,8,None);count,pointer=struct.unpack('<iI',self.m.read(at,8));assert 0<=count<=256
  mapped=[]
  for i in range(count):
   try:
    at=self.location(pointer+i*0x44,0x44,None);raw=self.m.read(at,0x44);file,size,address=struct.unpack_from('<3I',raw);bsp_datum,ltmp_datum=struct.unpack_from('<I',raw,0x14)[0],struct.unpack_from('<I',raw,0x1c)[0]
    length,bsp_root,ltmp_root,signature=struct.unpack('<4I',self.m.read(file,16));assert length==size and signature==0x73627370 and file+size<=self.m.size
    binding=dict(index=i,file=file,size=size,address=address,roots=[])
    for datum,root,code in ((bsp_datum,bsp_root,'sbsp'),(ltmp_datum,ltmp_root,'ltmp')):
     if datum==NULL:continue
     tag=self.d.tags.get(datum);assert tag and tag['group']==code;assert address+16<=root<address+size
     t=tag|dict(address=root,size=address+size-root);binding['roots'].append(dict(datum=f'{datum:08x}',group=code,name=tag['name'],source_root=f'{root:08x}'));mapped.append((t,binding))
    self.bindings.append(binding)
   except (AssertionError,ValueError) as error:self.bsp_errors.append(dict(index=i,error=str(error) or 'Invalid authored BSP resource/identity/root'))
  return mapped
 def run(self):
  for t in self.d.tags.values():
   if t['address'] and t['group'] not in ('sbsp','ltmp'):self.walk_tag(t)
  for t,bsp in self.bsps():self.walk_tag(t,bsp)
  self.events.close();return dict(path=str(self.m.path.resolve()),input_sha256=sha(self.m.path),scope=__doc__,counts=dict(self.counts),issues=dict(self.issues),samples=self.samples,bsp_bindings=self.bindings,bsp_errors=self.bsp_errors,schemas=self.schemas,tool_sha256=sha(Path(__file__)))

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--map',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=False);d=Directory(args.map);assert d.platform=='xbox','Reference walk currently authenticates the Xbox layouts only'
 try:result=Audit(d,args.output).run()
 finally:d.image.close()
 (args.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(args.map.name,result['counts'],'issues',result['issues'],'BSP errors',len(result['bsp_errors']),flush=True)
if __name__=='__main__':main()
