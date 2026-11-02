#!/usr/bin/env python3
"""Read only actual BEFORE banks at fields consumed by17A0F0/17A030."""
import argparse,bisect,json,os,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
class Before:
 def __init__(self,folder):
  self.folder=folder;self.metadata=folder/('capture-native-before.json'if(folder/'capture-native-before.json').exists()else'capture.json')
  self.capture=json.loads(self.metadata.read_text())
  self.rows=sorted(self.capture['regions'],key=lambda r:r['address']);self.starts=[r['address']for r in self.rows]
 def word(self,address,width=4):
  n=bisect.bisect_right(self.starts,address)-1
  if n<0:raise ValueError(f'Unmapped actual BEFORE address{address:x}')
  row=self.rows[n];assert address+width<=row['address']+row['size']
  file=(self.folder/row['before_file']).resolve();assert file.is_relative_to(self.folder)
  with file.open('rb')as stream:stream.seek(address-row['address']);value=stream.read(width)
  assert len(value)==width;return int.from_bytes(value,'little')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--include-lifetime-children',action='store_true');p.add_argument('--archive',type=Path)
 p.add_argument('--archive-sha');a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);contexts=[]
 base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
 selected=[(name,(base/('iteration-04-'+name)/'original/required-failure-inputs').resolve())for name in ('gemini-column0','gemini-column1','elongation-crates')]
 archive=None
 if a.archive:
  assert a.archive_sha and sha(a.archive)==a.archive_sha;destination=Path(os.environ['HALO2_TEST_SCRATCH'])/'actual-native-before';destination.mkdir()
  with tarfile.open(a.archive,'r:*')as stream:
   for member in stream.getmembers():
    target=(destination/member.name).resolve();assert target==destination or target.is_relative_to(destination);assert member.isfile()or member.isdir()
   stream.extractall(destination,filter='data')
  metadata=list(destination.rglob('capture-native-before.json'));assert len(metadata)==1
  selected=[('actual-native-pair',metadata[0].parent.resolve())];archive=dict(path=str(a.archive.resolve()),sha256=a.archive_sha)
 for name,folder in selected:
  m=Before(folder)
  pool=m.word(0x4f720c);extent=m.word(pool+0x38)
  stride=m.word(pool+0x24);bank=m.word(pool+0x44);assert extent<=4096
  records=[]
  for index in range(extent):
   row=bank+index*stride;salt=m.word(row,2)
   if not salt:continue
   datum=(salt<<16)|index;definition_datum=m.word(row+4);definition=m.word(m.word(0x4f0414)+(definition_datum&0xffff)*16+8)
   flags=m.word(row+2,1);definition_flags=m.word(definition);records.append(dict(datum=datum,row=row,flags=flags,
    link=m.word(row+8),definition_datum=definition_datum,definition=definition,definition_flags=definition_flags,
    definition_row_count=m.word(definition+0x14)if definition_flags&32 else None,
    immediate_delete_branch=not bool(flags&2)))
   if a.include_lifetime_children:records[-1].update(context=m.word(row+0x48),child_state=m.word(row+0xfc),child_head=m.word(row+0x100))
  tagged_values=None
  if a.include_lifetime_children:
   light_pool=m.word(0x4ecbec);tagged_values=dict(pool=light_pool)
   # These current-cache header/table fields are authenticated by the existing
   # native SourceRuntimeCacheTagImage delivery, not guessed table extents.
   cache_index=m.word(0x556d90);tag_count=m.word(cache_index+24);tag_table=m.word(0x4f0414)
   assert 0<tag_count<=65536;references=[]
   for rank in range(tag_count):
    entry=tag_table+rank*16
    if m.word(entry)==0x7464746c:references.append(dict(datum=m.word(entry+4),definition=m.word(entry+8)))
   tagged_values['actual_referenced_group_datums']=references
   if light_pool:
    enabled=m.word(light_pool+0x29,1);tagged_values['enabled']=enabled
    if enabled:
     light_extent=m.word(light_pool+0x38);light_stride=m.word(light_pool+0x24);light_bank=m.word(light_pool+0x44)
     assert light_extent<=8192;light_records=[]
     for index in range(light_extent):
      row=light_bank+index*light_stride;salt=m.word(row,2)
      if salt:light_records.append(dict(datum=(salt<<16)|index,row=row,reference=m.word(row+8),context=m.word(row+0xc),scalar_bits=m.word(row+0x10)))
     tagged_values.update(extent=light_extent,stride=light_stride,records=light_records)
  child_pool=None
  if a.include_lifetime_children:
   pointer=m.word(0x51df74);child_pool=dict(pool=pointer)
   if pointer:
    child_extent=m.word(pointer+0x38);child_stride=m.word(pointer+0x24);child_bank=m.word(pointer+0x44);assert child_extent<=65536
    children=[]
    for index in range(child_extent):
     row=child_bank+index*child_stride;salt=m.word(row,2)
     if salt:children.append(dict(datum=(salt<<16)|index,row=row,previous=m.word(row+0x38),next=m.word(row+0x3c),
      last_child=m.word(row+0x40),first_child=m.word(row+0x44),effect_reference=m.word(row+0x18),definition_index=m.word(row+0x10,2)))
    child_pool.update(extent=child_extent,stride=child_stride,records=children)
  contexts.append(dict(name=name,capture=str(m.metadata),capture_sha256=sha(m.metadata),
   pool=pool,stride=stride,extent=extent,source_valid_salt_records=records,tagged_values=tagged_values,child_pool=child_pool))
 save(a.output/'report.json',dict(read_only=True,captured_after_consumed=False,manufactured_rows=False,
  source_fields='17A0F0/17A030 authenticated owner listings',contexts=contexts,
  archive=archive,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]));return 0
if __name__=='__main__':raise SystemExit(main())
